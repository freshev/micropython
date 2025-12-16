/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
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

// This file is never compiled standalone, it's included directly from
// extmod/machine_adc.c via MICROPY_PY_MACHINE_ADC_INCLUDEFILE.

#include "py/runtime.h"
#include "py/mphal.h"
#include "api_hal_adc.h"

// The ADC class doesn't have any constants for this port.
#define MICROPY_PY_MACHINE_ADC_CLASS_CONSTANTS

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    ADC_Channel_t channel;
} machine_adc_obj_t;

extern const mp_obj_type_t machine_adc_type;

static machine_adc_obj_t machine_adc_0 = {{&machine_adc_type}, ADC_CHANNEL_0};
static machine_adc_obj_t machine_adc_1 = {{&machine_adc_type}, ADC_CHANNEL_1};


static void mp_machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "ADC(%d)", self->channel);
}

static mp_obj_t mp_machine_adc_make_new(const mp_obj_type_t *type,    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 2, false);

    mp_int_t chn = mp_obj_get_int(args[0]);

    machine_adc_obj_t *result;
    switch (chn) {
        case 0:
            result = &machine_adc_0;
            break;
        case 1:
            result = &machine_adc_1;
            break;
        default:
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "ADC(%d) doesn't exist", chn));
    }

    mp_int_t sampling_period = ADC_SAMPLE_PERIOD_100MS;
    if (n_args == 2) {
        sampling_period = mp_obj_get_int(args[1]);
    }

    ADC_Config_t config = {result->channel, sampling_period};
    ADC_Init(config);

    return result;
}

static mp_int_t mp_machine_adc_read_u16(machine_adc_obj_t *self) {
    uint16_t value, value_mV;
    if (!ADC_Read(self->channel, &value, &value_mV)) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to read ADC"));
        return 0;
    }
    return value;
}

