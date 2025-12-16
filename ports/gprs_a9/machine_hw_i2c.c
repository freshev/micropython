/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 nk2IsHere
 * Copyright (c) 2019 pulkin
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

#include "errno.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_hal_i2c.h"
#include "extmod/modmachine.h"

#define I2C_NUM_0   (1) /* I2C_ID_t.I2C1 */
#define I2C_NUM_MAX (4) /* I2C_ID_t.I2C_ID_MAX */

//#define I2C_DEFAULT_TIMEOUT_US (50000) // 50ms

typedef struct _machine_hw_i2c_obj_t {
    mp_obj_base_t base;
    I2C_ID_t id;
    I2C_FREQ_t freq;
    mp_int_t timeout; // in MS, not US
} machine_hw_i2c_obj_t;

static machine_hw_i2c_obj_t machine_hw_i2c_obj[I2C_NUM_MAX];


// ----------
// Exceptions
// ----------

I2C_FREQ_t mp_machine_i2c_private_get_I2C_FREQ(mp_int_t _frequency) {
    I2C_FREQ_t frequency;
    switch(_frequency) {
        case 400000:
            frequency = I2C_FREQ_400K;
            break;
        default:
            frequency = I2C_FREQ_100K;
    }
    return frequency;
}
mp_int_t mp_machine_i2c_private_get_freq(I2C_FREQ_t _frequency) {
    mp_int_t frequency;
    switch(_frequency) {
        case I2C_FREQ_400K:
            frequency = 400000;
            break;
        default:
            frequency = 100000;
    }
    return frequency;
}


void mp_machine_i2c_private_throw_I2C_Error(I2C_Error_t error) {
}

// ---------
// Methods
// ---------
static void machine_hw_i2c_init(machine_hw_i2c_obj_t *self, uint32_t freq, uint32_t timeout_us, bool first_init) {
    if (!first_init) {
        if(!I2C_Close(self->id)) {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C deinit fail"));
        }
    }
    I2C_Config_t config;
    self->freq = config.freq = mp_machine_i2c_private_get_I2C_FREQ(freq);
    self->timeout = timeout_us / 1000;
    int ret = I2C_Init(self->id, config);
    if(!ret) mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C init fail (err=%d)"), ret);
}

int machine_hw_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    I2C_Error_t err = I2C_ERROR_NONE;
    int data_len = 0;
    if(bufs != NULL && n > 0) {
        if(flags & MP_MACHINE_I2C_FLAG_READ) {
            for (; n--; ++bufs) {
                //Trace(1, "I2C read start");
                err = I2C_Receive(self->id, addr, (uint8_t*)bufs->buf, bufs->len, self->timeout);
                //Trace(1, "I2C read %i bytes (%s). Err: %i", bufs->len, (char*)bufs->buf, (uint8_t)err);
                data_len += bufs->len;
            }
            //Trace(1, "I2C total read %i bytes", data_len);
        } else {
            for (; n--; ++bufs) {
                // Trace(1, "I2C write start");
                if(bufs->buf != NULL && bufs->len > 0) {
                    // Trace(1, "I2C write normal");
                    err = I2C_Transmit(self->id, addr, (uint8_t*)bufs->buf, bufs->len, self->timeout);
                } else {
                    // Trace(1, "I2C write NULL");
                    uint8_t buff = 0xFF;
                    err = I2C_Transmit(self->id, addr, (uint8_t*)&buff, 1, self->timeout);
                }
                //Trace(1, "I2C wrote %i bytes (%s). Err: %i", bufs->len, (char*)bufs->buf, (uint8_t)err);
                data_len += bufs->len;
            }
            //Trace(1, "I2C total wrote %i bytes", data_len);
        }
    }

    switch(err) {
        case I2C_ERROR_RESOURCE_RESET:
            return -EBUSY;
        case I2C_ERROR_RESOURCE_BUSY:
            return -EBUSY;
        case I2C_ERROR_RESOURCE_TIMEOUT:
            return -ETIMEDOUT;
        case I2C_ERROR_RESOURCE_NOT_ENABLED:
            return -EACCES;
        case I2C_ERROR_BAD_PARAMETER:
            return -EINVAL;
        case I2C_ERROR_COMMUNICATION_FAILED:
            return  -ENOTCONN;
        default:
            break;
    }
    return data_len;
}

/******************************************************************************/
// MicroPython bindings for machine API

static void machine_hw_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "I2C(%u, freq=%u, timeout=%ums)", self->id, mp_machine_i2c_private_get_freq(self->freq), self->timeout);
}

mp_obj_t machine_hw_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    MP_MACHINE_I2C_CHECK_FOR_LEGACY_SOFTI2C_CONSTRUCTION(n_args, n_kw, all_args);

    // Parse args
    enum { ARG_id, ARG_scl, ARG_sda, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = I2C_DEFAULT_TIME_OUT * 1000} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus
    mp_int_t i2c_id = mp_obj_get_int(args[ARG_id].u_obj);
    if (!(I2C_NUM_0 <= i2c_id && i2c_id < I2C_NUM_MAX)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }
    // Get static peripheral object
    machine_hw_i2c_obj_t *self = (machine_hw_i2c_obj_t *)&machine_hw_i2c_obj[i2c_id];

    bool first_init = false;
    if (self->base.type == NULL) {
        // Created for the first time, set default pins
        self->base.type = &machine_i2c_type;
        first_init = true;
    }
    self->id = i2c_id;
    // Initialise the I2C peripheral
    machine_hw_i2c_init(self, args[ARG_freq].u_int, args[ARG_timeout].u_int, first_init);

    return MP_OBJ_FROM_PTR(self);
}

static const mp_machine_i2c_p_t machine_hw_i2c_p = {
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
