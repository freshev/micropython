/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Nick Moore
 * Copyright (c) 2021 Jonathan Hogg
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

#include "luat_adc.h"
#include "luat_debug.h"

// The description of adc0 and adc1:
// 1. The maximum input voltage allowed by the ADC pin is 3.4V
// 2. If internal voltage divider is used and no external voltage divider is used, the software can set the measurement range 
//    by calling the luat_adc_ctrl interface, which supports 1.2V, 1.4V, 1.6, 1.9, 2.4, 2.7, 3.2, and 3.8V ranges;
// 3. If external voltage divider is used and internal voltage is not used, the software can set the range to 1.2V 
//    by calling the luat_adc_ctrl interface;
// 4. It is not recommended to activate internal divider and external divider at the same time, as this ADC error will become larger.

// If external input voltage exceeds 3.4v:
// Only external voltage divider can be used, and no internal voltage divider is used. 
// The software sets the range to 1.2V by calling the luat_adc_ctrl interface;

// External input voltage, if it does not exceed 3.4V:
// 1. You can use external voltage divider and no internal voltage divider. 
//    The software can set the range to 1.2V by calling the luat_adc_ctrl interface;
// 2. You can also use internal voltage divider and no external voltage divider. 
//    The software calls the luat_adc_ctrl interface to set the range. 
//    If the set range is greater than or equal to the actual input maximum voltage, just select the closest range;
// Considering the hardware cost, the second option is recommended;
// From the perspective of measurement accuracy, in the second method, 12bit represents the maximum range of the setting, 
// and the maximum range/4096 is the measurement accuracy; 
// In the first method, calculate and compare by yourself based on the external hardware circuit;

// Just choose one of the internal and external divider; when external partial pressure is not possible, select internal partial pressure;
// If an external voltage divider is used, it is recommended to do it in one step. 
// After the external voltage divider, the input voltage is in the range of 0.1-1.2V (the pass-through mode is used internally, 
// and no voltage divider is required). The closer to 1.2V, the better, and the higher the accuracy.
// If internal voltage divider is used, the closer the configured range is to 1.2V, the better, and the higher the accuracy, 
// provided that the measurement requirements are met.

// ADC has the internal voltage divider turned on by default, and the configured range is 3.8V;
// The following three lines of commented-out code set the range of ADC0 to 1.2V and turn off the internal voltage divider.
// It is recommended to use an external voltage divider;
// If the external voltage dividing method is used, the user can open the following three lines of code according to his own needs;
// luat_adc_ctrl_param_t ctrl_param;
// ctrl_param.range = LUAT_ADC_AIO_RANGE_1_2;
// luat_adc_ctrl(0, LUAT_ADC_SET_GLOBAL_RANGE, ctrl_param);


//The descriptions of LUAT_ADC_CH_CPU and LUAT_ADC_CH_VBAT are as follows:
// LUAT_ADC_CH_CPU is used internally by the chip to detect temperature. 
// It only supports open, read and close operations on the software. Users cannot use it for other purposes;
// LUAT_ADC_CH_VBAT is used internally by the chip to detect the VBAT voltage. 
// It only supports open, read and close operations in the software. Users cannot use it for other purposes;

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    uint8_t channel_id;
    LUAT_ADC_RANGE_E range;
    uint8_t maxV;
    uint8_t atten;
    bool active;
} machine_adc_obj_t;


machine_adc_obj_t machine_adc_obj_table[] = {
    {{&machine_adc_type}, 0},
    {{&machine_adc_type}, 1}
};

#define ADC_ATTEN_DB_0   (0)
#define ADC_ATTEN_DB_1_5 (1)
#define ADC_ATTEN_DB_3_0 (2)
#define ADC_ATTEN_DB_4_0 (3)
#define ADC_ATTEN_DB_6_0 (4)
#define ADC_ATTEN_DB_7_5 (5)
#define ADC_ATTEN_DB_8_7 (6)
#define ADC_ATTEN_DB_10  (7)

#define MICROPY_PY_MACHINE_ADC_CLASS_CONSTANTS \
    { MP_ROM_QSTR(MP_QSTR_ATTN_0DB), MP_ROM_INT(ADC_ATTEN_DB_0) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_1_5DB), MP_ROM_INT(ADC_ATTEN_DB_1_5) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_3_0DB), MP_ROM_INT(ADC_ATTEN_DB_3_0) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_4_0DB), MP_ROM_INT(ADC_ATTEN_DB_4_0) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_6_0DB), MP_ROM_INT(ADC_ATTEN_DB_6_0) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_7_5DB), MP_ROM_INT(ADC_ATTEN_DB_7_5) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_8_7DB), MP_ROM_INT(ADC_ATTEN_DB_8_7) }, \
    { MP_ROM_QSTR(MP_QSTR_ATTN_10DB), MP_ROM_INT(ADC_ATTEN_DB_10) }

void modmachine_adc_init0(void) {
    int i ;
    for(int i = 0; i < 2; i++) {
        machine_adc_obj_table[i].active = (luat_adc_open(i, NULL) == 0);
    }
}
void modmachine_adc_deinit0(void) {
    for(int i = 0; i < 2; i++) luat_adc_close(i);
}


static void mp_machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "ADC(channel=%u, atten=%0.1f, range=0-%0.1fV)", self->channel_id, self->atten/10.0, self->maxV/10.0);
}

void mp_machine_adc_init_helper_internal(machine_adc_obj_t *self, size_t n_pos_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_atten };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_atten, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_pos_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t maxV = 12;
    uint8_t atten = 0;
    LUAT_ADC_RANGE_E range = LUAT_ADC_AIO_RANGE_1_2 ;
    mp_int_t atten_in = args[ARG_atten].u_int;
    
    switch(atten_in) {
        case ADC_ATTEN_DB_0:   atten = 00;break;
        case ADC_ATTEN_DB_1_5: atten = 15;break;
        case ADC_ATTEN_DB_3_0: atten = 30;break;
        case ADC_ATTEN_DB_4_0: atten = 40;break;
        case ADC_ATTEN_DB_6_0: atten = 60;break;
        case ADC_ATTEN_DB_7_5: atten = 75;break;
        case ADC_ATTEN_DB_8_7: atten = 87;break;
        case ADC_ATTEN_DB_10: atten = 100;break;
    }

    if(atten >= 100) { range = LUAT_ADC_AIO_RANGE_1_2; maxV = 12; }
    else if(atten >= 87 && atten < 100)  { range = LUAT_ADC_AIO_RANGE_1_4; maxV = 14; }
    else if(atten >= 75 && atten < 87) { range = LUAT_ADC_AIO_RANGE_1_6; maxV = 16; }
    else if(atten >= 60 && atten < 75) { range = LUAT_ADC_AIO_RANGE_1_9; maxV = 19; }
    else if(atten >= 40 && atten < 60) { range = LUAT_ADC_AIO_RANGE_2_4; maxV = 24; }
    else if(atten >= 30 && atten < 40) { range = LUAT_ADC_AIO_RANGE_2_7; maxV = 27; }
    else if(atten >= 15 && atten < 30) { range = LUAT_ADC_AIO_RANGE_3_2; maxV = 32; }
    else if(atten < 15)                 { range = LUAT_ADC_AIO_RANGE_3_8; maxV = 38; }
    self->atten = atten;
    self->range = range;
    self->maxV = maxV;
    //LUAT_DEBUG_PRINT("ADC channel %u, atten = %f, range = %u, maxV = %f", self->channel_id, self->atten/10.0, self->range, self->maxV/10.0);
    
    luat_adc_ctrl_param_t ctrl_param;
    ctrl_param.range = self->range;
    luat_adc_ctrl(self->channel_id, LUAT_ADC_SET_GLOBAL_RANGE, ctrl_param);
}

static void mp_machine_adc_init_helper(machine_adc_obj_t *self, size_t n_pos_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_machine_adc_init_helper_internal(self, n_pos_args, pos_args, kw_args);
}

static mp_obj_t mp_machine_adc_make_new(const mp_obj_type_t *type, size_t n_pos_args, size_t n_kw_args, const mp_obj_t *args) {
    mp_arg_check_num(n_pos_args, n_kw_args, 1, MP_OBJ_FUN_ARGS_MAX, true);
    uint8_t adc_channel = machine_pin_get_id(args[0]);
    
    if (adc_channel > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid ADC channel"));
    }
    if(!machine_adc_obj_table[adc_channel].active) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("ADC channel %u not active"), adc_channel);
    }

    machine_adc_obj_t *self = &machine_adc_obj_table[adc_channel];

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw_args, args + n_pos_args);
    mp_machine_adc_init_helper_internal(self, n_pos_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

static mp_int_t mp_machine_adc_read_u16(machine_adc_obj_t *self) {
    int val1, val2;
    luat_adc_read(self->channel_id , &val1, &val2);
    mp_int_t u16 = val1;
    return u16;
}

static mp_int_t mp_machine_adc_read_uv(machine_adc_obj_t *self) {
    int val1, val2;
    luat_adc_read(self->channel_id , &val1, &val2);
    mp_int_t uv = val2;
    return uv;
}

/*
# Test cases
from machine import ADC
adc = ADC(1, atten=ADC.ATTN_10DB)
print(adc)
adc.read_u16()
adc.read_uv()

*/