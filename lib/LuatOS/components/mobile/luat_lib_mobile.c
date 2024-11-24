
/*@Modules  mobile
@summary 蜂窝网络
@version 1.0
@date    2022.8.9
@demo    mobile
@tag LUAT_USE_MOBILE
@usage
-- 简单演示

log.info("imei", mobile.imei())
log.info("imsi", mobile.imsi())
local sn = mobile.sn()
if sn then
    log.info("sn",   sn:toHex())
end
log.info("muid", mobile.muid())
log.info("iccid", mobile.iccid())
log.info("csq", mobile.csq())
log.info("rssi", mobile.rssi())
log.info("rsrq", mobile.rsrq())
log.info("rsrp", mobile.rsrp())
log.info("snr", mobile.snr())
log.info("simid", mobile.simid())*/
#include "luat_base.h"
#include "luat_mem.h"
#include "luat_rtos.h"
#include "luat_msgbus.h"

#include "luat_mobile.h"
#include "luat_network_adapter.h"

#define LUAT_LOG_TAG "mobile"
#include "luat_log.h"
extern void luat_cc_start_speech(uint32_t param);
extern void luat_cc_play_tone(uint32_t param);
/**
Get IMEI
@api mobile.imei(index)
@int number, default 0. 0 or 1 will only appear on Moduless that support dual cards
@return string current IMEI value, return nil if failed
@usgae
-- Note that currently all Moduless only support single standby, so the IMEI is always the same*/
static int l_mobile_imei(lua_State* L) {
    char buff[24] = {0};
    // size_t len = 0;
    // size_t wlen = 0;
    int ret = 0;
    int index = luaL_optinteger(L, 1, 0);
    ret = luat_mobile_get_imei(index, buff, 24);
    // if (lua_isstring(L, 2)) {
    //     const char* wbuff = luaL_checklstring(L, 2, &wlen);
    //     if (wlen >= 15) {
    //         ret = luat_mobile_set_imei(index, wbuff, wlen);
    //         LLOGI("IMEI write %d %s ret %d", index, wbuff, ret);
    //     }
    // }
    if (ret > 0) {
        buff[23] = 0x00; // Make sure it ends
        lua_pushlstring(L, buff, strlen(buff));
    }
    else
        lua_pushnil(L);
    return 1;
}

/**
Get IMSI
@api mobile.imsi(index)
@int number, default 0. 0 or 1 will only appear on Moduless that support dual cards
@return string current IMSI value, if failed, return nil
@usgae
-- Note that currently all Moduless only support single standby, so the IMSI is always the same*/
static int l_mobile_imsi(lua_State* L) {
    char buff[24] = {0};
    // size_t len = 0;
    // size_t wlen = 0;
    int ret = 0;
    int index = luaL_optinteger(L, 1, 0);
    ret = luat_mobile_get_imsi(index, buff, 24);
    // if (lua_isstring(L, 2)) {
    //     const char* wbuff = luaL_checklstring(L, 2, &wlen);
    //     if (wlen >= 1) {
    //         ret = luat_mobile_set_imsi(index, wbuff, wlen);
    //         LLOGI("IMSI write %d %s ret %d", index, wbuff, ret);
    //     }
    // }
    if (ret > 0){
        buff[23] = 0x00; // Make sure it ends
        lua_pushlstring(L, buff, strlen(buff));
    }
    else
        lua_pushnil(L);
    return 1;
}



/**
Get SN
@api mobile.sn()
@return string The current SN value, if failed, returns nil. Note that SN may contain invisible characters
@usage
-- Note that the SN may not be written when leaving the factory.
-- A unique id for general purposes, which can be replaced by mobile.imei()
-- If you need a truly unique ID, use mcu.unique_id()*/
static int l_mobile_sn(lua_State* L) {
    char buff[32] = {0};
    // size_t len = 0;
    // size_t wlen = 0;
    int ret = 0;
    ret = luat_mobile_get_sn(buff, 32);
    // if (lua_isstring(L, 1)) {
    //     const char* wbuff = luaL_checklstring(L, 1, &wlen);
    //     if (wlen >= 1) {
    //         ret = luat_mobile_set_sn(wbuff, wlen);
    //         LLOGI("SN write %d %s ret %d", index, wbuff, ret);
    //     }
    // }
    if (ret > 0) {        
        //buff[63] = 0x00; // Make sure it ends
        lua_pushlstring(L, buff, ret);
    }
    else
        lua_pushnil(L);
    return 1;
}


/**
Get MUID
@api mobile.muid()
@return string current MUID value, return nil if failed*/
static int l_mobile_muid(lua_State* L) {
    char buff[33] = {0};
    // size_t len = 0;
    // size_t wlen = 0;
    int ret = 0;
    ret = luat_mobile_get_muid(buff, 32);
    if (lua_isstring(L, 1)) {
        // const char* wbuff = luaL_checklstring(L, 1, &wlen);
        // if (wlen >= 15) {
        //     ret = luat_mobile_set_muid(index, wbuff, wlen);
        //     LLOGI("SN write %d %s ret %d", index, wbuff, ret);
        // }
    }
    if (ret > 0)  {        
        lua_pushlstring(L, buff, strlen(buff));
    }
    else
        lua_pushnil(L);
    return 1;
}


/**
Get or set ICCID
@api mobile.iccid(id)
@int SIM card number, such as 0, 1, default 0
@return string ICCID value, return nil if failed*/
static int l_mobile_iccid(lua_State* L) {
    char buff[24] = {0};
    // size_t len = 0;
    // size_t wlen = 0;
    int ret = 0;
    int index = luaL_optinteger(L, 1, 0);
    ret = luat_mobile_get_iccid(index, buff, 24);
    if (ret > 0) {        
        buff[23] = 0x00; // Make sure it ends
        lua_pushlstring(L, buff, strlen(buff));
    }
    else
        lua_pushnil(L);
    return 1;
}

/**
Obtain the mobile phone card number. Note that it can only be read after the mobile phone number is written, so it may be empty when read.
@api mobile.number(id)
@int SIM card number, such as 0, 1, default 0
@return string number value, return nil if failed*/
static int l_mobile_number(lua_State* L) {
    char buff[24] = {0};
    // size_t len = 0;
    // size_t wlen = 0;
    int ret = 0;
    int index = luaL_optinteger(L, 1, 0);
    ret = luat_mobile_get_sim_number(index, buff, 24);
    if (ret > 0) {
        buff[23] = 0x00; // Make sure it ends
        lua_pushlstring(L, buff, strlen(buff));
    }
    else
        lua_pushnil(L);
    return 1;
}

/**
Get the current SIM card slot, or switch card slots
@api mobile.simid(id)
@int The number of the SIM card, such as 0, 1. If it supports dual cards, such as EC618, you can fill in 2 to adapt, but it will occupy 4 IOs (gpio4/5/6/23). If you do not fill it in, it will directly read the current card slot.
@boolean Whether to use SIM0 first? Only if the SIM card number is written as 2, adaptive will be useful! ! ! . True gives priority to SIM0, false depends on the specific platform. It supports dual-SIM dual standby and SIM0 takes priority. What is not supported is the last detected priority. The default is false. It must be configured at boot, otherwise it will be invalid.
@return int current sim card slot number, if failed, return -1
@usage
mobile.simid(0) -- Fixed use of SIM0
mobile.simid(1) -- Firmware uses SIM1
mobile.simid(2) -- Automatically identify SIM0, SIM1, the priority depends on the specific platform
mobile.simid(2, true) -- -- Automatically identify SIM0, SIM1, and SIM0 takes priority
-- Reminder, automatic recognition will increase the time*/
static int l_mobile_simid(lua_State* L) {
    // char buff[24] = {0};
    int ret = 0;
    int id = 0;
    if (lua_isinteger(L, 1)) {
        id = luaL_checkinteger(L, 1);
        ret = luat_mobile_set_sim_id(id);
        LLOGI("sim set to %d , ret %d", id, ret);
    }
    if (LUA_TBOOLEAN == lua_type(L, 2)) {
    	if (lua_toboolean(L, 2)) {
    		luat_mobile_set_sim_detect_sim0_first();
    	}
    }
    ret = luat_mobile_get_sim_id(&id);
    if (ret == 0) {
        lua_pushinteger(L, id);
    }
    else {
        lua_pushinteger(L, -1);
    }
    return 1;
}

/**
Check whether the current SIM card is ready and perform related operations on the PIN code of the SIM card.
@api mobile.simPin(id,operation,pin1,pin2)
@int The number of the SIM card, such as 0, 1, only needs to be selected if it supports dual card dual standby
@int PIN code operation type, can only be mobile.PIN_XXXX, leave it blank if no operation is required
@string The pin code used when changing the pin, or the pin code used for verification, or the PUK when unlocking the pin code, 4~8 bytes
@string New pin code when changing the pin code operation, new PIN when unlocking the pin code, 4~8 bytes
@return boolean When there is no PIN operation, it returns whether the SIM card is ready. When there is a PIN operation, it returns whether it is successful.
@usage
local cpin_is_ready = mobile.simPin() -- Whether the current sim card is ready. Generally, if it returns false, it means there is no card.
local succ = mobile.simPin(0, mobile.PIN_VERIFY, "1234") -- Enter pin code for verification*/
static int l_mobile_sim_pin(lua_State* L) {
    char old[9] = {0};
    char new[9] = {0};
    int id = luaL_optinteger(L, 1, 0);
    int operation = luaL_optinteger(L, 2, -1);
    size_t old_len, new_len;
    if (lua_isstring(L, 3))
    {
    	const char *old_pin = lua_tolstring(L, 3, &old_len);
    	memcpy(old, old_pin, (old_len > 8)?8:old_len);
    }
    if (lua_isstring(L, 4))
    {
    	const char *new_pin = lua_tolstring(L, 4, &new_len);
    	memcpy(new, new_pin, (new_len > 8)?8:new_len);
    }
    if (operation != -1)
    {
    	lua_pushboolean(L, (luat_mobile_set_sim_pin(id, operation, old, new) == 0));
    }
    else
    {
    	lua_pushboolean(L, (luat_mobile_get_sim_ready(id) == 1));
    }
    return 1;
}

/**
Set the RRC automatic release time interval. When turned on, extremely weak signals + frequent data operations may cause serious network failures, so additional automatic restart protocol stacks need to be set.
@api mobile.rtime(time, auto_reset_stack, data_first)
@int RRC automatic release time, which is equivalent to Air724's AT+RTIME, in seconds. Writing 0 or not writing it will disable it. Do not exceed 20 seconds, which is meaningless.
@boolean Try to automatically recover when the network encounters a serious failure, which conflicts with airplane mode/SIM card switching. True is on, false is off. If left blank, it will be automatically on if the time is set. This parameter will be obsolete on September 14, 2023
@boolean Whether to enable data transmission optimization, true to enable, false to disable, leave blank to false. After enabling, you must wait until TCP data ACK or timeout failure, or socket CONNECT is completed (regardless of success or failure) before RRC is allowed to be released early, which may increase functionality. Consumption. This parameter will be activated on August 12, 2024
@return nil no return value
@usage
mobile.rtime(3) -- Release RRC early after 3 seconds of no data interaction with the base station
mobile.rtime(3,nil,true) --Enable data transmission optimization and release RRC early after 3 seconds of no data interaction with the base station.*/
extern void net_lwip_check_switch(uint8_t onoff);
static int l_mobile_set_rrc_auto_release_time(lua_State* L) {
	uint8_t release_time = luaL_optinteger(L, 1, 0);

    if (LUA_TBOOLEAN == lua_type(L, 3)) {
    	net_lwip_check_switch(lua_toboolean(L, 3));
    }
    uint32_t idle_time = luaL_optinteger(L, 4, 0);
    if (idle_time >= 10)
    {
    	luat_mobile_set_auto_rrc(release_time, idle_time);
    }
    else
    {
    	luat_mobile_set_rrc_auto_release_time(release_time);
    }
    return 0;
}

/**
Set up some auxiliary periodic or automatic functions. Currently, it supports recovery after the SIM card is temporarily detached, periodically obtains cell information, and attempts to automatically recover when the network encounters a serious failure.
@api mobile.setAuto(check_sim_period, get_cell_period, search_cell_time, auto_reset_stack, network_check_period)
@int The SIM card automatic recovery time, in milliseconds, is recommended to be 5000~10000. It conflicts with airplane mode/SIM card switching and cannot be used at the same time. It must be staggered. Writing 0 or not writing will turn off the function.
@int The time interval for periodically obtaining cell information, in milliseconds. Obtaining cell information will increase some power consumption. Writing 0 or not writing will turn off the function.
@int The maximum search time for each cell search, in seconds. no more than 8 seconds
@boolean attempts to automatically recover when the network encounters a serious failure, which conflicts with airplane mode/SIM card switching. True is on, false is off. The starting state is false. If left blank, no changes will be made.
@int Set the timer to check whether the network is normal and recover by restarting the protocol stack when no network is detected for a long time. The no-network recovery time, in ms, is recommended to be more than 60000. Reserve enough time for network search. Leave it blank to not do it. Change
@return nil no return value*/
static int l_mobile_set_auto_work(lua_State* L) {
	luat_mobile_set_period_work(luaL_optinteger(L, 2, 0), luaL_optinteger(L, 1, 0), luaL_optinteger(L, 3, 0));
    if (LUA_TBOOLEAN == lua_type(L, 4)) {
    	luat_mobile_fatal_error_auto_reset_stack(lua_toboolean(L, 4));
    }
    if (lua_isinteger(L, 5)) {
    	luat_mobile_set_check_network_period(luaL_optinteger(L, 5, 0));
    }

	return 0;
}

/**
Obtain or set APN. The APN must be set before accessing the network, for example, before the SIM card recognition is completed.
@api mobile.apn(index, cid, new_apn_name, user_name, password, ip_type, protocol)
@int number, default 0. 0 or 1 will only appear on Moduless that support dual cards
@int cid, default 0, if you want to activate with non-default APN, it must be >0
@string New APN. If you don’t fill it in, you will get the APN. If you fill it in, you will set the APN. Whether setting is supported depends on the underlying implementation.
@string The username of the new APN. If the APN is not empty, it must be filled in. If not, leave an empty string "". If APN is empty, it can be nil
@string The password of the new APN. If the APN is not empty, it must be filled in. If not, leave an empty string "". If APN is empty, it can be nil
@int IP TYPE when activating APN, 1=IPV4 2=IPV6 3=IPV4V6, the default is 1
@int When activating APN, if username and password are required, the authentication protocol type must be written, 1~3, the default is 3, which means try both 1 and 2. If authentication is not required, write 0
@boolean Whether to delete APN, true is yes, others are no, it only has an effect when the new APN in parameter 3 is not a string
@return string The default APN value obtained, returns nil on failure
@usage
mobile.apn(0,1,"cmiot","","",nil,0) -- The mobile public network card sets the APN to cmiot. Generally, there is no need to set it.
mobile.apn(0,1,"name","user","password",nil,3) -- For the demo of private network card settings, name, user and password, please contact the card dealer to obtain*/
static int l_mobile_apn(lua_State* L) {
    char buff[64] = {0};
    size_t len = 0;
    size_t wlen = 0;
    int ret = 0;
    int index = luaL_optinteger(L, 1, 0);
    int cid = luaL_optinteger(L, 2, 0);
    ret = luat_mobile_get_apn(index, cid, buff, sizeof(buff) - 1);
	if (lua_isstring(L, 3)) {
		const char* wbuff = luaL_checklstring(L, 3, &wlen);
		size_t user_name_len = 0;
		size_t password_len = 0;
		const char* user_name = luaL_checklstring(L, 4, &user_name_len);
		const char* password = luaL_checklstring(L, 5, &password_len);
		uint8_t ip_type = luaL_optinteger(L, 6, 1);
		uint8_t protocol = luaL_optinteger(L, 7, 3);
		if (!user_name_len && !password_len)
		{
			protocol = 0;
		}
		if (wlen) {
			luat_mobile_user_apn_auto_active(index, cid, ip_type, protocol, wbuff, wlen, user_name, user_name_len, password, password_len);
		}
		else
		{
			luat_mobile_user_apn_auto_active(index, cid, ip_type, 0xff, NULL, 0, NULL, 0, NULL, 0);
		}
	}
	else
	{
    	if (lua_isboolean(L, 8) && lua_toboolean(L, 8))
    	{
    		luat_mobile_del_apn(index, cid, 0);
    	}
	}
    if (ret > 0) {
        lua_pushlstring(L, buff, strlen(buff));
    }
    else
        lua_pushnil(L);
    return 1;
}

/**
Whether to enable the IPV6 function by default must be set before connecting to the LTE network.
@api mobile.ipv6(onff)
@boolean switch true on false off
@return boolean true is currently on, false is currently off
@usage
-- Note that after turning on ipv6, booting up and connecting to the Internet will be 2~3 seconds slower.*/
static int l_mobile_ipv6(lua_State* L) {
    // char buff[24] = {0};
//	uint8_t onoff;
    if (LUA_TBOOLEAN == lua_type(L, 1)) {
    	luat_mobile_set_default_pdn_ipv6(lua_toboolean(L, 1));
    }
    lua_pushboolean(L, luat_mobile_get_default_pdn_ipv6());
    return 1;
}

/**
Get csq
@api mobile.csq()
@return int current CSQ value, if failed, return 0. Range 0 - 31, the bigger the better
@usage
-- Note that the CSQ value of the 4G Modules is for reference only, rsrp/rsrq is the real signal strength indicator.*/
static int l_mobile_csq(lua_State* L) {
    // luat_mobile_signal_strength_info_t info = {0};
    uint8_t csq = 0;
    if (luat_mobile_get_signal_strength(&csq) == 0) {
        lua_pushinteger(L, (int)csq);
    }
    else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

/**
Get rssi
@api mobile.rssi()
@return int current rssi value, if failure returns 0. Range 0 to -114, the smaller the better*/
static int l_mobile_rssi(lua_State* L) {
    luat_mobile_signal_strength_info_t info = {0};
    if (luat_mobile_get_signal_strength_info(&info) == 0) {
        lua_pushinteger(L, info.lte_signal_strength.rssi);
    }
    else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

/**
Get rsrp, reference signal received power
@api mobile.rsrp()
@return int current rsrp value, if failed, return 0. Value range: -44 ~ -140, the larger the value, the better*/
static int l_mobile_rsrp(lua_State* L) {
    luat_mobile_signal_strength_info_t info = {0};
    if (luat_mobile_get_signal_strength_info(&info) == 0) {
        lua_pushinteger(L, info.lte_signal_strength.rsrp);
    }
    else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

/**
Get rsrq, reference signal transmit power
@api mobile.rsrq()
@return int current rsrq value, if failed, return 0. Value range: -3 ~ -19.5, the larger the value, the better*/
static int l_mobile_rsrq(lua_State* L) {
    luat_mobile_signal_strength_info_t info = {0};
    if (luat_mobile_get_signal_strength_info(&info) == 0) {
        lua_pushinteger(L, info.lte_signal_strength.rsrq);
    }
    else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

/**
Get snr, signal-to-noise ratio
@api mobile.snr()
@return int current snq value, if failed, return 0. Range 0 - 30, the bigger the better*/
static int l_mobile_snr(lua_State* L) {
    luat_mobile_signal_strength_info_t info = {0};
    if (luat_mobile_get_signal_strength_info(&info) == 0) {
        lua_pushinteger(L, info.lte_signal_strength.snr);
    }
    else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

/**
Get the ECI (E-UTRAN Cell Identifier)   of the current serving cell
@api mobile.eci()
@return int current eci value, if failed, return -1*/
static int l_mobile_eci(lua_State* L) {
    uint32_t eci;
    if (luat_mobile_get_service_cell_identifier(&eci) == 0) {
        lua_pushinteger(L, eci);
    }
    else {
        lua_pushinteger(L, -1);
    }
    return 1;
}

/**
Get the TAC or LAC of the current serving cell
@api mobile.tac()
@return int current eci value, if it fails, it returns -1. If it has not been registered to the network, it will return 0
@usage
--This API was added on 2023.7.9*/
static int l_mobile_tac(lua_State* L) {
    uint32_t tac;
    if (luat_mobile_get_service_tac_or_lac(&tac) == 0) {
        lua_pushinteger(L, tac);
    }
    else {
        lua_pushinteger(L, -1);
    }
    return 1;
}

/**
Get the eNBID (eNodeB Identifier)   of the current serving cell
@api mobile.enbid()
@return int current enable value, if failed, return -1*/
static int l_mobile_enbid(lua_State* L) {
    uint32_t eci;
    if (luat_mobile_get_service_cell_identifier(&eci) == 0) {
        lua_pushinteger(L, eci>>8);
    }
    else {
        lua_pushinteger(L, -1);
    }
    return 1;
}

/**
Entering and exiting airplane mode
@api mobile.flymode(index, enable)
@int number, default 0. 0 or 1 will only appear on Moduless that support dual cards
@bool Whether to set to flight mode, true means setting, false means exit, optional
@return bool original flight mode status*/
static int l_mobile_flymode(lua_State* L) {
    int index = luaL_optinteger(L, 1, 0);
    int flymode = luat_mobile_get_flymode(index);
    if (lua_isboolean(L, 2)) {
        luat_mobile_set_flymode(index, lua_toboolean(L, 2));
    }
    lua_pushboolean(L, flymode == 0 ? 0 : 1);
    return 1;
}

/**
Configure base station synchronization time switch, enabled by default
@api mobile.syncTime(enable)
@bool is on, true is on, false is off, nil is not set
@return bool current switch status
@usage
mobile.syncTime() --Get the current switch status
mobile.syncTime(false) --Turn off base station synchronization time*/
static int l_mobile_sync_time(lua_State* L) {
    if (lua_isboolean(L, 1)) {
    	luat_mobile_set_sync_time(lua_toboolean(L, 1));
    }
    lua_pushboolean(L, luat_mobile_get_sync_time());
    return 1;
}

/**
Get network status
@api mobile.status()
@return int current network status
@usage
-- Status description
-- 0: The network is not registered
-- 1: The network has been registered
-- 2: Searching the Internet
-- 3: Network registration rejected
-- 4: Network status unknown
-- 5: Roaming and registered
-- 6: Only SMS available
-- 7: Only SMS is available and roaming status
-- 8: Emergency call only. Note that this state is not supported in China, and the Modules does not support emergency calls.

-- It is not recommended to use this API to determine the network status. It is recommended to use socket.localIP() to determine the status.*/
static int l_mobile_status(lua_State* L) {
    int LUAT_MOBILE_REGISTER_STATUS_E = luat_mobile_get_register_status();
    lua_pushinteger(L, LUAT_MOBILE_REGISTER_STATUS_E);
    return 1;
}

static inline uint16_t u162bcd(uint16_t src) {
    uint8_t high = (src >> 8) & 0xFF;
    uint8_t low  = src & 0xFF;
    uint16_t dst = 0;
    dst += (low & 0x0F) + (low >> 4) * 10;
    dst += ((high & 0x0F) + (high >> 4) * 10) * 100;
    //LLOGD("src %04X dst %d", src, dst);
    return dst;
}

/**
Get base station information
@api mobile.getCellInfo()
@return table array containing base station data
@usage
-- Note: Starting from 2023.06.20, you need to actively request reqCellInfo once to have base station data.

--Example output (the original data is a table, the following is the json formatted content)
--[[
[
    {"rsrq":-10,"rssi":-55,"cid":124045360,"mnc":17,"pci":115,"earfcn":1850,"snr":15,"rsrp":- 85,"mcc":1120,"tdd":0},
    {"pci":388,"rsrq":-11,"mnc":17,"earfcn":2452,"snr":5,"rsrp":-67,"mcc":1120,"cid":124045331 },
    {"pci":100,"rsrq":-9,"mnc":17,"earfcn":75,"snr":17,"rsrp":-109,"mcc":1120,"cid":227096712 }
]
]]

mobile.reqCellInfo(60)
-- Subscribe
sys.subscribe("CELL_INFO_UPDATE", function()
    log.info("cell", json.encode(mobile.getCellInfo()))
end)

-- Regular rotation training
sys.taskInit(function()
    sys.wait(3000)
    while 1 do
        mobile.reqCellInfo(15)
        sys.waitUntil("CELL_INFO_UPDATE", 15000)
        log.info("cell", json.encode(mobile.getCellInfo()))
    end
end)*/
static int l_mobile_get_cell_info(lua_State* L) {
    lua_newtable(L);
    luat_mobile_cell_info_t* info = luat_heap_malloc(sizeof(luat_mobile_cell_info_t));
    if (info == NULL) {
        LLOGE("out of memory when malloc cell_info");
        return 1;
    }
    int ret = luat_mobile_get_last_notify_cell_info(info);
    if (ret != 0) {
        LLOGI("none cell info found %d", ret);
        goto exit;
    }

    //LLOGD("cid %d neighbor %d", info->lte_service_info.cid, info->lte_neighbor_info_num);

    //Currently only returns lte information
    if (info->lte_info_valid == 0 || info->lte_service_info.cid == 0) {
        if (0 == luat_mobile_get_service_cell_identifier(&info->lte_service_info.cid) && info->lte_service_info.cid) {
            LLOGW("Please call mobile.reqCellInfo() first!!");
        }
        else {
            LLOGI("lte cell info not found");
        }
        goto exit;
    }
    
    lua_newtable(L);
    lua_pushinteger(L, info->lte_service_info.pci);
    lua_setfield(L, -2, "pci");
    lua_pushinteger(L, info->lte_service_info.cid);
    lua_setfield(L, -2, "cid");
    lua_pushinteger(L, info->lte_service_info.earfcn);
    lua_setfield(L, -2, "earfcn");
    lua_pushinteger(L, info->lte_service_info.rsrp);
    lua_setfield(L, -2, "rsrp");
    lua_pushinteger(L, info->lte_service_info.rsrq);
    lua_setfield(L, -2, "rsrq");
    lua_pushinteger(L, info->lte_service_info.rssi);
    lua_setfield(L, -2, "rssi");
    lua_pushinteger(L, info->lte_service_info.is_tdd);
    lua_setfield(L, -2, "tdd");
    lua_pushinteger(L, info->lte_service_info.snr);
    lua_setfield(L, -2, "snr");
    lua_pushinteger(L, u162bcd(info->lte_service_info.mcc));
    lua_setfield(L, -2, "mcc");
    lua_pushinteger(L, u162bcd(info->lte_service_info.mnc));
    lua_setfield(L, -2, "mnc");
    lua_pushinteger(L, info->lte_service_info.tac);
    lua_setfield(L, -2, "tac");
    lua_pushinteger(L, info->lte_service_info.band);
    lua_setfield(L, -2, "band");
    lua_pushinteger(L, info->lte_service_info.ulbandwidth);
    lua_setfield(L, -2, "ulbandwidth");
    lua_pushinteger(L, info->lte_service_info.dlbandwidth);
    lua_setfield(L, -2, "dlbandwidth");
    lua_seti(L, -2, 1);

    if (info->lte_neighbor_info_num > 0) {
        for (size_t i = 0; i < info->lte_neighbor_info_num; i++)
        {
            lua_settop(L, 1);
            //LLOGD("add neighbor %d", i);
            lua_newtable(L);
            lua_pushinteger(L, info->lte_info[i].pci);
            lua_setfield(L, -2, "pci");
            lua_pushinteger(L, info->lte_info[i].cid);
            lua_setfield(L, -2, "cid");
            if (0x8850 == info->version)
            {
                lua_pushinteger(L, info->lte_info[i].rssi);
                lua_setfield(L, -2, "rssi");
                lua_pushinteger(L, info->lte_info[i].celltype);
                lua_setfield(L, -2, "celltype");
                lua_pushinteger(L, info->lte_info[i].bandwidth);
                lua_setfield(L, -2, "bandwidth");
            }
            lua_pushinteger(L, info->lte_info[i].earfcn);
            lua_setfield(L, -2, "earfcn");
            lua_pushinteger(L, info->lte_info[i].rsrp);
            lua_setfield(L, -2, "rsrp");
            lua_pushinteger(L, info->lte_info[i].rsrq);
            lua_setfield(L, -2, "rsrq");
            lua_pushinteger(L, u162bcd(info->lte_info[i].mcc));
            lua_setfield(L, -2, "mcc");
            lua_pushinteger(L, u162bcd(info->lte_info[i].mnc));
            lua_setfield(L, -2, "mnc");
            lua_pushinteger(L, info->lte_info[i].snr);
            lua_setfield(L, -2, "snr");
            lua_pushinteger(L, info->lte_info[i].tac);
            lua_setfield(L, -2, "tac");

            lua_seti(L, -2, i + 2);
        }
    }
    lua_settop(L, 1);

    exit:
        luat_heap_free(info);
    return 1;
}
/**
Initiate base station information query, including neighboring cells
@api mobile.reqCellInfo(timeout)
@int Timeout length, unit seconds, default 15. Minimum 5, maximum 60
@return nil no return value
@usage
-- Refer to mobile.getCellInfo function*/
static int l_mobile_request_cell_info(lua_State* L) {
    int timeout = luaL_optinteger(L, 1, 15);
    if (timeout > 60)
        timeout = 60;
    else if (timeout < 5)
        timeout = 5;
    luat_mobile_get_cell_info_async(timeout);
    return 0;
}

/**
Restart the protocol stack
@api mobile.reset()
@return nil no return value
@usage
-- Restart the LTE protocol stack
mobile.reset()*/
static int l_mobile_reset(lua_State* L) {
    luat_mobile_reset_stack();
    return 0;
}

/**
Data volume traffic processing
@api mobile.dataTraffic(clearUplink, clearDownlink)
@boolean clears the cumulative value of upstream traffic, clears if true, and ignores others.
@boolean clears the cumulative value of downstream traffic, clears if true, and ignores others.
@return int Upstream traffic GB
@return int Upstream traffic B
@return int Downstream traffic GB
@return int Downstream traffic B
@usage
-- Get the cumulative value of upstream and downstream traffic
-- Uplink traffic value Byte = uplinkGB * 1024 * 1024 * 1024 + uplinkB
-- Downlink traffic value Byte = downlinkGB * 1024 * 1024 * 1024 + downlinkB
local uplinkGB, uplinkB, downlinkGB, downlinkB = mobile.dataTraffic()

--Clear the accumulated value of upstream and downstream traffic
mobile.dataTraffic(true, true)

--Only record traffic after power on, reset/restart will return to zero*/
static int l_mobile_data_traffic(lua_State* L) {
    uint64_t uplink;
    uint64_t downlink;
    uint8_t clear_uplink = 0;
    uint8_t clear_downlink = 0;
    volatile uint32_t temp;
    if (LUA_TBOOLEAN == lua_type(L, 1)) {
    	clear_uplink = lua_toboolean(L, 1);
    }
    if (LUA_TBOOLEAN == lua_type(L, 2)) {
    	clear_downlink = lua_toboolean(L, 2);
    }
    luat_mobile_get_ip_data_traffic(&uplink, &downlink);
    if (clear_uplink || clear_downlink) {
    	luat_mobile_clear_ip_data_traffic(clear_uplink, clear_downlink);
    }
    temp = (uint32_t)(uplink >> 30);
    lua_pushinteger(L, temp);
    temp = (((uint32_t)uplink) & 0x3FFFFFFF);
    lua_pushinteger(L, temp);
    temp = (uint32_t)(downlink >> 30);
    lua_pushinteger(L, temp);
    temp = (((uint32_t)downlink) & 0x3FFFFFFF);
    lua_pushinteger(L, temp);
    return 4;
}

/**
Network special configuration
@api mobile.config(item, value)
@int configuration project, see mobile.CONF_XXX
@int configuration value, determined according to the specific configured item
@return boolean whether successful
@usage
--There are different configurations for different platforms, use with caution. Currently, there are only EC618/EC718 series.

-- EC618 configures the cell reselection signal difference threshold, which cannot be greater than 15dbm and must be used in flight mode.
mobile.flymode(0,true)
mobile.config(mobile.CONF_RESELTOWEAKNCELL, 15)
mobile.config(mobile.CONF_STATICCONFIG, 1) --Enable network static optimization
mobile.flymode(0,false)

-- EC618 sets the statistics of SIM write times
-- Turn off statistics
mobile.config(mobile.CONF_SIM_WC_MODE, 0)
-- Turn on statistics, which is also turned on by default.
mobile.config(mobile.CONF_SIM_WC_MODE, 1)
-- Reading statistical values, asynchronous, needs to be obtained through the system message SIM_IND
sys.subscribe("SIM_IND", function(stats, value)
    log.info("SIM_IND", stats)
    if stats == "SIM_WC" then
        log.info("sim", "write counter", value)
    end
end)
mobile.config(mobile.CONF_SIM_WC_MODE, 2)
--Clear statistics
mobile.config(mobile.CONF_SIM_WC_MODE, 3)*/
static int l_mobile_config(lua_State* L) {
    uint8_t item = luaL_optinteger(L, 1, 0);
    uint32_t value = luaL_optinteger(L, 2, 0);
    if (!item)
    {
    	lua_pushboolean(L, 0);
    }
    else
    {
    	lua_pushboolean(L, !luat_mobile_config(item, value));
    }
    return 1;
}

#include "luat_uart.h"
#include "luat_zbuff.h"

/**
Get currently used/supported bands
@api mobile.getBand(band, is_default)
@zbuff output band
@boolean true is supported by default, false is currently supported, the default is false, it is currently a reserved function, do not write true
@return boolean Returns true if successful, false if failed
@usage
local buff = zbuff.create(40)
mobile.getBand(buff) --Output the currently used band, the band number is placed in buff, buff[0], buff[1], buff[2] .. buff[buff:used() - 1]*/
static int l_mobile_get_band(lua_State* L) {
    luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 1, LUAT_ZBUFF_TYPE));
    uint8_t total_num;
    int re;
    if (buff->len < 40)
    {
    	__zbuff_resize(buff, 40);
    }
    if (lua_isboolean(L, 2) && lua_toboolean(L, 2))
    {
    	re = luat_mobile_get_support_band(buff->addr,  &total_num);
    }
    else
    {
    	re = luat_mobile_get_band(buff->addr,  &total_num);
    }
    buff->used = total_num;
    lua_pushboolean(L, !re);
    return 1;
}

/**
Set the band used
@api mobile.setBand(band, num)
@zbuff Enter the band used
@int band number
@return boolean Returns true if successful, false if failed
@usage
local buff = zbuff.create(40)
buff[0] = 3
buff[1] = 5
buff[2] = 8
buff[3] = 40
mobile.setBand(buff, 4) --Sets a total of 4 bands to be used, 3, 5, 8, 40*/
static int l_mobile_set_band(lua_State* L) {
	luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 1, LUAT_ZBUFF_TYPE));
	int num = luaL_optinteger(L, 2, 1);
	lua_pushboolean(L, !luat_mobile_set_band(buff->addr,  num));
	return 1;
}

/**
RF test switches and configurations
@api mobile.nstOnOff(onoff, uart_id)
@boolean true turns on test mode, false turns it off
@int serial port number
@return nil no return value
@usage
mobile.nstOnOff(true, uart.VUART_0) --turn on the test mode and send the results using the virtual serial port
mobile.nstOnOff(false) --Turn off test mode*/
static int l_mobile_nst_test_onoff(lua_State* L) {
    luat_mobile_rf_test_mode(luaL_optinteger(L, 2, LUAT_VUART_ID_0), lua_toboolean(L, 1));
    return 0;
}
/**
RF test data input
@api mobile.nstInput(data)
@string or zbuff The data the user obtains from the serial port. Note that after obtaining all the data, you need to pass another nil to end the transmission.
@return nil no return value
@usage
mobile.nstInput(uart_data)
mobile.nstInput(nil)*/
static int l_mobile_nst_data_input(lua_State* L) {
    size_t len = 0;
    const char *buf = NULL;
    if(lua_isuserdata(L, 1))
    {
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 1, LUAT_ZBUFF_TYPE));
        len = buff->used;
        buf = buff->addr;
    }
    else if (lua_isstring(L, 1))
    {
        buf = lua_tolstring(L, 1, &len);//Get string data
    }
	luat_mobile_rf_test_input(buf, len);
    return 0;
}

/**
Initialize the built-in default virtual card function, which will be enabled on August 13, 2024 and requires firmware support.
@api mobile.vsimInit()
@return nil no return value
@usage
mobile.vsimInit()*/
static int l_mobile_init_vsim(lua_State* L) {
	luat_mobile_softsim_init_default();
    return 0;
}

/**
Switching between the built-in virtual card and the external physical card will be enabled on August 13, 2024. The virtual card requires firmware support, otherwise there will be no network after switching, and you need to switch in airplane mode, or restart the protocol stack after switching.
@api mobile.vsimOnOff(enable)
@bool on, true on, false off
@return nil no return value
@usage
mobile.vsimOnOff(true) --use built-in virtual card
mobile.vsimOnOff(false) --use external physical card*/
static int l_mobile_vsim_onoff(lua_State* L) {
    if (lua_isboolean(L, 1)) {
    	luat_mobile_softsim_onoff(lua_toboolean(L, 1));
    }
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_mobile[] = {
    {"status",          ROREG_FUNC(l_mobile_status)},
    {"imei",            ROREG_FUNC(l_mobile_imei)},
    {"imsi",            ROREG_FUNC(l_mobile_imsi)},
    {"sn",              ROREG_FUNC(l_mobile_sn)},
    {"iccid",           ROREG_FUNC(l_mobile_iccid)},
	{"number",          ROREG_FUNC(l_mobile_number)},
    {"muid",            ROREG_FUNC(l_mobile_muid)},
    {"apn",             ROREG_FUNC(l_mobile_apn)},
	{"ipv6",            ROREG_FUNC(l_mobile_ipv6)},
    {"csq",             ROREG_FUNC(l_mobile_csq)},
    {"rssi",            ROREG_FUNC(l_mobile_rssi)},
    {"rsrq",            ROREG_FUNC(l_mobile_rsrq)},
    {"rsrp",            ROREG_FUNC(l_mobile_rsrp)},
    {"snr",             ROREG_FUNC(l_mobile_snr)},
    {"eci",             ROREG_FUNC(l_mobile_eci)},
    {"tac",             ROREG_FUNC(l_mobile_tac)},
    {"enbid",           ROREG_FUNC(l_mobile_enbid)},
    {"flymode",         ROREG_FUNC(l_mobile_flymode)},
    {"simid",           ROREG_FUNC(l_mobile_simid)},
	{"simPin",          ROREG_FUNC(l_mobile_sim_pin)},
	{"rtime",           ROREG_FUNC(l_mobile_set_rrc_auto_release_time)},
	{"setAuto",         ROREG_FUNC(l_mobile_set_auto_work)},
    {"getCellInfo",     ROREG_FUNC(l_mobile_get_cell_info)},
    {"reqCellInfo",     ROREG_FUNC(l_mobile_request_cell_info)},
	{"reset",           ROREG_FUNC(l_mobile_reset)},
	{"dataTraffic",     ROREG_FUNC(l_mobile_data_traffic)},
	{"config",          ROREG_FUNC(l_mobile_config)},
	{"getBand",          ROREG_FUNC(l_mobile_get_band)},
	{"setBand",          ROREG_FUNC(l_mobile_set_band)},
	{"nstOnOff",          ROREG_FUNC(l_mobile_nst_test_onoff)},
	{"nstInput",          ROREG_FUNC(l_mobile_nst_data_input)},
	{"syncTime",          ROREG_FUNC(l_mobile_sync_time)},
	{"vsimInit",          ROREG_FUNC(l_mobile_init_vsim)},
	{"vsimOnOff",          ROREG_FUNC(l_mobile_vsim_onoff)},
	//@const UNREGISTER number not registered
    {"UNREGISTER",                  ROREG_INT(LUAT_MOBILE_STATUS_UNREGISTER)},
    //@const REGISTERED number registered
    {"REGISTERED",                  ROREG_INT(LUAT_MOBILE_STATUS_REGISTERED)},
	//@const SEARCH number is searching
	{"SEARCH",                      ROREG_INT(LUAT_MOBILE_STATUS_SEARCHING)},
	//@const DENIED number Registration was rejected
    {"DENIED",                      ROREG_INT(LUAT_MOBILE_STATUS_DENIED)},
    //@const UNKNOW number unknown
    {"UNKNOW",                      ROREG_INT(LUAT_MOBILE_STATUS_UNKNOW)},
    //@const REGISTERED_ROAMING number registered, roaming
    {"REGISTERED_ROAMING",          ROREG_INT(LUAT_MOBILE_STATUS_REGISTERED_ROAMING)},
    //@const SMS_ONLY_REGISTERED number registered, SMS only
    {"SMS_ONLY_REGISTERED",         ROREG_INT(LUAT_MOBILE_STATUS_SMS_ONLY_REGISTERED)},
    //@const SMS_ONLY_REGISTERED_ROAMING number registered, roaming, SMS only
    {"SMS_ONLY_REGISTERED_ROAMING", ROREG_INT(LUAT_MOBILE_STATUS_SMS_ONLY_REGISTERED_ROAMING)},
    //@const EMERGENCY_REGISTERED number registered, emergency service
    {"EMERGENCY_REGISTERED",        ROREG_INT(LUAT_MOBILE_STATUS_EMERGENCY_REGISTERED)},
    //@const CSFB_NOT_PREFERRED_REGISTERED number registered, non-main service
    {"CSFB_NOT_PREFERRED_REGISTERED",  ROREG_INT(LUAT_MOBILE_STATUS_CSFB_NOT_PREFERRED_REGISTERED)},
    //@const CSFB_NOT_PREFERRED_REGISTERED_ROAMING number registered, non-main service, roaming
    {"CSFB_NOT_PREFERRED_REGISTERED_ROAMING",  ROREG_INT(LUAT_MOBILE_STATUS_CSFB_NOT_PREFERRED_REGISTERED_ROAMING)},
	//@const CONF_RESELTOWEAKNCELL number Cell reselection signal difference threshold, requires flight mode setting
	{"CONF_RESELTOWEAKNCELL",   ROREG_INT(MOBILE_CONF_RESELTOWEAKNCELL)},
	//@const CONF_STATICCONFIG number Network static mode optimization, requires flight mode setting
	{"CONF_STATICCONFIG",       ROREG_INT(MOBILE_CONF_STATICCONFIG)},
	//@const CONF_QUALITYFIRST number Network switching prioritizes signal quality and requires flight mode setting. 0 is not enabled, 1 is enabled, 2 is enabled and accelerates switching, and power consumption will increase.
	{"CONF_QUALITYFIRST",       ROREG_INT(MOBILE_CONF_QUALITYFIRST)},
	//@const CONF_USERDRXCYCLE number LTE jump paging, requires flight mode setting, use with caution, 0 is not set, 1~7 increases or decreases the DrxCycle cycle multiple, 1:1/8 times, 2:1/4 times, 3:1 /2 times 4: 2 times 5: 4 times 6: 8 times 7: 16 times, 8~12 configures a fixed DrxCycle period. This configuration will only take effect when the period is greater than the DrxCycle period allocated by the network, 8: 320ms 9 :640ms 10:1280ms 11:2560ms 12:5120ms
	{"CONF_USERDRXCYCLE",       ROREG_INT(MOBILE_CONF_USERDRXCYCLE)},
	//@const CONF_T3324MAXVALUE number T3324 time in PSM mode, unit S
	{"CONF_T3324MAXVALUE",      ROREG_INT(MOBILE_CONF_T3324MAXVALUE)},
	//@const CONF_PSM_MODE number PSM mode switch, 0 off, 1 on
	{"CONF_PSM_MODE",           ROREG_INT(MOBILE_CONF_PSM_MODE)},
	//@const CONF_CE_MODE number attach mode, 0 is EPS ONLY and 2 is mixed. If you encounter IMSI detach problem, set it to 0. Note that the SMS function will be canceled when set to EPS ONLY.
	{"CONF_CE_MODE",            ROREG_INT(MOBILE_CONF_CE_MODE)},
    //@const CONF_SIM_WC_MODE number Configuration and reading of SIM write times
    {"CONF_SIM_WC_MODE",        ROREG_INT(MOBILE_CONF_SIM_WC_MODE)},
    //@const CONF_FAKE_CELL_BARTIME number The time when the pseudo base station is prohibited from accessing. If the value is 0, it will be canceled. 0xffff is permanent.
    {"CONF_FAKE_CELL_BARTIME",        ROREG_INT(MOBILE_CONF_FAKE_CELL_BARTIME)},
    //@const CONF_RESET_TO_FACTORY number Delete the saved protocol stack parameters, and the default configuration will be used after restarting
    {"CONF_RESET_TO_FACTORY",        ROREG_INT(MOBILE_CONF_RESET_TO_FACTORY)},
    //@const CONF_USB_ETHERNET number USB Ethernet card control of cellular network Modules, bit0 switch 1, on 0 off, bit1 mode 1NAT, 0 independent IP (currently forced to 1), bit2 protocol 1ECM, 0RNDIS, set in airplane mode
    {"CONF_USB_ETHERNET",        ROREG_INT(MOBILE_CONF_USB_ETHERNET)},
	//@const PIN_VERIFY number Verify PIN code operation
	{"PIN_VERIFY",              ROREG_INT(LUAT_SIM_PIN_VERIFY)},
	//@const PIN_CHANGE number Change PIN code operation
	{"PIN_CHANGE",              ROREG_INT(LUAT_SIM_PIN_CHANGE)},
	//@const PIN_ENABLE number enables PIN code verification
	{"PIN_ENABLE",              ROREG_INT(LUAT_SIM_PIN_ENABLE)},
	//@const PIN_DISABLE number Turn off PIN code verification
	{"PIN_DISABLE",             ROREG_INT(LUAT_SIM_PIN_DISABLE)},
	//@const PIN_UNBLOCK number unlock PIN code
	{"PIN_UNBLOCK",             ROREG_INT(LUAT_SIM_PIN_UNBLOCK)},
    {NULL,                      ROREG_INT(0)}
};

LUAMOD_API int luaopen_mobile( lua_State *L ) {
    luat_newlib2(L, reg_mobile);
    return 1;
}

static int l_mobile_event_handle(lua_State* L, void* ptr) {
    LUAT_MOBILE_EVENT_E event;
    uint8_t index;
    uint8_t status;
    int ret;


    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    event = msg->arg1;
    index = msg->arg2 >> 8;
    status = msg->arg2 & 0xFF;

	// luat_mobile_cell_info_t cell_info;
	// luat_mobile_signal_strength_info_t signal_info;
	// uint8_t csq, i;
	// char imsi[20];
	// char iccid[24] = {0};

    if (lua_getglobal(L, "sys_pub") != LUA_TFUNCTION) {
        return 0;
    };

	switch(event)
	{
	case LUAT_MOBILE_EVENT_CFUN:
		break;
	case LUAT_MOBILE_EVENT_SIM:
/*@sys_pub mobile
sim card status changes
SIM_IND
@usage
sys.subscribe("SIM_IND", function(status, value)
    --The values   of status are:
    -- RDY SIM card is ready, value is nil
    -- NORDY No SIM card, value is nil
    -- SIM_PIN requires entering PIN, value is nil
    -- GET_NUMBER gets the phone number (may not have a value), value is nil
    -- SIM_WC SIM card write count statistics, reset to 0 after power off, value is the statistical value
    log.info("sim status", status, value)
end)*/
        switch (status)
        {
        case LUAT_MOBILE_SIM_READY:
            lua_pushstring(L, "SIM_IND");
            lua_pushstring(L, "RDY");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_NO_SIM:
            lua_pushstring(L, "SIM_IND");
            lua_pushstring(L, "NORDY");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_SIM_NEED_PIN:
            lua_pushstring(L, "SIM_IND");
            lua_pushstring(L, "SIM_PIN");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_SIM_NUMBER:
            lua_pushstring(L, "SIM_IND");
            lua_pushstring(L, "GET_NUMBER");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_SIM_WC:
            lua_pushstring(L, "SIM_IND");
            lua_pushstring(L, "SIM_WC");
            uint32_t tmp = (uint32_t)ptr;
            lua_pushinteger(L, tmp);
            lua_call(L, 3, 0);
            break;
        default:
            break;
        }
		break;
	case LUAT_MOBILE_EVENT_REGISTER_STATUS:
		break;
	case LUAT_MOBILE_EVENT_CELL_INFO:
        switch (status)
        {
        case LUAT_MOBILE_CELL_INFO_UPDATE:
/*@sys_pub mobile
Base station data has been updated
CELL_INFO_UPDATE
@usage
-- Subscription
sys.subscribe("CELL_INFO_UPDATE", function()
    log.info("cell", json.encode(mobile.getCellInfo()))
end)*/
            lua_pushstring(L, "CELL_INFO_UPDATE");
            lua_call(L, 1, 0);
		    break;
        default:
            break;
        }
		break;
	case LUAT_MOBILE_EVENT_PDP:
		LLOGD("cid%d, state%d", index, status);
		break;
	case LUAT_MOBILE_EVENT_NETIF:
		switch (status)
		{
		case LUAT_MOBILE_NETIF_LINK_ON:
            LLOGD("NETIF_LINK_ON -> IP_READY");
/*@sys_pub mobile
Connected to the Internet
IP_READY
@usage
-- This message will be sent once after connecting to the Internet
sys.subscribe("IP_READY", function(ip, adapter)
    log.info("mobile", "IP_READY", ip, (adapter or -1) == socket.LWIP_GP)
end)*/
            lua_pushstring(L, "IP_READY");
            luat_ip_addr_t local_ip, net_mask, gate_way, ipv6;
            #ifdef LUAT_USE_LWIP
	        ipv6.type = 0xff;
	        int ret = network_get_full_local_ip_info(NULL, NW_ADAPTER_INDEX_LWIP_GPRS, &local_ip, &net_mask, &gate_way, &ipv6);
            #else
	        void* userdata = NULL;
	        network_adapter_info* info = network_adapter_fetch(NW_ADAPTER_INDEX_LWIP_GPRS, &userdata);
	        if (info == NULL)
		        ret = -1;
            else
                ret = info->get_local_ip_info(&local_ip, &net_mask, &gate_way, userdata);
            #endif
            if (ret == 0) {
                #ifdef LUAT_USE_LWIP
		        lua_pushfstring(L, "%s", ipaddr_ntoa(&local_ip));
                #else
                lua_pushfstring(L, "%d.%d.%d.%d", (local_ip.ipv4 >> 24) & 0xFF, (local_ip.ipv4 >> 16) & 0xFF, (local_ip.ipv4 >> 8) & 0xFF, (local_ip.ipv4 >> 0) & 0xFF);
                #endif
            }
            else {
                lua_pushliteral(L, "0.0.0.0");
            }
            lua_pushinteger(L, NW_ADAPTER_INDEX_LWIP_GPRS);
            lua_call(L, 3, 0);
			break;
        case LUAT_MOBILE_NETIF_LINK_OFF:
            LLOGD("NETIF_LINK_OFF -> IP_LOSE");
/*@sys_pub mobile
Disconnected
IP_LOSE
@usage
-- This message will be sent once after the network is disconnected
sys.subscribe("IP_LOSE", function(adapter)
    log.info("mobile", "IP_LOSE", (adapter or -1) == socket.LWIP_GP)
end)*/
            lua_pushstring(L, "IP_LOSE");
            lua_pushinteger(L, NW_ADAPTER_INDEX_LWIP_GPRS);
            lua_call(L, 2, 0);
            break;
		default:
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_TIME_SYNC:
/*@sys_pub mobile
Time has been synchronized
NTP_UPDATE
@usage
-- For telecommunications/mobile cards, after connecting to the Internet, the base station will issue the time, but China Unicom cards will not, so be sure to pay attention
sys.subscribe("NTP_UPDATE", function()
    log.info("mobile", "time", os.date())
end)*/
        LLOGD("TIME_SYNC %d", status);
        lua_pushstring(L, "NTP_UPDATE");
        lua_call(L, 1, 0);
		break;
	case LUAT_MOBILE_EVENT_CSCON:
//		LLOGD("CSCON %d", status);
		break;
	case LUAT_MOBILE_EVENT_BEARER:
		LLOGD("bearer act %d, result %d",status, index);
		break;
	case LUAT_MOBILE_EVENT_SMS:
		switch(status)
		{
		case LUAT_MOBILE_SMS_READY:
			LLOGI("sim%d sms ready", index);
			break;
		case LUAT_MOBILE_NEW_SMS:
			break;
		case LUAT_MOBILE_SMS_SEND_DONE:
			break;
		case LUAT_MOBILE_SMS_ACK:
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_IMS_REGISTER_STATUS:
        LLOGD("ims reg state %d", status);
		break;
    case LUAT_MOBILE_EVENT_CC:
        LLOGD("LUAT_MOBILE_EVENT_CC status %d",status);
/*@sys_pub mobile
Call status changes
CC_IND
@usage
sys.subscribe("CC_IND", function(status, value)
    log.info("cc status", status, value)
end)*/
        switch(status){
        case LUAT_MOBILE_CC_READY:
            LLOGD("LUAT_MOBILE_CC_READY");
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "READY");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_INCOMINGCALL:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "INCOMINGCALL");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_CALL_NUMBER:
            // lua_pushstring(L, "CC_IND");
            // lua_pushstring(L, "CALL_NUMBER");
            // lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_CONNECTED_NUMBER:
            // lua_pushstring(L, "CC_IND");
            // lua_pushstring(L, "CONNECTED_NUMBER");
            // lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_CONNECTED:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "CONNECTED");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_DISCONNECTED:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "DISCONNECTED");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_SPEECH_START:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "SPEECH_START");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_MAKE_CALL_OK:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "MAKE_CALL_OK");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_MAKE_CALL_FAILED:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "MAKE_CALL_FAILED");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_ANSWER_CALL_DONE:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "ANSWER_CALL_DONE");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_HANGUP_CALL_DONE:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "HANGUP_CALL_DONE");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_LIST_CALL_RESULT:
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "LIST_CALL_RESULT");
            lua_call(L, 2, 0);
            break;
        case LUAT_MOBILE_CC_PLAY:// first
            lua_pushstring(L, "CC_IND");
            lua_pushstring(L, "PLAY");
            lua_call(L, 2, 0);
            break;
        }
        break;
	default:
		break;
	}
    return 0;
}

void luat_mobile_event_cb(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status, void* ptr) {
#if defined LUAT_USE_VOLTE
		switch (event){
    case LUAT_MOBILE_EVENT_CC:
        switch(status){
        case LUAT_MOBILE_CC_SPEECH_START:
        	luat_cc_start_speech(index+1);
            break;
        case LUAT_MOBILE_CC_PLAY:
        	luat_cc_play_tone(index);
            break;
        }
        break;
    default:
        break;
	}
#endif
    rtos_msg_t msg = {
        .handler = l_mobile_event_handle,
        .arg1 = event,
        .arg2 = (index << 8) + status ,
        .ptr = ptr
    };
    luat_msgbus_put(&msg, 0);
}






