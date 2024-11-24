
/*@Modules libgnss
@summary NMEA data processing
@version 1.0
@date 2020.07.03
@demo libgnss
@tag LUAT_USE_LIBGNSS
@usage
-- Reminder: The coordinates output by this library are all WGS84 coordinate system
-- If it needs to be used on domestic maps, it must be converted into the coordinate system of the corresponding map, such as GCJ02 BD09
-- Related links: https://lbsyun.baidu.com/index.php?title=coordinate
-- Related links: https://www.openluat.com/GPS-Offset.html

-- Reminder: GPS function, GNSS function, and NMEA analysis function are all sub-functions of the current library
-- The main function of this library is to parse the NMEA protocol, supporting built-in GNSS and external GNSS

--The following is a sample code using this libgnss
-- Option 1, transfer data via lua layer
uart.setup(2, 115200)
uart.on(2, "recv", function(id, len)
    while 1 do
        local data = uart.read(id, 1024)
        if data and #data > 1 then
            libgnss.parse(data)
        else
            break
        end
    end
end)
-- Option 2, suitable for compiling firmware after 2022.12.26, which is more efficient
uart.setup(2, 115200)
libgnss.bind(2)

-- Optional debug mode
-- libgnss.debug(true)

sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    -- FIXED Positioning successful
    -- LOSE positioning lost
    -- ticks is the time when the event occurs, which can generally be ignored
    log.info("gnss", "state", event, ticks)
end)*/
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_mem.h"
#include "luat_uart.h"
#include "luat_mcu.h"
#include "luat_rtc.h"
#include "luat_zbuff.h"

#define LUAT_LOG_TAG "gnss"
#include "luat_log.h"

#include "minmea.h"

extern luat_libgnss_t gnssctx;
// extern luat_libgnss_t *libgnss_gnsstmp;
extern char* libgnss_recvbuff;
extern int libgnss_route_uart_id;
extern int gnss_debug;

static int gnss_raw_cb = 0;
static int gnss_txt_cb = 0;
// static int gnss_rmc_cb = 0;
static int gnss_other_cb = 0;

void luat_uart_set_app_recv(int id, luat_uart_recv_callback_t cb);

static inline void push_gnss_value(lua_State *L, struct minmea_float *f, int mode) {
    if (f->value == 0) {
        lua_pushinteger(L, 0);
        return;
    }
    switch (mode)
    {
    case 0:
        lua_pushnumber(L, minmea_tofloat(f));
        break;
    case 1:
        lua_pushinteger(L, minmea_tocoord2(f));
        break;
    case 2:
        lua_pushnumber(L, minmea_tocoord(f));
        break;
    default:
        lua_pushnumber(L, minmea_tocoord(f));
        break;
    }
}

static int luat_libgnss_state_handler(lua_State *L, void* ptr) {
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (!lua_isfunction(L, -1)) {
        return 0;
    }
/*@sys_pub libgnss
GNSS status changes
GNSS_STATE
@usage
sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    -- FIXED Positioning successful
    -- LOSE positioning lost
    -- ticks is the time when the event occurs, which can generally be ignored
    log.info("gnss", "state", event, ticks)
end)*/
    lua_pushliteral(L, "GNSS_STATE");
    switch (msg->arg1)
    {
    case GNSS_STATE_FIXED:
        lua_pushliteral(L, "FIXED");
        break;
    case GNSS_STATE_LOSE:
        lua_pushliteral(L, "LOSE");
        break;
    case GNSS_STATE_OPEN:
        lua_pushliteral(L, "OPEN");
        break;
    case GNSS_STATE_CLOSE:
        lua_pushliteral(L, "CLOSE");
        break;
    default:
        return 0;
    }
    lua_pushinteger(L, msg->arg2);
    lua_call(L, 3, 0);
    return 0;
}

int luat_libgnss_state_onchanged(int state) {
    rtos_msg_t msg = {0};
    msg.handler = luat_libgnss_state_handler;
    msg.arg1 = state;
    #ifdef LUAT_USE_MCU
    msg.arg2 = luat_mcu_ticks();
    #endif
    luat_msgbus_put(&msg, 0);
    return 0;
}

static void put_datetime(lua_State*L, struct tm* rtime) {
    lua_pushliteral(L, "year");
    lua_pushinteger(L, rtime->tm_year);
    lua_settable(L, -3);

    lua_pushliteral(L, "month");
    lua_pushinteger(L, rtime->tm_mon + 1); // I'm quite confused, should I be compatible with the old one?
    lua_settable(L, -3);

    lua_pushliteral(L, "day");
    lua_pushinteger(L, rtime->tm_mday);
    lua_settable(L, -3);

    lua_pushliteral(L, "hour");
    lua_pushinteger(L, rtime->tm_hour);
    lua_settable(L, -3);

    lua_pushliteral(L, "min");
    lua_pushinteger(L, rtime->tm_min);
    lua_settable(L, -3);

    lua_pushliteral(L, "sec");
    lua_pushinteger(L, rtime->tm_sec);
    lua_settable(L, -3);
}

static int l_gnss_callback(lua_State *L, void* ptr){
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_libgnss_uart_recv_cb(msg->arg1, msg->arg2);
	return 0;
}

static void l_libgnss_uart_recv_cb(int uart_id, uint32_t data_len)
{
    rtos_msg_t msg = {0};
    msg.handler = l_gnss_callback;
    msg.arg1 = uart_id;
    msg.arg2 = data_len;
    luat_msgbus_put(&msg, 0);

}
/**
Process nmea data
@api libgnss.parse(str)
@string original nmea data
@usage
-- Parse nmea data
libgnss.parse(indata)
log.info("nmea", json.encode(libgnss.getRmc(), "11g"))*/
static int l_libgnss_parse(lua_State *L) {
    size_t len = 0;
    const char* str = NULL;
    luat_zbuff_t* zbuff = NULL;
    if (lua_type(L, 1) == LUA_TSTRING) {
        str = luaL_checklstring(L, 1, &len);
    }
    else if (lua_isuserdata(L, 1)) {
        zbuff = tozbuff(L);
        str = (const char*)zbuff->addr;
        len = zbuff->used;
    }
    else {
        return 0;
    }
    
    if (len > 0) {
        luat_libgnss_parse_data(str, len);
    }
    return 0;
}

/**
Whether the positioning has been successful currently
@api libgnss.isFix()
@return boolean whether positioning is successful or not
@usage
log.info("nmea", "isFix", libgnss.isFix())*/
static int l_libgnss_is_fix(lua_State *L) {
    lua_pushboolean(L, gnssctx.frame_rmc.valid != 0 ? 1 : 0);
    return 1;
}

/**
Get location information
@api libgnss.getIntLocation(speed_type)
@int speed unit, default is m/h
@return int lat data, format is ddddddddd
@return int lng data, format is ddddddddd
@return int speed data, unit meter. Revised on 2023.9.26
@usage
-- It is recommended to use libgnss.getRmc(1)
log.info("nmea", "loc", libgnss.getIntLocation())

-- 2023.12.11 Added speed_type parameter
--[[
Speed   unit optional value
0 - m/h meters/hour, default value, integer
1 - m/s meters per second, floating point number
2 - km/h kilometers per hour, floating point number
3 - kn/h miles per hour, floating point number
]]
--Default meters/hour
log.info("nmea", "loc", libgnss.getIntLocation())
-- meters/second
log.info("nmea", "loc", libgnss.getIntLocation(1))
--km/h
log.info("nmea", "loc", libgnss.getIntLocation(2))
-- miles per hour
log.info("nmea", "loc", libgnss.getIntLocation(3))*/
static int l_libgnss_get_int_location(lua_State *L) {
    if (gnssctx.frame_rmc.valid) {
        lua_pushinteger(L, gnssctx.frame_rmc.latitude.value);
        lua_pushinteger(L, gnssctx.frame_rmc.longitude.value);
        int speed_type = luaL_optinteger(L, 1, 0);
        switch (speed_type)
        {
        case 1: // meters/second
            lua_pushnumber(L, (minmea_tofloat(&(gnssctx.frame_rmc.speed)) * 1852 / 3600));
            break;
        case 2: // kilometers/hour
            lua_pushnumber(L, (minmea_tofloat(&(gnssctx.frame_rmc.speed)) * 1.852));
            break;
        case 3: // miles/hour
            lua_pushnumber(L, minmea_tofloat(&(gnssctx.frame_rmc.speed)));
            break;
        default: // meters/hour
            lua_pushinteger(L, (int32_t)(minmea_tofloat(&(gnssctx.frame_rmc.speed)) * 1852));
            break;
        }
        
    } else {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    }
    return 3;
}

/**
Get original RMC location information
@api libgnss.getRmc(data_mode)
@int format of coordinate data, 0-DDMM.MMM format, 1-DDDDDDD format, 2-DD.DDDDD format, 3-original RMC string
@return table original rmc data
@usage
-- parse nmea
log.info("nmea", "rmc", json.encode(libgnss.getRmc(2)))
--Instance output
--[[
{
    "course":0,
    "valid":true, // true positioning is successful, false positioning is lost
    "lat":23.4067, // Latitude, positive number is north latitude, negative number is south latitude
    "lng":113.231, // Longitude, positive number is east longitude, negative number is west longitude
    "variation":0, // Ground heading, in degrees, calculated clockwise from north
    "speed":0 // Ground speed, unit is "knot"
    "year":2023, // year
    "month":1, // month, 1-12
    "day":5, // day of month, 1-31
    "hour":7, // hour, 0-23
    "min":23, // minutes, 0-59
    "sec":20, // seconds, 0-59
}
]]*/
static int l_libgnss_get_rmc(lua_State *L) {
    int mode = luaL_optinteger(L, 1, 0);
    lua_settop(L, 0);
    lua_createtable(L, 0, 12);

    struct tm rtime = {0};

    if (mode == 3) {
        if (gnssctx.rmc == NULL)
            return 0;
        lua_pushstring(L, gnssctx.rmc->data);
        return 1;
    }

    if (1) {
        lua_pushboolean(L, gnssctx.frame_rmc.valid);
        lua_setfield(L, -2, "valid");

        if (gnssctx.frame_rmc.valid) {
            push_gnss_value(L, &(gnssctx.frame_rmc.latitude), mode);
        }
        else
            lua_pushinteger(L, 0);
        lua_setfield(L, -2, "lat");

        if (gnssctx.frame_rmc.valid) {
            push_gnss_value(L, &(gnssctx.frame_rmc.longitude), mode);
        }
        else
            lua_pushinteger(L, 0);
        lua_setfield(L, -2, "lng");

        if (gnssctx.frame_rmc.valid) {
            push_gnss_value(L, &(gnssctx.frame_rmc.speed), 0);
        }
        else
            lua_pushinteger(L, 0);
        lua_setfield(L, -2, "speed");

        if (gnssctx.frame_rmc.valid) {
            push_gnss_value(L, &(gnssctx.frame_rmc.course), 0);
        }
        else
            lua_pushinteger(L, 0);
        lua_setfield(L, -2, "course");

        if (gnssctx.frame_rmc.valid) {
            push_gnss_value(L, &(gnssctx.frame_rmc.variation), 0);
        }
        else
            lua_pushinteger(L, 0);
        lua_setfield(L, -2, "variation");

        // time class
        minmea_getdatetime(&rtime, &gnssctx.frame_rmc.date, &gnssctx.frame_rmc.time);
        put_datetime(L, &rtime);
    }

    return 1;
}

/**
Get original GSV information
@api libgnss.getGsv()
@return table original GSV data
@usage
-- parse nmea
log.info("nmea", "gsv", json.encode(libgnss.getGsv()))
--[[Example output
{
    "total_sats":24, // Total number of visible satellites
    "sats":[
        {
            "snr":27, // signal-to-noise ratio
            "azimuth":278, // direction angle
            "elevation":59, // elevation angle
            "tp":0, // 0 - GPS, 1 - BD
            "nr":4 // Satellite number
        },
        //The information of 22 satellites is ignored here
        {
            "snr":0,
            "azimuth":107,
            "elevation":19,
            "tp":1,
            "nr":31
        }
    ]
}
]]*/
static int l_libgnss_get_gsv(lua_State *L) {
    lua_createtable(L, 0, 2);
    size_t count = 1;
    uint64_t tnow = luat_mcu_tick64_ms();
    struct minmea_sentence_gsv frame_gsv = {0};
    lua_createtable(L, FRAME_GSV_MAX, 0);
    for (size_t i = 0; i < FRAME_GSV_MAX; i++)
    {
        if (!luat_libgnss_data_check(gnssctx.gsv[i], 3500, tnow) || !minmea_parse_gsv(&frame_gsv, gnssctx.gsv[i]->data)) {
            continue;
        }

        for (size_t j = 0; j < 4; j++)
        {
            if (!frame_gsv.sats[j].nr) {
                continue;
            }
            lua_pushinteger(L, count);
            lua_createtable(L, 0, 4);

            lua_pushliteral(L, "nr");
            lua_pushinteger(L, frame_gsv.sats[j].nr);
            lua_settable(L, -3);
    
            lua_pushliteral(L, "snr");
            lua_pushinteger(L, frame_gsv.sats[j].snr);
            lua_settable(L, -3);
    
            lua_pushliteral(L, "elevation");
            lua_pushinteger(L, frame_gsv.sats[j].elevation);
            lua_settable(L, -3);
    
            lua_pushliteral(L, "azimuth");
            lua_pushinteger(L, frame_gsv.sats[j].azimuth);
            lua_settable(L, -3);
    
            // Distinguish between different satellite systems
            // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GSA.html
            lua_pushliteral(L, "tp");
            if (memcmp(gnssctx.gsv[i], "$GP", 3) == 0) {
                lua_pushinteger(L, 0);
            }
            else if (memcmp(gnssctx.gsv[i], "$GL", 3) == 0) {
                lua_pushinteger(L, 2);
            }
            else if (memcmp(gnssctx.gsv[i], "$GA", 3) == 0) {
                lua_pushinteger(L, 3);
            }
            else if (memcmp(gnssctx.gsv[i], "$BD", 3) == 0 || memcmp(gnssctx.gsv[i], "$GB", 3) == 0) {
                lua_pushinteger(L, 1);
            }
            else if (memcmp(gnssctx.gsv[i], "$QZ", 3) == 0) {
                lua_pushinteger(L, 4);
            }
            else {
                lua_pushinteger(L, 0);
            }
            lua_settable(L, -3);

            // Add a new type, string, which is really difficult to deal with various changes.
            lua_pushliteral(L, "tpstr");
            lua_pushlstring(L, gnssctx.gsv[i]->data + 1, 2);
            lua_settable(L, -3);

            lua_settable(L, -3);
            count = count + 1;
        }
    }
    lua_setfield(L, -2, "sats");

    lua_pushliteral(L, "total_sats");
    lua_pushinteger(L, count - 1);
    lua_settable(L, -3);

    return 1;
}


/**
Get original GSA information
@api libgnss.getGsa(data_mode)
@int pattern
@return table original GSA data
@usage
-- get
log.info("nmea", "gsa", json.encode(libgnss.getGsa(), "11g"))
-- Sample data (mode 0, which is the default mode)
--[[
{
    "sats":[ // Satellite number in use
        9,
        6,
        16,
        16,
        26,
        twenty one,
        27,
        27,
        4,
        36,
        3,
        7,
        8,
        194
    ],
    "vdop":0.03083333, // Vertical precision factor, 0.00 - 99.99, the value is 99.99 when not positioned
    "pdop":0.0455, // Horizontal precision factor, 0.00 - 99.99, the value is 99.99 when not positioned
    "fix_type":3, // Positioning mode, 1-unpositioned, 2-2D positioning, 3-3D positioning
    "hdop":0.0335 // Position accuracy factor, 0.00 - 99.99, the value is 99.99 when not positioning
}
]]

-- Sample data (mode 1), added on 2024.5.26
[
    {"pdop":7.8299999,"sats":[13,15,18,23],"vdop":3.2400000,"hdop":7.1300001,"fix_type":3},
    {"pdop":7.8299999,"sats":[20,35,8,13],"vdop":3.2400000,"hdop":7.1300001,"fix_type":3}
]*/

static int l_libgnss_get_gsa_mode0(lua_State *L) {
    struct minmea_sentence_gsa frame_gsa = {0};
    uint64_t tnow = luat_mcu_tick64_ms();
    lua_createtable(L, 0, 12);

    for (size_t i = 0; i < FRAME_GSA_MAX; i++)
    {
        if (!luat_libgnss_data_check(gnssctx.gsa[i], 1500, tnow) || minmea_parse_gsa(&frame_gsa, gnssctx.gsa[i]->data) != 1)
        {
            continue;
        }
        lua_pushliteral(L, "fix_type");
        lua_pushinteger(L, frame_gsa.fix_type);
        lua_settable(L, -3);

        lua_pushliteral(L, "pdop");
        push_gnss_value(L, &(frame_gsa.pdop), 0);
        lua_settable(L, -3);

        lua_pushliteral(L, "hdop");
        push_gnss_value(L, &(frame_gsa.hdop), 0);
        lua_settable(L, -3);

        lua_pushliteral(L, "vdop");
        push_gnss_value(L, &(frame_gsa.vdop), 0);
        lua_settable(L, -3);

        lua_pushliteral(L, "sysid");
        lua_pushinteger(L, frame_gsa.sysid);
        lua_settable(L, -3);
        break;
    }

    lua_pushliteral(L, "sats");
    lua_createtable(L, FRAME_GSA_MAX, 0);
    size_t pos = 1;
    for (size_t i = 0; i < FRAME_GSA_MAX; i++) {
        if (gnssctx.gsa[i] == NULL || minmea_parse_gsa(&frame_gsa, gnssctx.gsa[i]->data) != 1)
        {
            continue;
        }
        if (tnow - gnssctx.gsa[i]->tm > 1000) {
            continue;
        }
        // LLOGD("GSA: %s", gnssctx.gsa[i]->data);
        for (size_t j = 0; j < 12; j++)
        {
            if (frame_gsa.sats[j] == 0)
                continue;
            
            lua_pushinteger(L, frame_gsa.sats[j]);
            lua_seti(L, -2, pos);
            pos ++;
        }
    }
    lua_settable(L, -3);
    return 1;
}

static int l_libgnss_get_gsa_mode1(lua_State *L) {
    struct minmea_sentence_gsa frame_gsa = {0};
    uint64_t tnow = luat_mcu_tick64_ms();
    
    lua_createtable(L, FRAME_GSA_MAX, 0);
    size_t count = 0;
    for (size_t i = 0; i < FRAME_GSA_MAX; i++) {
        if (gnssctx.gsa[i] == NULL || minmea_parse_gsa(&frame_gsa, gnssctx.gsa[i]->data) != 1)
        {
            continue;
        }
        if (tnow - gnssctx.gsa[i]->tm > 1000) {
            continue;
        }
        count ++;
        lua_createtable(L, 0, 12);
        lua_pushliteral(L, "fix_type");
        lua_pushinteger(L, frame_gsa.fix_type);
        lua_settable(L, -3);

        lua_pushliteral(L, "pdop");
        push_gnss_value(L, &(frame_gsa.pdop), 0);
        lua_settable(L, -3);

        lua_pushliteral(L, "hdop");
        push_gnss_value(L, &(frame_gsa.hdop), 0);
        lua_settable(L, -3);

        lua_pushliteral(L, "vdop");
        push_gnss_value(L, &(frame_gsa.vdop), 0);
        lua_settable(L, -3);

        lua_pushliteral(L, "sysid");
        lua_pushinteger(L, frame_gsa.sysid);
        lua_settable(L, -3);
        
        lua_pushliteral(L, "sats");
        lua_createtable(L, 12, 0);
        size_t pos = 1;
        for (size_t j = 0; j < 12; j++)
        {
            if (frame_gsa.sats[j] == 0)
                continue;
            
            lua_pushinteger(L, frame_gsa.sats[j]);
            lua_seti(L, -2, pos);
            pos ++;
        }
        lua_settable(L, -3);
        lua_seti(L, -2, count);
    }
    return 1;
}
static int l_libgnss_get_gsa(lua_State *L) {
    int mode = luaL_optinteger(L, 1, 0);
    lua_settop(L, 0);
    if (1 == mode) {
        return l_libgnss_get_gsa_mode1(L);
    }
    else {
        return l_libgnss_get_gsa_mode0(L);
    }
}


/**
Get VTA speed information
@api libgnss.getVtg(data_mode)
@int optional, 3-original string, if no or other values   are passed, a floating point value will be returned
@return table original VTA data
@usage
-- parse nmea
log.info("nmea", "vtg", json.encode(libgnss.getVtg()))
-- Example
--[[
{
    "speed_knots":0, // speed, miles per hour
    "true_track_degrees":0, // True north direction angle
    "magnetic_track_degrees":0, // Magnetic north direction angle
    "speed_kph":0 // Speed, kilometers/hour
}
-- Reminder: Air780EG and Air510U will not return the direction angle when the speed is <5km/h
]]*/
static int l_libgnss_get_vtg(lua_State *L) {
    int mode = luaL_optinteger(L, 1, 0);
    lua_settop(L, 0);
    if (gnssctx.vtg == NULL)
        return 0;
    if (mode == 3) {
        lua_pushstring(L, gnssctx.vtg->data);
        return 1;
    }
    lua_createtable(L, 0, 10);
    struct minmea_sentence_vtg frame_vtg = {0};
    minmea_parse_vtg(&frame_vtg, gnssctx.vtg->data);

    // lua_pushliteral(L, "faa_mode");
    // lua_pushlstring(L, gnssctx.frame_vtg.faa_mode, 1);
    // lua_settable(L, -3);

    lua_pushliteral(L, "true_track_degrees");
    push_gnss_value(L, &(frame_vtg.true_track_degrees), 0);
    lua_settable(L, -3);

    lua_pushliteral(L, "magnetic_track_degrees");
    push_gnss_value(L, &(frame_vtg.magnetic_track_degrees), 0);
    lua_settable(L, -3);

    lua_pushliteral(L, "speed_knots");
    push_gnss_value(L, &(frame_vtg.speed_knots), 0);
    lua_settable(L, -3);

    lua_pushliteral(L, "speed_kph");
    push_gnss_value(L, &(frame_vtg.speed_kph), 0);
    lua_settable(L, -3);

    return 1;
}

/**
Get original ZDA time and date information
@api libgnss.getZda()
@return table original data
@usage
log.info("nmea", "zda", json.encode(libgnss.getZda()))
--Instance output
--[[
{
    "minute_offset":0, // The minute in the local time zone, usually fixed to output 0
    "hour_offset":0, // The hour in the local time zone, usually fixed to output 0
    "year":2023 // UTC year, four digits
    "month":1, // UTC month, two digits, 01 ~ 12
    "day":5, // UTC day, two digits, 01 ~ 31
    "hour":7, // hours
    "min":50, // minutes
    "sec":14, // seconds
}
]]*/
static int l_libgnss_get_zda(lua_State *L) {
    lua_createtable(L, 0, 9);
    struct tm rtime = {0};
    if (gnssctx.zda != NULL) {
        struct minmea_sentence_zda frame_zda = {0};
        minmea_parse_zda(&frame_zda, gnssctx.zda->data);

        lua_pushliteral(L, "hour_offset");
        lua_pushinteger(L, frame_zda.hour_offset);
        lua_settable(L, -3);

        lua_pushliteral(L, "minute_offset");
        lua_pushinteger(L, frame_zda.minute_offset);
        lua_settable(L, -3);

        // time related
        minmea_getdatetime(&rtime, &frame_zda.date, &frame_zda.time);
        put_datetime(L, &rtime);
    }

    return 1;
}

/**
Set debug mode
@api libgnss.debug(mode)
@bool true turns on debugging, false turns off debugging, the default is false
@usage
-- Turn on debugging, and the GNSS raw data will be output to the log.
libgnss.debug(true)
-- Turn off debugging
libgnss.debug(false)*/
static int l_libgnss_debug(lua_State *L) {
    if (lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
        LLOGD("Debug ON");
        gnss_debug = 1;
    }
    else
    {
        LLOGD("Debug OFF");
        gnss_debug = 0;
    }
    return 0;
}

/*Get GGA data
@api libgnss.getGga(data_mode)
@int format of coordinate data, 0-DDMM.MMM format, 1-DDDDDDD format, 2-DD.DDDDD format, 3-original string
@return table GGA data, if it does not exist, nil will be returned
@usage
local gga = libgnss.getGga(2)
if gga then
    log.info("GGA", json.encode(gga, "11g"))
end
--Example output
--[[
{
    "dgps_age":0, // Differential correction delay, unit is seconds
    "fix_quality":1, // Positioning status indicator 0 - invalid, 1 - single point positioning, 2 - differential positioning
    "satellites_tracked":14, // Number of satellites participating in positioning
    "altitude":0.255, // Sea level separation, or altitude, in meters,
    "hdop":0.0335, // Horizontal precision factor, 0.00 - 99.99, the value is 99.99 when not positioning
    "longitude":113.231, // Longitude, positive number is east longitude, negative number is west longitude
    "latitude":23.4067, // Latitude, positive number is north latitude, negative number is south latitude
    "height":0 //Height of ellipsoid, fixed output is 1 decimal place
}
]]*/
static int l_libgnss_get_gga(lua_State* L) {
    int mode = luaL_optinteger(L, 1, 0);
    lua_settop(L, 0);
    if (gnssctx.gga == NULL)
        return 0;
    if (mode == 3) {
        lua_pushstring(L, gnssctx.gga->data);
        return 1;
    }
    lua_newtable(L);
    struct minmea_sentence_gga frame_gga = {0};
    minmea_parse_gga(&frame_gga, gnssctx.gga->data);

    lua_pushstring(L, "altitude");
    push_gnss_value(L, &(frame_gga.altitude), 0);
    lua_settable(L, -3);

    lua_pushstring(L, "latitude");
    push_gnss_value(L, &(frame_gga.latitude), mode);
    lua_settable(L, -3);

    lua_pushstring(L, "longitude");
    push_gnss_value(L, &(frame_gga.longitude), mode);
    lua_settable(L, -3);

    lua_pushstring(L, "fix_quality");
    lua_pushinteger(L, frame_gga.fix_quality);
    lua_settable(L, -3);

    lua_pushstring(L, "satellites_tracked");
    lua_pushinteger(L, frame_gga.satellites_tracked);
    lua_settable(L, -3);

    lua_pushstring(L, "hdop");
    push_gnss_value(L, &(frame_gga.hdop), 0);
    lua_settable(L, -3);

    lua_pushstring(L, "height");
    push_gnss_value(L, &(frame_gga.height), 0);
    lua_settable(L, -3);

    lua_pushstring(L, "dgps_age");
    push_gnss_value(L, &(frame_gga.dgps_age), 0);
    lua_settable(L, -3);

    return 1;
}

/*Get GLL data
@api libgnss.getGll(data_mode)
@int format of coordinate data, 0-DDMM.MMM format, 1-DDDDDD format, 2-DD.DDDDD format
@return table GLL data, if it does not exist, nil will be returned
@usage
local gll = libgnss.getGll(2)
if gll then
    log.info("GLL", json.encode(gll, "11g"))
end
--Instance data
--[[
{
    "status":"A", // Positioning status, A is valid, B is invalid
    "mode":"A", // Positioning mode, V is invalid, A single point solution, D difference solution
    "sec":20, // seconds, UTC time shall prevail
    "min":23, // Minutes, UTC time shall prevail
    "hour":7, // hour, UTC time shall prevail
    "longitude":113.231, // Longitude, positive numbers are east longitude, negative numbers are west longitude
    "latitude":23.4067, // Latitude, positive number is north latitude, negative number is south latitude
    "us":0 // Microsecond number, usually 0
}
]]*/
static int l_libgnss_get_gll(lua_State* L) {
    int mode = luaL_optinteger(L, 1, 0);
    lua_settop(L, 0);
    if (gnssctx.gll == NULL)
        return 0;
    if (mode == 3) {
        lua_pushstring(L, gnssctx.vtg->data);
        return 1;
    }
    lua_newtable(L);
    struct minmea_sentence_gll frame_gll = {0};
    minmea_parse_gll(&frame_gll, gnssctx.gll->data);

    lua_pushstring(L, "latitude");
    push_gnss_value(L, &(frame_gll.latitude), mode);
    lua_settable(L, -3);

    lua_pushstring(L, "longitude");
    push_gnss_value(L, &(frame_gll.longitude), mode);
    lua_settable(L, -3);

    lua_pushstring(L, "mode");
    lua_pushfstring(L, "%c", frame_gll.mode);
    lua_settable(L, -3);

    lua_pushstring(L, "status");
    lua_pushfstring(L, "%c", frame_gll.status);
    lua_settable(L, -3);

    lua_pushstring(L, "hour");
    lua_pushinteger(L, frame_gll.time.hours);
    lua_settable(L, -3);
    lua_pushstring(L, "us");
    lua_pushinteger(L, frame_gll.time.microseconds);
    lua_settable(L, -3);
    lua_pushstring(L, "min");
    lua_pushinteger(L, frame_gll.time.minutes);
    lua_settable(L, -3);
    lua_pushstring(L, "sec");
    lua_pushinteger(L, frame_gll.time.seconds);
    lua_settable(L, -3);

    return 1;
}

/**
Clear historical location data
@api libgnss.clear()
@return nil no return value
@usage
-- This operation will clear all positioning data*/
static int l_libgnss_clear(lua_State*L) {
    (void)L;
    luat_libgnss_init(true);
    return 0;
}

/*Bind uart port for GNSS data reading
@api libgnss.bind(id, next_id)
@int uart port number
@int id forwarded to uart, for example virtual uart.VUART_0
@usage
-- Configure serial port information, usually 115200 8N1
uart.setup(2, 115200)
-- Bind uart and start parsing GNSS data immediately
libgnss.bind(2)
-- No need to call uart.on and then libgnss.parse
-- Debugging logs can be turned on during development period
libgnss.debug(true)

-- Firmware compiled after 2023-01-02 is valid
-- Read and parse from uart2, and forward to virtual serial port 0 at the same time
libgnss.bind(2, uart.VUART_0)*/
static int l_libgnss_bind(lua_State* L) {
    int uart_id = luaL_checkinteger(L, 1);
    l_libgnss_clear(L);
    if (libgnss_recvbuff == NULL) {
        libgnss_recvbuff = luat_heap_malloc(RECV_BUFF_SIZE);
    }
    if (luat_uart_exist(uart_id)) {
        //uart_app_recvs[uart_id] = nmea_uart_recv_cb;
        luat_uart_set_app_recv(uart_id, l_libgnss_uart_recv_cb);
    }
    if (lua_isinteger(L, 2)) {
        libgnss_route_uart_id = luaL_checkinteger(L, 2);
    }
    return 0;
}


/**
Get location string
@api libgnss.locStr(mode)
@int string mode. 0- format required by Air780EG
@return a string specifying the pattern
@usage
-- Only recommended to be called after successful positioning*/
static int l_libgnss_locStr(lua_State *L) {
    int mode = luaL_optinteger(L, 1, 0);
    char buff[64] = {0};
    float lat_f = minmea_tofloat(&gnssctx.frame_rmc.latitude);
    float lng_f = minmea_tofloat(&gnssctx.frame_rmc.longitude);
    switch (mode)
    {
    case 0:
        snprintf_(buff, 63, "%.7g,%c,%.7g,%c,1.0", 
                            fabs(lat_f), lat_f > 0 ? 'N' : 'S', 
                            fabs(lng_f), lng_f > 0 ? 'E' : 'W');
        break;
    case 1:
        snprintf_(buff, 63, "%d,%d", gnssctx.frame_rmc.latitude.value, gnssctx.frame_rmc.longitude.value);
        break;
    default:
        break;
    }
    lua_pushstring(L, buff);
    return 1;
}

/**
Automatically set RTC after successful positioning
@api libgnss.rtcAuto(enable)
@bool Whether to enable or not, the default is false to close
@usage
-- Turn on automatic setting of RTC
libgnss.rtcAuto(true)*/
static int l_libgnss_rtc_auto(lua_State *L) {
    if (lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
        gnssctx.rtc_auto = 1;
        LLOGD("GNSS->RTC Auto-Set now is ON");
    }
    else {
        gnssctx.rtc_auto = 0;
        LLOGD("GNSS->RTC Auto-Set now is OFF");
    }
    return 0;
}

static int l_libgnss_data_cb(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    // lua_getglobal(L, "sys_pub");
    lua_geti(L, LUA_REGISTRYINDEX, msg->arg2);
    if (lua_isfunction(L, -1)) {
        // lua_pushliteral(gnss_L, "GNSS_RAW_DATA");
        lua_pushlstring(L, ptr, msg->arg1);
        luat_heap_free(ptr);
        ptr = NULL;
        lua_call(L, 1, 0);
    }
    else {
        luat_heap_free(ptr);
    }
    return 0;
}

int luat_libgnss_on_rawdata(const char* data, size_t len, int type) {
    int cb = 0;
    if (type == 0) {
        if (gnss_raw_cb == 0)
            return 0;
        cb = gnss_raw_cb;
    }
    else if (type == 1) {
        if (gnss_txt_cb == 0)
            return 0;
        cb = gnss_txt_cb;
    }
    else if (type == 2) {
        if (gnss_other_cb == 0)
            return 0;
        cb = gnss_other_cb;
    }
    else {
        return 0;
    }
    char* ptr = luat_heap_malloc(len);
    if (ptr == NULL)
        return 0;
    memcpy(ptr, data, len);
    rtos_msg_t msg = {
        .handler = l_libgnss_data_cb,
        .arg1 = len,
        .arg2 = cb,
        .ptr = ptr
    };
    luat_msgbus_put(&msg, 0);
    return 0;
}

/**
Underlying event callback
@api libgnss.on(tp, fn)
@string event type, currently supports "raw"
@usage
-- This function is generally used for debugging and to obtain the data actually received by the underlying layer.
libgnss.on("raw", function(data)
    log.info("GNSS", data)
end)*/
static int l_libgnss_on(lua_State *L) {
    size_t len = 0;
    const char* tp = luaL_checklstring(L, 1, &len);
    if (!strcmp("raw", tp)) {
        if (gnss_raw_cb != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, gnss_raw_cb);
            gnss_raw_cb = 0;
        }
        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            gnss_raw_cb = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    else if (!strcmp("txt", tp)) {
        if (gnss_txt_cb != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, gnss_txt_cb);
            gnss_txt_cb = 0;
        }
        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            gnss_txt_cb = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    else if (!strcmp("other", tp)) {
        if (gnss_other_cb != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, gnss_other_cb);
            gnss_other_cb = 0;
        }
        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            gnss_other_cb = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    return 0;
}

/**
Get non-standard GPTXT data
@api libgnss.getTxt()
@return the string carried by GPTXT
@usage
-- This function was added on 2023.6.6
log.info("gnss", "txt", libgnss.getTxt())

-- test statement
libgnss.parse("$GPTXT,01,01,01,ANTENNA SHORT*63\r\n")
log.info("GNSS", libgnss.getTxt())
libgnss.parse("$GPTXT,01,01,01,ANTENNA OPEN*25\r\n")
log.info("GNSS", libgnss.getTxt())
libgnss.parse("$GPTXT,01,01,01,ANTENNA OK*35\r\n")
log.info("GNSS", libgnss.getTxt())*/
static int l_libgnss_get_txt(lua_State *L) {
    if (gnssctx.txt == NULL) {
        lua_pushliteral(L, "");
        return 1;
    }
    struct minmea_sentence_txt txt = {0};
    minmea_parse_txt(&txt, gnssctx.txt->data);
    txt.txt[FRAME_TXT_MAX_LEN] = 0x00;
    lua_pushstring(L, txt.txt);
    return 1;
}

/*Synthesize the auxiliary positioning data required by Air530Z
@api libgnss.casic_aid(dt, loc)
@table time information
@table latitude, longitude and altitude
@return string auxiliary positioning data
@usage
-- This function is suitable for the synthesis of auxiliary positioning information for CASIC series GNSS Moduless.
-- This function was added on 2023.11.14

-- First is the time information, note that it is UTC time
-- There are many time sources. It is generally recommended to use socket.sntp() to synchronize the system time.
local dt = os.date("!*t")

--Then there are auxiliary positioning coordinates
-- Sources come in many ways:
-- 1. Obtained from historical positioning data, for example, the previous positioning was saved to the local file system after successful positioning.
-- 2. Obtained through base station positioning or wifi positioning
-- 3. Obtain approximate coordinates through IP positioning
--The coordinate system is WGS84, but since it is auxiliary positioning, accuracy is not a key factor
local lla = {
    lat = 23.12,
    lng = 114.12
}

local aid = libgnss.casic_aid(dt, lla)*/
#include "luat_casic_gnss.h"
double strtod(const char *s,char **ptr);
static int l_libgnss_casic_aid(lua_State* L) {
    DATETIME_STR dt = {0};
    POS_LLA_STR lla = {0};
    const char* data = "";

    if (lua_istable(L, 1)) {
        if (LUA_TNUMBER == lua_getfield(L, 1, "day")) {
            dt.day = lua_tointeger(L, -1);
        };
        lua_pop(L, 1);
        // This is compatible with both month and mon, os.date and rtc.get
        if (LUA_TNUMBER == lua_getfield(L, 1, "month")) {
            dt.month = lua_tointeger(L, -1);
        };
        lua_pop(L, 1);
        if (LUA_TNUMBER == lua_getfield(L, 1, "mon")) {
            dt.month = lua_tointeger(L, -1);
        };
        lua_pop(L, 1);

        if (LUA_TNUMBER == lua_getfield(L, 1, "year")) {
            dt.year = lua_tointeger(L, -1);
            if (dt.year > 2022)
                dt.valid = 1;
        };
        lua_pop(L, 1);
        if (LUA_TNUMBER == lua_getfield(L, 1, "hour")) {
            dt.hour = lua_tointeger(L, -1);
        };
        lua_pop(L, 1);
        if (LUA_TNUMBER == lua_getfield(L, 1, "min")) {
            dt.minute = lua_tointeger(L, -1);
        };
        lua_pop(L, 1);
        if (LUA_TNUMBER == lua_getfield(L, 1, "sec")) {
            dt.second = lua_tointeger(L, -1);
        };
        lua_pop(L, 1);
    }
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "lat");
        if (LUA_TNUMBER == lua_type(L, -1)) {
            lla.lat = lua_tonumber(L, -1);
        }
        else if (LUA_TSTRING == lua_type(L, -1)) {
            data = luaL_checkstring(L, -1);
            lla.lat = strtod(data, NULL);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "lng");
        if (LUA_TNUMBER == lua_type(L, -1)) {
            lla.lon = lua_tonumber(L, -1);
        }
        else if (LUA_TSTRING == lua_type(L, -1)) {
            data = luaL_checkstring(L, -1);
            lla.lon = strtod(data, NULL);
        }
        lua_pop(L, 1);
        if (LUA_TNUMBER == lua_getfield(L, 2, "alt")) {
            lla.alt = lua_tonumber(L, -1);
        };

        if (lla.lat > 0.001 || lla.lat < -0.01)
            if (lla.lon > 0.001 || lla.lon < -0.01)
                lla.valid = 1;
    }
    char tmp[66] = {0};
    casicAgnssAidIni(&dt, &lla, tmp);
    lua_pushlstring(L, tmp, 66);
    return 1;
};

#include "rotable2.h"
static const rotable_Reg_t reg_libgnss[] =
{
    { "parse", ROREG_FUNC(l_libgnss_parse)},
    { "isFix", ROREG_FUNC(l_libgnss_is_fix)},
    { "getIntLocation", ROREG_FUNC(l_libgnss_get_int_location)},
    { "getRmc", ROREG_FUNC(l_libgnss_get_rmc)},
    { "getGsv", ROREG_FUNC(l_libgnss_get_gsv)},
    { "getGsa", ROREG_FUNC(l_libgnss_get_gsa)},
    { "getVtg", ROREG_FUNC(l_libgnss_get_vtg)},
    { "getGga", ROREG_FUNC(l_libgnss_get_gga)},
    { "getGll", ROREG_FUNC(l_libgnss_get_gll)},
    { "getZda", ROREG_FUNC(l_libgnss_get_zda)},
    { "locStr", ROREG_FUNC(l_libgnss_locStr)},
    { "rtcAuto",ROREG_FUNC(l_libgnss_rtc_auto)},
    { "on",     ROREG_FUNC(l_libgnss_on)},
    
    { "debug",  ROREG_FUNC(l_libgnss_debug)},
    { "clear",  ROREG_FUNC(l_libgnss_clear)},
    { "bind",   ROREG_FUNC(l_libgnss_bind)},

    { "getTxt", ROREG_FUNC(l_libgnss_get_txt)},
    { "casic_aid",   ROREG_FUNC(l_libgnss_casic_aid)},

	{ NULL,      ROREG_INT(0)}
};

LUAMOD_API int luaopen_libgnss( lua_State *L ) {
    luat_newlib2(L, reg_libgnss);
    return 1;
}
