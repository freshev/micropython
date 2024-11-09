/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * Originally written by Adafruit Industries.
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 freshev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "math.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "api_os.h"
#include "api_hal_gpio.h"

#include "modmachine.h"
#include "mphalport.h"

#define DHT_MIN_INTERVAL 2000
#define DHT_TIMEOUT UINT32_MAX

enum DHTTypes {
    DHT11 = 11,
    DHT12 = 12,
    DHT21 = 21,
    DHT22 = 22,
    AM2301 = 21
};

typedef struct _dht_obj_t {
    mp_obj_base_t base;
    mp_obj_t pin_obj;
    uint8_t _pin;
    uint8_t _type;
    uint8_t data[5];
    uint32_t _lastreadtime;
    uint32_t _maxcycles;
    uint8_t _lastresult;
    uint8_t pullTime; // Time (in usec) to pull up data line before reading
} dht_obj_t;

static int dht_inited = 0;

STATIC int moddht_microsecondsToClockCycles(int us) {
    return us * CLOCKS_PER_MSEC;
}

STATIC void moddht_begin(dht_obj_t *self, uint8_t usec) {
    if(dht_inited == 0) {
        // set up the pins!
        mp_hal_pin_input(self->_pin);

        // Using this value makes sure that millis() - lastreadtime will be
        // >= DHT_MIN_INTERVAL right away. Note that this assignment wraps around,
        // but so will the subtraction.
        self->_lastreadtime = mp_hal_ticks_ms_64() - DHT_MIN_INTERVAL;
        //Trace(1, "DHT max clock cycles: %d", self->_maxcycles);
        self->pullTime = usec;
        dht_inited = 1;
    }
}

// Expect the signal line to be at the specified level for a period of time and
// return a count of loop cycles spent at that level (this cycle count can be
// used to compare the relative time of two pulses).  If more than a millisecond
// ellapses without the level changing then the call fails with a 0 response.
//   https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/wiring_pulse.c
STATIC uint32_t moddht_expectPulse(dht_obj_t *self, uint8_t level_in) {
    uint32_t count = 0;
    GPIO_LEVEL level;
    while (true) {
        if(GPIO_Get(self->_pin, &level)) {
            if(level != level_in) break;
            if (count++ >= self->_maxcycles) {
               Trace(1, "DHT TIMEOUT, count: %d", count);
               return DHT_TIMEOUT;
            }
        } else {
           Trace(1, "DHT can not get GPIO level");
        }
    }
    return count;
}

STATIC uint8_t moddht_read(dht_obj_t *self, uint8_t force) {
    // Check if sensor was read less than two seconds ago and return early
    // to use last reading.
    uint32_t currenttime = mp_hal_ticks_ms_64();
    if (!force && ((currenttime - self->_lastreadtime) < DHT_MIN_INTERVAL)) {
        return self->_lastresult; // return last correct measurement
    }
    self->_lastreadtime = currenttime;

    // Reset 40 bits of received data to zero.
    self->data[0] = self->data[1] = self->data[2] = self->data[3] = self->data[4] = 0;

    // Send start signal.  See DHT datasheet for full signal diagram:
    //   http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

    // Go into high impedence state to let pull-up raise data line level and
    // start the reading process.
    mp_hal_pin_input(self->_pin);
    OS_Sleep(1);

    // First set data line low for a period according to sensor type
    mp_hal_pin_output(self->_pin);
    mp_hal_pin_write(self->_pin, GPIO_LEVEL_LOW);

    switch (self->_type) {
        case DHT22:
        case DHT21:
            OS_SleepUs(1100); // data sheet says "at least 1ms"
            break;

        case DHT11:
        default:
            OS_Sleep(20); // data sheet says at least 18ms, 20ms just to be safe
            break;
    }

    uint32_t cycles[80];
    // -------------------------
    // Time critical code start
    // -------------------------
    // End the start signal by setting data line high for 55 microseconds.
    mp_hal_pin_input(self->_pin);

    // Delay a moment to let sensor pull data line low.
    OS_SleepUs(self->pullTime);

    // Now start reading the data line to get the value from the DHT sensor.
    // Turn off interrupts temporarily because the next sections
    // are timing critical and we don't want any interruptions.
    uint32_t status = MICROPY_BEGIN_ATOMIC_SECTION();

    // First expect a low signal for ~80 microseconds followed by a high signal
    // for ~80 microseconds again.
    if (moddht_expectPulse(self, GPIO_LEVEL_LOW) == DHT_TIMEOUT) {
        Trace(1, "DHT timeout waiting for start signal low pulse.");
        self->_lastresult = false;
        MICROPY_END_ATOMIC_SECTION(status);
        return self->_lastresult;
    }
    if (moddht_expectPulse(self, GPIO_LEVEL_HIGH) == DHT_TIMEOUT) {
        Trace(1, "DHT timeout waiting for start signal high pulse.");
        self->_lastresult = false;
        MICROPY_END_ATOMIC_SECTION(status);
        return self->_lastresult;
    }

    // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
    // microsecond low pulse followed by a variable length high pulse.  If the
    // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
    // then it's a 1.  We measure the cycle count of the initial 50us low pulse
    // and use that to compare to the cycle count of the high pulse to determine
    // if the bit is a 0 (high state cycle count < low state cycle count), or a
    // 1 (high state cycle count > low state cycle count). Note that for speed
    // all the pulses are read into a array and then examined in a later step.
    for (int i = 0; i < 80; i += 2) {
        cycles[i] = moddht_expectPulse(self, GPIO_LEVEL_LOW);
        cycles[i + 1] = moddht_expectPulse(self, GPIO_LEVEL_HIGH);
    }
    // -------------------------
    // Time critical code end
    // -------------------------
    MICROPY_END_ATOMIC_SECTION(status);


    // Inspect pulses and determine which ones are 0 (high state cycle count < low
    // state cycle count), or 1 (high state cycle count > low state cycle count).
    for (int i = 0; i < 40; ++i) {
        uint32_t lowCycles = cycles[2 * i];
        uint32_t highCycles = cycles[2 * i + 1];
        // Trace(1, "DHT pulses[%d]: lowCycles: %d, highCycles: %d", i, lowCycles, highCycles);
        if ((lowCycles == DHT_TIMEOUT) || (highCycles == DHT_TIMEOUT)) {
           Trace(1, "DHT timeout waiting for pulse.");
           self->_lastresult = false;
           return self->_lastresult;
        }
        self->data[i / 8] <<= 1;
        // Now compare the low and high cycle times to see if the bit is a 0 or 1.
        if (highCycles > lowCycles) {
            // High cycles are greater than 50us low cycle count, must be a 1.
            self->data[i / 8] |= 1;
        }
        // Else high cycles are less than (or equal to, a weird case) the 50us low
        // cycle count so this must be a zero.  Nothing needs to be changed in the
        // stored data.
    }

    // Trace(1, "Received from DHT: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x", self->data[0], self->data[1], self->data[2], self->data[3], self->data[4]);
    // Trace(1, "Check sum: 0x%02x vs 0x%02x", (self->data[0] + self->data[1] + self->data[2] + self->data[3]) & 0xFF, self->data[4]);

    // Check we read 40 bits and that the checksum matches.
    if (self->data[4] == ((self->data[0] + self->data[1] + self->data[2] + self->data[3]) & 0xFF)) {
        self->_lastresult = true;
        return self->_lastresult;
    } else {
        Trace(1, "DHT checksum failure!");
        self->_lastresult = false;
        return self->_lastresult;
    }
}

STATIC float moddht_convertCtoF(float c) { return c * 1.8 + 32; }

// ------------
// DHT Print
// ------------

void dht_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    dht_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char temp[3];
    mp_printf(print, "DHT(pin:%d, type:DHT%s)", self->_pin, itoa(self->_type, temp, 10));
}

// ---------------
// DHT Constructor
// ---------------
extern const mp_obj_type_t dht_type;
extern mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t dht_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    enum { ARG_pin, ARG_type};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin, MP_ARG_INT, {.u_int = 30 } }, // Pin30 - common GPIO, Pin20 - I2C2_SDA
        { MP_QSTR_type, MP_ARG_INT, {.u_int = DHT21} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    dht_obj_t * self = mp_obj_malloc(dht_obj_t, &dht_type);

    self->_pin = args[ARG_pin].u_int;
    self->_type = args[ARG_type].u_int;
    self->_maxcycles = moddht_microsecondsToClockCycles(1000);  // was 1000

    //init pin via machine_pin to turn on power on pin
    mp_obj_t pin_args[1];
    pin_args[0] = MP_OBJ_NEW_SMALL_INT(args[ARG_pin].u_int);
    mp_obj_t pin_obj = mp_pin_make_new(NULL, 1, 0, pin_args);
    self->pin_obj = pin_obj;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t moddht_readTemperature(mp_obj_t self_in, mp_obj_t S_in, mp_obj_t force_in) {
    // ========================================
    // Read temperature.
    // Args:
    //    S (bool): Scale
    //              - true = Fahrenheit
    //              - false = Celcius
    //    force: true if in force mode
    // Returns: Temperature value in selected scale
    // ========================================
    dht_obj_t *self = self_in;
    uint8_t S = mp_obj_get_int(S_in);
    uint8_t force = mp_obj_get_int(force_in);
    moddht_begin(self, 55);

    float f = NAN;

    if (moddht_read(self, force)) {
        switch (self->_type) {
            case DHT11:
                f = self->data[2];
                if (self->data[3] & 0x80) {
                    f = -1 - f;
                }
                f += (self->data[3] & 0x0f) * 0.1;
                if (S) f = moddht_convertCtoF(f);
                break;
            case DHT12:
                f = self->data[2];
                f += (self->data[3] & 0x0f) * 0.1;
                if (self->data[2] & 0x80) {
                    f *= -1;
                }
                if (S) f = moddht_convertCtoF(f);
                break;
            case DHT22:
            case DHT21:
                 f = ((uint16_t)(self->data[2] & 0x7F)) << 8 | self->data[3];
                 f *= 0.1;
                 if (self->data[2] & 0x80) f *= -1;
                 if (S) f = moddht_convertCtoF(f);
                 break;
        }
    }
    return mp_obj_new_float(f);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(moddht_readTemperature_obj, moddht_readTemperature);


STATIC mp_obj_t moddht_readHumidity(mp_obj_t self_in, mp_obj_t force_in) {
    // ========================================
    // Read hunidity.
    // Args:
    //    force: true if in force mode
    // Returns: Humidity value
    // ========================================
    dht_obj_t *self = self_in;
    uint8_t force = mp_obj_get_int(force_in);
    moddht_begin(self, 55);

    float f = NAN;
    if (moddht_read(self, force)) {
        switch (self->_type) {
            case DHT11:
            case DHT12:
                 f = self->data[0] + self->data[1] * 0.1;
                 break;
            case DHT22:
            case DHT21:
                 f = ((uint16_t)self->data[0]) << 8 | self->data[1];
                 f *= 0.1;
                 break;
        }
    }
    return mp_obj_new_float(f);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(moddht_readHumidity_obj, moddht_readHumidity);

// -------
// Locals
// -------
STATIC const mp_rom_map_elem_t dht_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_readTemperature), MP_ROM_PTR(&moddht_readTemperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_readHumidity), MP_ROM_PTR(&moddht_readHumidity_obj) }
};

STATIC MP_DEFINE_CONST_DICT(dht_locals_dict, dht_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    dht_type,
    MP_QSTR_dht,
    MP_TYPE_FLAG_NONE,
    make_new, dht_make_new,
    print, dht_print,
    locals_dict, &dht_locals_dict
    );


// -------
// Modules
// -------
STATIC const mp_map_elem_t mp_module_dht_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_dht) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_dht), (mp_obj_t)MP_ROM_PTR(&dht_type) },
    { MP_ROM_QSTR(MP_QSTR_DHT11), MP_ROM_INT(DHT11) },
    { MP_ROM_QSTR(MP_QSTR_DHT12), MP_ROM_INT(DHT12) },
    { MP_ROM_QSTR(MP_QSTR_DHT21), MP_ROM_INT(DHT21) },
    { MP_ROM_QSTR(MP_QSTR_DHT22), MP_ROM_INT(DHT22) },
    { MP_ROM_QSTR(MP_QSTR_AM2301), MP_ROM_INT(AM2301) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_dht_globals, mp_module_dht_globals_table);

const mp_obj_module_t dht_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_dht_globals,
};

MP_REGISTER_MODULE(MP_QSTR_dht, dht_module);