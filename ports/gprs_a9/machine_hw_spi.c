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

#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_os.h"
#include "api_debug.h"
#include "api_hal_pm.h"
#include "api_hal_spi.h"
#include "modmachine.h"
#include "extmod/modmachine.h"

// ----------
// Exceptions
// ----------

MP_DEFINE_EXCEPTION(SPIError, Exception)

NORETURN void mp_raise_SPIError(const char *msg) {
    mp_raise_msg(&mp_type_SPIError, msg);
}

// ----------
// Globals
// ----------
// singleton SPI objects
machine_hw_spi_obj_t *spi_obj[2][2] = {{NULL, NULL}, {NULL, NULL}};
uint8_t _spi_inited[2][2] = {{0,0},{0,0}};
uint32_t _spi_dma_delay[2] = {4,4};
const char * SPI_WRITE_FAILED = "SPI write failed";
const char * SPI_READ_FAILED = "SPI read failed";
const char * SPI_WRITE_BURST_FAILED = "SPI write burst failed";
const char * SPI_READ_BURST_FAILED = "SPI read burst failed";
const char * SPI_NOT_INITED = "SPI NOT inited";
const char * SPI_MALLOC_FAILED = "SPI memory allocation failed";


// ---------
// Internals
// ---------

void _spi_debug(machine_hw_spi_obj_t * self, char * message, ...) {
    char mess[1024];
    if(self->debug == 1 || self->debug_hst == 1) {
        memset(mess, 0, sizeof(mess));
        va_list args;
        va_start(args, message);
        vsnprintf(mess, sizeof(mess), message, args);
        va_end(args);
        if(self->debug_hst == 1) Trace(2, mess);
        if(self->debug == 1) mp_printf(&mp_plat_print, "%s\n", mess);
    }
}


void _spi_transfer_internal(machine_hw_spi_obj_t *self_in, const uint8_t* wbuffer, uint8_t* rbuffer, uint32_t length, uint8_t debug_hst) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    SPI_ID_t id = self->id;
    SPI_CS_t cs = self->cs;

    if(_spi_inited[id-1][cs] == 0) {
        Trace(1, SPI_NOT_INITED);
        mp_raise_SPIError(SPI_NOT_INITED);
        return;
    }
    uint32_t res;
    if (wbuffer == NULL) {
        Trace(1, "SPI transfer write buffer is NULL");
        mp_raise_SPIError(SPI_WRITE_FAILED);
        return;
    }
    if(self->mode == 0 && length > 0x0F) {
        Trace(1, "Cannot send over 15 bytes in SPI direct mode");
        mp_raise_SPIError("Cannot send over 15 bytes in SPI direct mode");
        return;
    }

    uint8_t newrb = 0;
    if(rbuffer == NULL) {
       rbuffer = malloc(length);
       if(rbuffer == NULL) {
          Trace(1, SPI_MALLOC_FAILED);
          mp_raise_SPIError(SPI_MALLOC_FAILED);
          return;
       }
       newrb = 1;
    }

    // SPI direct mode
    if(self->mode == 0) {
        if(debug_hst) Trace(1, "SPI transfer write %d bytes", length);
        res = SPI_Write(id, wbuffer, length);
        if(debug_hst) Trace(1, "SPI transfer write result %d", res);
        if(res != length) {
            if(newrb) free(rbuffer);
            Trace(1, "SPI transfer write length %d, result length %d", length, res);
            mp_raise_SPIError(SPI_WRITE_FAILED);
        }
        while(!SPI_IsTxDone(id));
        if(debug_hst) Trace(1, "SPI transfer read %d bytes", length);
        res = SPI_Read(id, rbuffer, length);
        if(debug_hst) Trace(1, "SPI transfer read result %d", length);
        if(res != length) {
            if(newrb) free(rbuffer);
            Trace(1, "SPI transfer read length %d, result length %d", length, res);
            mp_raise_SPIError(SPI_READ_FAILED);
        }
    }
    // SPI DMA mode
    if(self->mode == 1) {
        if(debug_hst) Trace(1, "SPI DMA transfer read %d bytes", length);
        res = SPI_Read(id, rbuffer, length);
        if(debug_hst) Trace(1, "SPI DMA transfer read result %d", length);
        if(debug_hst && length == 1) Trace(1, "SPI DMA transfer read byte 0x%02X", rbuffer[0]);
        if(res != length) {
            if(newrb) free(rbuffer);
            Trace(1, "SPI transfer DMA read length %d, result length %d", length, res);
            mp_raise_SPIError(SPI_READ_FAILED);
        }
        if(debug_hst) Trace(1, "SPI DMA transfer write %d bytes", length);
        res = SPI_Write(id, wbuffer, length);
        if(debug_hst) Trace(1, "SPI DMA transfer write result %d", res);
        if(debug_hst && length == 1) Trace(1, "SPI DMA transfer write byte 0x%02X", wbuffer[0]);
        if(res != length) {
            if(newrb) free(rbuffer);
            Trace(1, "SPI DMA transfer write length %d, result length %d", length, res);
            mp_raise_SPIError(SPI_WRITE_FAILED);
        }
        OS_SleepUs(_spi_dma_delay[id - 1]); // Without this sleep little length responses can be empty
        while((!SPI_IsTxDmaDone(id))&&(!SPI_IsRxDmaDone(id)));
        SPI_ClearTxDmaDone(id);
        SPI_ClearRxDmaDone(id);
        if(newrb) free(rbuffer);
    }
}

uint32_t _get_SPI_FREQ(mp_int_t _frequency) {
    return (_frequency > SPI_FREQ_MAX) ? SPI_FREQ_MAX : _frequency;
}


STATIC void machine_hw_spi_init_internal(machine_hw_spi_obj_t* self) {
    if(_spi_inited[self->id - 1][(int)self->cs] == 0) {
        if(self->mode == 0) {
            SPI_Config_t config = {
                .cs = self->cs,
                .txMode = SPI_MODE_DIRECT_POLLING,
                .rxMode = SPI_MODE_DIRECT_POLLING,
                .freq = self->baudrate,
                .line = self->line, // SPI_LINE_4 (duplex mode), SPI_LINE_3 - half-duplex
                .txOnly = false,
                .cpol = self->cpol, // SPI Clk Polarity
                .cpha = self->cpha, // SPI Clk Phase
                .csActiveLow = self->cs_active_low, // SPI Cs Active Polarity
                .dataBits = self->bits,
                .irqHandler = NULL,
                .irqMask = {0,0,0,0,0}
            };
            if(!SPI_Init(self->id, config)) mp_raise_SPIError("SPI init failure");
        }
        if(self->mode == 1) {
            SPI_Config_t config = {
                .cs = self->cs,
                .txMode = SPI_MODE_DMA_POLLING,
                .rxMode = SPI_MODE_DMA_POLLING,
                .freq = self->baudrate,
                .line = self->line, // SPI_LINE_4 (duplex mode), SPI_LINE_3 - half-duplex
                .txOnly = false,
                .cpol = self->cpol, // SPI Clk Polarity
                .cpha = self->cpha, // SPI Clk Phase
                .csActiveLow = self->cs_active_low, // SPI Cs Active Polarity
                .dataBits = self->bits,
                .irqHandler = NULL,
                .irqMask = {0,0,0,0,0}
            };
            if(!SPI_Init(self->id, config)) mp_raise_SPIError("SPI DMA init failure");
        }

        _spi_inited[self->id - 1][self->cs] = 1;
        _spi_dma_delay[self->id - 1] = self->dma_delay;

        _spi_debug(self, "SPI%d_CS%d inited", self->id, self->cs);

    } else mp_warning(MP_WARN_CAT(RuntimeWarning), "SPI already inited");
}

STATIC void machine_hw_spi_deinit_internal(machine_hw_spi_obj_t* self) {
    if(_spi_inited[self->id - 1][self->cs] == 1) {
        if(SPI_Close(self->id)) {
           _spi_inited[self->id - 1][0] = 0;
           _spi_inited[self->id - 1][1] = 0;
           _spi_dma_delay[self->id - 1] = 4;
        } else mp_raise_SPIError("SPI deinit failure");
    } else mp_warning(MP_WARN_CAT(RuntimeWarning), "SPI already deinited");
}

// ------------------------
// Constructor & Destructor
// ------------------------

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_mode, ARG_cs, ARG_duplex, ARG_cs_active_low, ARG_dma_delay, ARG_debug, ARG_debug_hst};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = SPI1 } },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 10000000} }, // 10 MHz default
        { MP_QSTR_polarity, MP_ARG_INT, {.u_int = 0 } },
        { MP_QSTR_phase, MP_ARG_INT, {.u_int = 1 } },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = SPI_DATA_BITS_8 } },
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = 1 } }, // DMA mode
        { MP_QSTR_cs, MP_ARG_INT, {.u_int = SPI_CS_0 } },
        { MP_QSTR_duplex, MP_ARG_INT, {.u_int = 1 } },
        { MP_QSTR_cs_active_low, MP_ARG_INT, {.u_int = 1 } },
        { MP_QSTR_dma_delay, MP_ARG_INT, {.u_int = 4 } },
        { MP_QSTR_debug, MP_ARG_INT, {.u_int = 0 } },
        { MP_QSTR_debug_hst, MP_ARG_INT, {.u_int = 0 } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t temp_id = 0;
    uint8_t temp_cs = 0;

    switch (args[ARG_id].u_int) {
        case SPI1: temp_id = SPI1; break;
        case SPI2: temp_id = SPI2; break;
        default: mp_raise_ValueError("Unknown 'SPI ID' argument value"); return mp_const_none;
    }

    switch (args[ARG_cs].u_int) {
        case SPI_CS_0: temp_cs = SPI_CS_0; break;
        case SPI_CS_1: temp_cs = SPI_CS_1; break;
        default: mp_raise_ValueError("Unknown 'SPI CS' argument"); return mp_const_none;
    }
    // Trace(1, "SPI make new ID=%i, CS=%i", temp_id, temp_cs);

    machine_hw_spi_obj_t *self = NULL;
    if(spi_obj[temp_id - 1][temp_cs] == NULL) {
        self = mp_obj_malloc(machine_hw_spi_obj_t, &machine_spi_type);
        self->id = temp_id;
        self->cs = temp_cs;
        spi_obj[temp_id - 1][temp_cs] = self;
    } else self = spi_obj[temp_id - 1][temp_cs];

    self->baudrate = _get_SPI_FREQ(args[ARG_baudrate].u_int);

    switch (args[ARG_duplex].u_int) {
        case 0: self->line = SPI_LINE_3; break;
        case 1: self->line = SPI_LINE_4; break;
        default:  mp_raise_ValueError("Unknown duplex argument"); return mp_const_none;
    }

    switch (args[ARG_polarity].u_int) {
        case 0: self->cpol = 0; break;
        case 1: self->cpol = 1; break;
        default:  mp_raise_ValueError("Unknown clock polarity argument"); return mp_const_none;
    }

    switch (args[ARG_phase].u_int) {
        case 0: self->cpha = 0; break;
        case 1: self->cpha = 1; break;
        default:  mp_raise_ValueError("Unknown clock phase argument"); return mp_const_none;
    }

    switch (args[ARG_cs_active_low].u_int) {
        case 0: self->cs_active_low = 0; break;
        case 1: self->cs_active_low = 1; break;
        default:  mp_raise_ValueError("Unknown CS active low argument"); return mp_const_none;
    }

    switch (args[ARG_bits].u_int) {
        case 8: self->bits = SPI_DATA_BITS_8; break;
        case 16: self->bits = SPI_DATA_BITS_16; break;
        default:  mp_raise_ValueError("Unknown data bits argument"); return mp_const_none;
    }

    self->dma_delay = args[ARG_dma_delay].u_int;

    switch (args[ARG_debug].u_int) {
        case 0: self->debug = 0; break;
        case 1: self->debug = 1; break;
        default: mp_raise_ValueError("Unknown debug argument"); return mp_const_none;
    }

    switch (args[ARG_debug_hst].u_int) {
        case 0: self->debug_hst = 0; break;
        case 1: self->debug_hst = 1; break;
        default: mp_raise_ValueError("Unknown debug_hst argument"); return mp_const_none;
    }
    self->debug_hst = 0;

    switch (args[ARG_mode].u_int) {
        case 0: self->mode = 0; break; // direct mode
        case 1: self->mode = 1; break; // DMA mode
        default: mp_raise_ValueError("Unknown mode argument"); return mp_const_none;
    }
    machine_hw_spi_init_internal(self);
    return MP_OBJ_FROM_PTR(self);
}
STATIC void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;

    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_mode, ARG_cs, ARG_duplex, ARG_cs_active_low, ARG_dma_delay, ARG_debug, ARG_debug_hst};

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = SPI1 } },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 10000000} }, // 10 MHz default
        { MP_QSTR_polarity, MP_ARG_INT, {.u_int = 0 } },
        { MP_QSTR_phase, MP_ARG_INT, {.u_int = 1 } },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = SPI_DATA_BITS_8 } },
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = 1 } }, // DMA mode
        { MP_QSTR_cs, MP_ARG_INT, {.u_int = SPI_CS_0 } },
        { MP_QSTR_duplex, MP_ARG_INT, {.u_int = 1 } },
        { MP_QSTR_cs_active_low, MP_ARG_INT, {.u_int = 1 } },
        { MP_QSTR_dma_delay, MP_ARG_INT, {.u_int = 4 } },
        { MP_QSTR_debug, MP_ARG_INT, {.u_int = 0 } },
        { MP_QSTR_debug_hst, MP_ARG_INT, {.u_int = 0 } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t temp_id = 0;
    uint8_t temp_cs = 0;

    switch (args[ARG_id].u_int) {
        case SPI1: temp_id = SPI1; break;
        case SPI2: temp_id = SPI2; break;
        default: mp_raise_ValueError("Unknown 'SPI ID' argument value (init)");
    }

    switch (args[ARG_cs].u_int) {
        case SPI_CS_0: temp_cs = SPI_CS_0; break;
        case SPI_CS_1: temp_cs = SPI_CS_1; break;
        default: mp_raise_ValueError("Unknown 'SPI CS' argument (init)");
    }

    self->baudrate = _get_SPI_FREQ(args[ARG_baudrate].u_int);

    switch (args[ARG_duplex].u_int) {
        case 0: self->line = SPI_LINE_3; break;
        case 1: self->line = SPI_LINE_4; break;
        default:  mp_raise_ValueError("Unknown duplex argument (init)");
    }

    switch (args[ARG_polarity].u_int) {
        case 0: self->cpol = 0; break;
        case 1: self->cpol = 1; break;
        default:  mp_raise_ValueError("Unknown clock polarity argument (init)");
    }

    switch (args[ARG_phase].u_int) {
        case 0: self->cpha = 0; break;
        case 1: self->cpha = 1; break;
        default:  mp_raise_ValueError("Unknown clock phase argument (init)");
    }

    switch (args[ARG_cs_active_low].u_int) {
        case 0: self->cs_active_low = 0; break;
        case 1: self->cs_active_low = 1; break;
        default:  mp_raise_ValueError("Unknown CS active low argument (init)");
    }

    switch (args[ARG_bits].u_int) {
        case 8: self->bits = SPI_DATA_BITS_8; break;
        case 16: self->bits = SPI_DATA_BITS_16; break;
        default:  mp_raise_ValueError("Unknown data bits argument (init)");
    }

    self->dma_delay = args[ARG_dma_delay].u_int;

    switch (args[ARG_debug].u_int) {
        case 0: self->debug = 0; break;
        case 1: self->debug = 1; break;
        default: mp_raise_ValueError("Unknown debug argument (init)");
    }

    switch (args[ARG_debug_hst].u_int) {
        case 0: self->debug_hst = 0; break;
        case 1: self->debug_hst = 1; break;
        default: mp_raise_ValueError("Unknown debug_hst argument (init)");
    }

    switch (args[ARG_mode].u_int) {
        case 0: self->mode = 0; break; // direct mode
        case 1: self->mode = 1; break; // DMA mode
        default: mp_raise_ValueError("Unknown mode argument");
    }
    machine_hw_spi_init_internal(self);
}

STATIC void machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;
    machine_hw_spi_deinit_internal(self);
}


STATIC void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Round to nearest whole set of bits
    int bits_to_send = len * 8 / self->bits * self->bits;
    if (!bits_to_send) mp_raise_ValueError(MP_ERROR_TEXT("buffer too short"));
    _spi_transfer_internal(self, src, dest, len, self->debug_hst);
}


// ---------
// SPI Print
// ---------
STATIC void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI%d_CS%d(baudrate :%d, dma_delay:%d, line:%s, clock_polarity:%d, clock_phase:%d, cs_active_low:%d, bits:%s, mode:%s, ptr: %p)",
        self->id, self->cs, self->baudrate, self->dma_delay, self->line == SPI_LINE_3 ? "SPI_LINE_3" : "SPI_LINE_4", self->cpol, self->cpha,
        self->cs_active_low, self->bits == SPI_DATA_BITS_8 ? "SPI_DATA_BITS_8" : "SPI_DATA_BITS_16", self->mode == 0 ? "DIRECT" : "DMA", self);
}


STATIC const mp_machine_spi_p_t machine_hw_spi_p = {
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
