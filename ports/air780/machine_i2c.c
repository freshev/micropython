/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"

#include "common_api.h"
#include "luat_i2c.h"
#include "luat_debug.h"

#define I2C_DEFAULT_TIMEOUT_US (50000) // 50ms

typedef struct _machine_hw_i2c_obj_t {
    mp_obj_base_t base;
    int id;
    int scl;
    int sda;
    int freq;
    mp_int_t timeout; // in MS, not US
} machine_hw_i2c_obj_t;

#define MICROPY_HW_I2C0_SCL 17
#define MICROPY_HW_I2C0_SDA 16
#define MICROPY_HW_I2C1_SCL 9
#define MICROPY_HW_I2C1_SDA 8


static machine_hw_i2c_obj_t machine_hw_i2c_obj[I2C_MAX];

static void machine_hw_i2c_init(machine_hw_i2c_obj_t *self, uint32_t freq, uint32_t timeout_us, bool first_init) {
    if (!first_init) {
        luat_i2c_close(self->id);
    }
    int speed = freq == 100000 ? 0 : 1;
    luat_debug_print_onoff(0);
    luat_i2c_setup(self->id, speed);
    luat_debug_print_onoff(1);
    
    self->timeout = timeout_us / 1000;
    luat_i2c_set_global_timeout(timeout_us / 1000);
}

int machine_hw_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int data_len = 0;
    int err = 0;
    luat_debug_print_onoff(0);
    if(bufs != NULL && n > 0) {
        if(flags & MP_MACHINE_I2C_FLAG_READ) {
            for (; n--; ++bufs) {
                // LUAT_DEBUG_PRINT("I2C read start");
                err = luat_i2c_recv(self->id, addr, (uint8_t*)bufs->buf, bufs->len);
                // LUAT_DEBUG_PRINT("I2C read %i bytes (%s). Err: %i", bufs->len, (char*)bufs->buf, (uint8_t)err);
                data_len += bufs->len;
            }
            // LUAT_DEBUG_PRINT("I2C total read %i bytes", data_len);
        } else {
            for (; n--; ++bufs) {
                // LUAT_DEBUG_PRINT("I2C write start");
                if(bufs->buf != NULL && bufs->len > 0) {
                    // LUAT_DEBUG_PRINT("I2C write normal");
                    err = luat_i2c_send(self->id, addr, (uint8_t*)bufs->buf, bufs->len, 1);
                } else {
                    // LUAT_DEBUG_PRINT("I2C write NULL");
                    uint8_t buff = 0xFF;
                    err = luat_i2c_send(self->id, addr, (uint8_t*)&buff, 1, 1);
                }
                // LUAT_DEBUG_PRINT("I2C wrote %i bytes (%s). Err: %i", bufs->len, (char*)bufs->buf, (uint8_t)err);
                data_len += bufs->len;
            }
            // LUAT_DEBUG_PRINT("I2C total wrote %i bytes", data_len);
        }
    }
    luat_debug_print_onoff(1);
    if(err) {
    	switch(err) {
			case -ERROR_OPERATION_FAILED:return -MP_ENODEV;
			case -ERROR_TIMEOUT:return -MP_ETIMEDOUT;
			default:return -abs(err);
		}
    }
    return data_len;
}

/******************************************************************************/
// MicroPython bindings for machine API

static void machine_hw_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int h, l;
    mp_printf(print, "I2C(%u, scl=%u, sda=%u, freq=%u, timeout=%ums)", self->id, self->scl, self->sda, self->freq, self->timeout);
}

mp_obj_t machine_hw_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Parse args
    enum { ARG_id, ARG_scl, ARG_sda, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = I2C_ID0} },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = I2C_DEFAULT_TIMEOUT_US} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus
    int i2c_id = args[ARG_id].u_int;

    // Check if the I2C bus is valid
    if (i2c_id < I2C_ID0 || i2c_id >= I2C_MAX) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist (SoC unsupported)"), i2c_id);
    }
    #if !defined(RTE_I2C0)
    if(i2c_id == 0) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist (not configured)"), i2c_id);
    #endif
    #if !defined(RTE_I2C1)
    if(i2c_id == 1) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist (not configured)"), i2c_id);
    #endif

    // Get static peripheral object
    machine_hw_i2c_obj_t *self = (machine_hw_i2c_obj_t *)&machine_hw_i2c_obj[i2c_id];

    bool first_init = false;
    if (self->base.type == NULL) {
        // Created for the first time, set default pins
        self->base.type = &machine_i2c_type;
        self->id = i2c_id;
        if (self->id == I2C_ID0) {
            self->scl = MICROPY_HW_I2C0_SCL;
            self->sda = MICROPY_HW_I2C0_SDA;
        } else {
            self->scl = MICROPY_HW_I2C1_SCL;
            self->sda = MICROPY_HW_I2C1_SDA;
        }
        first_init = true;
    }

    if (args[ARG_freq].u_int == 100000 || args[ARG_freq].u_int == 400000) {
        self->freq = args[ARG_freq].u_int;
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C frequency should be 100000 or 400000"));
    }


    // Initialise the I2C peripheral
    machine_hw_i2c_init(self, args[ARG_freq].u_int, args[ARG_timeout].u_int, first_init);

    return MP_OBJ_FROM_PTR(self);
}

static const mp_machine_i2c_p_t machine_hw_i2c_p = {
    // .transfer_supports_write1 = true,
    .transfer = machine_hw_i2c_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_i2c_make_new,
    print, machine_hw_i2c_print,
    protocol, &machine_hw_i2c_p,
    locals_dict, &mp_machine_i2c_locals_dict
    );

/*
# Test cases
from machine import I2C
i2c = I2C(0, timeout=500000)
devs = i2c.scan();dev = devs[0]; com = "ping"
i2c.writeto(dev, bytearray(com, "utf-8"))
i2c.readfrom(dev, 1)
*/