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

// Uncomment to see HAL SPI info when call SPI_Init(...)
//#define DEBUG_HAL

#ifdef DEBUG_HAL
//------------------------------------------------------------------------------------
// HAL definitions from RDA8955_W17.44_IDH-master\soft\platform\chip\hal\src\hal_spi.c
//------------------------------------------------------------------------------------
typedef uint32_t UINT32;
typedef uint8_t UINT8;
typedef void VOID;
#define HAL_SPI_QTY (2)
#define HAL_SPI_CS_MAX_QTY (4)

typedef enum {
    HAL_SPI_DIRECT_POLLING = 0,
    HAL_SPI_DIRECT_IRQ,
    HAL_SPI_DMA_POLLING,
    HAL_SPI_DMA_IRQ,
    HAL_SPI_OFF,
    HAL_SPI_TM_QTY
} HAL_SPI_TRANSFERT_MODE_T;

typedef struct {
    UINT32 rxOvf:1; /// receive FIFO overflow irq
    UINT32 txTh:1; /// transmit FIFO threshold irq
    UINT32 txDmaDone:1; /// transmit Dma Done irq
    UINT32 rxTh:1; /// receive FIFO threshold irq    
    UINT32 rxDmaDone:1; /// receive Dma Done irq
} HAL_SPI_IRQ_STATUS_T;

typedef VOID (*HAL_SPI_IRQ_HANDLER_T)(HAL_SPI_IRQ_STATUS_T);

typedef struct {
    UINT32       ctrl;
    UINT32       cfg;
    HAL_SPI_TRANSFERT_MODE_T rxMode; 	///< Reception transfer mode
    HAL_SPI_TRANSFERT_MODE_T txMode; 	///< Emission transfer mode    
    UINT32      freq; 					/// Desired freq    
    UINT32      bytesPerFrame; 			/// Number of bytes per frame. Used to check that we only send complete frames.
    HAL_SPI_IRQ_HANDLER_T irqHandler;    
    UINT32      irqMask;				/// Image of the irq mask register
} HAL_SPI_SETTINGS_T;

typedef enum {
    /// Active CS0 to CS3 are the first one to
    /// keep compatibility with the HAL_SPI_CS_T type.
    HAL_SPI_ACTIVE_CS_0,
    HAL_SPI_ACTIVE_CS_1,
    HAL_SPI_ACTIVE_CS_2,
    HAL_SPI_ACTIVE_CS_3,
    HAL_SPI_ACTIVE_NONE,    
    HAL_SPI_OPERATION_IN_PROGRESS, /// To avoid race and several things done simultaneously.
    HAL_SPI_ACTIVE_QTY
} HAL_SPI_ACTIVE_T;

typedef enum {
    HAL_SYS_FREQ_32K     = 32768,
    HAL_SYS_FREQ_13M     = 13000000,
    HAL_SYS_FREQ_26M     = 26000000,
    HAL_SYS_FREQ_39M     = 39000000,
    HAL_SYS_FREQ_52M     = 52000000,
    HAL_SYS_FREQ_78M     = 78000000,
    HAL_SYS_FREQ_89M     = 89142857, //624M/7
    HAL_SYS_FREQ_104M    = 104000000,
    HAL_SYS_FREQ_113M    = 113454545, //624M/5.5
    HAL_SYS_FREQ_125M    = 124800000,
    HAL_SYS_FREQ_139M    = 138666667, //624M/4.5
    HAL_SYS_FREQ_156M    = 156000000,
    HAL_SYS_FREQ_178M    = 178285714, //624M/3.5
    HAL_SYS_FREQ_208M    = 208000000,
    HAL_SYS_FREQ_250M    = 249600000, //624M/2.5
    HAL_SYS_FREQ_312M    = 312000000,
    HAL_SYS_FREQ_UNKNOWN = 0,
} HAL_SYS_FREQ_T;


typedef struct {    
    HAL_SPI_SETTINGS_T          cfg[HAL_SPI_CS_MAX_QTY]; 	/// Configuration per CS    
    UINT32                      ctrl;						/// Control Register picture
    UINT8              			rxIfcCh;
    UINT8              			txIfcCh;
    HAL_SPI_TRANSFERT_MODE_T    rxMode;
    HAL_SPI_TRANSFERT_MODE_T    txMode;
    HAL_SPI_ACTIVE_T            activeStatus;
    HAL_SPI_IRQ_HANDLER_T       irqHandler;
    HAL_SYS_FREQ_T              requiredSysFreq;
} HAL_SPI_PROP_T;

HAL_SPI_PROP_T * g_halSpiPropArrayPtr = (HAL_SPI_PROP_T*)((int *) 0x8203dfe4);
//------------------------------------------------------------------------------------
#endif

typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    SPI_ID_t id;
    SPI_CS_t cs;
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
    uint8_t mode;
    uint8_t dma_delay;
    uint8_t debug;
    uint8_t debug_hst;
} machine_hw_spi_obj_t;

// ----------
// Exceptions
// ----------

MP_DEFINE_EXCEPTION(SPIError, Exception)

NORETURN void mp_raise_SPIError(const char *msg) {
    mp_raise_msg(&mp_type_SPIError, MP_ERROR_TEXT(msg));
}

// ----------
// Globals
// ----------
// singleton SPI objects
machine_hw_spi_obj_t machine_hw_spi_obj[SPI_CS_MAX];

const char * SPI_WRITE_FAILED = "SPI write failed";
const char * SPI_READ_FAILED = "SPI read failed";
const char * SPI_WRITE_BURST_FAILED = "SPI write burst failed";
const char * SPI_READ_BURST_FAILED = "SPI read burst failed";
const char * SPI_NOT_INITED = "SPI NOT inited";
const char * SPI_MALLOC_FAILED = "SPI memory allocation failed";

// Common arguments for init() and make new
enum { ARG_id, ARG_cs, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso, ARG_mode, ARG_dma_delay, ARG_debug, ARG_debug_hst };
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
    { MP_QSTR_mode,     MP_ARG_INT, {.u_int = 1 } },
	{ MP_QSTR_dma_delay, MP_ARG_INT, {.u_int = 2 } },
    { MP_QSTR_debug, MP_ARG_INT, {.u_int = 0 } },
    { MP_QSTR_debug_hst, MP_ARG_INT, {.u_int = 0 } },
};

// ---------
// Internals
// ---------
static const int8_t machine_hw_spi_default_pins[SPI_CS_MAX * 2][5] = {
    { 8, 12, 13 },
    { 8, 12, 13 },
    { 0, 3, 4 },
    { 0, 3, 4 },
};

void _spi_debug(machine_hw_spi_obj_t * self, char * message, ...) {
    char mess[256];
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

    if (self->state == MACHINE_HW_SPI_STATE_DEINIT) {
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
    // Trace(1, "SPI: transfer mode %d, dma_delay: %d", self->mode, self->dma_delay);

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
        res = SPI_Read(id, rbuffer, length); // Actually hal_SpiGetData(...)
        if(debug_hst) Trace(1, "SPI DMA transfer read result %d", length);
        if(debug_hst && length == 1) Trace(1, "SPI DMA transfer read byte 0x%02X", rbuffer[0]);
        if(res != length) {
            if(newrb) free(rbuffer);
            Trace(1, "SPI transfer DMA read length %d, result length %d", length, res);
            mp_raise_SPIError(SPI_READ_FAILED);
        }
        if(debug_hst) Trace(1, "SPI DMA transfer write %d bytes", length);
        res = SPI_Write(id, wbuffer, length); // Actually hal_SpiSendData
        if(debug_hst) Trace(1, "SPI DMA transfer write result %d", res);
        if(debug_hst && length == 1) Trace(1, "SPI DMA transfer write byte 0x%02X", wbuffer[0]);
        if(res != length) {
            if(newrb) free(rbuffer);
            Trace(1, "SPI DMA transfer write length %d, result length %d", length, res);
            mp_raise_SPIError(SPI_WRITE_FAILED);
        }
        //OS_SleepUs(self->dma_delay); // Without this sleep little length responses can be empty
        OS_SleepUs(self->dma_delay * length); // Without this sleep little length responses can be empty
        if(debug_hst) Trace(1, "SPI DMA OS_SleepUs: %d", self->dma_delay * length);

        while((!SPI_IsTxDmaDone(id))&&(!SPI_IsRxDmaDone(id)));
        SPI_ClearTxDmaDone(id);
        SPI_ClearRxDmaDone(id);
        if(newrb) free(rbuffer);
    }
}

uint32_t _get_SPI_FREQ(mp_int_t _frequency) {
    return (_frequency > SPI_FREQ_MAX) ? SPI_FREQ_MAX : _frequency;
}

static void machine_hw_spi_deinit_internal(machine_hw_spi_obj_t* self) {
    // Trace(1, "SPI: self->id = %d", self->id);
    // Trace(1, "SPI: self->cs = %d", self->cs);
    if(!SPI_Close(self->id)) mp_raise_SPIError("SPI deinit failure");
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

    if (args[ARG_baudrate].u_int != -1) {
        // calculate the actual clock frequency that the SPI peripheral can produce
        uint32_t baudrate = _get_SPI_FREQ(args[ARG_baudrate].u_int);
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

    if (args[ARG_mode].u_int != -1 && args[ARG_mode].u_int != self->mode) {
        self->mode = args[ARG_mode].u_int;
        changed = true;
    }

    if (args[ARG_dma_delay].u_int != -1 && args[ARG_dma_delay].u_int != self->dma_delay) {
        self->dma_delay = args[ARG_dma_delay].u_int;
        changed = true;
    }

    if (args[ARG_debug].u_int != -1 && args[ARG_debug].u_int != self->debug) {
        self->debug = args[ARG_debug].u_int;
        changed = true;
    }

    if (args[ARG_debug_hst].u_int != -1 && args[ARG_debug_hst].u_int != self->debug_hst) {
        self->debug_hst = args[ARG_debug_hst].u_int;
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

    if(self->mode == 0) {
        SPI_Config_t config = {
            .cs = self->cs,
            .txMode = SPI_MODE_DIRECT_POLLING,
            .rxMode = SPI_MODE_DIRECT_POLLING,
            .freq = self->baudrate,
            .line = SPI_LINE_4, // SPI_LINE_4 (duplex mode), SPI_LINE_3 - half-duplex
            .txOnly = false,
            .cpol = self->polarity, // SPI Clk Polarity
            .cpha = self->phase, // SPI Clk Phase
            .csActiveLow = 1, // SPI Cs Active Polarity
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
            .line = SPI_LINE_4, // SPI_LINE_4 (duplex mode), SPI_LINE_3 - half-duplex
            .txOnly = false,
            .cpol = self->polarity, // SPI Clk Polarity
            .cpha = self->phase, // SPI Clk Phase
            .csActiveLow = 1, // SPI Cs Active Polarity
            .dataBits = self->bits,
            .irqHandler = NULL,
            .irqMask = {0,0,0,0,0}
        };
        if(!SPI_Init(self->id, config)) mp_raise_SPIError("SPI DMA init failure");
    }
     
#ifdef DEBUG_HAL
    for(int i = 0; i < HAL_SPI_QTY; i++) {
        HAL_SPI_PROP_T spi = g_halSpiPropArrayPtr[i];
        Trace_MemBlock(1, (uint8_t*)(g_halSpiPropArrayPtr + i), sizeof(HAL_SPI_PROP_T), 16);
        Trace(1, "SPI SPI%d start ===============================", i + 1);
    	for(int j = 0; j < HAL_SPI_CS_MAX_QTY; j++) {
        	HAL_SPI_SETTINGS_T cfg = spi.cfg[j];
    		Trace(1, "SPI SPI%d_CS%d", i + 1, j);
    		Trace(1, "SPI -------------------------");
        	Trace(1, "SPI cfg ctrl");
        	Trace(1, "SPI cfg ctrl ENABLE      : 0x%01X", (cfg.ctrl >> 0) & 1);
        	Trace(1, "SPI cfg ctrl enabledCS   : 0x%01X", (cfg.ctrl >> 1) & 3);
        	// skip bit ?
        	Trace(1, "SPI cfg ctrl inputEn     : 0x%01X", (cfg.ctrl >> 4) & 1);
        	Trace(1, "SPI cfg ctrl clkFallEdge : 0x%01X", (cfg.ctrl >> 5) & 1);
        	Trace(1, "SPI cfg ctrl clkDelay    : 0x%01X", (cfg.ctrl >> 6) & 3);
        	Trace(1, "SPI cfg ctrl doDelay     : 0x%01X", (cfg.ctrl >> 8) & 3);
        	Trace(1, "SPI cfg ctrl diDelay     : 0x%01X", (cfg.ctrl >> 10) & 3);
        	Trace(1, "SPI cfg ctrl csDelay     : 0x%01X", (cfg.ctrl >> 12) & 3);
        	Trace(1, "SPI cfg ctrl csPulse     : 0x%01X", (cfg.ctrl >> 14) & 3);
        	Trace(1, "SPI cfg ctrl frameSize   : 0x%01X", ((cfg.ctrl >> 16) & 0x1F) + 1);
        	Trace(1, "SPI cfg ctrl oeRatio     : 0x%01X", (cfg.ctrl >> 24) & 0x1F);
        	Trace(1, "SPI cfg ctrl inputSel    : 0x%01X", (cfg.ctrl >> 30) & 3);
        	
        	Trace(1, "SPI cfg cfg : 0x%08X", cfg.cfg);
        	Trace(1, "SPI cfg rxMode: 0x%04X", cfg.rxMode);
        	Trace(1, "SPI cfg txMode: 0x%04X", cfg.txMode);
        	Trace(1, "SPI cfg freq: %d", cfg.freq);
        	Trace(1, "SPI cfg bytesPerFrame: %d", cfg.bytesPerFrame);
        	Trace(1, "SPI cfg irqHandler: %p", cfg.irqHandler);
        	Trace(1, "SPI cfg irqMask: 0x%04X", cfg.irqMask);
        	break; // comment if need more
        }
        Trace(1, "SPI -----------------------");
        Trace(1, "SPI ctrl: 0x%08X", spi.ctrl);
        Trace(1, "SPI rxIfcCh: 0x%02X", spi.rxIfcCh);
        Trace(1, "SPI txIfcCh: 0x%02X", spi.txIfcCh);
        Trace(1, "SPI rxMode: 0x%04X (%d)", spi.rxMode, sizeof(spi.rxMode));
        Trace(1, "SPI txMode: 0x%04X (%d)", spi.txMode, sizeof(spi.txMode));
        Trace(1, "SPI activeStatus: 0x%04X (%d)", spi.activeStatus, sizeof(spi.activeStatus));
        Trace(1, "SPI irqHandler: %p", spi.irqHandler);
        Trace(1, "SPI requiredSysFreq: %d (%d)", spi.requiredSysFreq, sizeof(spi.requiredSysFreq));
        Trace(1, "SPI SPI%d end ===============================", i + 1);
        break; // comment if need more
    }
    
    /*
    Patch GPRS C SDK via patch-lod.py.
    Attention: doing this patch leads to loosing SPI_DATA_BITS_16 config
    Patch corrects SPI doDelay and diDelay for different SPI clock phase ("cpha" field in SPI_Config_t)
    if cpha == 0 doDelay should be 0, diDelay = 2 
    if cpha == 1 doDelay should be 0, diDelay = 1 

    In SPI_Init(...) make following changes:
    ---------------------------- first ---------------------------
    if (in_config.cpha != 0x0) {  hal_config.doDelay = 1; hal_config.diDelay = 1; }
    replace with --------------->
    if (in_config.cpha != 0x0) {  hal_config.diDelay = 1; } // correct doDelay = 0 for cpha=1

    8200876c 02  23           beqz       v1,LAB_82008772
    8200876e 07  d2           sw         v0,hal_config.doDelay (sp)
    82008770 08  d2           sw         v0,hal_config.diDelay (sp)
    82008772 14  96           lw         in_config_txMode ,hal_config +0x40 (sp)
    replace with --------------->
    8200876c 02  23           beqz       v1,LAB_82008772
    8200876e 00  65           nop
    82008770 08  d2           sw         v0,hal_config.diDelay (sp)
    82008772 14  96           lw         in_config_txMode ,hal_config +0x40 (sp)

    ---------------------------- second ---------------------------
  	if (config.dataBits == 16) hal_config.frameSize = 16; 
  	replace with --------------->
  	if (in_config.cpha == 0x0) hal_config.diDelay = 2; // correct diDelay for cpha=0

    820087d4 16  92           lw         dataBits ,hal_config +0x48 (sp)
    820087d6 10  72           cmpi       dataBits ,16
    820087d8 02  61           btnez      LAB_820087de
    820087da 10  6c           li         in_id ,16
    820087dc 0b  d4           sw         in_id ,hal_config.frameSize (sp)
    820087de 28  37           sll        in_config_rxMode ,spi_id ,0x2
    replace with --------------->
    820087d4 19  92           lw         dataBits ,0x64 (sp)
    820087d6 00  72           cmpi       dataBits ,0x0
    820087d8 02  61           btnez      LAB_820087de
    820087da 02  6c           li         in_id ,0x2
    820087dc 08  d4           sw         in_id ,0x20 (sp)
    820087de 28  37           sll        in_config_rxMode ,spi_id ,0x2
    */
#endif    

    self->state = MACHINE_HW_SPI_STATE_INIT;
    _spi_debug(self, "SPI%d_CS%d inited", self->id, self->cs);
}

static void machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;
    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        machine_hw_spi_deinit_internal(self);
    }
}

// ------------------------
// Constructor & Destructor
// ------------------------

// Set constant values, depending of SPI id
static void machine_hw_spi_argcheck(mp_arg_val_t args[], const int8_t *default_pins) {
    for (int i = ARG_sck; i <= ARG_miso; i++) {
        args[i].u_int = default_pins ? default_pins[i - ARG_sck] : -1;
    }
}

static void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;

    mp_arg_val_t args[MP_ARRAY_SIZE(spi_allowed_args)];
    // offset arg lists by 1 to skip first arg, id, which is not valid for init()
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(spi_allowed_args) - 1,
        spi_allowed_args + 1, args + 1);

    machine_hw_spi_argcheck(args, NULL);
    machine_hw_spi_init_internal(self, args);
}

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    mp_arg_val_t args[MP_ARRAY_SIZE(spi_allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(spi_allowed_args), spi_allowed_args, args);

    const mp_int_t spi_id = args[ARG_id].u_int;
    const mp_int_t spi_cs = args[ARG_cs].u_int;

    if ((spi_id == SPI1 || spi_id == SPI2) && (spi_cs == 0 || spi_cs == 1)) {
        machine_hw_spi_argcheck(args, machine_hw_spi_default_pins[(spi_id - 1) * 2 + spi_cs]);
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI%d_CS%d doesn't exist"), spi_id, spi_cs);
    }

    // Replace -1 non-pin args with default values
    static const mp_int_t defaults[] = { 500000, 0, 0, 8, MICROPY_PY_MACHINE_SPI_MSB };
    for (int i = ARG_baudrate; i <= ARG_firstbit; i++) {
        if (args[i].u_int == -1) {
            args[i].u_int = defaults[i - ARG_baudrate];
        }
    }
    if (args[ARG_dma_delay].u_int < 1 || args[ARG_dma_delay].u_int > 100) {
    	mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("dma_delay should be between 1 and 100"));
    }

    machine_hw_spi_obj_t *self = &machine_hw_spi_obj[spi_id];
    self->id = spi_id;

    self->base.type = &machine_spi_type;

    machine_hw_spi_init_internal(self, args);

    return MP_OBJ_FROM_PTR(self);
}

static void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Round to nearest whole set of bits
    int bits_to_send = len * 8 / self->bits * self->bits;
    if (!bits_to_send) mp_raise_ValueError(MP_ERROR_TEXT("buffer too short"));
    _spi_transfer_internal(self, src, dest, len, self->debug_hst);
}


// ---------
// SPI Print
// ---------
static void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI%d_CS%d(baudrate :%d, polarity:%d, phase:%d, bits:%s, mode:%s, dma_delay:%d, ptr: %p)",
        self->id, self->cs, self->baudrate, self->polarity, self->phase,
        self->bits == SPI_DATA_BITS_8 ? "8" : "16", self->mode == 0 ? "DIRECT" : "DMA", self->dma_delay, self);
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
s1 = SPI(1)

from machine import SPI
s1 = SPI(1, phase=1, polarity=1, baudrate=5000000)
s1.read(2, 0x31 | 0xC0)
response: b'\x00\x14'
s1.deinit()

from machine import SPI
s2 = SPI(2, phase=1, polarity=1, baudrate=5000000)
s2.read(2, 0x31 | 0xC0)
s2.deinit()
*/
