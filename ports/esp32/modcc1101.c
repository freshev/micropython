/*

 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
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
#include "time.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "extmod/modmachine.h"
#include "esp_log.h"

//#ifdef CONFIG_CC1101_MODULE

#define   WRITE_BURST       0x40 // write burst mask
#define   READ_SINGLE       0x80 // read single mask
#define   READ_BURST        0xC0 // read burst mask
#define   BYTES_IN_RXFIFO   0x7F // byte count mask in RXfifo
#define   BYTES_IN_TXFIFO   0x7F // byte count mask in TXfifo
#define   max_modul 6

#define CC1101_IOCFG2       0x00 // GDO2 output pin configuration
#define CC1101_IOCFG1       0x01 // GDO1 output pin configuration
#define CC1101_IOCFG0       0x02 // GDO0 output pin configuration
#define CC1101_FIFOTHR      0x03 // RX FIFO and TX FIFO thresholds
#define CC1101_SYNC1        0x04 // Sync word, high byte
#define CC1101_SYNC0        0x05 // Sync word, low byte
#define CC1101_PKTLEN       0x06 // Packet length
#define CC1101_PKTCTRL1     0x07 // Packet automation control 1
#define CC1101_PKTCTRL0     0x08 // Packet automation control 0
#define CC1101_ADDR         0x09 // Device address
#define CC1101_CHANNR       0x0A // Channel number
#define CC1101_FSCTRL1      0x0B // Frequency synthesizer control 1
#define CC1101_FSCTRL0      0x0C // Frequency synthesizer control 2
#define CC1101_FREQ2        0x0D // Frequency control word, high byte
#define CC1101_FREQ1        0x0E // Frequency control word, middle byte
#define CC1101_FREQ0        0x0F // Frequency control word, low byte
#define CC1101_MDMCFG4      0x10 // Modem configuration 4
#define CC1101_MDMCFG3      0x11 // Modem configuration 3
#define CC1101_MDMCFG2      0x12 // Modem configuration 2
#define CC1101_MDMCFG1      0x13 // Modem configuration 1
#define CC1101_MDMCFG0      0x14 // Modem configuration 0
#define CC1101_DEVIATN      0x15 // Modem deviation setting
#define CC1101_MCSM2        0x16 // Main Radio Control State Machine configuration 2
#define CC1101_MCSM1        0x17 // Main Radio Control State Machine configuration 1
#define CC1101_MCSM0        0x18 // Main Radio Control State Machine configuration 0
#define CC1101_FOCCFG       0x19 // Frequency Offset Compensation configuration
#define CC1101_BSCFG        0x1A // Bit Synchronization configuration
#define CC1101_AGCCTRL2     0x1B // AGC control 2
#define CC1101_AGCCTRL1     0x1C // AGC control 1
#define CC1101_AGCCTRL0     0x1D // AGC control 0
#define CC1101_WOREVT1      0x1E // High byte Event 0 timeout
#define CC1101_WOREVT0      0x1F // Low byte Event 0 timeout
#define CC1101_WORCTRL      0x20 // Wake On Radio control
#define CC1101_FREND1       0x21 // Front end RX configuration 1
#define CC1101_FREND0       0x22 // Front end TX configuration 0
#define CC1101_FSCAL3       0x23 // Frequency synthesizer calibration 3
#define CC1101_FSCAL2       0x24 // Frequency synthesizer calibration 2
#define CC1101_FSCAL1       0x25 // Frequency synthesizer calibration 1
#define CC1101_FSCAL0       0x26 // Frequency synthesizer calibration 0
#define CC1101_RCCTRL1      0x27 // RC oscillator configuration 1
#define CC1101_RCCTRL0      0x28 // RC oscillator configuration 0
#define CC1101_FSTEST       0x29 // Frequency synthesizer calibration control
#define CC1101_PTEST        0x2A // Production test
#define CC1101_AGCTEST      0x2B // AGC test
#define CC1101_TEST2        0x2C // Various test settings
#define CC1101_TEST1        0x2D // Various test settings
#define CC1101_TEST0        0x2E // Various test settings

//CC1101 commands
#define CC1101_SRES         0x30        // Reset
#define CC1101_SFSTXON      0x31        // Enable and calibrate frequency synthesizer
#define CC1101_SXOFF        0x32        // Turn off oscillator
#define CC1101_SCAL         0x33        // Calibrate frequency synthesizer and turn it off
#define CC1101_SRX          0x34        // Enter RX state
#define CC1101_STX          0x35        // Enter TX state
#define CC1101_SIDLE        0x36        // Enter Idle state
#define CC1101_SAFC         0x37        // Perform AFC adjustment
#define CC1101_SWOR         0x38        // Start automatic RX polling sequence (Wake-on-Radio)
#define CC1101_SPWD         0x39        // Enter power down state
#define CC1101_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CC1101_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CC1101_SWORRST      0x3C        // Reset real time clock.
#define CC1101_SNOP         0x3D        // No operation

// CC1101 STATUS REGISTER
#define CC1101_PARTNUM      0x30 // Part number
#define CC1101_VERSION      0x31 // Current version number
#define CC1101_FREQEST      0x32 // Frequency Offset Estimate
#define CC1101_LQI          0x33 // Demodulator estimate for Link Quality
#define CC1101_RSSI         0x34 // Received signal strength indication
#define CC1101_MARCSTATE    0x35 // Control state machine state
#define CC1101_WORTIME1     0x36 // High byte of WOR timer
#define CC1101_WORTIME0     0x37 // Low byte of WOR timer
#define CC1101_PKTSTATUS    0x38 // Current GDOx status and packet status
#define CC1101_VCO_VC_DAC   0x39 // Current setting from PLL calibration module
#define CC1101_TXBYTES      0x3A // Underflow and number of bytes in the TX FIFO
#define CC1101_RXBYTES      0x3B // Overflow and number of bytes in the RX FIFO
#define CC1101_RC1_STATUS   0x3C // Last RC oscillator calibration result
#define CC1101_RC0_STATUS   0x3D // Last RC oscillator calibration result

// PATABLE, TXFIFO, RXFIFO
#define CC1101_PATABLE      0x3E
#define CC1101_TXFIFO       0x3F
#define CC1101_RXFIFO       0x3F

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


typedef struct _cc1101_obj_t {
    mp_obj_base_t base;
    machine_hw_spi_obj_t *spi;
    uint8_t gdo0_state;

    float MHz;
    int32_t power; // dBm
    uint8_t modulation;
    uint8_t chan;
    uint8_t ccmode;

    uint8_t trxstate;
    uint8_t last_power;
    uint8_t P_TABLE[8];
    uint8_t m4RxBw;
    uint8_t m4DaRa;
    uint8_t m2DCOFF;
    uint8_t m2MODFM;
    uint8_t m2MANCH;
    uint8_t m2SYNCM;

    uint8_t m1FEC;
    uint8_t m1PRE;
    uint8_t m1CHSP;

    uint8_t pc0WDATA;
    uint8_t pc0PktForm;
    uint8_t pc0CRC_EN;
    uint8_t pc0LenConf;
    uint8_t pc1PQT;
    uint8_t pc1CRC_AF;
    uint8_t pc1APP_ST;
    uint8_t pc1ADRCHK;

    uint8_t clb1[2];
    uint8_t clb2[2];
    uint8_t clb3[2];
    uint8_t clb4[2];
    uint8_t debug;
    uint8_t debug_hst;

} cc1101_obj_t;

uint8_t P_INIT[8]       = {0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00};
// power, dBm              -30  -20  -15  -10  0    5    7    10
uint8_t P_TABLE_315[8]  = {0x12,0x0D,0x1C,0x34,0x51,0x85,0xCB,0xC2};           //300 - 348 MHz
// power, dBm              -30  -20  -15  -10  0    5    7    10
uint8_t P_TABLE_433[8]  = {0x12,0x0E,0x1D,0x34,0x60,0x84,0xC8,0xC0};           //387 - 464 MHz
// power, dBm              -30  -20  -15  -10  -6   0    5    7    10   12
uint8_t P_TABLE_868[10] = {0x03,0x17,0x1D,0x26,0x37,0x50,0x86,0xCD,0xC5,0xC0}; //779 - 899.99 MHz
// power, dBm              -30  -20  -15  -10  -6   0    5    7    10   11
uint8_t P_TABLE_915[10] = {0x03,0x0E,0x1E,0x27,0x38,0x8E,0x84,0xCC,0xC3,0xC0,};//900 - 928 MHz

const char * CC1101_MALLOC_FAILED = "CC1101 memory allocation failed";

// ----------
// Exceptions
// ----------

MP_DEFINE_EXCEPTION(CC1101Error, Exception)

NORETURN void mp_raise_CC1101Error(const char *msg) {
    mp_raise_msg(&mp_type_CC1101Error, MP_ERROR_TEXT(msg));
}

// --------
// Forwards
// --------
void _cc1101_calibrate(cc1101_obj_t * self);
void _cc1101_reset(cc1101_obj_t * self);
void _cc1101_reg_config_settings(cc1101_obj_t * self);
void _cc1101_split_PKTCTRL0(cc1101_obj_t * self);
void _cc1101_split_PKTCTRL1(cc1101_obj_t * self);
void _cc1101_split_MDMCFG1(cc1101_obj_t * self);
void _cc1101_split_MDMCFG2(cc1101_obj_t * self);
void _cc1101_split_MDMCFG4(cc1101_obj_t * self);
uint8_t _cc1101_digital_read_gdo0(cc1101_obj_t * self, uint8_t need_debug);
void _cc1101_debug(cc1101_obj_t * self, char * message, ...);
uint8_t _cc1101_map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);

// ------------------------
// SPI related functions
// ------------------------

void _cc1101_transfer_addr_burst(cc1101_obj_t *self, uint8_t addr, uint8_t* wbuffer,  uint8_t* rbuffer, uint32_t length) {
    machine_hw_spi_obj_t *spi = self->spi;

    uint8_t* m_wbuffer = malloc(length + 1);
    uint8_t* m_rbuffer = malloc(length + 1);
    if(m_wbuffer != NULL && m_rbuffer != NULL) {
        *m_wbuffer = addr;
        memcpy(m_wbuffer + 1, wbuffer, length);

        mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(spi);
        mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
        spi_p->transfer(s, length + 1, m_wbuffer, m_rbuffer);

        memset(rbuffer, 0, length);
        memcpy(rbuffer, m_rbuffer + 1, length);
        free(m_rbuffer);
        free(m_wbuffer);
    } else mp_raise_CC1101Error(CC1101_MALLOC_FAILED);
}

uint8_t _cc1101_transfer_byte(cc1101_obj_t *self, uint8_t wbyte) {
    machine_hw_spi_obj_t *spi = self->spi;

    uint8_t rbyte = 0;
    mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(spi);
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
    spi_p->transfer(s, 1, &wbyte, &rbyte);

    return rbyte;
}

uint8_t _cc1101_transfer_addr_byte(cc1101_obj_t *self, uint8_t addr, uint8_t wbyte) {
    machine_hw_spi_obj_t *spi = self->spi;

    uint8_t m_wbyte[2];
    uint8_t m_rbyte[2];
    m_wbyte[0] = addr;
    m_wbyte[1] = wbyte;
    m_rbyte[1] = 0;

    mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(spi);
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
    spi_p->transfer(s, 2, m_wbyte, m_rbyte);

    return m_rbyte[1];
}

// ------------------------
// Constructor & Destructor
// ------------------------

extern const mp_obj_type_t cc1101_type;
extern mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args);
mp_obj_t cc1101_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    enum { ARG_id, ARG_cs, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso, ARG_debug, ARG_debug_hst};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0 } }, // SPI_ID = 0
        { MP_QSTR_cs,       MP_ARG_INT, {.u_int = -1 } },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 5000000 } }, // 5MHz (more - unstable)
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_debug, MP_ARG_INT, {.u_int = 0 } },
        { MP_QSTR_debug_hst, MP_ARG_INT, {.u_int = 0 } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);    

    if (args[ARG_baudrate].u_int > 11000000) {
       mp_raise_ValueError(MP_ERROR_TEXT("CC1101 baudrate should be <= 11 MHz")); return mp_const_none;
    }

    cc1101_obj_t * self = mp_obj_malloc(cc1101_obj_t, &cc1101_type);

    self->MHz = 433.92;
    self->modulation = 2; // ASK
    self->chan = 0;
    self->ccmode = 0;
    self->power = 12; // dBm

    self->trxstate = 0;
    self->m4RxBw = 0;
    self->m4DaRa = 0;
    self->m1FEC = 0;
    self->m1PRE = 0;
    self->m1CHSP = 0;
    self->pc0WDATA = 0;
    self->pc0PktForm = 0;
    self->pc0CRC_EN = 0;
    self->pc0LenConf = 0;
    self->pc1PQT = 0;
    self->pc1CRC_AF = 0;
    self->pc1APP_ST = 0;
    self->pc1ADRCHK = 0;

    for(int i = 0 ; i < sizeof(P_INIT); i++) self->P_TABLE[i] = P_INIT[i];
    self->clb1[0] = 24; self->clb1[1] = 28;
    self->clb2[0] = 31; self->clb2[1] = 38;
    self->clb3[0] = 65; self->clb3[1] = 76;
    self->clb4[0] = 77; self->clb4[1] = 79;

    self->gdo0_state = 0;

    switch (args[ARG_debug].u_int) {
        case 0: self->debug = 0; break;
        case 1: self->debug = 1; break;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Unknown debug argument")); return mp_const_none;
    }
    switch (args[ARG_debug_hst].u_int) {
        case 0: self->debug_hst = 0; break;
        case 1: self->debug_hst = 1; break;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Unknown debug_hst argument")); return mp_const_none;
    }

    mp_obj_t spi_args[] = {
        mp_obj_new_int(args[ARG_id].u_int),
        mp_obj_new_int(args[ARG_cs].u_int),
        mp_obj_new_int(args[ARG_baudrate].u_int),

        MP_OBJ_NEW_QSTR(MP_QSTR_polarity),
        mp_obj_new_int(args[ARG_polarity].u_int),
        MP_OBJ_NEW_QSTR(MP_QSTR_phase),
        mp_obj_new_int(args[ARG_phase].u_int),
        MP_OBJ_NEW_QSTR(MP_QSTR_bits),
        mp_obj_new_int(args[ARG_bits].u_int),
        MP_OBJ_NEW_QSTR(MP_QSTR_firstbit),
        mp_obj_new_int(args[ARG_firstbit].u_int),
        MP_OBJ_NEW_QSTR(MP_QSTR_sck),
        args[ARG_sck].u_obj,
        MP_OBJ_NEW_QSTR(MP_QSTR_mosi),
        args[ARG_mosi].u_obj,
        MP_OBJ_NEW_QSTR(MP_QSTR_miso),
        args[ARG_miso].u_obj,
    };
    self->spi = MP_OBJ_TO_PTR(machine_hw_spi_make_new(&machine_spi_type, 3, 7, spi_args));

    _cc1101_debug(self, "CC1101 object created");
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t cc1101_deinit(mp_obj_t self_in) {
    cc1101_obj_t * self = MP_OBJ_TO_PTR(self_in);
    machine_hw_spi_obj_t *spi = self->spi;

    mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(spi);
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
    spi_p->deinit(s);

    _cc1101_debug(self, "CC1101 object deinited");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_deinit_obj, cc1101_deinit);

// ----------------
// CC1101 SPI Read
// ----------------
uint8_t _cc1101_read(mp_obj_t self_in,  uint8_t m_addr, uint8_t need_debug) {
    cc1101_obj_t * self = self_in;
    //_cc1101_debug(self, "CC1101 read at 0x%02X", m_addr);
    byte rbyte = _cc1101_transfer_addr_byte(self, m_addr, 0); // The written value is ignored, reg value is read
    if(need_debug) _cc1101_debug(self, "CC1101 read at 0x%02X -> 0x%02X (%lu)", m_addr, rbyte, clock());
    return rbyte;
}
static mp_obj_t cc1101_read(mp_obj_t self_in,  mp_obj_t addr) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr);
    return mp_obj_new_int(_cc1101_read(self, m_addr, self->debug_hst));
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_read_obj, &cc1101_read);


static mp_obj_t cc1101_read_burst(mp_obj_t self_in, mp_obj_t addr, mp_obj_t length) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr);
    uint32_t m_length = mp_obj_get_int(length);

    vstr_t vstr;
    vstr_init_len(&vstr, m_length);

    uint8_t * m_write_buffer = malloc(m_length);
    if(m_write_buffer != NULL) {
         memset(m_write_buffer, 0, m_length);
        _cc1101_debug(self, "CC1101 read at 0x%02X 0x%02X bytes (%lu)", m_addr, m_length, clock());
        _cc1101_transfer_addr_burst(self, m_addr, m_write_buffer, (uint8_t*)vstr.buf, m_length);
        free(m_write_buffer);
        return mp_obj_new_bytes_from_vstr(&vstr);
        //return mp_obj_new_bytes_from_vstr(&vstr);
    } else mp_raise_CC1101Error(CC1101_MALLOC_FAILED);
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_read_burst_obj, &cc1101_read_burst);


// ---------
// SPI Write
// ---------
uint8_t _cc1101_write(mp_obj_t self_in, uint8_t m_addr, uint8_t m_wbyte) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 write at 0x%02X: 0x%02X (%lu)", m_addr, m_wbyte, clock());
    uint8_t rbyte = _cc1101_transfer_addr_byte(self, m_addr, m_wbyte);
    return rbyte;
}

static mp_obj_t cc1101_write(mp_obj_t self_in, mp_obj_t addr, mp_obj_t wbyte) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr);
    uint8_t m_wbyte = mp_obj_get_int(wbyte);
    return mp_obj_new_int(_cc1101_write(self, m_addr, m_wbyte));
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_write_obj, &cc1101_write);


void _cc1101_write_burst(mp_obj_t self_in, uint8_t w_addr, uint8_t* m_write_buffer, uint32_t m_length) {
    cc1101_obj_t * self = self_in;
    uint8_t * m_read_buffer = malloc(m_length);
    if(m_read_buffer != NULL) {
        _cc1101_debug(self, "CC1101 write at 0x%02X 0x%02X bytes (%lu)", w_addr, m_length, clock());
        _cc1101_transfer_addr_burst(self, w_addr, (uint8_t *)m_write_buffer, m_read_buffer, m_length);
        free(m_read_buffer);
    } else mp_raise_CC1101Error(CC1101_MALLOC_FAILED);
}
static mp_obj_t cc1101_write_burst(mp_obj_t self_in, mp_obj_t addr, mp_obj_t wbuffer) {
    cc1101_obj_t * self = self_in;
    uint8_t w_addr = mp_obj_get_int(addr);

    mp_buffer_info_t m_write_buffer;
    mp_get_buffer_raise(wbuffer, &m_write_buffer, MP_BUFFER_READ);
    uint32_t m_length = m_write_buffer.len;
    _cc1101_write_burst(self, w_addr, (uint8_t *)m_write_buffer.buf, m_length);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_write_burst_obj, &cc1101_write_burst);


uint8_t _cc1101_command(mp_obj_t self_in, uint8_t m_wbyte) {
    cc1101_obj_t * self = self_in;
    byte rbyte = _cc1101_transfer_byte(self, m_wbyte);
    if(self->debug_hst) {
        char name[10];
        switch(m_wbyte) {
             case 0x30: strcpy(name, "set_sres");break;
             case 0x31: strcpy(name, "set_scalw");break;
             case 0x32: strcpy(name, "turn_off");break;
             case 0x33: strcpy(name, "set_scal");break;
             case 0x34: strcpy(name, "set_rx");break;
             case 0x35: strcpy(name, "set_tx");break;
             case 0x36: strcpy(name, "set_idle");break;
             case 0x38: strcpy(name, "start_WoR");break;
             case 0x39: strcpy(name, "set_PwD");break;
             case 0x3A: strcpy(name, "flush_rx");break;
             case 0x3B: strcpy(name, "flush_tx");break;
             case 0x3C: strcpy(name, "rst_clock");break;
             case 0x3D: strcpy(name, "no_op");break;
             default: strcpy(name, "unknown");
        }
        _cc1101_debug(self, "CC1101 command 0x%02X (%s) -> 0x%02X (%lu)", m_wbyte, name, rbyte, clock());
    }
    return rbyte;
}
static mp_obj_t cc1101_command(mp_obj_t self_in, mp_obj_t wbyte) {
    cc1101_obj_t * self = self_in;
    uint8_t m_wbyte = mp_obj_get_int(wbyte);
    return mp_obj_new_int(_cc1101_command(self, m_wbyte));
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_command_obj, &cc1101_command);


static mp_obj_t cc1101_strobe(mp_obj_t self_in, mp_obj_t wbyte) {
    cc1101_obj_t * self = self_in;
    uint8_t m_wbyte = mp_obj_get_int(wbyte);
    return mp_obj_new_int(_cc1101_command(self, m_wbyte));
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_strobe_obj, &cc1101_strobe);

static mp_obj_t cc1101_flush(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    // Nothing to do
    _cc1101_debug(self, "CC1101 flush FIFO buffers");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_flush_obj, &cc1101_flush);


// ------------
// CC1101 Print
// ------------

void cc1101_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cc1101_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "CC1101 SPI%d_CS%d(baudrate:%d, spi_mode:%s)", self->spi->id, self->spi->cs, self->spi->baudrate, "FULL_DUPLEX");
}

// ---------------
// CC1101 specific
// ---------------

// byte SpiReadStatus(byte addr);
uint8_t _cc1101_spi_read_status(mp_obj_t self_in, uint8_t addr, uint8_t need_debug) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = addr | READ_BURST;
    byte rbyte = _cc1101_read(self,  m_addr, need_debug);
    if(need_debug) _cc1101_debug(self, "CC1101 read status at 0x%02X: 0x%02X (%lu)", addr, rbyte, clock());
    return rbyte;
}
static mp_obj_t cc1101_spi_read_status(mp_obj_t self_in, mp_obj_t addr) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr);
    return mp_obj_new_int(_cc1101_spi_read_status(self,  m_addr, self->debug_hst));
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_spi_read_status_obj, &cc1101_spi_read_status);


// void SpiWriteReg(byte addr, byte value);
static mp_obj_t cc1101_spi_write_reg(mp_obj_t self_in, mp_obj_t addr, mp_obj_t value) {
    cc1101_obj_t * self = self_in;
    cc1101_write(self, addr, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_spi_write_reg_obj, &cc1101_spi_write_reg);

// void SpiWriteBurstReg(byte addr, byte *buffer, byte num);
static mp_obj_t cc1101_spi_write_burst_reg(mp_obj_t self_in, mp_obj_t addr, mp_obj_t wbuffer) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr) | WRITE_BURST;
    cc1101_write_burst(self, mp_obj_new_int(m_addr), wbuffer);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_spi_write_burst_reg_obj, &cc1101_spi_write_burst_reg);

// byte SpiReadReg(byte addr);
static mp_obj_t cc1101_spi_read_reg(mp_obj_t self_in, mp_obj_t addr) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr) | READ_SINGLE;
    return cc1101_read(self, mp_obj_new_int(m_addr));
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_spi_read_reg_obj, &cc1101_spi_read_reg);

// void SpiReadBurstReg(byte addr, byte *buffer, byte num);
static mp_obj_t cc1101_spi_read_burst_reg(mp_obj_t self_in, mp_obj_t addr, mp_obj_t num) {
    cc1101_obj_t * self = self_in;
    uint8_t m_addr = mp_obj_get_int(addr) | READ_BURST;
    return cc1101_read_burst(self, mp_obj_new_int(m_addr), num);
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_spi_read_burst_reg_obj, &cc1101_spi_read_burst_reg);


// void setPA(int p);
static mp_obj_t cc1101_set_pa(mp_obj_t self_in, mp_obj_t p_in) {
    cc1101_obj_t * self = self_in;
    int8_t m_p = mp_obj_get_int(p_in);

    int p = 0;
    self->power = m_p;
    _cc1101_debug(self, "CC1101 set_pa(%d)", self->power);

    if (self->MHz >= 300 && self->MHz <= 348) {
        if (m_p <= -30)                   p = P_TABLE_315[0];
        else if (m_p > -30 && m_p <= -20) p = P_TABLE_315[1];
        else if (m_p > -20 && m_p <= -15) p = P_TABLE_315[2];
        else if (m_p > -15 && m_p <= -10) p = P_TABLE_315[3];
        else if (m_p > -10 && m_p <= 0)   p = P_TABLE_315[4];
        else if (m_p > 0   && m_p <= 5)   p = P_TABLE_315[5];
        else if (m_p > 5   && m_p <= 7)   p = P_TABLE_315[6];
        else if (m_p > 7)                 p = P_TABLE_315[7];
        self->last_power = 1;
    }
    else if (self->MHz >= 378 && self->MHz <= 464) {
        if (m_p <= -30)                   p = P_TABLE_433[0];
        else if (m_p > -30 && m_p <= -20) p = P_TABLE_433[1];
        else if (m_p > -20 && m_p <= -15) p = P_TABLE_433[2];
        else if (m_p > -15 && m_p <= -10) p = P_TABLE_433[3];
        else if (m_p > -10 && m_p <= 0)   p = P_TABLE_433[4];
        else if (m_p > 0   && m_p <= 5)   p = P_TABLE_433[5];
        else if (m_p > 5   && m_p <= 7)   p = P_TABLE_433[6];
        else if (m_p > 7)                 p = P_TABLE_433[7];
        self->last_power = 2;
    }
    else if (self->MHz >= 779 && self->MHz <= 899.99) {
        if (m_p <= -30)                   p = P_TABLE_868[0];
        else if (m_p > -30 && m_p <= -20) p = P_TABLE_868[1];
        else if (m_p > -20 && m_p <= -15) p = P_TABLE_868[2];
        else if (m_p > -15 && m_p <= -10) p = P_TABLE_868[3];
        else if (m_p > -10 && m_p <= -6)  p = P_TABLE_868[4];
        else if (m_p > -6  && m_p <= 0)   p = P_TABLE_868[5];
        else if (m_p > 0   && m_p <= 5)   p = P_TABLE_868[6];
        else if (m_p > 5   && m_p <= 7)   p = P_TABLE_868[7];
        else if (m_p > 7   && m_p <= 10)  p = P_TABLE_868[8];
        else if (m_p > 10)                p = P_TABLE_868[9];
        self->last_power = 3;
    }
    else if (self->MHz >= 900 && self->MHz <= 928) {
        if (m_p <= -30)                   p = P_TABLE_915[0];
        else if (m_p > -30 && m_p <= -20) p = P_TABLE_915[1];
        else if (m_p > -20 && m_p <= -15) p = P_TABLE_915[2];
        else if (m_p > -15 && m_p <= -10) p = P_TABLE_915[3];
        else if (m_p > -10 && m_p <= -6)  p = P_TABLE_915[4];
        else if (m_p > -6  && m_p <= 0)   p = P_TABLE_915[5];
        else if (m_p > 0   && m_p <= 5)   p = P_TABLE_915[6];
        else if (m_p > 5   && m_p <= 7)   p = P_TABLE_915[7];
        else if (m_p > 7   && m_p <= 10)  p = P_TABLE_915[8];
        else if (m_p > 10)                p = P_TABLE_915[9];
        self->last_power = 4;
    }
    if (self->modulation == 2) {
       self->P_TABLE[0] = 0;
       self->P_TABLE[1] = p;
    } else {
       self->P_TABLE[0] = p;
       self->P_TABLE[1] = 0;
    }
    _cc1101_write_burst(self, CC1101_PATABLE, self->P_TABLE, sizeof(self->P_TABLE));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_pa_obj, &cc1101_set_pa);


// void setModulation(byte m);
static mp_obj_t cc1101_set_modulation(mp_obj_t self_in, mp_obj_t modulation) {
    cc1101_obj_t * self = MP_OBJ_TO_PTR(self_in);
    uint8_t m_modulation = mp_obj_get_int(modulation);
    if (m_modulation > 4) m_modulation = 4;
    self->modulation = m_modulation;
    uint8_t frend0 = 0x10;
    _cc1101_debug(self, "CC1101 set_modulation(%d)", self->modulation);

    _cc1101_split_MDMCFG2(self);
    switch (m_modulation) {
        case 0: self->m2MODFM = 0x00; frend0 = 0x10; break; // 2-FSK
        case 1: self->m2MODFM = 0x10; frend0 = 0x10; break; // GFSK
        case 2: self->m2MODFM = 0x30; frend0 = 0x11; break; // ASK
        case 3: self->m2MODFM = 0x40; frend0 = 0x10; break; // 4-FSK
        case 4: self->m2MODFM = 0x70; frend0 = 0x10; break; // MSK
    }
    _cc1101_write(self, CC1101_MDMCFG2, self->m2DCOFF + self->m2MODFM + self->m2MANCH + self->m2SYNCM);
    _cc1101_write(self, CC1101_FREND0,  frend0);
    cc1101_set_pa(self, mp_obj_new_int(self->power));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_modulation_obj, &cc1101_set_modulation);


// void setCCMode(bool s);
static mp_obj_t cc1101_set_cc_mode(mp_obj_t self_in, mp_obj_t mode) {
    cc1101_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t m_mode = mp_obj_get_int(mode);
    self->ccmode = m_mode;
    _cc1101_debug(self, "CC1101 set_cc_mode(%d)", self->ccmode);
    if(self->ccmode == 1) {
       _cc1101_write(self, CC1101_IOCFG2,   0x0B);
       _cc1101_write(self, CC1101_IOCFG0,   0x06);
       _cc1101_write(self, CC1101_PKTCTRL0, 0x05);
       _cc1101_write(self, CC1101_MDMCFG3,  0xF8);
       _cc1101_write(self, CC1101_MDMCFG4,  11 + self->m4RxBw);
    } else {
       _cc1101_write(self, CC1101_IOCFG2,   0x0D);
       _cc1101_write(self, CC1101_IOCFG0,   0x0D);
       _cc1101_write(self, CC1101_PKTCTRL0, 0x32);
       _cc1101_write(self, CC1101_MDMCFG3,  0x93);
       _cc1101_write(self, CC1101_MDMCFG4,  7 + self->m4RxBw);
    }
    cc1101_set_modulation(self, mp_obj_new_int(self->modulation));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_cc_mode_obj, &cc1101_set_cc_mode);


// void setMHZ(float mhz);
static mp_obj_t cc1101_set_mhz(mp_obj_t self_in, mp_obj_t mhz_in) {
    cc1101_obj_t * self = self_in;
    uint8_t freq2 = 0;
    uint8_t freq1 = 0;
    uint32_t freq0 = 0;
    float mhz = mp_obj_get_float(mhz_in);

    self->MHz = mhz;

    while(1) {
        if (mhz >= 26) {
            mhz -= 26;
            freq2 += 1;
        }
        else if (mhz >= 0.1015625) {
            mhz -= 0.1015625;
            freq1 += 1;
        }
        else if (mhz >= 0.00039675) {
            mhz -= 0.00039675;
            freq0 += 1;
        }
        else break;
    }
    if (freq0 > 255) {
        freq1 += 1;
        freq0 -= 256;
    }

    _cc1101_write(self, CC1101_FREQ2, freq2);
    _cc1101_write(self, CC1101_FREQ1, freq1);
    _cc1101_write(self, CC1101_FREQ0, freq0);

    _cc1101_calibrate(self);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_mhz_obj, &cc1101_set_mhz);

// void getMHZ(float mhz);
static mp_obj_t cc1101_get_mhz(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    uint8_t freq2 = _cc1101_read(self, CC1101_FREQ2 | READ_SINGLE, self->debug_hst);
    uint8_t freq1 = _cc1101_read(self, CC1101_FREQ1 | READ_SINGLE, self->debug_hst);
    uint8_t freq0 = _cc1101_read(self, CC1101_FREQ0 | READ_SINGLE, self->debug_hst);
    float mhz = 26 * freq2 + 26/256.0 * freq1 + 26/256.0/256.0 * freq0;
    self->MHz = mhz;

    return mp_obj_new_float(mhz);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_get_mhz_obj, &cc1101_get_mhz);


void _cc1101_calibrate(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 calibrate()");
    if (self->MHz >= 300 && self->MHz <= 348) {
        _cc1101_write(self, CC1101_FSCTRL0, _cc1101_map(self->MHz, 300, 348, self->clb1[0], self->clb1[1]));
        if (self->MHz < 322.88) _cc1101_write(self, CC1101_TEST0, 0x0B);
        else {
            _cc1101_write(self, CC1101_TEST0, 0x09);
            int s = _cc1101_spi_read_status(self, CC1101_FSCAL2, self->debug_hst);
            if (s < 32) _cc1101_write(self, CC1101_FSCAL2, s + 32);
            if (self->last_power != 1) cc1101_set_pa(self, mp_obj_new_int(self->power));
        }
    }
    else if (self->MHz >= 378 && self->MHz <= 464) {
        _cc1101_write(self, CC1101_FSCTRL0, _cc1101_map(self->MHz, 378, 464, self->clb2[0], self->clb2[1]));
        if (self->MHz < 430.5) _cc1101_write(self, CC1101_TEST0, 0x0B);
        else {
            _cc1101_write(self, CC1101_TEST0, 0x09);
            int s = _cc1101_spi_read_status(self, CC1101_FSCAL2, self->debug_hst);
            if (s < 32) _cc1101_write(self, CC1101_FSCAL2, s + 32);
            if (self->last_power != 2) cc1101_set_pa(self, mp_obj_new_int(self->power));
        }
    }
    else if (self->MHz >= 779 && self->MHz <= 899.99) {
        _cc1101_write(self, CC1101_FSCTRL0, _cc1101_map(self->MHz, 779, 899, self->clb3[0], self->clb3[1]));
        if (self->MHz < 861) _cc1101_write(self, CC1101_TEST0,0x0B);
        else {
            _cc1101_write(self, CC1101_TEST0,0x09);
            int s = _cc1101_spi_read_status(self, CC1101_FSCAL2, self->debug_hst);
            if (s < 32) _cc1101_write(self, CC1101_FSCAL2, s + 32);
            if (self->last_power != 3) cc1101_set_pa(self, mp_obj_new_int(self->power));
        }
    }
    else if (self->MHz >= 900 && self->MHz <= 928) {
        _cc1101_write(self, CC1101_FSCTRL0, _cc1101_map(self->MHz, 900, 928, self->clb4[0], self->clb4[1]));
        _cc1101_write(self, CC1101_TEST0, 0x09);
        int s = _cc1101_spi_read_status(self, CC1101_FSCAL2, self->debug_hst);
        if (s < 32) _cc1101_write(self, CC1101_FSCAL2, s+32);
        if (self->last_power != 4) cc1101_set_pa(self, mp_obj_new_int(self->power));
    }
}


// void setChannel(byte chnl);
static mp_obj_t cc1101_set_channel(mp_obj_t self_in, mp_obj_t chan) {
    cc1101_obj_t * self = self_in;
    uint8_t m_chan = mp_obj_get_int(chan);
    self->chan = m_chan;
    _cc1101_debug(self, "CC1101 set_channel(%d)", self->chan);
    _cc1101_write(self, CC1101_CHANNR, self->chan);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_channel_obj, &cc1101_set_channel);

// void getChannel(byte chnl);
static mp_obj_t cc1101_get_channel(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    uint8_t m_chan = _cc1101_read(self, CC1101_CHANNR | READ_SINGLE, self->debug_hst);
    self->chan = m_chan;
    return mp_obj_new_int(self->chan);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_get_channel_obj, &cc1101_get_channel);

// void setChsp(float f);
static mp_obj_t cc1101_set_chsp(mp_obj_t self_in, mp_obj_t f_in) {
    cc1101_obj_t * self = self_in;
    float f = mp_obj_get_float(f_in);

    _cc1101_debug(self, "CC1101 set_chsp(%f)", f);

    _cc1101_split_MDMCFG1(self);
    uint8_t MDMCFG0 = 0;
    self->m1CHSP = 0;
    if (f > 405.456543) f = 405.456543;
    if (f < 25.390625) f = 25.390625;
    for (int i = 0; i < 5; i++) {
        if (f <= 50.682068) {
            f -= 25.390625;
            f /= 0.0991825;
            MDMCFG0 = f;
            float s1 = (f - MDMCFG0) *10;
            if (s1 >= 5)MDMCFG0++;
            i = 5;
        } else {
            self->m1CHSP++;
            f /= 2;
        }
    }
    _cc1101_write(self, CC1101_MDMCFG1, self->m1CHSP + self->m1FEC + self->m1PRE);
    _cc1101_write(self, CC1101_MDMCFG0, MDMCFG0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_chsp_obj, &cc1101_set_chsp);

// void setRxBW(float f);
static mp_obj_t cc1101_set_rx_bw(mp_obj_t self_in, mp_obj_t f_in) {
    cc1101_obj_t * self = self_in;
    float f = mp_obj_get_float(f_in);

    _cc1101_debug(self, "CC1101 set_rxbw(%f)", f);

    _cc1101_split_MDMCFG4(self);
    int s1 = 3;
    int s2 = 3;
    for (int i = 0; i < 3; i++) {
        if (f > 101.5625) { f /= 2; s1--; }
        else i=3;
    }
    for (int i = 0; i < 3; i++) {
        if (f > 58.1) { f /= 1.25; s2--; }
        else i=3;
    }
    s1 *= 64;
    s2 *= 16;
    self->m4RxBw = s1 + s2;
    _cc1101_write(self, CC1101_MDMCFG4, self->m4RxBw + self->m4DaRa);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_rx_bw_obj, &cc1101_set_rx_bw);

// void setDRate(float d);
static mp_obj_t cc1101_set_d_rate(mp_obj_t self_in, mp_obj_t d_in) {
    cc1101_obj_t * self = self_in;
    float d = mp_obj_get_float(d_in);

    _cc1101_split_MDMCFG4(self);
    uint8_t MDMCFG3 = 0;
    if (d > 1621.83) d = 1621.83;
    if (d < 0.0247955) d = 0.0247955;
    self->m4DaRa = 0;
    for (int i = 0; i<20; i++) {
        if (d <= 0.0494942) {
            d = d - 0.0247955;
            d = d / 0.00009685;
            MDMCFG3 = d;
            float s1 = (d - MDMCFG3) * 10;
            if (s1 >= 5) MDMCFG3++;
            i = 20;
        } else {
            self->m4DaRa++;
            d = d / 2;
        }
    }
    _cc1101_write(self, CC1101_MDMCFG4,  self->m4RxBw + self->m4DaRa);
    _cc1101_write(self, CC1101_MDMCFG3,  MDMCFG3);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_d_rate_obj, &cc1101_set_d_rate);

// void setDeviation(float d);
static mp_obj_t cc1101_set_deviation(mp_obj_t self_in, mp_obj_t d_in) {
    cc1101_obj_t * self = self_in;
    float d = mp_obj_get_float(d_in);

    _cc1101_debug(self, "CC1101 set_deviation(%f)", d);

    float f = 1.586914;
    float v = 0.19836425;
    int c = 0;
    if (d > 380.859375) d = 380.859375;
    if (d < 1.586914) d = 1.586914;
    for (int i = 0; i < 255; i++) {
        f += v;
        if (c == 7) {
            v *= 2;
            c =- 1;
            i += 8;
        }
        if (f >= d) {
            c = i;
            i = 255;
        }
        c++;
    }
    _cc1101_write(self, CC1101_DEVIATN, c);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_deviation_obj, &cc1101_set_deviation);

// void SetTx(void);
static mp_obj_t cc1101_set_tx(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_tx()");
    _cc1101_command(self, CC1101_SIDLE);
    _cc1101_command(self, CC1101_STX);
    self->trxstate = 1;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_tx_obj, &cc1101_set_tx);

// void SetRx(void);
static mp_obj_t cc1101_set_rx(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_rx()");
    _cc1101_command(self, CC1101_SIDLE);
    _cc1101_command(self, CC1101_SRX);
    self->trxstate = 2;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_rx_obj, &cc1101_set_rx);

// void SetTx(float mhz);
static mp_obj_t cc1101_set_tx_freq(mp_obj_t self_in, mp_obj_t mhz) {
    cc1101_obj_t * self = self_in;
    float f = mp_obj_get_float(mhz);
    _cc1101_debug(self, "CC1101 set_tx_freq(%f)", f);
    _cc1101_command(self, CC1101_SIDLE);
    cc1101_set_mhz(self, mhz);
    _cc1101_command(self, CC1101_STX);
    self->trxstate = 1;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_tx_freq_obj, &cc1101_set_tx_freq);

// void SetRx(float mhz);
static mp_obj_t cc1101_set_rx_freq(mp_obj_t self_in, mp_obj_t mhz) {
    cc1101_obj_t * self = self_in;
    float f = mp_obj_get_float(mhz);
    _cc1101_debug(self, "CC1101 set_rx_freq(%f)", f);
    _cc1101_command(self, CC1101_SIDLE);
    cc1101_set_mhz(self, mhz);
    _cc1101_command(self, CC1101_SRX);
    self->trxstate = 2;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_rx_freq_obj, &cc1101_set_rx_freq);


// int getRssi(void);
static mp_obj_t cc1101_get_rssi(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    int rssi;
    _cc1101_debug(self, "CC1101 get_rssi()");
    rssi = _cc1101_spi_read_status(self, CC1101_RSSI, self->debug_hst);
    if (rssi >= 128) rssi = (rssi - 256) / 2 - 74;
    else rssi = (rssi / 2) - 74;
    return mp_obj_new_int(rssi);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_get_rssi_obj, &cc1101_get_rssi);


// byte getLqi(void);
static mp_obj_t cc1101_get_lqi(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 get_lqi()");
    uint8_t lqi = _cc1101_spi_read_status(self, CC1101_LQI, self->debug_hst);
    return mp_obj_new_int(lqi);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_get_lqi_obj, &cc1101_get_lqi);


// void setSres(void);
static mp_obj_t cc1101_set_sres(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_sres()");
    _cc1101_command(self, CC1101_SRES);
    self->trxstate = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_sres_obj, &cc1101_set_sres);

// Enable and calibrate frequency synthesizer
static mp_obj_t cc1101_set_sfs_tx_on(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_sfs_tx_on()");
    _cc1101_command(self, CC1101_SFSTXON);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_sfs_tx_on_obj, &cc1101_set_sfs_tx_on);

// Turn off crystal oscillator
static mp_obj_t cc1101_set_sx_off(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_sx_off()");
    _cc1101_command(self, CC1101_SXOFF);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_sx_off_obj, &cc1101_set_sx_off);

// Calibrate frequency synthesizer and turn it off
static mp_obj_t cc1101_set_scal(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_scal()");
    _cc1101_command(self, CC1101_SCAL);
    vTaskDelay(1); // About 10 ms
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_scal_obj, &cc1101_set_scal);

// Enable RX
static mp_obj_t cc1101_set_srx(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_srx()");
    _cc1101_command(self, CC1101_SRX);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_srx_obj, &cc1101_set_srx);

// Enable TX
static mp_obj_t cc1101_set_stx(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_stx()");
    _cc1101_command(self, CC1101_STX);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_stx_obj, &cc1101_set_stx);

// Ends RX and/or TX, turn off frequency synthesizer and exit
static mp_obj_t cc1101_set_sidle(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 set_sidle()");
    _cc1101_command(self, CC1101_SIDLE);
    self->trxstate = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_set_sidle_obj, &cc1101_set_sidle);

// Flush RX FIFO buffer
static mp_obj_t cc1101_flush_rx(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 flush_rx()");
    _cc1101_command(self, CC1101_SFRX);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_flush_rx_obj, &cc1101_flush_rx);

// Flush TX FIFO buffer
static mp_obj_t cc1101_flush_tx(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 flush_tx()");
    _cc1101_command(self, CC1101_SFTX);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_flush_tx_obj, &cc1101_flush_tx);


// void goSleep(void);
static mp_obj_t cc1101_go_sleep(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 go_sleep()");
    _cc1101_command(self, CC1101_SIDLE);
    _cc1101_command(self, CC1101_SPWD);
    self->trxstate = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_go_sleep_obj, &cc1101_go_sleep);

// void SendData(byte *txBuffer, byte size);
static mp_obj_t cc1101_send_data(mp_obj_t self_in, mp_obj_t wbuffer, mp_obj_t timeout) {
    cc1101_obj_t * self = self_in;
    uint32_t m_timeout = mp_obj_get_int(timeout);
    _cc1101_debug(self, "CC1101 send_data()");

    mp_buffer_info_t m_write_buffer;
    mp_get_buffer_raise(wbuffer, &m_write_buffer, MP_BUFFER_READ);
    uint32_t m_length = m_write_buffer.len;
    _cc1101_write(self, CC1101_TXFIFO, m_length);
    mp_hal_delay_us(10); // was 5, do not touch !!!!
    _cc1101_write_burst(self, CC1101_TXFIFO | WRITE_BURST, (uint8_t *)m_write_buffer.buf, m_length);

    _cc1101_command(self, CC1101_SIDLE); // 0x36
    _cc1101_command(self, CC1101_STX);   // 0x35

    if(m_timeout == 0) {
        while(!_cc1101_digital_read_gdo0(self, 1)); // Wait for GDO0 to be set -> sync transmitted
        while(_cc1101_digital_read_gdo0(self, 1));  // Wait for GDO0 to be cleared -> end of packet
    } else {
       if(m_timeout / portTICK_PERIOD_MS == 0) vTaskDelay(1);
       else vTaskDelay(m_timeout / portTICK_PERIOD_MS);
    }

    _cc1101_command(self, CC1101_SFTX);
    self->trxstate = 1;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_send_data_obj, &cc1101_send_data);

// byte CheckReceiveFlag(void);
static mp_obj_t cc1101_check_receive_flag(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    uint32_t counter = 0;
    uint32_t timeout = 100000; // delay 100ms

    if (self->trxstate != 2) cc1101_set_rx(self);
    uint8_t val = _cc1101_digital_read_gdo0(self, 1);
    if(self->gdo0_state == 1 && val == 0) return mp_obj_new_int(1); // reentered with changed GDO0

    if (val) {
        while (val && counter < timeout) {
           val = _cc1101_digital_read_gdo0(self, 1);
           mp_hal_delay_us(1);
           counter++;
        }
        self->gdo0_state = val;
        if(counter == timeout) return mp_obj_new_int(0);
        return mp_obj_new_int(1);
    }
    else return mp_obj_new_int(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_check_receive_flag_obj, &cc1101_check_receive_flag);

// byte ReceiveData(byte *rxBuffer);
static mp_obj_t cc1101_receive_data(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;

    if(_cc1101_spi_read_status(self, CC1101_RXBYTES, self->debug_hst) & BYTES_IN_RXFIFO)
    {
        uint8_t size = _cc1101_read(self, CC1101_RXFIFO | READ_SINGLE, self->debug_hst);
        mp_hal_delay_us(5); // do not touch !!!

        //SpiReadBurstReg(CC1101_RXFIFO,rxBuffer,size);
        mp_obj_t rxbuffer = cc1101_read_burst(self, mp_obj_new_int(CC1101_RXFIFO | READ_BURST), mp_obj_new_int(size));

        //SpiReadBurstReg(CC1101_RXFIFO,status,2);
        mp_obj_t status = cc1101_read_burst(self, mp_obj_new_int(CC1101_RXFIFO | READ_BURST), mp_obj_new_int(2));
        uint8_t m_status[2];
        memset(m_status, 0, sizeof(m_status));
        mp_buffer_info_t m_status_buffer;
        mp_get_buffer_raise(status, &m_status_buffer, MP_BUFFER_READ);
        if (m_status_buffer.len == 2) {
            m_status[0] = *((uint8_t*)m_status_buffer.buf);
            m_status[1] = *((uint8_t*)m_status_buffer.buf + 1);
            _cc1101_debug(self, "CC1101 receive_data() -> 0x%02X, 0x%02X (%lu)", m_status[0], m_status[1], clock());
        }

        //_cc1101_command(self, CC1101_SFRX);
        //_cc1101_command(self, CC1101_SRX);

        // Rearm RX
        _cc1101_command(self, CC1101_SFRX);
        _cc1101_command(self, CC1101_SIDLE);
        _cc1101_command(self, CC1101_SRX);
        self->trxstate = 2;

        return rxbuffer;
    } else {
        //_cc1101_command(self, CC1101_SFRX);
        //_cc1101_command(self, CC1101_SRX);

        // Rearm RX
        _cc1101_command(self, CC1101_SFRX);
        _cc1101_command(self, CC1101_SIDLE);
        _cc1101_command(self, CC1101_SRX);
        self->trxstate = 2;

        vstr_t vstr;
        vstr_init_len(&vstr, 0);
        _cc1101_debug(self, "CC1101 receive_data() -> NODATA");
        return mp_obj_new_bytes_from_vstr(&vstr);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_receive_data_obj, &cc1101_receive_data);


// bool CheckCRC(void);
static mp_obj_t cc1101_check_crc(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    uint8_t lqi = _cc1101_spi_read_status(self, CC1101_LQI, self->debug_hst);
    uint8_t crc_ok = (lqi & 0x80) >> 7;
    if (crc_ok == 1) {
         _cc1101_debug(self, "CC1101 check_crc() -> 0x01");
         return mp_obj_new_int(1);
    } else {
        _cc1101_debug(self, "CC1101 check_crc() -> 0x00");
        _cc1101_command(self, CC1101_SFRX);
        _cc1101_command(self, CC1101_SRX);
        return mp_obj_new_int(0);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_check_crc_obj, &cc1101_check_crc);

// void setClb(byte b, byte s, byte e);
static mp_obj_t cc1101_set_clb(size_t n_args, const mp_obj_t *arg) {
    if (n_args != 4) {
        mp_raise_CC1101Error("4 arguments required\n");
    } else {
        cc1101_obj_t * self = MP_OBJ_TO_PTR(arg[0]);
        uint8_t b = mp_obj_get_int(arg[1]);
        uint8_t s = mp_obj_get_int(arg[2]);
        uint8_t e = mp_obj_get_int(arg[3]);
        _cc1101_debug(self, "CC1101 set_clb(0x%02X, 0x%02X, 0x%02X)", b, s, e);

        if (b == 1) {
            self->clb1[0] = s;
            self->clb1[1] = e;
        }
        else if (b == 2) {
            self->clb2[0] = s;
            self->clb2[1] = e;
        }
        else if (b == 3) {
            self->clb3[0] = s;
            self->clb3[1] = e;
        }
        else if (b == 4) {
            self->clb4[0] = s;
            self->clb4[1] = e;
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cc1101_set_clb_obj, 4, 4, &cc1101_set_clb);

// bool getCC1101(void);
static mp_obj_t cc1101_get_cc1101(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 get_cc1101()");
    uint8_t status = _cc1101_spi_read_status(self, CC1101_VERSION, self->debug_hst);
    if (status != 0 && status != 0xFF) return mp_obj_new_int(1);
    else return mp_obj_new_int(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_get_cc1101_obj, &cc1101_get_cc1101);

// byte getMode(void);
static mp_obj_t cc1101_get_mode(mp_obj_t self_in) {
    cc1101_obj_t * self = self_in;
    _cc1101_debug(self, "CC1101 get_mode()");
    return mp_obj_new_int(self->trxstate);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc1101_get_mode_obj, &cc1101_get_mode);

// void setSyncWord(byte sh, byte sl);
static mp_obj_t cc1101_set_sync_word(mp_obj_t self_in, mp_obj_t sh, mp_obj_t sl) {
    cc1101_obj_t * self = self_in;
    uint8_t m_sh = mp_obj_get_int(sh);
    uint8_t m_sl = mp_obj_get_int(sl);
    _cc1101_debug(self, "CC1101 set_sync_word(0x%02X, 0x%02X)", m_sh, m_sl);
    _cc1101_write(self, CC1101_SYNC1, m_sh);
    _cc1101_write(self, CC1101_SYNC0, m_sl);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(cc1101_set_sync_word_obj, &cc1101_set_sync_word);

// void setAddr(byte v);
static mp_obj_t cc1101_set_addr(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_addr(0x%02X)", m_v);
    _cc1101_write(self, CC1101_ADDR, m_v);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_addr_obj, &cc1101_set_addr);

// void setWhiteData(bool v);
static mp_obj_t cc1101_set_white_data(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_white_data(0x%02X)", m_v);
    _cc1101_split_PKTCTRL0(self);
    self->pc0WDATA = 0;
    if (m_v == 1) self->pc0WDATA = 64;
    _cc1101_write(self, CC1101_PKTCTRL0, self->pc0WDATA + self->pc0PktForm + self->pc0CRC_EN + self->pc0LenConf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_white_data_obj, &cc1101_set_white_data);

// void setPktFormat(byte v);
static mp_obj_t cc1101_set_pkt_format(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_pkt_format(0x%02X)", m_v);
    _cc1101_split_PKTCTRL0(self);
    self->pc0PktForm = 0;
    if (m_v > 3) m_v = 3;
    self->pc0PktForm = m_v * 16;
    _cc1101_write(self, CC1101_PKTCTRL0, self->pc0WDATA + self->pc0PktForm + self->pc0CRC_EN + self->pc0LenConf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_pkt_format_obj, &cc1101_set_pkt_format);

// void setCrc(bool v);
static mp_obj_t cc1101_set_crc(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_crc(0x%02X)", m_v);
    _cc1101_split_PKTCTRL0(self);
    self->pc0CRC_EN = 0;
    if (m_v == 1) self->pc0CRC_EN = 4;
    _cc1101_write(self, CC1101_PKTCTRL0, self->pc0WDATA + self->pc0PktForm + self->pc0CRC_EN + self->pc0LenConf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_crc_obj, &cc1101_set_crc);

// void setLengthConfig(byte v);
static mp_obj_t cc1101_set_length_config(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_length_config(0x%02X)", m_v);
    _cc1101_split_PKTCTRL0(self);
    self->pc0LenConf = 0;
    if (m_v > 3) m_v = 3;
    self->pc0LenConf = m_v;
    _cc1101_write(self, CC1101_PKTCTRL0, self->pc0WDATA + self->pc0PktForm + self->pc0CRC_EN + self->pc0LenConf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_length_config_obj, &cc1101_set_length_config);

// void setPacketLength(byte v);
static mp_obj_t cc1101_set_packet_length(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_packet_length(0x%02X)", m_v);
    _cc1101_write(self, CC1101_PKTLEN, m_v);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_packet_length_obj, &cc1101_set_packet_length);

// void setDcFilterOff(bool v);
static mp_obj_t cc1101_set_dc_filter_off(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_dc_filter_off(0x%02X)", m_v);
    _cc1101_split_MDMCFG2(self);
    self->m2DCOFF = 0;
    if (m_v == 1) self->m2DCOFF = 128;
    _cc1101_write(self, CC1101_MDMCFG2, self->m2DCOFF + self->m2MODFM + self->m2MANCH + self->m2SYNCM);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_dc_filter_off_obj, &cc1101_set_dc_filter_off);

// void setManchester(bool v);
static mp_obj_t cc1101_set_manchester(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_manchester(0x%02X)", m_v);
    _cc1101_split_MDMCFG2(self);
    self->m2MANCH = 0;
    if (m_v == 1) self->m2MANCH = 8;
    _cc1101_write(self, CC1101_MDMCFG2, self->m2DCOFF + self->m2MODFM + self->m2MANCH + self->m2SYNCM);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_manchester_obj, &cc1101_set_manchester);

// void setSyncMode(byte v);
static mp_obj_t cc1101_set_sync_mode(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_sync_mode(0x%02X)", m_v);
    _cc1101_split_MDMCFG2(self);
    self->m2SYNCM = 0;
    if (m_v > 7) m_v = 7;
    self->m2SYNCM = m_v;
    _cc1101_write(self, CC1101_MDMCFG2, self->m2DCOFF + self->m2MODFM + self->m2MANCH + self->m2SYNCM);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_sync_mode_obj, &cc1101_set_sync_mode);

// void setFEC(bool v);
static mp_obj_t cc1101_set_fec(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_fec(0x%02X)", m_v);
    _cc1101_split_MDMCFG1(self);
    self->m1FEC = 0;
    if (m_v == 1) self->m1FEC = 128;
    _cc1101_write(self, CC1101_MDMCFG1, self-> m1FEC + self->m1PRE + self->m1CHSP);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_fec_obj, &cc1101_set_fec);

// void setPRE(byte v);
static mp_obj_t cc1101_set_pre(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_pre(0x%02X)", m_v);
    _cc1101_split_MDMCFG1(self);
    self->m1PRE = 0;
    if (m_v > 7) m_v = 7;
    self->m1PRE = m_v * 16;
    _cc1101_write(self, CC1101_MDMCFG1, self->m1FEC + self->m1PRE + self->m1CHSP);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_pre_obj, &cc1101_set_pre);

// void setPQT(byte v);
static mp_obj_t cc1101_set_pqt(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_pqt(0x%02X)", m_v);
    _cc1101_split_PKTCTRL1(self);
    self->pc1PQT = 0;
    if (m_v > 7) m_v = 7;
    self->pc1PQT = m_v * 32;
    _cc1101_write(self, CC1101_PKTCTRL1, self->pc1PQT + self->pc1CRC_AF + self->pc1APP_ST + self->pc1ADRCHK);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_pqt_obj, &cc1101_set_pqt);

// void setCRC_AF(bool v);
static mp_obj_t cc1101_set_crc_af(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_crc_af(0x%02X)", m_v);
    _cc1101_split_PKTCTRL1(self);
    self->pc1CRC_AF = 0;
    if (m_v == 1) self->pc1CRC_AF = 8;
    _cc1101_write(self, CC1101_PKTCTRL1, self->pc1PQT + self->pc1CRC_AF + self->pc1APP_ST + self->pc1ADRCHK);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_crc_af_obj, &cc1101_set_crc_af);

// void setAppendStatus(bool v);
static mp_obj_t cc1101_set_append_status(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_append_status(0x%02X)", m_v);
    _cc1101_split_PKTCTRL1(self);
    self->pc1APP_ST = 0;
    if (m_v == 1) self->pc1APP_ST = 4;
    _cc1101_write(self, CC1101_PKTCTRL1, self->pc1PQT + self->pc1CRC_AF + self->pc1APP_ST + self->pc1ADRCHK);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_append_status_obj, &cc1101_set_append_status);

// void setAdrChk(byte v);
static mp_obj_t cc1101_set_adr_chk(mp_obj_t self_in, mp_obj_t v) {
    cc1101_obj_t * self = self_in;
    uint8_t m_v = mp_obj_get_int(v);
    _cc1101_debug(self, "CC1101 set_adr_chk(0x%02X)", m_v);
    _cc1101_split_PKTCTRL1(self);
    self->pc1ADRCHK = 0;
    if (m_v > 3) m_v = 3;
    self->pc1ADRCHK = m_v;
    _cc1101_write(self, CC1101_PKTCTRL1, self->pc1PQT + self->pc1CRC_AF + self->pc1APP_ST + self->pc1ADRCHK);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_set_adr_chk_obj, &cc1101_set_adr_chk);

// uint8_t CheckRxFifo(int t);
static mp_obj_t cc1101_check_rx_fifo(mp_obj_t self_in, mp_obj_t t) {
    cc1101_obj_t * self = self_in;
    uint8_t m_t = mp_obj_get_int(t);
    if (self->trxstate != 2) cc1101_set_rx(self);
    if (_cc1101_spi_read_status(self, CC1101_RXBYTES, self->debug_hst) & BYTES_IN_RXFIFO) {
        _cc1101_debug(self, "CC1101 check_rx_fifo(0x%02X) -> 0x01", m_t);
        if(m_t / portTICK_PERIOD_MS == 0) vTaskDelay(1); // About 10 ms
        else vTaskDelay(m_t / portTICK_PERIOD_MS);
        return mp_obj_new_int(1);
    } else {
        _cc1101_debug(self, "CC1101 check_rx_fifo(0x%02X) -> 0x00", m_t);
        return mp_obj_new_int(0);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc1101_check_rx_fifo_obj, &cc1101_check_rx_fifo);


mp_obj_t cc1101_crc8(mp_obj_t self_in, mp_obj_t data, mp_obj_t polynome) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);
    uint8_t poly = mp_obj_get_int(polynome);

    uint8_t crc = 0;
    for (size_t i = 0; i < bufinfo.len; i++) {
        uint8_t bt = ((uint8_t *)bufinfo.buf)[i];
        crc ^= bt;
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ poly);
            else
                crc <<= 1;
        }
    }
    return MP_OBJ_NEW_SMALL_INT(crc);
}
MP_DEFINE_CONST_FUN_OBJ_3(cc1101_crc8_obj, cc1101_crc8);

mp_obj_t cc1101_ba2hex(mp_obj_t self_in, mp_obj_t data) {
    char mess[1024];
    memset(mess, 0, sizeof(mess));

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);
    uint32_t length = 0;
    for(int i = 0; i < bufinfo.len;i++) {
        if(length + 4 < sizeof(mess)) {
            length += snprintf(mess + length, sizeof(mess) - length, "%02X ", *((uint8_t*)bufinfo.buf + i));
        }
    }
    if(length > 0 && length < sizeof(mess)) mess[length - 1] = 0; // trim last space
    return mp_obj_new_str(mess, strlen(mess));
}
MP_DEFINE_CONST_FUN_OBJ_2(cc1101_ba2hex_obj, cc1101_ba2hex);


// ---------
// Internals
// ---------


void _cc1101_reset(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 reset()");
    _cc1101_command(self, CC1101_SRES);
}
void _cc1101_reg_config_settings(cc1101_obj_t * self) {

    _cc1101_debug(self, "CC1101 reg_config_settings()");
    _cc1101_write(self, CC1101_FSCTRL1,  0x06);

    cc1101_set_cc_mode(self, mp_obj_new_int(self->ccmode));
    cc1101_set_mhz(self, mp_obj_new_float(self->MHz));

    _cc1101_write(self, CC1101_MDMCFG1,  0x02);
    _cc1101_write(self, CC1101_MDMCFG0,  0xF8);
    _cc1101_write(self, CC1101_CHANNR,   self->chan);
    _cc1101_write(self, CC1101_DEVIATN,  0x47);
    _cc1101_write(self, CC1101_FREND1,   0x56);
    _cc1101_write(self, CC1101_MCSM0 ,   0x18);
    _cc1101_write(self, CC1101_FOCCFG,   0x16);
    _cc1101_write(self, CC1101_BSCFG,    0x1C);
    _cc1101_write(self, CC1101_AGCCTRL2, 0xC7);
    _cc1101_write(self, CC1101_AGCCTRL1, 0x00);
    _cc1101_write(self, CC1101_AGCCTRL0, 0xB2);
    _cc1101_write(self, CC1101_FSCAL3,   0xE9);
    _cc1101_write(self, CC1101_FSCAL2,   0x2A);
    _cc1101_write(self, CC1101_FSCAL1,   0x00);
    _cc1101_write(self, CC1101_FSCAL0,   0x1F);
    _cc1101_write(self, CC1101_FSTEST,   0x59);
    _cc1101_write(self, CC1101_TEST2,    0x81);
    _cc1101_write(self, CC1101_TEST1,    0x35);
    _cc1101_write(self, CC1101_TEST0,    0x09);
    _cc1101_write(self, CC1101_PKTCTRL1, 0x04);
    _cc1101_write(self, CC1101_ADDR,     0x00);
    _cc1101_write(self, CC1101_PKTLEN,   0x00);
}

void _cc1101_split_PKTCTRL0(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 split_PKTCTRL0");
    int calc = _cc1101_spi_read_status(self, CC1101_PKTCTRL0, self->debug_hst);
    self->pc0WDATA = 0;
    self->pc0PktForm = 0;
    self->pc0CRC_EN = 0;
    self->pc0LenConf = 0;
    while(1) {
        if (calc >= 64) {
            calc -= 64;
            self->pc0WDATA += 64;
        }
        else if (calc >= 16) {
            calc -= 16;
            self->pc0PktForm += 16;
        }
        else if (calc >= 4) {
            calc -= 4;
            self->pc0CRC_EN += 4;
        }
        else {
            self->pc0LenConf = calc;
            break;
        }
    }
}

void _cc1101_split_PKTCTRL1(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 split_PKTCTRL1");
    int calc = _cc1101_spi_read_status(self, CC1101_PKTCTRL1, self->debug_hst);
    self->pc1PQT = 0;
    self->pc1CRC_AF = 0;
    self->pc1APP_ST = 0;
    self->pc1ADRCHK = 0;
    while(1) {
        if (calc >= 32) {
            calc -= 32;
            self->pc1PQT += 32;
        }
        else if (calc >= 8) {
            calc -= 8;
            self->pc1CRC_AF += 8;
        }
        else if (calc >= 4) {
            calc -= 4;
            self->pc1APP_ST += 4;
        }
        else {
            self->pc1ADRCHK = calc;
            break;
        }
    }
}
void _cc1101_split_MDMCFG1(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 split_MDMCFG1");
    int calc = _cc1101_spi_read_status(self, CC1101_MDMCFG1, self->debug_hst);
    self->m1FEC = 0;
    self->m1PRE = 0;
    self->m1CHSP = 0;
    while(1) {
        if (calc >= 128) {
           calc -= 128;
           self->m1FEC += 128;
        }
        else if (calc >= 16) {
           calc -= 16;
           self->m1PRE += 16;
        }
        else {
           self->m1CHSP = calc;
           break;
        }
    }
}
void _cc1101_split_MDMCFG2(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 split_MDMCFG2");
    int calc = _cc1101_spi_read_status(self, CC1101_MDMCFG2, self->debug_hst);
    self->m2DCOFF = 0;
    self->m2MODFM = 0;
    self->m2MANCH = 0;
    self->m2SYNCM = 0;
    while(1) {
        if (calc >= 128) {
            calc -= 128;
            self->m2DCOFF += 128;
        }
        else if (calc >= 16) {
            calc -= 16;
            self->m2MODFM += 16;
        }
        else if (calc >= 8) {
            calc -= 8;
            self->m2MANCH += 8;
        }
        else {
            self->m2SYNCM = calc;
            break;
        }
    }
}

void _cc1101_split_MDMCFG4(cc1101_obj_t * self) {
    _cc1101_debug(self, "CC1101 split_MDMCFG4");
    int calc = _cc1101_spi_read_status(self, CC1101_MDMCFG4, self->debug_hst);
    self->m4RxBw = 0;
    self->m4DaRa = 0;
    while(1) {
        if (calc >= 64) {
            calc -= 64;
            self->m4RxBw += 64;
        }
        else if (calc >= 16) {
            calc -= 16;
            self->m4RxBw += 16;
        }
        else {
            self->m4DaRa = calc;
            break;
        }
    }
}

uint8_t _cc1101_digital_read_gdo0(cc1101_obj_t * self, uint8_t need_debug) {
    int rbyte =_cc1101_spi_read_status(self, CC1101_PKTSTATUS, 0) & 0x01;
    if(need_debug) _cc1101_debug(self, "CC1101 read_gdo0() -> 0x%02X (%lu)", rbyte, clock());
    mp_hal_delay_us(5); 
    return rbyte;
}

void _cc1101_debug(cc1101_obj_t * self, char * message, ...) {
    char mess[256];
    if(self->debug == 1 || self->debug_hst == 1) {
        memset(mess, 0, sizeof(mess));
        va_list args;
        va_start(args, message);
        vsnprintf(mess, sizeof(mess), message, args);
        va_end(args);
        if(self->debug_hst == 1) ESP_LOGE("CC1101","%s", mess);
        if(self->debug == 1) mp_printf(&mp_plat_print, "%s\n", mess);
    }
}

uint8_t _cc1101_map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
  return (uint8_t)((x - in_min) * (out_max - out_min) / ((float)(in_max - in_min)) + out_min);
}


// -------
// Locals
// -------
static const mp_rom_map_elem_t cc1101_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&cc1101_deinit_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&cc1101_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_burst), MP_ROM_PTR(&cc1101_read_burst_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&cc1101_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_burst), MP_ROM_PTR(&cc1101_write_burst_obj) },
    { MP_ROM_QSTR(MP_QSTR_strobe), MP_ROM_PTR(&cc1101_strobe_obj) }, // alias for "command"
    { MP_ROM_QSTR(MP_QSTR_command), MP_ROM_PTR(&cc1101_command_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&cc1101_flush_obj) },

    { MP_ROM_QSTR(MP_QSTR_spi_read_status), MP_ROM_PTR(&cc1101_spi_read_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_write_reg), MP_ROM_PTR(&cc1101_spi_write_reg_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_write_burst_reg), MP_ROM_PTR(&cc1101_spi_write_burst_reg_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_read_reg), MP_ROM_PTR(&cc1101_spi_read_reg_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_read_burst_reg), MP_ROM_PTR(&cc1101_spi_read_burst_reg_obj) },

    { MP_ROM_QSTR(MP_QSTR_set_cc_mode), MP_ROM_PTR(&cc1101_set_cc_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_modulation), MP_ROM_PTR(&cc1101_set_modulation_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pa), MP_ROM_PTR(&cc1101_set_pa_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_mhz), MP_ROM_PTR(&cc1101_set_mhz_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mhz), MP_ROM_PTR(&cc1101_get_mhz_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_channel), MP_ROM_PTR(&cc1101_set_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_channel), MP_ROM_PTR(&cc1101_get_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_chsp), MP_ROM_PTR(&cc1101_set_chsp_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rx_bw), MP_ROM_PTR(&cc1101_set_rx_bw_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_d_rate), MP_ROM_PTR(&cc1101_set_d_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_deviation), MP_ROM_PTR(&cc1101_set_deviation_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_tx), MP_ROM_PTR(&cc1101_set_tx_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rx), MP_ROM_PTR(&cc1101_set_rx_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_tx_freq), MP_ROM_PTR(&cc1101_set_tx_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rx_freq), MP_ROM_PTR(&cc1101_set_rx_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_rssi), MP_ROM_PTR(&cc1101_get_rssi_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_lqi), MP_ROM_PTR(&cc1101_get_lqi_obj) },

    { MP_ROM_QSTR(MP_QSTR_set_sres), MP_ROM_PTR(&cc1101_set_sres_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sfs_tx_on), MP_ROM_PTR(&cc1101_set_sfs_tx_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sx_off), MP_ROM_PTR(&cc1101_set_sx_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_scal), MP_ROM_PTR(&cc1101_set_scal_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_srx), MP_ROM_PTR(&cc1101_set_srx_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_stx), MP_ROM_PTR(&cc1101_set_stx_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sidle), MP_ROM_PTR(&cc1101_set_sidle_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush_rx), MP_ROM_PTR(&cc1101_flush_rx_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush_tx), MP_ROM_PTR(&cc1101_flush_tx_obj) },
    { MP_ROM_QSTR(MP_QSTR_go_sleep), MP_ROM_PTR(&cc1101_go_sleep_obj) },

    { MP_ROM_QSTR(MP_QSTR_send_data), MP_ROM_PTR(&cc1101_send_data_obj) },
    { MP_ROM_QSTR(MP_QSTR_check_receive_flag), MP_ROM_PTR(&cc1101_check_receive_flag_obj) },
    { MP_ROM_QSTR(MP_QSTR_receive_data), MP_ROM_PTR(&cc1101_receive_data_obj) },
    { MP_ROM_QSTR(MP_QSTR_check_crc), MP_ROM_PTR(&cc1101_check_crc_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_clb), MP_ROM_PTR(&cc1101_set_clb_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_cc1101), MP_ROM_PTR(&cc1101_get_cc1101_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mode), MP_ROM_PTR(&cc1101_get_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sync_word), MP_ROM_PTR(&cc1101_set_sync_word_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_addr), MP_ROM_PTR(&cc1101_set_addr_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_white_data), MP_ROM_PTR(&cc1101_set_white_data_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pkt_format), MP_ROM_PTR(&cc1101_set_pkt_format_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_crc), MP_ROM_PTR(&cc1101_set_crc_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_length_config), MP_ROM_PTR(&cc1101_set_length_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_packet_length), MP_ROM_PTR(&cc1101_set_packet_length_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_dc_filter_off), MP_ROM_PTR(&cc1101_set_dc_filter_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_manchester), MP_ROM_PTR(&cc1101_set_manchester_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sync_mode), MP_ROM_PTR(&cc1101_set_sync_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_fec), MP_ROM_PTR(&cc1101_set_fec_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pre), MP_ROM_PTR(&cc1101_set_pre_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pqt), MP_ROM_PTR(&cc1101_set_pqt_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_crc_af), MP_ROM_PTR(&cc1101_set_crc_af_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_append_status), MP_ROM_PTR(&cc1101_set_append_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_adr_chk), MP_ROM_PTR(&cc1101_set_adr_chk_obj) },
    { MP_ROM_QSTR(MP_QSTR_check_rx_fifo), MP_ROM_PTR(&cc1101_check_rx_fifo_obj) },
    { MP_ROM_QSTR(MP_QSTR_crc8), MP_ROM_PTR(&cc1101_crc8_obj) },
    { MP_ROM_QSTR(MP_QSTR_ba2hex), MP_ROM_PTR(&cc1101_ba2hex_obj) },
};

static MP_DEFINE_CONST_DICT(cc1101_locals_dict, cc1101_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    cc1101_type,
    MP_QSTR_cc1101,
    MP_TYPE_FLAG_NONE,
    make_new, cc1101_make_new,
    print, cc1101_print,
    locals_dict, &cc1101_locals_dict
    );

// -------
// Modules
// -------
static const mp_map_elem_t mp_module_cc1101_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_cc1101) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_cc1101), (mp_obj_t)MP_ROM_PTR(&cc1101_type) },
};

static MP_DEFINE_CONST_DICT(mp_module_cc1101_globals, mp_module_cc1101_globals_table);

const mp_obj_module_t cc1101_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_cc1101_globals,
};

MP_REGISTER_MODULE(MP_QSTR_cc1101, cc1101_module);

//#endif

/*
Test cases
import cc1101
c1 = cc1101.cc1101(1)
c1.get_cc1101()
c1.set_sres()
c1.ba2hex(c1.spi_read_burst_reg(0x0, 0x30))
*/