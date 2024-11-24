
/*@Modules wlan
@summary wifi operation
@catalog Peripheral API
@version 1.0
@date 2022.09.30
@demo wlan
@tag LUAT_USE_WLAN
@usage
--[[
remind:
For Moduless that only support wifiscan, only the init/scan/scanResult function is available

For example: Air780E/Air600E/Air780EG etc. only support wifiscan
]]*/

#include "luat_base.h"
#include "luat_wlan.h"

#define LUAT_LOG_TAG "wlan"
#include "luat_log.h"

uint32_t ipaddr_addr(const char *cp);

static inline void to_ipv4(const char* data, uint8_t* dst) {
    uint32_t tmpip = ipaddr_addr(data);
    dst[3] = (tmpip >> 24) & 0xFF;
    dst[2] = (tmpip >> 16) & 0xFF;
    dst[1] = (tmpip >> 8) & 0xFF;
    dst[0] = (tmpip >> 0) & 0xFF;
}

/*initialization
@api wlan.init()
@return bool returns true if successful, otherwise returns false*/
static int l_wlan_init(lua_State* L){
    int ret = luat_wlan_init(NULL);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Set wifi mode
@api wlan.setMode(mode)
@int wifi mode
@return bool returns true if successful, otherwise returns false
@usage
-- Set to AP mode, broadcast ssid, and receive links from wifi clients
wlan.setMode(wlan.AP)

-- Set to STATION mode, which is also the default mode after initialization
wlan.setMode(wlan.STATION)

-- Mixed mode, do AP and STATION
wlan.setMode(wlan.APSTA)*/
static int l_wlan_mode(lua_State* L){
    int mode = LUAT_WLAN_MODE_STA;
    if (lua_isinteger(L, 1)) {
        mode = lua_tointeger(L, 1);
    }
    else if (lua_isinteger(L, 2)) {
        mode = lua_tointeger(L, 2);
    }

    if (mode <= LUAT_WLAN_MODE_NULL || mode >= LUAT_WLAN_MODE_MAX) {
        mode = LUAT_WLAN_MODE_STA;
    }

    // switch (mode)
    // {
    // case LUAT_WLAN_MODE_NULL:
    //     LLOGD("wlan mode NULL");
    //     break;
    // case LUAT_WLAN_MODE_STA:
    //     LLOGD("wlan mode STATION");
    //     break;
    // case LUAT_WLAN_MODE_AP:
    //     LLOGD("wlan mode AP");
    //     break;
    // case LUAT_WLAN_MODE_APSTA:
    //     LLOGD("wlan mode AP-STATION");
    //     break;
    
    // default:
    //     break;
    // }
    luat_wlan_config_t conf = {
        .mode = mode
    };
    int ret = luat_wlan_mode(&conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*When acting as a STATION, have you connected to the AP and obtained the IP successfully?
@api wlan.ready()
@return bool Returns true if the connection is successful, otherwise returns false*/
static int l_wlan_ready(lua_State* L){
    lua_pushboolean(L, luat_wlan_ready());
    return 1;
}

/*When acting as STATION, connect to the specified AP
@api wlan.connect(ssid, password, auto_reconnect, bssid)
@string AP’s ssid
@string AP password, optional
@int 0 turns off automatic reconnection, 1 turns on automatic reconnection. Currently, automatic reconnection is forced to be turned on
@string bssid of AP, optional, must be 6 bytes
@return bool Returns true if the connection is initiated successfully, otherwise returns false. Note that it does not mean that the connection to the AP is successful!!
@usage

-- Normal mode, with password
wlan.connect("myap", "12345678")
--Normal mode, without password
wlan.connect("myap")
-- Special mode, reuse the previous ssid and password, connect directly this time
-- Note that the premise is that the ssid and or password have been passed after this power-on, otherwise it will fail.
wlan.connect()

-- Special mode, using ssid and password, specify bssid for this connection, new in 2024.5.7
local bssid = string.fromHex("00182946365f")
wlan.connect("myap", "12345678", 1, bssid)*/
static int l_wlan_connect(lua_State* L){
    const char* ssid = luaL_optstring(L, 1, "");
    const char* password = luaL_optstring(L, 2, "");
    size_t len = 0;
    luat_wlan_conninfo_t info = {0};
    info.auto_reconnection = 1;
    memcpy(info.ssid, ssid, strlen(ssid));
    memcpy(info.password, password, strlen(password));
    const char* bssid = luaL_optlstring(L, 4, "", &len);
    if (len == 6) {
        memcpy(info.bssid, bssid, 6);
    }

    int ret = luat_wlan_connect(&info);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*When acting as STATION, disconnect the AP
@api wlan.disconnect()*/
static int l_wlan_disconnect(lua_State* L){
    (void)L;
    luat_wlan_disconnect();
    return 0;
}

/*Scan wifi band
@api wlan.scan()
@usage
-- Note that wlan.scan() is an asynchronous API and will return immediately after starting the scan.

-- After the wifi scan is successful, there will be a WLAN_SCAN_DONE message, just read it
sys.subscribe("WLAN_SCAN_DONE", function ()
    local results = wlan.scanResult()
    log.info("scan", "results", #results)
    for k,v in pairs(results) do
        log.info("scan", v["ssid"], v["rssi"], (v["bssid"]:toHex()))
    end
end)

--The following demonstration is a scheduled scan after initializing wifi. Please modify it according to actual business needs.
sys.taskInit(function()
    sys.wait(1000)
    wlan.init()
    while 1 do
        wlan.scan()
        sys.wait(15000)
    end
end)*/
static int l_wlan_scan(lua_State* L){
    (void)L;
    luat_wlan_scan();
    return 0;
}

/*Get wifi scan results
@api wlan.scanResult()
@return table scan results
@usage
-- Please refer to the wlan.scan() function for usage.*/
static int l_wlan_scan_result(lua_State* L) {
    int ap_limit = luaL_optinteger(L, 1, 20);
    if (ap_limit > 32)
        ap_limit = 32;
    else if (ap_limit < 8)
        ap_limit = 8;
    lua_newtable(L);
    luat_wlan_scan_result_t *results = luat_heap_malloc(sizeof(luat_wlan_scan_result_t) * ap_limit);
    if (results == NULL) {
        LLOGE("out of memory when malloc scan result");
        return 1;
    }
    memset(results, 0, sizeof(luat_wlan_scan_result_t) * ap_limit);
    int len = luat_wlan_scan_get_result(results, ap_limit);
    for (int i = 0; i < len; i++)
    {
        lua_newtable(L);

        lua_pushstring(L, (const char *)results[i].ssid);
        lua_setfield(L, -2, "ssid");

        // lua_pushfstring(L, "%02X%02X%02X%02X%02X%02X", results[i].bssid[0], 
        //                                                results[i].bssid[1], 
        //                                                results[i].bssid[2], 
        //                                                results[i].bssid[3], 
        //                                                results[i].bssid[4], 
        //                                                results[i].bssid[5]);
        lua_pushlstring(L, (const char *)results[i].bssid, 6);
        lua_setfield(L, -2, "bssid");

        lua_pushinteger(L, results[i].ch);
        lua_setfield(L, -2, "channel");

        lua_pushinteger(L, results[i].rssi);
        lua_setfield(L, -2, "rssi");

        lua_seti(L, -2, i + 1);
    }
    luat_heap_free(results);
    return 1;
}

/*Distribution network
@api wlan.smartconfig(mode)
@int Distribution mode, the default is esptouch, if 0 is passed, the network configuration will be automatically stopped.
@return bool If the start is successful or the stop is successful, return true, otherwise return false
@usage
wlan.smartconfig()
local ret, ssid, passwd = sys.waitUntil("SC_RESULT", 180*1000) -- wait up to 3 minutes
log.info("sc", ret, ssid, passwd)
-- Please check the demo for detailed usage.*/
static int l_wlan_smartconfig(lua_State *L) {
    int tp = luaL_optinteger(L, 1, LUAT_SC_TYPE_ESPTOUCH);
    if (tp == LUAT_SC_TYPE_STOP) {
        luat_wlan_smartconfig_stop();
        lua_pushboolean(L, 1);
    }
    else {
        int ret = luat_wlan_smartconfig_start(tp);
        lua_pushboolean(L, ret == 0 ? 1 : 0);
    }
    return 1;
}

/*get mac
@api wlan.getMac(tp, hexstr)
@int What kind of mac address to set? For the ESP32 series, you can only set the STA address, which is 0. The default value is also 0.
@bool Whether to convert HEX characters, the default is true, that is, the hex string is output
@return string MAC address, hexadecimal string form "AABBCCDDEEFF" or original data

log.info("wlan mac", wlan.getMac())*/
static int l_wlan_get_mac(lua_State* L){
    char tmp[6] = {0};
    char tmpbuff[16] = {0};
    luat_wlan_get_mac(luaL_optinteger(L, 1, 0), tmp);
    if (lua_isboolean(L, 2) && !lua_toboolean(L, 2)) {
        lua_pushlstring(L, tmp, 6);
    }
    else {
        sprintf_(tmpbuff, "%02X%02X%02X%02X%02X%02X", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
        lua_pushstring(L, tmpbuff);
    }
    return 1;
}


/*set up mac
@api wlan.setMac(tp, mac)
@int What kind of mac address to set? For the ESP32 series, you can only set the STA address, which is 0
@string MAC address to be set, length 6 bytes
@return bool returns true if successful, otherwise returns false
@usage
--Set the MAC address, firmware compiled after 2023-03-01 is available
local mac = string.fromHex("F01122334455")
wlan.setMac(0, mac)

-- Some Moduless support restoring default MAC, such as esp32 series
-- Firmware compiled after 2023-11-01 is available
local mac = string.fromHex("000000000000")
wlan.setMac(0, mac)*/
static int l_wlan_set_mac(lua_State* L){
    // int id = luaL_optinteger(L, 1, 0);
    const char* mac = luaL_checkstring(L, 2);
    int ret = luat_wlan_set_mac(luaL_optinteger(L, 1, 0), mac);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}


/*Get IP, only meaningful in STATION or APSTA mode
@api wlan.getIP()
@return string ip address, currently only ipv4 address is returned, such as "192.168.1.25"*/
static int l_wlan_get_ip(lua_State* L){
    char tmpbuff[16] = {0};
    luat_wlan_get_ip(luaL_optinteger(L, 1, 0), tmpbuff);
    lua_pushstring(L, tmpbuff);
    return 1;
}

/*Start AP
@api wlan.createAP(ssid, passwd, gateway, netmask, channel, opts)
@string SSID of AP, required
@string AP password, optional
@string AP gateway address, default 192.168.4.1
@string AP gateway mask, default 255.255.255.0
@int Channel established by AP, default 6
@table AP configuration options, optional
@return bool Returns true if successfully created, otherwise returns false
@usage
-- Note that when calling this AP, if the wifi mode is STATION, it will automatically switch to APSTA.
wlan.createAP("luatos1234", "12341234")
-- Set gateway IP, mask, channel, new on 2023.7.13, BSP may not support it
-- wlan.createAP("luatos1234", "12341234", "192.168.4.1", "255.255.255.0", 6)

--opts more configuration items, added in 2024.3.5
--[[
{
    hidden = false, -- whether to hide the SSID, default false, not hidden
    max_conn = 4 -- maximum number of clients, default 4
}
]]*/
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
static int l_wlan_ap_start(lua_State *L) {
    size_t ssid_len = 0;
    size_t password_len = 0;
    luat_wlan_apinfo_t apinfo = {0};
    const char* ssid = luaL_checklstring(L, 1, &ssid_len);
    const char* password = luaL_optlstring(L, 2, "", &password_len);
    const char* gateway = luaL_optstring(L, 3, "192.168.4.1");
    const char* netmask = luaL_optstring(L, 4, "255.255.255.0");
    if (strlen(gateway) > 7) {
        to_ipv4(gateway,  apinfo.gateway);
    }
    if (strlen(netmask) > 7) {
        to_ipv4(netmask,  apinfo.netmask);
    }

    apinfo.channel = (uint8_t)luaL_optinteger(L, 5, 6);
    if (ssid_len < 1) {
        LLOGE("ssid MUST NOT EMTRY");
        return 0;
    }
    if (ssid_len > 32) {
        LLOGE("ssid too long [%s]", ssid);
        return 0;
    }
    if (password_len > 63) {
        LLOGE("password too long [%s]", password);
        return 0;
    }

    if (lua_istable(L, 6)) {
        lua_getfield(L, 6, "hidden");
        apinfo.hidden = lua_toboolean(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 6, "max_conn");
        apinfo.max_conn = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    memcpy(apinfo.ssid, ssid, ssid_len);
    memcpy(apinfo.password, password, password_len);

    int ret = luat_wlan_ap_start(&apinfo);
    if (ret)
        LLOGD("apstart ret %d", ret);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/**
Turn off AP function
@api wlan.stopAP()
@return bool Returns true if successfully created, otherwise returns false
@usage
wlan.stopAP()*/
static int l_wlan_ap_stop(lua_State *L) {
    int ret = luat_wlan_ap_stop();
    if (ret)
        LLOGD("apstop ret %d", ret);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Obtain information, such as AP's bssid and signal strength, which can be obtained after STA is connected to the Internet
@api wlan.getInfo()
@return table details, key value pair format
@usage

log.info("wlan", "info", json.encode(wlan.getInfo()))
--[[
Typical output
{
    "bssid" : "xxxxxx",
    "rssi" : -89,
    "gw" : "192.168.1.1"
}
]]*/
static int l_wlan_get_info(lua_State *L) {
    char buff[48] = {0};
    char buff2[32] = {0};
    lua_newtable(L);

    luat_wlan_get_ap_bssid(buff);
    sprintf_(buff2, "%02X%02X%02X%02X%02X%02X", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
    lua_pushstring(L, buff2);
    lua_setfield(L, -2, "bssid");

    memset(buff, 0, 48);
    luat_wlan_get_ap_gateway(buff);
    lua_pushstring(L, buff);
    lua_setfield(L, -2, "gw");

    lua_pushinteger(L, luat_wlan_get_ap_rssi());
    lua_setfield(L, -2, "rssi");

    return 1;
}

/*Read or set power saving mode
@api wlan.powerSave(mode)
@int Power saving mode, optional, passed in is the setting, such as wlan.PS_NONE
@return int current power saving mode/set power saving mode
@usage
-- Please refer to the constant table PS_NONE/PS_MIN_MODEM/PS_MAX_MODEM
log.info("wlan", "PS", wlan.powerSave(wlan.PS_NONE))
--This API was added on 2023.03.31*/
static int l_wlan_powerSave(lua_State *L) {
    int mode = 0;
    if (lua_isinteger(L, 1)) {
        mode = luaL_checkinteger(L, 1);
        luat_wlan_set_ps(mode);
    }
    mode = luat_wlan_get_ps();
    lua_pushinteger(L, mode);
    return 1;
}

/*Read or set Hostname
@api wlan.hostname(new_name)
@string New hostname, optional, passing in is setting
@return string current hostname or set hostname
@usage
--This API was added on 2023.07.23
-- This function should be set before wlan.init, and at the latest before wlan.connect
--The default value of hostname is "LUATOS_" + the MAC value of the device
-- For example: LUATOS_0022EECC2399

wlan.hostname("My wifi IoT device")*/
static int l_wlan_get_set_hostname(lua_State *L) {
    if (lua_isstring(L, 1)) {
        size_t len = 0;
        const char* hostname = luaL_checklstring(L, 1, &len);
        if (len > 0) {
            if (len > 31) {
                LLOGE("hostname is too long");
                return 0;
            }
            luat_wlan_set_hostname(0, hostname);
        }
    }
    const char* tmp = luat_wlan_get_hostname(0);
    lua_pushstring(L, tmp);
    return 1;
}

/*Set the IP acquisition mode in Station mode
@api wlan.staIp(dhcp_enable, ip, netmask, gateway)
@bool Whether to enable DHCP, the default is true
@string Local IP address, such as 192.168.2.200, required when DHCP is disabled
@string Local IP mask, such as 255.255.255.0, required when DHCP is disabled
@string Local IP gateway, such as 192.168.2.1, required when DHCP is disabled
@return bool returns true if successful, otherwise returns false
@usage
--This API was added on 2023.10.06
-- This function needs to be called after wlan.init

-- Enable DHCP. DHCP is also enabled by default. Here is a demonstration of API usage.
wlan.staIp(true)
-- Disable DHCP and set your own IP/mask/gateway
wlan.staIp(false, "192.168.2.200", "255.255.255.0", "192.168.2.1")*/
static int l_wlan_set_sta_ip(lua_State *L) {
    luat_wlan_station_info_t info = {
        .dhcp_enable = 1
    };
    const char *data = NULL;
    size_t len = 0;
    // Whether DHCP
    if (lua_isinteger(L, 1))
        info.dhcp_enable = luaL_optinteger(L, 1, 1);
    else if (lua_isboolean(L, 1))
        info.dhcp_enable = lua_toboolean(L, 1);

    // local IP
    data = luaL_optlstring(L, 2, "192.168.1.201", &len);
    to_ipv4(data, info.ipv4_addr);

    //mask
    data = luaL_optlstring(L, 3, "255.255.255.0", &len);
    to_ipv4(data, info.ipv4_netmask);

    // gateway
    data = luaL_optlstring(L, 4, "192.168.1.1", &len);
    to_ipv4(data, info.ipv4_gateway);

    int ret = luat_wlan_set_station_ip(&info);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_wlan[] =
{
    { "init",               ROREG_FUNC(l_wlan_init)},
    { "scan",               ROREG_FUNC(l_wlan_scan)},
    { "scanResult",         ROREG_FUNC(l_wlan_scan_result)},
#ifndef LUAT_USE_WLAN_SCANONLY
    { "mode",               ROREG_FUNC(l_wlan_mode)},
    { "setMode",            ROREG_FUNC(l_wlan_mode)},
    { "ready",              ROREG_FUNC(l_wlan_ready)},
    { "connect",            ROREG_FUNC(l_wlan_connect)},
    { "disconnect",         ROREG_FUNC(l_wlan_disconnect)},

    // Distribution network related
    { "smartconfig",         ROREG_FUNC(l_wlan_smartconfig)},

    { "getIP",               ROREG_FUNC(l_wlan_get_ip)},
    { "getInfo",             ROREG_FUNC(l_wlan_get_info)},
    { "getMac",              ROREG_FUNC(l_wlan_get_mac)},
    { "setMac",              ROREG_FUNC(l_wlan_set_mac)},
    { "hostname",            ROREG_FUNC(l_wlan_get_set_hostname)},

    { "powerSave",           ROREG_FUNC(l_wlan_powerSave)},

    { "staIp",               ROREG_FUNC(l_wlan_set_sta_ip)},

    // AP related
    { "createAP",            ROREG_FUNC(l_wlan_ap_start)},
    { "stopAP",              ROREG_FUNC(l_wlan_ap_stop)},
    // wifi mode
    //@const NONE WLAN mode, disabled
    {"NONE",                ROREG_INT(LUAT_WLAN_MODE_NULL)},
    //@const STATION WLAN mode, STATION mode, active connection to AP
    {"STATION",             ROREG_INT(LUAT_WLAN_MODE_STA)},
    //@const AP WLAN mode, AP mode, accept STATION connection
    {"AP",                  ROREG_INT(LUAT_WLAN_MODE_AP)},
    //@const AP WLAN mode, mixed mode
    {"STATIONAP",           ROREG_INT(LUAT_WLAN_MODE_APSTA)},

    // Distribution network mode
    //@const STOP Stop network distribution
    {"STOP",                ROREG_INT(LUAT_SC_TYPE_STOP)},
    //@const ESPTOUCH esptouch distribution network, V1
    {"ESPTOUCH",            ROREG_INT(LUAT_SC_TYPE_ESPTOUCH)},
    //@const AIRKISS Airkiss distribution network, commonly used on WeChat
    {"AIRKISS",             ROREG_INT(LUAT_SC_TYPE_AIRKISS)},
    //@const ESPTOUCH_AIRKISS esptouch and Airkiss hybrid distribution network
    {"ESPTOUCH_AIRKISS",    ROREG_INT(LUAT_SC_TYPE_ESPTOUCH_AIRKISS)},
    //@const ESPTOUCH_V2 esptouch distribution network, V2, not tested
    {"ESPTOUCH_V2",         ROREG_INT(LUAT_SC_TYPE_ESPTOUCH_V2)},

    //@const PS_NONE Turn off power saving mode
    {"PS_NONE",             ROREG_INT(0)},
    //@const PS_MIN_MODEM Minimum Modem power saving mode
    {"PS_MIN_MODEM",        ROREG_INT(1)},
    //@const PS_MAX_MODEM Maximum Modem power saving mode
    {"PS_MAX_MODEM",        ROREG_INT(2)},

#endif
	{ NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_wlan( lua_State *L ) {
    luat_newlib2(L, reg_wlan);
    return 1;
}
