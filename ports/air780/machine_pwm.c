/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2021 Damien P. George
 * Copyright (c) 2018 Alan Dragomirecky
 * Copyright (c) 2020 Antoine Aubert
 * Copyright (c) 2021 Ihor Nehrutsa
 * Copyright (c) 2025 freshev
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

// This file is never compiled standalone, it's included directly from
// extmod/machine_pwm.c via MICROPY_PY_MACHINE_PWM_INCLUDEFILE.

#include "py/mphal.h"

#include "platform_define.h"
#include "common_api.h"
#include "luat_debug.h"
#include "luat_pwm.h"

// Total number of channels
#define PWM_CHANNEL_MAX     (6) 

// Params for PWM operation
// 1khz is default frequency
#define PWM_FREQ (10000)

#define MP_DUTY_MAX (1024)
#define LUAT_DUTY_MAX (1000)

// channel_idx is an index (end-to-end sequential numbering) for all channels
// available on the chip and described in chans[]
//#define CHANNEL_IDX(mode, channel) (mode * PWM_CHANNEL_DIV + channel)
//#define CHANNEL_IDX_TO_MODE(channel_idx) (channel_idx / PWM_CHANNEL_DIV)
//#define CHANNEL_IDX_TO_CHANNEL(channel_idx) (channel_idx % PWM_CHANNEL_DIV)


// PWM channels to pin assignment
typedef struct _pwm_chan_t {
    int8_t pin;
} pwm_chan_t;

// List of PWM channels
static pwm_chan_t pwm_channels[PWM_CHANNEL_MAX];
    

typedef struct _pwm_gpio_t {
    int8_t pin;
    int8_t phys;
    int8_t channel;
} pwm_gpio_t;

// List of PWM GPIOs
// See https://docs.openluat.com/air780e/luatos/app/driver/pwm/
static pwm_gpio_t machine_pwm_gpio_table[] = {
    {0, HAL_GPIO_0, -1},    // no channel
    {1, HAL_GPIO_1, 0},     // channel = 10
    {2, HAL_GPIO_2, 1},     // channel = 11
    {3, HAL_GPIO_3, -1},    // no channel
    {4, HAL_GPIO_4, -1},   // no channel
    {5, HAL_GPIO_5, -1},   // no channel
    {6, HAL_GPIO_6, -1},   // no channel
    {7, HAL_GPIO_7, -1},   // no channel
    {8, HAL_GPIO_8, -1},   // no channel
    {9, HAL_GPIO_9, -1},   // no channel
    {10, HAL_GPIO_10, -1},  // no channel
    {11, HAL_GPIO_11, -1},  // no channel
    {12, HAL_GPIO_12, -1},  // no channel
    {13, HAL_GPIO_13, -1},  // no channel
    {14, HAL_GPIO_14, -1},  // no channel
    {15, HAL_GPIO_15, -1},  // no channel
    {16, HAL_GPIO_16, 2},   // channel = 12
    {17, HAL_GPIO_17, -1},  // no channel
    {18, HAL_GPIO_18, 4},   // channel = 14
    {19, HAL_GPIO_19, -1},  // no channel
    {20, HAL_GPIO_20, -1},  // no channel
    {21, HAL_GPIO_21, -1},  // no channel
    {22, HAL_GPIO_22, -1},  // no channel
    {23, HAL_GPIO_23, 0},   // channel = 0
    {24, HAL_GPIO_24, 1},   // channel = 1
    {25, HAL_GPIO_25, 2},   // channel = 2
    {26, HAL_GPIO_26, -1},  // no channel
    {27, HAL_GPIO_27, 4},   // channel = 4
    {28, HAL_GPIO_28, -1},  // no channel
    {29, HAL_GPIO_29, 1},   // channel = 21
    {30, HAL_GPIO_30, 2},   // channel = 22
    {31, HAL_GPIO_30, -1},  // no channel
};

// MicroPython PWM object struct
typedef struct _machine_pwm_obj_t {
    mp_obj_base_t base;
    uint8_t pin;
    bool active;
    uint8_t channel;
    uint32_t duty;    
    uint32_t freq;
} machine_pwm_obj_t;

void modmachine_pwm_init0(void) {
    // Initial condition: no channels assigned
    for (int i = 0; i < PWM_CHANNEL_MAX; ++i) {
        pwm_channels[i].pin = -1;
        // luat_pwm_setup always returns -1 ...
        /*luat_pwm_conf_t config = {
            .channel = i,
            .period = 1000,         // Hz
            .pulse = 500,           // half a period
            .pnum = 0,              // 0 - continuous output
            .precision = PWM_FREQ   // 100, 256 or 1000
        };
        if(i != 3 && i != 5) {
            int res = luat_pwm_setup(&config);
            if (res != 0) LUAT_DEBUG_PRINT("PWM init for channel %d failed (%d)", i, res);
        }*/
    }    
}

// Deinit channel by index
void modmachine_pwm_deinit(int channel_idx) {
    if ((channel_idx >= 0) && (channel_idx < PWM_CHANNEL_MAX)) {
        int pin = pwm_channels[channel_idx].pin;
        if (pin != -1) {
            luat_pwm_close(channel_idx);
        }
        pwm_channels[channel_idx].pin = -1;
    }
}

// This called from Ctrl-D soft reboot
void modmachine_pwm_deinit0(void) {
    for (int channel_idx = 0; channel_idx < PWM_CHANNEL_MAX; ++channel_idx) {
        modmachine_pwm_deinit(channel_idx);
    }
}

int pwm_find_channel(int pin) {
    int gpio_idx;
    for (gpio_idx = 0; gpio_idx < HAL_GPIO_MAX; ++gpio_idx) {
        if (machine_pwm_gpio_table[gpio_idx].pin == pin) {
            return machine_pwm_gpio_table[gpio_idx].channel;
        }
    }
    return -1;
}


static void set_duty_internal(machine_pwm_obj_t *self) {
    LUAT_DEBUG_PRINT("duty_in = %d", self->duty);
    if (self->duty == 0) {
        self->duty = 1;
    } else if (self->duty > LUAT_DUTY_MAX) {
        self->duty = LUAT_DUTY_MAX;
    }
    LUAT_DEBUG_PRINT("duty_out = %d", self->duty);
    luat_pwm_update_dutycycle(self->channel, self->duty);
}

static void pwm_is_active(machine_pwm_obj_t *self) {
    if (self->active == false) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM inactive"));
    }
}

static int ns_to_duty(machine_pwm_obj_t *self, int ns) {
    int64_t duty = ((int64_t)ns * LUAT_DUTY_MAX * self->freq) / 1000000000LL ;
    if ((ns > 0) && (duty == 0)) {
        duty = 1;
    } else if (duty > LUAT_DUTY_MAX) {
        duty = LUAT_DUTY_MAX;
    }
    return duty;
}

static int duty_to_ns(machine_pwm_obj_t *self, int duty) {
    int64_t ns = ((int64_t)duty * 1000000000LL + self->freq * LUAT_DUTY_MAX / 2) / ((int64_t)self->freq * LUAT_DUTY_MAX);
    return ns;
}



static uint32_t get_duty_u16(machine_pwm_obj_t *self) {
    pwm_is_active(self);
    return (self->duty << 6) * MP_DUTY_MAX / LUAT_DUTY_MAX;
}

static uint32_t get_duty_u10(machine_pwm_obj_t *self) {
    pwm_is_active(self);
    return self->duty * MP_DUTY_MAX / LUAT_DUTY_MAX; 
}

static uint32_t get_duty_ns(machine_pwm_obj_t *self) {
    pwm_is_active(self);
    return duty_to_ns(self, self->duty);
}

static void set_duty_u16(machine_pwm_obj_t *self, int duty) {
    pwm_is_active(self);
    if ((duty < 0) || (duty > MP_DUTY_MAX << 6)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("duty_u16 must be from 0 to %d"), MP_DUTY_MAX << 6);
    }
    self->duty = (duty >> 6) * LUAT_DUTY_MAX / MP_DUTY_MAX;
    set_duty_internal(self);
}

static void set_duty_u10(machine_pwm_obj_t *self, int duty) {
    pwm_is_active(self);
    if ((duty < 0) || (duty > MP_DUTY_MAX)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("duty must be from 0 to %u"), MP_DUTY_MAX);
    }    
    self->duty = duty * LUAT_DUTY_MAX / MP_DUTY_MAX;
    set_duty_internal(self);
}

static void set_duty_ns(machine_pwm_obj_t *self, int ns) {
    pwm_is_active(self);
    if ((ns < 0) || (ns > duty_to_ns(self, LUAT_DUTY_MAX))) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("duty_ns must be from 0 to %d ns"), duty_to_ns(self, LUAT_DUTY_MAX));
    }
    self->duty = ns_to_duty(self, ns);
    set_duty_internal(self);
}



/******************************************************************************/
// MicroPython bindings for PWM

static void mp_machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    mp_printf(print, "PWM(active=%d, channel=%d, pin=%u, freq=%u, duty=%d)", self->active, self->channel, self->pin, self->freq, self->duty * MP_DUTY_MAX / LUAT_DUTY_MAX);
}

// This called from pwm.deinit() method
static void mp_machine_pwm_deinit(machine_pwm_obj_t *self) {
    modmachine_pwm_deinit(self->channel);
    self->active = false;
    self->channel = -1;
    self->duty = 0;
}

// This called from pwm.init() method
static void mp_machine_pwm_init_helper(machine_pwm_obj_t *self,
    size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_freq, ARG_duty };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_duty, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 50} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int channel = pwm_find_channel(self->pin);
    if (channel == -1) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Pin %u can not assign PWM channel"), self->pin); 
    }

    int freq = args[ARG_freq].u_int;
    // Check if freq wasn't passed as an argument
    if (freq == -1) {
        freq = PWM_FREQ;
    }
    if ((freq <= 0) || (freq > 13000000)) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM frequency must be from 1Hz to 13MHz"));
    }

    self->channel = channel;
    self->freq = freq;
    self->duty = args[ARG_duty].u_int * LUAT_DUTY_MAX / MP_DUTY_MAX;

    // New PWM assignment
    if (pwm_channels[channel].pin == -1) {
        int res = luat_pwm_open(self->channel, self->freq, self->duty * LUAT_DUTY_MAX / MP_DUTY_MAX, 0); // 0 - continuous output
        if (res == -1) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM channel selection error"));
        if (res == -2) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM frequency setting error"));
        if (res == -3) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM duty cycle setting error"));
        if (res == -4) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM channel already used"));
        pwm_channels[channel].pin = self->pin;                                                          
    } else if (pwm_channels[channel].pin == self->pin) {        
        // reopen PWM with new freq and/or duty
        luat_pwm_close(self->channel); 
        int res = luat_pwm_open(self->channel, self->freq, self->duty * LUAT_DUTY_MAX / MP_DUTY_MAX, 0); 
        if (res != 0) mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM reopen channel failed (%d)"), res);
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("PWM channel %u already occupied by GPIO%u"), channel, self->pin);
    }
    self->active = true;
}

// This called from PWM() constructor
static mp_obj_t mp_machine_pwm_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 2, true);
    uint8_t pin_id = machine_pin_get_id(args[0]);

    // create PWM object from the given pin
    machine_pwm_obj_t *self = mp_obj_malloc(machine_pwm_obj_t, &machine_pwm_type);
    self->pin = pin_id;
    self->active = false;
    self->channel = -1;
    self->duty = 0;

    // start the PWM running for this channel
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_pwm_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

// Set and get methods of PWM class

static mp_obj_t mp_machine_pwm_freq_get(machine_pwm_obj_t *self) {
    pwm_is_active(self);
    return MP_OBJ_NEW_SMALL_INT(self->freq);
}

static void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_int_t freq) {
    pwm_is_active(self);
    luat_pwm_close(self->channel); 
    int res = luat_pwm_open(self->channel, freq, self->duty * LUAT_DUTY_MAX / MP_DUTY_MAX, 0); 
    if (res != 0) mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("PWM set freq failed (%d)"), res);
    self->freq = freq;
}

static mp_obj_t mp_machine_pwm_duty_get(machine_pwm_obj_t *self) {
    return MP_OBJ_NEW_SMALL_INT(get_duty_u10(self));
}

static void mp_machine_pwm_duty_set(machine_pwm_obj_t *self, mp_int_t duty) {
    set_duty_u10(self, duty);
}

static mp_obj_t mp_machine_pwm_duty_get_u16(machine_pwm_obj_t *self) {
    return MP_OBJ_NEW_SMALL_INT(get_duty_u16(self));
}

static void mp_machine_pwm_duty_set_u16(machine_pwm_obj_t *self, mp_int_t duty_u16) {
    set_duty_u16(self, duty_u16);
}

static mp_obj_t mp_machine_pwm_duty_get_ns(machine_pwm_obj_t *self) {
    return MP_OBJ_NEW_SMALL_INT(get_duty_ns(self));
}

static void mp_machine_pwm_duty_set_ns(machine_pwm_obj_t *self, mp_int_t duty_ns) {
    set_duty_ns(self, duty_ns);
}


/*
# Test cases
from machine import PWM, Pin
pwm = PWM(Pin(27), freq=1000, duty=512)
print(pwm)
pwm = PWM(Pin(27), freq=1000)
pwm.duty(512)
pwm.duty_u16(10000)

pwm.freq(1000)
pwm.duty_ns(100000)
*/