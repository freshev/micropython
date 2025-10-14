/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2023 Damien P. George
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

// TODO: wake pin processing

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/modmachine.h"
#include "extmod/virtpin.h"
#include "mphalport.h"

#include "platform_define.h"
#include "luat_gpio.h"
#include "luat_debug.h"

typedef struct _machine_pin_irq_obj_t {
    mp_obj_base_t base;
    uint8_t trigger;
} machine_pin_irq_obj_t;

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    uint8_t pin;
    uint8_t phys;
    machine_pin_irq_obj_t irq;
} machine_pin_obj_t;

const mp_obj_type_t machine_pin_irq_type;
const mp_obj_type_t machine_pin_type;

machine_pin_obj_t machine_pin_obj_table[] = {
    {{&machine_pin_type},  0, HAL_GPIO_0, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  1, HAL_GPIO_1, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  2, HAL_GPIO_2, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  3, HAL_GPIO_3, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  4, HAL_GPIO_4, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  5, HAL_GPIO_5, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  6, HAL_GPIO_6, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  7, HAL_GPIO_7, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  8, HAL_GPIO_8, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  9, HAL_GPIO_9, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  10, HAL_GPIO_10, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  11, HAL_GPIO_11, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  12, HAL_GPIO_12, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  13, HAL_GPIO_13, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  14, HAL_GPIO_14, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  15, HAL_GPIO_15, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  16, HAL_GPIO_16, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  17, HAL_GPIO_17, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  18, HAL_GPIO_18, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  19, HAL_GPIO_19, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  20, HAL_GPIO_20, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  21, HAL_GPIO_21, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  22, HAL_GPIO_22, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  23, HAL_GPIO_23, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  24, HAL_GPIO_24, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  25, HAL_GPIO_25, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  26, HAL_GPIO_26, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  27, HAL_GPIO_27, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  28, HAL_GPIO_28, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  29, HAL_GPIO_29, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  30, HAL_GPIO_30, {{&machine_pin_irq_type}}},
    {{&machine_pin_type},  31, HAL_GPIO_31, {{&machine_pin_irq_type}}}
};

// Return the mp_hal_pin_obj_t index for a given machine_pin_obj_t pointer.
#define PIN_OBJ_PTR_INDEX(self) ((self) - &machine_pin_obj_table[0])
// Return the machine_pin_obj_t pointer corresponding to a machine_pin_irq_obj_t pointer.
#define PIN_OBJ_PTR_FROM_IRQ_OBJ_PTR(self) ((machine_pin_obj_t *)((uintptr_t)(self) - offsetof(machine_pin_obj_t, irq)))

void modmachine_pin_init0(void) {
    static bool did_install = false;
    if (!did_install) {
        //gpio_install_isr_service(0);
        did_install = true;
    }
    memset(&MP_STATE_PORT(mp_machine_pin_irq_handler[0]), 0, sizeof(MP_STATE_PORT(mp_machine_pin_irq_handler)));
}

void modmachine_pin_deinit0(void) {
    for (int i = 0; i < (int)MP_ARRAY_SIZE(machine_pin_obj_table); ++i) {
        if (machine_pin_obj_table[i].base.type != NULL) {
            //gpio_isr_handler_remove(i);
            luat_gpio_close(machine_pin_obj_table[i].pin);
        }
    }
}

// IRQ handler 
// to avoid module freeze when trigger == IRQ_LOW_LEVEL or IRQ_HIGH_LEVEL, call "luat_gpio_ctrl" with "LUAT_GPIO_NO_IRQ" parameter
// After that reinstall IRQ_LOW_LEVEL or IRQ_HIGH_LEVEL, calling Pin.irq(...)
static int mp_machine_pin_isr_handler(int pin, void *arg) {

    machine_pin_obj_t pin_obj = machine_pin_obj_table[pin];
    uint8_t trigger = pin_obj.irq.trigger;

    if(trigger == LUAT_GPIO_HIGH_IRQ || trigger == LUAT_GPIO_LOW_IRQ) {
        // reset IRQ mode
        luat_gpio_ctrl(pin, LUAT_GPIO_CMD_SET_IRQ_MODE, LUAT_GPIO_NO_IRQ);
    }

    mp_obj_t handler = MP_STATE_PORT(mp_machine_pin_irq_handler)[pin];
    if(handler != MP_OBJ_NULL) {
        mp_sched_schedule(handler, MP_OBJ_FROM_PTR(&machine_pin_obj_table[pin]));
    }

    // WAKEUP
    mp_hal_wake_main_task_from_isr();
    return 0;
}

static const machine_pin_obj_t *machine_pin_find(mp_obj_t pin_in) { 
    if (mp_obj_is_type(pin_in, &machine_pin_type)) {
        return pin_in;
    }

    // Try to find the pin via integer index into the array of all pins.
    if (mp_obj_is_int(pin_in)) {
        int wanted_pin = mp_obj_get_int(pin_in);
        if (0 <= wanted_pin && wanted_pin < (int)MP_ARRAY_SIZE(machine_pin_obj_table)) {
            const machine_pin_obj_t *self = (machine_pin_obj_t *)&machine_pin_obj_table[wanted_pin];
            if (self->base.type != NULL) {
                return self;
            }
        }
    }
    mp_raise_ValueError(MP_ERROR_TEXT("Invalid pin"));
}

mp_hal_pin_obj_t machine_pin_get_id(mp_obj_t pin_in) {
    const machine_pin_obj_t *self = machine_pin_find(pin_in);
    return PIN_OBJ_PTR_INDEX(self);
}

static void mp_machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "Pin(%u)", PIN_OBJ_PTR_INDEX(self));
}

// pin.init(mode=None, pull=-1, *, value, drive, hold)
static mp_obj_t mp_machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value, ARG_drive, ARG_hold };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(-1)}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_drive, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_hold, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_hal_pin_obj_t index = PIN_OBJ_PTR_INDEX(self);

    // do not use GPIOs: 14, 18, 19, 23, 20(Rising DNTW), 21(Rising, Falling DNTW), 22(Rising, Falling DNTW)
    if(index == 14 || index == 23) {
        mp_raise_ValueError(MP_ERROR_TEXT("Failed. GPIO14 and GPIO23 use prohibited")); 
    }
    if(index == 18 || index == 19) {
        mp_raise_ValueError(MP_ERROR_TEXT("Failed. GPIO18 and GPIO19 used for UART1")); 
    }
    if(index == 20 || index == 21 || index == 22) {
        mp_printf(&mp_plat_print, "Warning: GPIO20, GPIO21 and GPIO22 used for WAKEUP\n");
    }

    // reset the pin to digital if this is a mode-setting init (grab it back from ADC)
    if (args[ARG_mode].u_obj != mp_const_none) { 
        luat_gpio_close(index); 
    }

    // configure the pin for gpio
    luat_gpio_cfg_t gpio_cfg;
    luat_gpio_set_default_cfg(&gpio_cfg); // set all to zeroes (mode = OUTPUT, level = LOW, PULL = OPEN_DRAIN)
    gpio_cfg.pin = index;

    // set initial value
    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
        gpio_cfg.output_level = mp_obj_is_true(args[ARG_value].u_obj);
    }

    // configure mode
    if (args[ARG_mode].u_obj != mp_const_none) {
        mp_int_t pin_io_mode = mp_obj_get_int(args[ARG_mode].u_obj);
        gpio_cfg.mode = pin_io_mode;
        if(pin_io_mode == LUAT_GPIO_OUTPUT && (index == 20 || index == 21 || index == 22)) {
             mp_printf(&mp_plat_print, "Warning: GPIO20, GPIO21 and GPIO22 can not be set to OUTPUT mode\n");
        }
    }

    // configure pull
    if (args[ARG_pull].u_obj != MP_OBJ_NEW_SMALL_INT(-1)) {
        int pull = 0;
        if (args[ARG_pull].u_obj != mp_const_none) pull = mp_obj_get_int(args[ARG_pull].u_obj);
        gpio_cfg.pull = pull;
        if(gpio_cfg.mode == LUAT_GPIO_INPUT) { // Pin.IN
            mp_printf(&mp_plat_print, "Warning: Pull up/down can not be set in INPUT mode. Please use external pull\n");
        }
    }
    luat_gpio_open(&gpio_cfg);

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    const machine_pin_obj_t *self = machine_pin_find(args[0]);

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        mp_machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
static mp_obj_t mp_machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    mp_hal_pin_obj_t index = PIN_OBJ_PTR_INDEX(self);
    if (n_args == 0) {
        // get pin
        return MP_OBJ_NEW_SMALL_INT(luat_gpio_get(index));
    } else {
        // set pin
        luat_gpio_set(index, mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}

// pin.init(mode, pull)
static mp_obj_t mp_machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return mp_machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, mp_machine_pin_obj_init);

// pin.value([value])
static mp_obj_t mp_machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return mp_machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, mp_machine_pin_value);

// pin.off()
static mp_obj_t mp_machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    luat_gpio_set(PIN_OBJ_PTR_INDEX(self), 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, mp_machine_pin_off);

// pin.on()
static mp_obj_t mp_machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    luat_gpio_set(PIN_OBJ_PTR_INDEX(self), 1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, mp_machine_pin_on);

// pin.irq(handler = None, trigger = IRQ_FALLING | IRQ_RISING | IRQ_LOW_LEVEL | IRQ_HIGH_LEVEL)
static mp_obj_t mp_machine_pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger, ARG_wake };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = LUAT_GPIO_RISING_IRQ | LUAT_GPIO_FALLING_IRQ } },
        { MP_QSTR_wake, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (n_args > 1 || kw_args->used != 0) {

        // configure irq
        mp_hal_pin_obj_t index = PIN_OBJ_PTR_INDEX(self);
        mp_obj_t handler = args[ARG_handler].u_obj;
        uint32_t trigger = args[ARG_trigger].u_int;
        mp_obj_t wake_obj = args[ARG_wake].u_obj;

        //LUAT_DEBUG_PRINT("index = %d, trigger = %d, self = %p", index, trigger, self);

        if ((trigger == LUAT_GPIO_LOW_IRQ || trigger == LUAT_GPIO_HIGH_IRQ) && wake_obj != mp_const_none) {
            mp_int_t wake;
            if (mp_obj_get_int_maybe(wake_obj, &wake)) {
                if (wake < 2 || wake > 7) {
                    mp_raise_ValueError(MP_ERROR_TEXT("Bad wake value"));
                }
            } else {
                mp_raise_ValueError(MP_ERROR_TEXT("Bad wake value"));
            }

            // TODO: wake pin config
            /*if (machine_rtc_config.wake_on_touch) { // not compatible
                mp_raise_ValueError(MP_ERROR_TEXT("no resources"));
            }

            if (!RTC_IS_VALID_EXT_PIN(index)) {
                mp_raise_ValueError(MP_ERROR_TEXT("invalid pin for wake"));
            }

            if (machine_rtc_config.ext0_pin == -1) {
                machine_rtc_config.ext0_pin = index;
            } else if (machine_rtc_config.ext0_pin != index) {
                mp_raise_ValueError(MP_ERROR_TEXT("no resources"));
            }

            machine_rtc_config.ext0_level = trigger == GPIO_INTR_LOW_LEVEL ? 0 : 1;
            machine_rtc_config.ext0_wake_types = wake;
            */

        } else {
            if (handler == mp_const_none) {
                handler = MP_OBJ_NULL;
                trigger = 0;
            }
            uint8_t level = luat_gpio_get(index);
            if(trigger == LUAT_GPIO_HIGH_IRQ && level == 1) {
                mp_raise_ValueError(MP_ERROR_TEXT("You should set pin level LOW first"));
            }
            if(trigger == LUAT_GPIO_LOW_IRQ && level == 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("You should set pin level HIGH first"));
            }

            luat_gpio_ctrl(index, LUAT_GPIO_CMD_SET_IRQ_MODE, LUAT_GPIO_NO_IRQ);
            MP_STATE_PORT(mp_machine_pin_irq_handler)[index] = handler;
            luat_gpio_ctrl(index, LUAT_GPIO_CMD_SET_IRQ_MODE, trigger);
            luat_gpio_set_irq_cb(index, mp_machine_pin_isr_handler, (void *)self);
            machine_pin_obj_table[index].irq.trigger = trigger;
        }
    }
    // return the irq object
    return MP_OBJ_FROM_PTR(&self->irq);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, mp_machine_pin_irq);

static const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_pin_irq_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(LUAT_GPIO_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(LUAT_GPIO_OUTPUT) },

    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(LUAT_GPIO_DEFAULT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(LUAT_GPIO_PULLUP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(LUAT_GPIO_PULLDOWN) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(LUAT_GPIO_RISING_IRQ) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(LUAT_GPIO_FALLING_IRQ) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_LOW_LEVEL), MP_ROM_INT(LUAT_GPIO_LOW_IRQ) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_HIGH_LEVEL), MP_ROM_INT(LUAT_GPIO_HIGH_IRQ) },
};

static mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = self_in;
    mp_hal_pin_obj_t index = PIN_OBJ_PTR_INDEX(self);

    switch (request) {
        case MP_PIN_READ: {
            return luat_gpio_get(index);
        }
        case MP_PIN_WRITE: {
            luat_gpio_set(index, arg);
            return 0;
        }
    }
    return -1;
}

static MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

static const mp_pin_p_t pin_pin_p = {
    .ioctl = pin_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, mp_pin_make_new,
    print, mp_machine_pin_print,
    call, mp_machine_pin_call,
    protocol, &pin_pin_p,
    locals_dict, &machine_pin_locals_dict
    );

/******************************************************************************/
// Pin IRQ object

static mp_obj_t mp_machine_pin_irq_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = self_in;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    machine_pin_obj_t *self_pin = PIN_OBJ_PTR_FROM_IRQ_OBJ_PTR(self);
    int index = self_pin->pin;
    mp_machine_pin_isr_handler(index, (void *)PIN_OBJ_PTR_FROM_IRQ_OBJ_PTR(self));
    return mp_const_none;
}

static mp_obj_t mp_machine_pin_irq_trigger(size_t n_args, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = args[0];
    mp_hal_pin_obj_t index = PIN_OBJ_PTR_INDEX(PIN_OBJ_PTR_FROM_IRQ_OBJ_PTR(self));
    uint32_t old_trig = machine_pin_obj_table[index].irq.trigger;
    //LUAT_DEBUG_PRINT("index = %d, old trigger = %d", index, old_trig);

    if (n_args == 2) {
        // set trigger
        uint8_t level = luat_gpio_get(index);
        int trigger = mp_obj_get_int(args[1]);
        if(trigger == LUAT_GPIO_HIGH_IRQ && level == 1) {
            mp_raise_ValueError(MP_ERROR_TEXT("You should set pin level LOW first!"));
        }
        if(trigger == LUAT_GPIO_LOW_IRQ && level == 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("You should set pin level HIGH first!"));
        }
        luat_gpio_ctrl(index, LUAT_GPIO_CMD_SET_IRQ_MODE, LUAT_GPIO_NO_IRQ);
        luat_gpio_ctrl(index, LUAT_GPIO_CMD_SET_IRQ_MODE, trigger);
        machine_pin_obj_table[index].irq.trigger = trigger;
    }
    // return original trigger value
    return MP_OBJ_NEW_SMALL_INT(old_trig);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_pin_irq_trigger_obj, 1, 2, mp_machine_pin_irq_trigger);

static const mp_rom_map_elem_t machine_pin_irq_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_trigger), MP_ROM_PTR(&mp_machine_pin_irq_trigger_obj) },
};
static MP_DEFINE_CONST_DICT(machine_pin_irq_locals_dict, machine_pin_irq_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_irq_type,
    MP_QSTR_IRQ,
    MP_TYPE_FLAG_NONE,
    call, mp_machine_pin_irq_call,
    locals_dict, &machine_pin_irq_locals_dict
    );

MP_REGISTER_ROOT_POINTER(mp_obj_t mp_machine_pin_irq_handler[32]); // HAL_GPIO_MAX


/* 
# Test cases
def run(pin): print("Fired on ", pin)

from machine import Pin 
p=Pin(20,Pin.OUT)
p.value(1)

#p.irq(handler=run, trigger=Pin.IRQ_HIGH_LEVEL)
#p.irq(handler=run, trigger=Pin.IRQ_LOW_LEVEL)
p.irq(handler=run, trigger=Pin.IRQ_RISING) # 0
#p.irq(handler=run, trigger=Pin.IRQ_FALLING) # 1

p.value(0)
p.value(1)
p.value(0)
p.value(1)

p.irq().trigger(Pin.IRQ_LOW_LEVEL)
*/