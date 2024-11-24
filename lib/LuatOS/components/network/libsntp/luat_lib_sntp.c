/*@Modules socket
@summary network interface
@version 1.0
@date 2022.11.13*/

#include "luat_base.h"

#include "luat_network_adapter.h"
#include "luat_rtos.h"
#include "luat_msgbus.h"
#include "luat_mcu.h"
#include "luat_mem.h"
#include "luat_rtc.h"

#include "luat_sntp.h"

#define LUAT_LOG_TAG "sntp"
#include "luat_log.h"

#define SNTP_SERVER_COUNT       3
#define SNTP_SERVER_LEN_MAX     32

#define NTP_UPDATE 1
#define NTP_ERROR  2
#define NTP_TIMEOUT 3

extern sntp_ctx_t g_sntp_ctx;
extern char* sntp_servers[];

int luat_ntp_on_result(network_ctrl_t *sntp_netc, int result);

int l_sntp_event_handle(lua_State* L, void* ptr) {
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    // Clean up the scene
    if (msg->arg1 == NTP_TIMEOUT) {
        luat_ntp_on_result(g_sntp_ctx.ctrl, NTP_ERROR);
        return 0;
    }
    ntp_cleanup();
    if (lua_getglobal(L, "sys_pub") != LUA_TFUNCTION) {
        return 0;
    };
    switch (msg->arg1)
    {
/*@sys_pub socket
Time has been synchronized
NTP_UPDATE
@usage
sys.subscribe("NTP_UPDATE", function()
    log.info("socket", "sntp", os.date())
end)*/
    case NTP_UPDATE:
        lua_pushstring(L, "NTP_UPDATE");
        break;
/*@sys_pub socket
Time synchronization failed
NTP_ERROR
@usage
sys.subscribe("NTP_ERROR", function()
    log.info("socket", "sntp error")
end)*/
    case NTP_ERROR:
        lua_pushstring(L, "NTP_ERROR");
        break;
    }
    lua_call(L, 1, 0);
    return 0;
}


/*sntp time synchronization
@api socket.sntp(sntp_server)
@tag LUAT_USE_SNTP
@string/table sntp server address optional
@int Adapter serial number, which can only be socket.ETH0 (external Ethernet), socket.LWIP_ETH (built-in Ethernet), socket.LWIP_STA (built-in WIFI STA), socket.LWIP_AP (built-in WIFI AP), socket.LWIP_GP (GPRS with built-in cellular network), socket.USB (external USB network card), if not filled in, give priority to the adapter that comes with the soc platform that can connect to the external network. If it is still not available, choose the last registered adapter.
@usage
socket.sntp()
--socket.sntp("ntp.aliyun.com") --Customize sntp server address
--socket.sntp({"ntp.aliyun.com","ntp1.aliyun.com","ntp2.aliyun.com"}) --sntp custom server address
--socket.sntp(nil, socket.ETH0) --sntp custom adapter serial number
sys.subscribe("NTP_UPDATE", function()
    log.info("sntp", "time", os.date())
end)
sys.subscribe("NTP_ERROR", function()
    log.info("socket", "sntp error")
    socket.sntp()
end)*/
int l_sntp_get(lua_State *L) {
    size_t len = 0;
	if (lua_isstring(L, 1)){
        const char * server_addr = luaL_checklstring(L, 1, &len);
        if (len < SNTP_SERVER_LEN_MAX - 1){
            memcpy(sntp_servers[0], server_addr, len + 1);
        }else{
            LLOGE("server_addr too long %s", server_addr);
        }
	}else if(lua_istable(L, 1)){
        size_t count = lua_rawlen(L, 1);
        if (count > SNTP_SERVER_COUNT){
            count = SNTP_SERVER_COUNT;
        }
		for (size_t i = 1; i <= count; i++){
			lua_geti(L, 1, i);
			const char * server_addr = luaL_checklstring(L, -1, &len);
            if (len < SNTP_SERVER_LEN_MAX - 1){
                memcpy(sntp_servers[i-1], server_addr, len + 1);
            }else{
                LLOGE("server_addr too long %s", server_addr);
            }
			lua_pop(L, 1);
		}
	}
    if (g_sntp_ctx.is_running) {
        LLOGI("sntp is running");
        return 0;
    }
    int adapter_index = luaL_optinteger(L, 2, network_get_last_register_adapter());
    int ret = ntp_get(adapter_index);
    if (ret) {
#ifdef __LUATOS__
        rtos_msg_t msg;
        msg.handler = l_sntp_event_handle;
        msg.arg1 = NTP_ERROR;
        luat_msgbus_put(&msg, 0);
#else
        ntp_cleanup();
#endif
    }
	return 0;
}

/*Timestamp after network time synchronization (ms level)
@api socket.ntptm()
@return table data containing time information
@usage
--This API was added on 2023.11.15
-- Note that this function is not valid until socket.sntp() is executed and the NTP time is obtained.
-- And the more accurate value is after 2 sntp times.
-- The smaller the network fluctuation, the more stable the timestamp is.
local tm = socket.ntptm()

--The corresponding table contains multiple data, all of which are integer values.

--Standard data
-- tsec current seconds, starting from 1900.1.1 0:0:0, UTC time
-- tms current milliseconds
-- whether vaild is valid, true or nil

-- Debugging data, used for debugging, general users do not need to worry about it
-- ndelay average network delay, in milliseconds
-- ssec The offset in seconds between the system startup time and 1900.1.1 0:0:0
-- sms millisecond offset between system startup time and 1900.1.1 0:0:0
-- lsec local seconds counter, based on mcu.tick64()
-- lms local millisecond counter, based on mcu.tick64()

log.info("tm data", json.encode(tm))
log.info("Timestamp", string.format("%u.%03d", tm.tsec, tm.tms))*/
int l_sntp_tm(lua_State *L) {
    lua_newtable(L);

    lua_pushinteger(L, g_sntp_ctx.network_delay_ms);
    lua_setfield(L, -2, "ndeley");
    lua_pushinteger(L, g_sntp_ctx.sysboot_diff_sec);
    lua_setfield(L, -2, "ssec");
    lua_pushinteger(L, g_sntp_ctx.sysboot_diff_ms);
    lua_setfield(L, -2, "sms");

    uint64_t tick64 = luat_mcu_tick64();
    uint32_t us_period = luat_mcu_us_period();
    uint64_t ll_sec = tick64 /us_period/ 1000 / 1000;
    uint64_t ll_ms  = (tick64 /us_period/ 1000) % 1000;
    uint64_t tmp = ll_sec + g_sntp_ctx.sysboot_diff_sec;
    tmp *= 1000;
    tmp += ll_ms + g_sntp_ctx.sysboot_diff_ms;
    uint64_t tsec = tmp / 1000;
    uint64_t tms = (tmp % 1000) & 0xFFFF;

    
    lua_pushinteger(L, tsec);
    lua_setfield(L, -2, "tsec");
    lua_pushinteger(L, tms);
    lua_setfield(L, -2, "tms");
    lua_pushinteger(L, ll_sec);
    lua_setfield(L, -2, "lsec");
    lua_pushinteger(L, ll_ms);
    lua_setfield(L, -2, "lms");

    if (g_sntp_ctx.sysboot_diff_sec > 0) {
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, "vaild");
    }

    return 1;
}

/*Set the port number of the SNTP server
@api socket.sntp_port(port)
@int port port number, default 123
@return int returns the current port number
@usage
-- This function was added on 2024.5.17
-- In most cases, there is no need to set the port number of the NTP server. The default is 123.*/
int l_sntp_port(lua_State *L) {
    if (lua_type(L, 1) == LUA_TNUMBER){
        uint16_t port = (uint16_t)luaL_checkinteger(L, 1);
        if (port > 0){
            g_sntp_ctx.port = port;
        }
    }
    lua_pushinteger(L, g_sntp_ctx.port ? g_sntp_ctx.port : 123);
    return 1;
}
