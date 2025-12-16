/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "extmod/modmachine.h"

#include "common_api.h"
#include "luat_gpio.h"
#include "luat_spi.h"
#include "luat_debug.h"

typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    int id;
    int cs;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
    int8_t sck;
    int8_t mosi;
    int8_t miso;
    enum {
        MACHINE_HW_SPI_STATE_NONE,
        MACHINE_HW_SPI_STATE_INIT,
        MACHINE_HW_SPI_STATE_DEINIT
    } state;
} machine_hw_spi_obj_t;

// SPI0 mappings 
// { PAD_PIN23},  // 0 : gpio8  / 1 : SPI0 SSn
// { PAD_PIN24},  // 0 : gpio9  / 1 : SPI0 MOSI
// { PAD_PIN25},  // 0 : gpio10 / 1 : SPI0 MISO
// { PAD_PIN26},  // 0 : gpio11 / 1 : SPI0 SCLK
#define MICROPY_HW_SPI0_SCLK 11
#define MICROPY_HW_SPI0_MOSI 9
#define MICROPY_HW_SPI0_MISO 10
#define MICROPY_HW_SPI0_CS_0 8
#define MICROPY_HW_SPI0_CS_1 3

// SPI1 mappings 
// { PAD_PIN27},  // 0 : gpio12  / 1 : SPI1 SSn
// { PAD_PIN28},  // 0 : gpio13  / 1 : SPI1 MOSI
// { PAD_PIN29},  // 0 : gpio14  / 1 : SPI1 MISO
// { PAD_PIN30},  // 0 : gpio15  / 1 : SPI1 SCLK
#define MICROPY_HW_SPI1_SCLK 15
#define MICROPY_HW_SPI1_MOSI 13
#define MICROPY_HW_SPI1_MISO 14
#define MICROPY_HW_SPI1_CS_0 12
#define MICROPY_HW_SPI1_CS_1 16

#ifdef RTE_SPI1
/** If you want to maintain the unilog function and use SPI1, you need to multiplex the IO of UART0 to other places. See the following operation.*/
extern int32_t soc_unilog_callback(void *pdata, void *param);
bool soc_init_unilog_uart(uint8_t port, uint32_t baudrate, bool startRecv)
{
    
	soc_get_unilog_br(&baudrate);
	// this should be rewritten if using SPI1_CS1 (HAL_GPIO_16)
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_16, 0), 3, 0, 0); // UART0_RXD -> GPIO16
	GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_16, 0), 1, 1);
	//
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_17, 0), 3, 0, 0);   // UART0_TXD -> GPIO17
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_14, 0), 0, 0, 0);	 //The original UART0 TXRX changes back to GPIO function
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_15, 0), 0, 0, 0);
	Uart_BaseInitEx(port, baudrate, 0, 256, UART_DATA_BIT8, UART_PARITY_NONE, UART_STOP_BIT1, soc_unilog_callback);
	return true;
}
#endif

typedef struct _machine_hw_spi_default_pins_t {
    union {
        int8_t array[3];
        struct {
            // Must be in enum's ARG_sck, ARG_mosi, ARG_miso, etc. order
            int8_t sck;
            int8_t mosi;
            int8_t miso;
        } pins;
    };
} machine_hw_spi_default_pins_t;

// Default pin mappings for the hardware SPI instances
static const machine_hw_spi_default_pins_t machine_hw_spi_default_pins[SPI_MAX] = {
    { .pins = { .sck = MICROPY_HW_SPI0_SCLK, .mosi = MICROPY_HW_SPI0_MOSI, .miso = MICROPY_HW_SPI0_MISO }},
    { .pins = { .sck = MICROPY_HW_SPI1_SCLK, .mosi = MICROPY_HW_SPI1_MOSI, .miso = MICROPY_HW_SPI1_MISO }},
};

// Common arguments for init() and make new
enum { ARG_id, ARG_cs, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
static const mp_arg_t spi_allowed_args[] = {
    { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_cs,       MP_ARG_INT, {.u_int = 0 } },
    { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
};

// Static objects mapping to SPI0 & SPI1 hardware peripherals.
static machine_hw_spi_obj_t machine_hw_spi_obj[SPI_MAX];

static void machine_hw_spi_deinit_internal(machine_hw_spi_obj_t *self) {
    if(luat_spi_close(self->id) != 0) {
       mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
    }
    // SPI0_SSn1 and SPI1_SSn1 currently not supported by luatos SDK
    // dead code
    if(self->cs == 1) {
        if(self->id == 0) GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_3, 0), 0, 0, 0);
        if(self->id == 1) GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_16, 0), 0, 0, 0);
    }
    // end of dead code
}

static void machine_hw_spi_init_internal(machine_hw_spi_obj_t *self, mp_arg_val_t args[]) {

    // if we're not initialized, then we're
    // implicitly 'changed', since this is the init routine
    bool changed = self->state != MACHINE_HW_SPI_STATE_INIT;

    machine_hw_spi_obj_t old_self = *self;

    if (args[ARG_cs].u_int != -1 && args[ARG_cs].u_int != self->cs) {
        self->cs = args[ARG_cs].u_int;
        changed = true;
    }
    if(self->cs != 0) mp_raise_ValueError(MP_ERROR_TEXT("CS value not supported"));

    if (args[ARG_baudrate].u_int != -1) {
        uint32_t baudrate = args[ARG_baudrate].u_int;
        if(baudrate < 100000) {
            mp_raise_ValueError(MP_ERROR_TEXT("SPI baudrate should be more than 100000"));
        }
        if(baudrate > 25600000) {
            mp_raise_ValueError(MP_ERROR_TEXT("SPI baudrate should be less than 25600000"));
        }
        if (baudrate != self->baudrate) {
            self->baudrate = baudrate;
            changed = true;
        }
    }

    if (args[ARG_polarity].u_int != -1 && args[ARG_polarity].u_int != self->polarity) {
        self->polarity = args[ARG_polarity].u_int;
        changed = true;
    }

    if (args[ARG_phase].u_int != -1 && args[ARG_phase].u_int != self->phase) {
        self->phase = args[ARG_phase].u_int;
        changed = true;
    }

    if (args[ARG_bits].u_int != -1 && args[ARG_bits].u_int <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid bits"));
    }

    if (args[ARG_bits].u_int != -1 && args[ARG_bits].u_int != self->bits) {
        self->bits = args[ARG_bits].u_int;
        changed = true;
    }

    if (args[ARG_firstbit].u_int != -1 && args[ARG_firstbit].u_int != self->firstbit) {
        self->firstbit = args[ARG_firstbit].u_int;
        changed = true;
    }
    if (args[ARG_sck].u_int != -2 && args[ARG_sck].u_int != self->sck) {
        self->sck = args[ARG_sck].u_int;
        changed = true;
    }

    if (args[ARG_mosi].u_int != -2 && args[ARG_mosi].u_int != self->mosi) {
        self->mosi = args[ARG_mosi].u_int;
        changed = true;
    }

    if (args[ARG_miso].u_int != -2 && args[ARG_miso].u_int != self->miso) {
        self->miso = args[ARG_miso].u_int;
        changed = true;
    }

    if (changed) {
        if (self->state == MACHINE_HW_SPI_STATE_INIT) {
            self->state = MACHINE_HW_SPI_STATE_DEINIT;
            machine_hw_spi_deinit_internal(&old_self);
        }
    } else {
        return; // no changes
    }

    // SPI0_SSn1 and SPI1_SSn1 currently not supported by luatos SDK
    // dead code
    if(self->cs == 1) {
        if(self->id == 0) GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_3, 0), 4, 1, 0);
        if(self->id == 1) GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_16, 0), 4, 1, 0);
    }
    // end of dead code

    // LUAT_DEBUG_PRINT("SPI id=%d, cs=%d, CPHA=%d, CPOL=%d, dataw=%d, baudrate=%d, sck=%d, mosi=%d, miso=%d", self->id, self->cs, self->phase, self->polarity, self->bits, self->baudrate, self->sck, self->mosi, self->miso);
    luat_spi_t spi_conf = {
        .id = self->id,
        .CPHA = self->phase,
        .CPOL = self->polarity,
        .dataw = self->bits,
        .bit_dict = self->firstbit == MICROPY_PY_MACHINE_SPI_MSB ? 1 : 0,
        .master = 1,
        .mode = 1,             // mode is set to 1, full duplex 
        .bandrate = self->baudrate,
        .cs = self->id == 0 ? (self->cs == 0 ? MICROPY_HW_SPI0_CS_0 : MICROPY_HW_SPI0_CS_1) : (self->cs == 0 ? MICROPY_HW_SPI1_CS_0 : MICROPY_HW_SPI1_CS_1)
    };

    if(luat_spi_setup(&spi_conf) != 0) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
    }

    self->state = MACHINE_HW_SPI_STATE_INIT;
}

static void machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;
    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        machine_hw_spi_deinit_internal(self);
    }
}

static mp_uint_t gcd(mp_uint_t x, mp_uint_t y) {
    while (x != y) {
        if (x > y) {
            x -= y;
        } else {
            y -= x;
        }
    }
    return x;
}

static void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->state == MACHINE_HW_SPI_STATE_DEINIT) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("transfer on deinitialized SPI"));
        return;
    }

    // Round to nearest whole set of bits
    int bits_to_send = len * 8 / self->bits * self->bits;

    if (!bits_to_send) {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer too short"));
    }

    int res;
    res = luat_spi_transfer(self->id, (const char*)src, len, (const char*)dest, len);

    if(res != len) {
        mp_warning(MP_WARN_CAT(RuntimeWarning), "SPI%d received %d/%d bytes", self->id, res, len);
    }
}

/******************************************************************************/
// MicroPython bindings for hw_spi

static void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI%d_CS%d(baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%u, sck=%d, mosi=%d, miso=%d)",
        self->id, self->cs, self->baudrate, self->polarity,
        self->phase, self->bits, self->firstbit,
        self->sck, self->mosi, self->miso);
}

// Set constant values, depending of SPI id
static void machine_hw_spi_argcheck(mp_arg_val_t args[], const machine_hw_spi_default_pins_t *default_pins) {
    for (int i = ARG_sck; i <= ARG_miso; i++) {
        args[i].u_int = default_pins ? default_pins->array[i - ARG_sck] : -1;
    }
}

static void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;

    mp_arg_val_t args[MP_ARRAY_SIZE(spi_allowed_args)];
    // offset arg lists by 1 to skip first arg, id, which is not valid for init()
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(spi_allowed_args) - 1,
        spi_allowed_args + 1, args + 1);

    machine_hw_spi_argcheck(args, &machine_hw_spi_default_pins[self->id]);
    machine_hw_spi_init_internal(self, args);
}

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // MP_MACHINE_SPI_CHECK_FOR_LEGACY_SOFTSPI_CONSTRUCTION(n_args, n_kw, all_args);

    mp_arg_val_t args[MP_ARRAY_SIZE(spi_allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(spi_allowed_args), spi_allowed_args, args);

    const mp_int_t spi_id = args[ARG_id].u_int;
    if (spi_id >= 0 && spi_id < SPI_MAX) {
        machine_hw_spi_argcheck(args, &machine_hw_spi_default_pins[spi_id]);
        #if !defined(RTE_SPI0)
        if(spi_id == 0) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist (not configured)"), spi_id);
        #endif
        #if !defined(RTE_SPI1)
        if(spi_id == 1) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist (not configured)"), spi_id);
        #endif
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist (SoC unsupported)"), spi_id);
    }
    // Replace -1 non-pin args with default values
    //                                   baudrate, polarity, phase, bits, firstbit  
    static const mp_int_t defaults[] = { 10000000, 0,        0,     8,    MICROPY_PY_MACHINE_SPI_MSB };
    for (int i = ARG_baudrate; i <= ARG_firstbit; i++) {
        if (args[i].u_int == -1) {
            args[i].u_int = defaults[i - ARG_baudrate];
        }
    }

    machine_hw_spi_obj_t *self = &machine_hw_spi_obj[spi_id];
    self->id = spi_id;

    self->base.type = &machine_spi_type;

    machine_hw_spi_init_internal(self, args);

    return MP_OBJ_FROM_PTR(self);
}

static const mp_machine_spi_p_t machine_hw_spi_p = {
    .init = machine_hw_spi_init,
    .deinit = machine_hw_spi_deinit,
    .transfer = machine_hw_spi_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_spi_make_new,
    print, machine_hw_spi_print,
    protocol, &machine_hw_spi_p,
    locals_dict, &mp_machine_spi_locals_dict
    );


/*
# Test cases
from machine import SPI
s0 = SPI(0)

from machine import SPI
s0 = SPI(0, phase=1, polarity=1, baudrate=5000000)
s0.read(2, 0x31 | 0xC0)
response: b'\x00\x14'
s0.deinit()

from machine import SPI
s1 = SPI(0, phase=1, polarity=1, baudrate=5000000)
s1.read(2, 0x31 | 0xC0)
s1.deinit()
*/
