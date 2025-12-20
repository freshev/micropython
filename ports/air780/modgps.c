/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 pulkin
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

#include "mphalport.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objexcept.h"
#include "shared/timeutils/timeutils.h"
#include "py/mperrno.h"
#include "minmea.h"

#include "platform_define.h"
#include "luat_uart.h"
#include "luat_gpio.h"
#include "luat_debug.h"

#ifdef GPS_MODULE

///////////////////////////////////////////////////////////////
///////////////////////configuration///////////////////////////
#define GPS_PARSE_MAX_GSA_NUMBER 2
#define GPS_PARSE_MAX_GSV_NUMBER 20
////////////////////configuration end//////////////////////////
typedef struct{
    struct minmea_sentence_rmc rmc;
    struct minmea_sentence_gsa gsa[GPS_PARSE_MAX_GSA_NUMBER];
    struct minmea_sentence_gga gga;
    struct minmea_sentence_gll gll;
    struct minmea_sentence_gst gst;
    struct minmea_sentence_gsv gsv[GPS_PARSE_MAX_GSV_NUMBER];
    struct minmea_sentence_vtg vtg;
    struct minmea_sentence_zda zda;
} GPS_Info_t;

static GPS_Info_t gpsInfo = {0};
static bool gpsIsOpen = false;
static char GPS_Hardware[30] = {0}; 
static char GPS_Firmware[30] = {0}; 
static char GPS_Description[30] = {0}; 
static char GPS_Serial[15] = {0}; 

// Note: 780EG internal gnss chip is connected to serial port 2
#define UART_ID 2
#define DEFAULT_GPS_TIMEOUT 60
#define REQUIRES_GPS_ON do { if (!GPS_IsOpen()) {mp_raise_OSError(MP_EPERM); return mp_const_none;}} while(0)


STATIC mp_obj_t modgps_off(void);
STATIC void luat_uart_recv_callback(int uart_num, uint32_t data_len);

void modgps_init0(void) {
    modgps_off();
}


bool GPS_Init() {
    luat_uart_t uart = {
        .id = UART_ID,
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0};
    luat_uart_pre_setup(UART_ID, 1);
    luat_uart_setup(&uart);

    luat_uart_ctrl(UART_ID, LUAT_UART_SET_RECV_CALLBACK, luat_uart_recv_callback);

    // The gnss power supply actually controls the GPIO that will be automatically pulled high when it is powered on. 
    // If you do not need to turn it on immediately after powering on, please change cfg.output_level to 0
    luat_gpio_cfg_t cfg = {0};
    cfg.pin = HAL_GPIO_13;
    cfg.mode = LUAT_GPIO_OUTPUT;
    cfg.output_level = 1;
    cfg.alt_fun = 4;
    luat_gpio_open(&cfg);

    gpsIsOpen = true; 
    return 1;
}

void GPS_Close() {
   luat_uart_close(UART_ID);
   gpsIsOpen = false;
}

bool GPS_IsOpen() {
   return gpsIsOpen;
}

/**
 * @Author: Neucrack 
 * Parse a full frame gps NMEA message.
 * @param nmeas: A full GPA NMEA message frame. e.g.
 *                $GNGGA,084257.000,2234.7758,N,11354.9654,E,2,12,1.00,59.4,M,-2.8,M,,*56
 *                $GPGSA,A,3,19,28,09,03,23,193,,,,,,,1.28,1.00,0.80*32
 *                $BDGSA,A,3,04,01,07,03,06,09,,,,,,,1.28,1.00,0.80*1F
 *                $GPGSV,4,1,14,193,60,100,40,17,54,020,14,28,53,165,42,06,52,308,*43
 *                $GPGSV,4,2,14,19,46,346,13,42,46,122,33,02,23,268,,03,21,041,18*75
 *                $GPGSV,4,3,14,09,17,125,32,23,13,088,35,30,04,180,34,05,02,211,23*7B
 *                $GPGSV,4,4,14,24,01,292,,12,01,325,*74
 *                $BDGSV,3,1,12,03,65,189,37,10,55,226,,01,51,128,35,08,49,000,*67
 *                $BDGSV,3,2,12,13,49,322,,02,48,238,,17,44,136,,07,40,185,40*68
 *                $BDGSV,3,3,12,04,33,110,33,06,27,160,36,05,24,256,,09,12,183,34*6B
 *                $GNRMC,084257.000,A,2234.7758,N,11354.9654,E,0.032,306.43,140618,,,D*46
 *                $GNVTG,306.43,T,,M,0.032,N,0.059,K,D*29
 */
bool parse_nmea(uint8_t* nmea, uint8_t flag) {
    static uint8_t flag_old = 0xff;
    static uint8_t gsv_count = 0;
    static uint8_t gsa_count = 0;
    char* line = (char*)nmea;

    //first line of multiple line NMEA message
    if(flag != flag_old) {
        flag_old = flag;
        gsv_count = 0;
        gsa_count = 0;
    }
    const char *np = " sentence is not parsed";

    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC:
            if (!minmea_parse_rmc(&gpsInfo.rmc, line)) mp_warning(NULL, "RMC%s\n", np);
            break;
        case MINMEA_SENTENCE_GGA: 
            if (!minmea_parse_gga(&gpsInfo.gga, line)) mp_warning(NULL, "GGA%s\n", np);
            break;
        case MINMEA_SENTENCE_GST: 
            if (!minmea_parse_gst(&gpsInfo.gst, line)) mp_warning(NULL, "GST%s\n", np);
            break;
        case MINMEA_SENTENCE_GSV: 
            if(gsv_count < GPS_PARSE_MAX_GSV_NUMBER) {
                if (!minmea_parse_gsv(&gpsInfo.gsv[gsv_count++], line)) mp_warning(NULL, "GSV%s\n", np);
            }
            break;
        case MINMEA_SENTENCE_VTG: 
            if (!minmea_parse_vtg(&gpsInfo.vtg, line)) mp_warning(NULL, "VTG%s\n", np);
            break;
        case MINMEA_SENTENCE_ZDA: 
            if (!minmea_parse_zda(&gpsInfo.zda, line)) mp_warning(NULL, "ZDA%s\n", np);
            break;
        case MINMEA_SENTENCE_GSA:
            if(gsa_count < GPS_PARSE_MAX_GSA_NUMBER){
                if (!minmea_parse_gsa(&gpsInfo.gsa[gsa_count++], line)) mp_warning(NULL, "GSA%s\n", np);
            }
            break;
        case MINMEA_SENTENCE_GLL:
            if (!minmea_parse_gll(&gpsInfo.gll, line)) mp_warning(NULL, "GLL%s\n", np);
            break;
        case MINMEA_INVALID:
        default:
            if(strstr((const char*)nmea, "UC6228CI")) { strcpy(GPS_Description, (const char*)nmea); } 		//  "UC6228CI lite G1B1 COM1"       Description
            else if(strstr((const char*)nmea, "PN ")) { strcpy(GPS_Serial, (const char*)nmea + 3); } 		//  "PN 2400513000279" 				Part number
            else if(strstr((const char*)nmea, "HWVer ")) { strcpy(GPS_Hardware, (const char*)nmea + 6); } 	//  "HWVer 2.0"						Hardware version
            else if(strstr((const char*)nmea, "FWVer ")) { strcpy(GPS_Firmware, (const char*)nmea + 6); } 	//  "FWVer R3.2.10..."				Firmware version
            else if(strstr((const char*)nmea, "Copyright")) {} 	//  "Copyright (c) ..."				Copyright
            else if(strstr((const char*)nmea, "All rights reserved.")) {} 	
            else {
            	mp_warning(NULL, "%s: %s\n", (np + 1), line);
            	return false;
            }
    }
    return true;
}


void luat_uart_recv_callback(int uart_id, uint32_t data_len) {
    char *data_buff = malloc(data_len + 1);
    memset(data_buff, 0, data_len + 1);
    luat_uart_read(uart_id, data_buff, data_len);    
    
    static uint8_t flag  = 0;
    size_t prev = 0;
    static char nmea_tmp_buff[86] = {0}; // nmea has a maximum length of 82, including newline characters
    for (size_t offset = 0; offset < data_len; offset++) {
        // \r == 0x0D  \n == 0x0A
        if (data_buff[offset] == 0x0A) {
            // The shortest one also needs to be OK\r\n
            // should\r\n
            // too long
            if (offset - prev < 3 || data_buff[offset - 1] != 0x0D || offset - prev > 82) {
                prev = offset + 1;
                continue;
            }
            memcpy(nmea_tmp_buff, data_buff + prev, offset - prev - 1);
            nmea_tmp_buff[offset - prev - 1] = 0x00;
            parse_nmea((const char *)nmea_tmp_buff, flag);
            prev = offset + 1;
        }
        flag++;
    }
    if(data_buff != NULL) free(data_buff);
}


// -------
// Methods
// -------
STATIC mp_obj_t modgps_get_description(void) {
    return mp_obj_new_str(GPS_Description, strlen(GPS_Description));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_description_obj, modgps_get_description);

STATIC mp_obj_t modgps_get_serial(void) {
    return mp_obj_new_str(GPS_Serial, strlen(GPS_Serial));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_serial_obj, modgps_get_serial);

STATIC mp_obj_t modgps_get_firmware_version(void) {
    return mp_obj_new_str(GPS_Firmware, strlen(GPS_Firmware));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_firmware_version_obj, modgps_get_firmware_version);

STATIC mp_obj_t modgps_get_hardware_version(void) {
    return mp_obj_new_str(GPS_Hardware, strlen(GPS_Hardware));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_hardware_version_obj, modgps_get_hardware_version);


STATIC mp_obj_t modgps_on(size_t n_args, const mp_obj_t *arg) {
    // ========================================
    // Turns GPS on.
    // Args:
    //     timeout (int): timeout in seconds;
    // Raises:
    //     ValueError if failed to turn GPS on.
    // ========================================
    uint32_t timeout = 0;
    if (n_args == 0) {
        timeout = DEFAULT_GPS_TIMEOUT;
    } else {
        timeout = mp_obj_get_int(arg[0]);
    }
    timeout *= 1000;
    if(GPS_Init()) {
        WAIT_UNTIL(gpsInfo.rmc.latitude.value, timeout, 100, mp_raise_OSError(MP_ETIMEDOUT));
    } else {
       mp_raise_RuntimeError("Failed to activate GPS");
    }
    return MP_OBJ_NEW_SMALL_INT(1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modgps_on_obj, 0, 1, modgps_on);

STATIC mp_obj_t modgps_off(void) {
    // ========================================
    // Turns GPS off.
    // ========================================
    GPS_Close();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_off_obj, modgps_off);


STATIC mp_obj_t modgps_get_last_location(void) {
    // ========================================
    // Retrieves the last GPS location.
    // Returns:
    //     Location reported by GPS: longitude and latitude (degrees).
    // ========================================
    // REQUIRES_VALID_GPS_INFO;

    /*int temp = (int)(gpsInfo->rmc.latitude.value/gpsInfo->rmc.latitude.scale/100);
    double latitude = temp+(double)(gpsInfo->rmc.latitude.value - temp*gpsInfo->rmc.latitude.scale*100)/gpsInfo->rmc.latitude.scale/60.0;
    temp = (int)(gpsInfo->rmc.longitude.value/gpsInfo->rmc.longitude.scale/100);
    double longitude = temp+(double)(gpsInfo->rmc.longitude.value - temp*gpsInfo->rmc.longitude.scale*100)/gpsInfo->rmc.longitude.scale/60.0;
    mp_obj_t tuple[2] = {
        mp_obj_new_float(latitude),
        mp_obj_new_float(longitude),
    };*/
    mp_obj_t tuple[2] = {
        mp_obj_new_float(minmea_tocoord(&gpsInfo.rmc.latitude)),
        mp_obj_new_float(minmea_tocoord(&gpsInfo.rmc.longitude)),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_last_location_obj, modgps_get_last_location);

STATIC mp_obj_t modgps_get_location(void) {
    // ========================================
    // Retrieves the current GPS location.
    // Returns:
    //     Location reported by GPS: longitude and latitude (degrees).
    // ========================================
    REQUIRES_GPS_ON;

    return modgps_get_last_location();
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_location_obj, modgps_get_location);

STATIC mp_obj_t modgps_get_satellites(void) {
    // ========================================
    // Retrieves the number of visible GPS satellites.
    // Returns:
    //     The number of visible GPS satellites.
    // ========================================
    REQUIRES_GPS_ON;

    mp_obj_t tuple[2] = {
        mp_obj_new_int(gpsInfo.gga.satellites_tracked),
        mp_obj_new_int(gpsInfo.gsv[0].total_sats),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_satellites_obj, modgps_get_satellites);

STATIC mp_obj_t modgps_time(void) {
    // ========================================
    // GPS time.
    // Returns:
    //     Seconds since 2000 as int.
    // ========================================
    REQUIRES_GPS_ON;

    struct minmea_date date = gpsInfo.rmc.date;
    struct minmea_time time = gpsInfo.rmc.time;

    mp_uint_t result = timeutils_mktime(date.year + 2000, date.month, date.day, time.hours, time.minutes, time.seconds);
    // Note: the module may report dates between 1980 and 2000 as well: they will be mapped onto the time frame 2080-2100
    return mp_obj_new_int_from_uint(result);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_time_obj, modgps_time);

STATIC mp_obj_t _get_time(struct minmea_date date, struct minmea_time time) {
    mp_uint_t result = timeutils_mktime(date.year + 2000, date.month, date.day, time.hours, time.minutes, time.seconds);
    // Note: the module may report dates between 1980 and 2000 as well: they will be mapped onto the time frame 2080-2100
    return mp_obj_new_int_from_uint(result);
}

STATIC mp_obj_t _get_float(struct minmea_float f) {
    return mp_obj_new_float(minmea_tofloat(&f));
}

STATIC mp_obj_t _get_prn(int* prn) {
    uint8_t prn_u[12];
    for (int i = 0; i < 12; i++) prn_u[i] = prn[i];
    return mp_obj_new_bytearray(sizeof(prn), prn);
}

STATIC mp_obj_t _get_sat_info(struct minmea_sat_info s) {
    mp_obj_t tuple[4] = {
        mp_obj_new_int(s.nr),
        mp_obj_new_int(s.elevation),
        mp_obj_new_int(s.azimuth),
        mp_obj_new_int(s.snr),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_rmc(struct minmea_sentence_rmc s) {
    mp_obj_t tuple[7] = {
        _get_time(s.date, s.time),
        mp_obj_new_bool(s.valid),
        _get_float(s.latitude),
        _get_float(s.longitude),
        _get_float(s.speed),
        _get_float(s.course),
        _get_float(s.variation),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gsa(struct minmea_sentence_gsa s) {
    mp_obj_t tuple[6] = {
        mp_obj_new_int_from_uint(s.mode),
        mp_obj_new_int(s.fix_type),
        _get_prn(s.sats),
        _get_float(s.pdop),
        _get_float(s.hdop),
        _get_float(s.vdop),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gga(struct minmea_sentence_gga s) {
    struct minmea_date no_date = {1, 1, 0};

    mp_obj_t tuple[11] = {
        _get_time(no_date, s.time),
        _get_float(s.latitude),
        _get_float(s.longitude),
        mp_obj_new_int(s.fix_quality),
        mp_obj_new_int(s.satellites_tracked),
        _get_float(s.hdop),
        _get_float(s.altitude),
        mp_obj_new_int_from_uint(s.altitude_units),
        _get_float(s.height),
        mp_obj_new_int_from_uint(s.height_units),
        _get_float(s.dgps_age),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gll(struct minmea_sentence_gll s) {
    struct minmea_date no_date = {1, 1, 0};

    mp_obj_t tuple[5] = {
        _get_float(s.latitude),
        _get_float(s.longitude),
        _get_time(no_date, s.time),
        mp_obj_new_int_from_uint(s.status),
        mp_obj_new_int_from_uint(s.mode),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gst(struct minmea_sentence_gst s) {
    struct minmea_date no_date = {1, 1, 0};

    mp_obj_t tuple[8] = {
        _get_time(no_date, s.time),
        _get_float(s.rms_deviation),
        _get_float(s.semi_major_deviation),
        _get_float(s.semi_minor_deviation),
        _get_float(s.semi_major_orientation),
        _get_float(s.latitude_error_deviation),
        _get_float(s.longitude_error_deviation),
        _get_float(s.altitude_error_deviation),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gsv(struct minmea_sentence_gsv s) {
    mp_obj_t sat_info_tuple[4];
    for (int i = 0; i < 4; i++)
        sat_info_tuple[i] = _get_sat_info(s.sats[i]);

    mp_obj_t tuple[4] = {
        mp_obj_new_int(s.total_msgs),
        mp_obj_new_int(s.msg_nr),
        mp_obj_new_int(s.total_sats),
        mp_obj_new_tuple(4, sat_info_tuple),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_vtg(struct minmea_sentence_vtg s) {
    mp_obj_t tuple[5] = {
        _get_float(s.true_track_degrees),
        _get_float(s.magnetic_track_degrees),
        _get_float(s.speed_knots),
        _get_float(s.speed_kph),
        mp_obj_new_int_from_uint((uint8_t) s.faa_mode),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_zda(struct minmea_sentence_zda s) {
    mp_obj_t tuple[3] = {
        _get_time(s.date, s.time),
        mp_obj_new_int(s.hour_offset),
        mp_obj_new_int(s.minute_offset),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t modgps_nmea_data(void) {
    // ========================================
    // NMEA data in a tuple.
    // Returns:
    //     A tuple with rmc, gsa, gga, gll,
    //     gst, gsv, vtg and zda messages.
    // ========================================
    REQUIRES_GPS_ON;

    mp_obj_t gsa_tuple[GPS_PARSE_MAX_GSA_NUMBER];
    for (int i = 0; i < GPS_PARSE_MAX_GSA_NUMBER; i++)
        gsa_tuple[i] = _get_gsa(gpsInfo.gsa[i]);

    mp_obj_t gsv_tuple[GPS_PARSE_MAX_GSV_NUMBER];
    for (int i = 0; i < GPS_PARSE_MAX_GSV_NUMBER; i++)
        gsv_tuple[i] = _get_gsv(gpsInfo.gsv[i]);

    mp_obj_t tuple[8] = {
        _get_rmc(gpsInfo.rmc),
        mp_obj_new_tuple(GPS_PARSE_MAX_GSA_NUMBER, gsa_tuple),
        _get_gga(gpsInfo.gga),
        _get_gll(gpsInfo.gll),
        _get_gst(gpsInfo.gst),
        mp_obj_new_tuple(GPS_PARSE_MAX_GSV_NUMBER, gsv_tuple),
        _get_vtg(gpsInfo.vtg),
        _get_zda(gpsInfo.zda),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_nmea_data_obj, modgps_nmea_data);

STATIC const mp_map_elem_t mp_module_gps_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gps) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&modgps_on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&modgps_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_description), (mp_obj_t)&modgps_get_description_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_serial), (mp_obj_t)&modgps_get_serial_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_firmware_version), (mp_obj_t)&modgps_get_firmware_version_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_hardware_version), (mp_obj_t)&modgps_get_hardware_version_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_location), (mp_obj_t)&modgps_get_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_last_location), (mp_obj_t)&modgps_get_last_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_satellites), (mp_obj_t)&modgps_get_satellites_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_time), (mp_obj_t)&modgps_time_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_nmea_data), (mp_obj_t)&modgps_nmea_data_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_gps_globals, mp_module_gps_globals_table);

const mp_obj_module_t gps_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_gps_globals,
};

MP_REGISTER_MODULE(MP_QSTR_gps, gps_module);

#endif