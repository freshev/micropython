/*@Modules socket
@summary network interface
@version 1.0
@date 2022.11.13
@demo socket
@tagLUAT_USE_NETWORK*/
#include "luat_base.h"
#include "luat_mem.h"
// #ifdef LUAT_USE_NETWORK
#include "luat_network_adapter.h"
#include "luat_rtos.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "socket"
#include "luat_log.h"
//static const char NW_TYPE[] = "NWA*";
#define LUAT_NW_CTRL_TYPE "NWCTRL*"
typedef struct
{
	network_ctrl_t *netc;
	int cb_ref;	//callback function
	char *task_name;
	uint8_t adapter_index;
}luat_socket_ctrl_t;

#define L_CTRL_CHECK 	do {if (!l_ctrl || !l_ctrl->netc){return 0;}}while(0)

network_adapter_info* network_adapter_fetch(int id, void** userdata);

/*Get local ip
@api socket.localIP(adapter)
@int Adapter serial number, which can only be socket.ETH0 (external Ethernet), socket.LWIP_ETH (built-in Ethernet), socket.LWIP_STA (built-in WIFI STA), socket.LWIP_AP (built-in WIFI AP), socket.LWIP_GP (GPRS with built-in cellular network), socket.USB (external USB network card), if not filled in, give priority to the adapter that comes with the soc platform that can connect to the external network. If it is still not available, choose the last registered adapter.
@return string is usually the internal network ip, or it may be the external network ip, depending on the allocation by the operator
@return string network mask
@return string gateway IP
@usage
sys.taskInit(function()
    while 1 do
        sys.wait(3000)
        log.info("socket", "ip", socket.localIP())
-- Output example
-- 62.39.244.10 255.255.255.255 0.0.0.0
    end
end)*/
static int l_socket_local_ip(lua_State *L)
{
	luat_ip_addr_t local_ip, net_mask, gate_way, ipv6;
	int adapter_index = luaL_optinteger(L, 1, network_get_last_register_adapter());
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY)
	{
		return 0;
	}
#ifdef LUAT_USE_LWIP
	network_set_ip_invaild(&ipv6);
	int ret = network_get_full_local_ip_info(NULL, adapter_index, &local_ip, &net_mask, &gate_way, &ipv6);
#else
	void* userdata = NULL;
	network_adapter_info* info = network_adapter_fetch(adapter_index, &userdata);
	if (info == NULL)
		return 0;

	int ret = info->get_local_ip_info(&local_ip, &net_mask, &gate_way, userdata);
#endif
	if (ret == 0) {
#ifdef LUAT_USE_LWIP
		lua_pushfstring(L, "%s", ipaddr_ntoa(&local_ip));
		lua_pushfstring(L, "%s", ipaddr_ntoa(&net_mask));
		lua_pushfstring(L, "%s", ipaddr_ntoa(&gate_way));
#if LWIP_IPV6
		if (IPADDR_TYPE_V6 == ipv6.type)
		{
			char *ipv6_string = ip6addr_ntoa(&ipv6.u_addr.ip6);
			lua_pushfstring(L, "%s", ipv6_string);
		}
		else
#endif
		{
			lua_pushnil(L);
		}
		return 4;
#else

		lua_pushfstring(L, "%d.%d.%d.%d", (local_ip.ipv4 >> 0) & 0xFF, (local_ip.ipv4 >> 8) & 0xFF, (local_ip.ipv4 >> 16) & 0xFF, (local_ip.ipv4 >> 24) & 0xFF);
		lua_pushfstring(L, "%d.%d.%d.%d", (net_mask.ipv4 >> 0) & 0xFF, (net_mask.ipv4 >> 8) & 0xFF, (net_mask.ipv4 >> 16) & 0xFF, (net_mask.ipv4 >> 24) & 0xFF);
		lua_pushfstring(L, "%d.%d.%d.%d", (gate_way.ipv4 >> 0) & 0xFF, (gate_way.ipv4 >> 8) & 0xFF, (gate_way.ipv4 >> 16) & 0xFF, (gate_way.ipv4 >> 24) & 0xFF);
		return 3;
#endif
	}
	return 0;
}


static int32_t l_socket_callback(lua_State *L, void* ptr)
{
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_socket_ctrl_t *l_ctrl =(luat_socket_ctrl_t *)msg->ptr;
    if (l_ctrl->netc)
    {
    	if (l_ctrl->cb_ref)
    	{
            lua_geti(L, LUA_REGISTRYINDEX, l_ctrl->cb_ref);
            if (lua_isfunction(L, -1)) {
            	lua_pushlightuserdata(L, l_ctrl);
            	lua_pushinteger(L, msg->arg1);
            	lua_pushinteger(L, msg->arg2);
                lua_call(L, 3, 0);
            }
    	}
    	else if (l_ctrl->task_name)
    	{
    	    lua_getglobal(L, "sys_send");
    	    if (lua_isfunction(L, -1)) {
    	        lua_pushstring(L, l_ctrl->task_name);
    	        lua_pushinteger(L, msg->arg1);
    	        lua_pushinteger(L, msg->arg2);
    	        lua_call(L, 3, 0);
    	    }
    	}
    	else
    	{
    	    lua_getglobal(L, "sys_pub");
    	    if (lua_isfunction(L, -1)) {
    	        lua_pushstring(L, LUAT_NW_CTRL_TYPE);
    	        lua_pushinteger(L, l_ctrl->netc->adapter_index);
    	        lua_pushinteger(L, l_ctrl->netc->socket_id);
    	        lua_pushinteger(L, msg->arg1);
    	        lua_pushinteger(L, msg->arg2);
    	        lua_call(L, 5, 0);
    	    }
    	}
    }
    lua_pushinteger(L, 0);
    return 1;
}

static int32_t luat_lib_socket_callback(void *data, void *param)
{
	OS_EVENT *event = (OS_EVENT *)data;
	rtos_msg_t msg;
    msg.handler = l_socket_callback;
    msg.ptr = param;
    msg.arg1 = event->ID & 0x0fffffff;
    msg.arg2 = event->Param1;
    luat_msgbus_put(&msg, 0);
    return 0;
}

static luat_socket_ctrl_t * l_get_ctrl(lua_State *L, int index)
{
	if (luaL_testudata(L, 1, LUAT_NW_CTRL_TYPE))
	{
		return ((luat_socket_ctrl_t *)luaL_checkudata(L, 1, LUAT_NW_CTRL_TYPE));
	}
	else
	{
		return ((luat_socket_ctrl_t *)lua_touserdata(L, 1));
	}
}

// __gc
static int l_socket_gc(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	if (!l_ctrl){return 0;}
    if (l_ctrl->netc)
    {
    	network_force_close_socket(l_ctrl->netc);
    	network_release_ctrl(l_ctrl->netc);
    	l_ctrl->netc = NULL;
    }
    if (l_ctrl->cb_ref)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, l_ctrl->cb_ref);
        l_ctrl->cb_ref = 0;
    }
    if (l_ctrl->task_name)
    {
    	luat_heap_free(l_ctrl->task_name);
    	l_ctrl->task_name = 0;
    }
    return 0;
}

/*Apply for a socket_ctrl on an adapted network card
@api socket.create(adapter, cb)
@int Adapter serial number, which can only be socket.ETH0 (external Ethernet), socket.LWIP_ETH (built-in Ethernet), socket.LWIP_STA (built-in WIFI STA), socket.LWIP_AP (built-in WIFI AP), socket.LWIP_GP (GPRS with built-in cellular network), socket.USB (external USB network card), if not filled in, give priority to the adapter that comes with the soc platform that can connect to the external network. If it is still not available, choose the last registered adapter.
@string or function string is the taskName of the message notification, function is the callback function, if the firmware does not have built-in sys_wait, it must be function
When calling back a message through the callback function, a total of 3 parameters are input to the function:
param1 is the applied network_ctrl
param2 is a specific message, which can only be socket.RESET, socket.LINK, socket.ON_LINE, socket.TX_OK, socket.RX_NEW, socket.CLOSED, etc.
param3 is the parameter corresponding to the message
@return userdata Successfully returns network_ctrl, failure returns nil
@usage
--Apply a network_ctrl on the Ethernet network card and call back relevant messages through socket_cb_fun
local netc = socket.create(socket.ETH0, socket_cb_fun)
--Apply a network_ctrl on the Ethernet network card, and notify the taskName of "IOT_TASK" callback related messages through sendMsg.
local netc = socket.create(socket.ETH0, "IOT_TASK")

--Create a network_ctrl on the default network adapter
local netc = socket.create(nil, "MySocket")*/
static int l_socket_create(lua_State *L)
{
	int adapter_index = luaL_optinteger(L, 1, network_get_last_register_adapter());
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY)
	{
		lua_pushnil(L);
		return 1;
	}

	luat_socket_ctrl_t *l_ctrl = (luat_socket_ctrl_t *)lua_newuserdata(L, sizeof(luat_socket_ctrl_t));
	if (!l_ctrl)
	{
		lua_pushnil(L);
		return 1;
	}
	l_ctrl->adapter_index = adapter_index;
	l_ctrl->netc = network_alloc_ctrl(adapter_index);
	if (!l_ctrl->netc)
	{
		LLOGD("create fail");
		lua_pushnil(L);
		return 1;
	}
	network_init_ctrl(l_ctrl->netc, NULL, luat_lib_socket_callback, l_ctrl);
	if (lua_isfunction(L, 2))
	{
        lua_pushvalue(L, 2);
        l_ctrl->cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        l_ctrl->task_name = NULL;
	}
	else if (lua_isstring(L, 2))
	{
		l_ctrl->cb_ref = 0;
	    size_t len;
	    const char *buf;
        buf = lua_tolstring(L, 2, &len);//Get string data
		l_ctrl->task_name = luat_heap_malloc(len + 1);
		memset(l_ctrl->task_name, 0, len + 1);
		memcpy(l_ctrl->task_name, buf, len);
	}
	luaL_setmetatable(L, LUAT_NW_CTRL_TYPE);
	return 1;
}

/*Configure whether to turn on debug information
@api socket.debug(ctrl, onoff)
ctrl obtained by @user_data socket.create
@boolean true turns on debug switch
@return nil no return value
@usage
-- Turn on debugging information, the default is off
socket.debug(ctrl, true)*/
static int l_socket_set_debug(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	if (lua_isboolean(L, 2))
	{
		l_ctrl->netc->is_debug = lua_toboolean(L, 2);
	}
	return 0;
}

/*Configure some network information,
@api socket.config(ctrl, local_port, is_udp, is_tls, keep_idle, keep_interval, keep_cnt, server_cert, client_cert, client_key, client_password)
ctrl obtained by @user_data socket.create
@int Local port number, little endian format. If not written, one will be automatically assigned. If the user fills in the port number, it needs to be less than 60000. It is not written by default.
@boolean Whether it is UDP, default false
@boolean Whether the transmission is encrypted, default false
@int The idle time (seconds) in tcp keep live mode. If left blank, it means it is not enabled. If it is a network card that does not support the standard posix interface (such as W5500), it is the heartbeat interval.
@int tcp keep live mode detection interval (seconds)
@int The number of detections in tcp keep live mode
@string Server ca certificate data in TCP mode, PSK in UDP mode, do not need to be encrypted for transmission, write nil, and all subsequent parameters are also nil
@string Client ca certificate data in TCP mode, PSK-ID in UDP mode, ignore if client certificate verification is not required in TCP mode, generally no client certificate verification is required
@string Client private key encrypted data in TCP mode
@string Client private key password data in TCP mode
@return boolean returns true if successful, false if failed
@usage
--The most common TCP transmission
socket.config(ctrl)
--The most common encrypted TCP transmission, the kind that does not require certificate verification
socket.config(ctrl, nil, nil,true)*/
static int l_socket_config(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	uint8_t is_udp = 0;
	uint8_t is_tls = 0;
	int param_pos = 1;
	uint32_t keep_idle, keep_interval, keep_cnt;
	const char *server_cert = NULL;
	const char *client_cert = NULL;
	const char *client_key = NULL;
	const char *client_password = NULL;
	size_t server_cert_len, client_cert_len, client_key_len, client_password_len;

	uint16_t local_port = luaL_optinteger(L, ++param_pos, 0);
	if (lua_isboolean(L, ++param_pos))
	{
		is_udp = lua_toboolean(L, param_pos);
	}
	if (lua_isboolean(L, ++param_pos))
	{
		is_tls = lua_toboolean(L, param_pos);
	}
#ifndef LUAT_USE_TLS
	if (is_tls){
		LLOGE("NOT SUPPORT TLS");
		lua_pushboolean(L, 0);
		return 1;
	}
#endif
	keep_idle = luaL_optinteger(L, ++param_pos, 0);
	keep_interval = luaL_optinteger(L, ++param_pos, 0);
	keep_cnt = luaL_optinteger(L, ++param_pos, 0);
	if (lua_isstring(L, ++param_pos))
	{
		server_cert_len = 0;
		server_cert = luaL_checklstring(L, param_pos, &server_cert_len);
	}
	if (lua_isstring(L, ++param_pos))
	{
		client_cert_len = 0;
		client_cert = luaL_checklstring(L, param_pos, &client_cert_len);
	}

	if (lua_isstring(L, ++param_pos))
	{
		client_key_len = 0;
		client_key = luaL_checklstring(L, param_pos, &client_key_len);
	}
	if (lua_isstring(L, ++param_pos))
	{
		client_password_len = 0;
		client_password = luaL_checklstring(L, param_pos, &client_password_len);
	}
	network_set_base_mode(l_ctrl->netc, !is_udp, 10000, keep_idle, keep_idle, keep_interval, keep_cnt);
	network_set_local_port(l_ctrl->netc, local_port);
	if (is_tls)
	{
		network_init_tls(l_ctrl->netc, (server_cert || client_cert)?2:0);
		if (is_udp)
		{
			network_set_psk_info(l_ctrl->netc, (const unsigned char *)server_cert, server_cert_len, (const unsigned char *)client_key, client_key_len);
		}
		else
		{
			if (server_cert)
			{
				network_set_server_cert(l_ctrl->netc, (const unsigned char *)server_cert, server_cert_len + 1);
			}
			if (client_cert)
			{
				network_set_client_cert(l_ctrl->netc, (const unsigned char *)client_cert, client_cert_len + 1,
						(const unsigned char *)client_key, client_key_len + 1,
						(const unsigned char *)client_password, client_password_len + 1);
			}
		}
	}
	else
	{
		network_deinit_tls(l_ctrl->netc);
	}
	lua_pushboolean(L, 1);
	return 1;
}

/*Waiting for network card linkup
@api socket.linkup(ctrl)
ctrl obtained by @user_data socket.create
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value
@return boolean true has been linked up, false has not been linked up, and then needs to receive socket.LINK messages
@usage
-- Check whether you are connected to the Internet
local succ, result = socket.linkup(ctrl)*/
static int l_socket_linkup(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	int result = network_wait_link_up(l_ctrl->netc, 0);
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, result == 0);
	return 2;


}

/*Connect to server as client
@api socket.connect(ctrl, ip, remote_port, need_ipv6_dns)
ctrl obtained by @user_data socket.create
@string or int ip or domain name, if it is IPV4, it can be an int value in big endian format
@int server port number, little endian format
@boolean Whether domain name resolution requires IPV6, true yes, false no, default false no, only the protocol stack that supports IPV6 will be effective
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value, if there is an exception, follow up with close
@return boolean true has been connected, false has not been connected, and then you need to receive socket.ON_LINE messages
@usage

local succ, result = socket.connect(ctrl, "netlab.luatos.com", 40123)

--[[
Common code values   for connection failures will be displayed in the log.
-1 Insufficient underlying memory
-3 timeout
-8 port is already occupied
-11 link not established
-13 Module actively disconnects
-14 The server actively disconnects
]]*/
static int l_socket_connect(lua_State *L)
{
#ifdef LUAT_USE_LWIP
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	luat_ip_addr_t ip_addr;
	const char *ip = NULL;
	size_t ip_len;
	network_set_ip_invaild(&ip_addr);
	if (lua_isinteger(L, 2))
	{
		network_set_ip_ipv4(&ip_addr, lua_tointeger(L, 2));
		ip = NULL;
		ip_len = 0;
	}
	else
	{
		ip_len = 0;
	    ip = luaL_checklstring(L, 2, &ip_len);
	}
	uint16_t remote_port = luaL_checkinteger(L, 3);
	LLOGD("connect to %s,%d", ip, remote_port);
	if (!network_ip_is_vaild_ipv4(&ip_addr))
	{
		if (LUA_TBOOLEAN == lua_type(L, 4))
		{
			network_connect_ipv6_domain(l_ctrl->netc, lua_toboolean(L, 4));
		}
		else
		{
			network_connect_ipv6_domain(l_ctrl->netc, 0);
		}
	}
	int result = network_connect(l_ctrl->netc, ip, ip_len, (!network_ip_is_vaild_ipv4(&ip_addr))?NULL:&ip_addr, remote_port, 0);
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, result == 0);
	return 2;
#else
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	luat_ip_addr_t ip_addr;
	const char *ip = NULL;
	size_t ip_len;
	ip_addr.is_ipv6 = 0xff;
	if (lua_isinteger(L, 2))
	{
		ip_addr.is_ipv6 = 0;
		ip_addr.ipv4 = lua_tointeger(L, 2);
		ip = NULL;
		ip_len = 0;
	}
	else
	{
		ip_len = 0;
	    ip = luaL_checklstring(L, 2, &ip_len);
	}
	uint16_t remote_port = luaL_checkinteger(L, 3);
	LLOGD("connect to %s,%d", ip, remote_port);
	int result = network_connect(l_ctrl->netc, ip, ip_len, ip_addr.is_ipv6?NULL:&ip_addr, remote_port, 0);
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, result == 0);
	return 2;
#endif
}

/*Disconnect as client
@api socket.discon(ctrl)
ctrl obtained by @user_data socket.create
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value
@return boolean true has been disconnected, false has not been disconnected, and then the socket.CLOSED message needs to be received.
@usage
local succ, result = socket.discon(ctrl)*/
static int l_socket_disconnect(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	int result = network_close(l_ctrl->netc, 0);
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, result == 0);
	return 2;
}

/*Force close socket
@api socket.close(ctrl)
ctrl obtained by @user_data socket.create
@return nil no return value*/
static int l_socket_close(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	network_force_close_socket(l_ctrl->netc);
	return 0;
}

/*When sending data to the peer, a single UDP transmission should not exceed 1460 bytes, otherwise it will easily fail.
@api socket.tx(ctrl, data, ip, port, flag)
ctrl obtained by @user_data socket.create
@string or user_data zbuff The data to be sent
@string or int Peer IP. If it is a TCP application, ignore it. If it is UDP, if it is left blank, use the parameters when connecting. If it is IPV4, it can be an int value in big endian format.
@int Peer port number, little endian format. If it is a TCP application, it will be ignored. If it is a UDP application, if it is left blank, the parameters at the time of connect will be used.
@int sends parameters, currently reserved, does not work
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value, if false, follow up with close
@return boolean true the buffer is full, false not full. If true, you need to wait for a while or wait until the socket.TX_OK message before trying to send, while ignoring the next return value
@return boolean true has received a response, false has not received a response, and then needs to receive the socket.TX_OK message, or you can ignore it and continue sending until full==true
@usage

local succ, full, result = socket.tx(ctrl, "123456", "xxx.xxx.xxx.xxx", xxxx)*/
static int l_socket_tx(lua_State *L)
{
#ifdef LUAT_USE_LWIP
	char ip_buf[68] = {0};
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	luat_ip_addr_t ip_addr = {0};
	luat_zbuff_t *buff = NULL;
	const char *ip = NULL;
	const char *data = NULL;
	size_t ip_len = 0, data_len = 0;
	network_set_ip_invaild(&ip_addr);
	if (lua_isstring(L, 2))
	{
		data_len = 0;
		data = luaL_checklstring(L, 2, &data_len);
	}
	else
	{
		buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
		data = (const char*)buff->addr;
		data_len = buff->used;
	}
	if (lua_isinteger(L, 3))
	{
		network_set_ip_ipv4(&ip_addr, lua_tointeger(L, 3));
	}
	else if (lua_isstring(L, 3))
	{
		ip_len = 0;
	    ip = luaL_checklstring(L, 3, &ip_len);
	    memcpy(ip_buf, ip, ip_len);
	    ip_buf[ip_len] = 0;
	    ipaddr_aton(ip_buf, &ip_addr);

	}
	uint32_t tx_len;
	int result = network_tx(l_ctrl->netc, (const uint8_t *)data, data_len, luaL_optinteger(L, 5, 0), network_ip_is_vaild(&ip_addr)?&ip_addr:NULL, luaL_optinteger(L, 4, 0), &tx_len, 0);
#else
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	luat_ip_addr_t ip_addr = {0};
	luat_zbuff_t *buff = NULL;
	const char *ip = NULL;
	const char *data = NULL;
	size_t ip_len = 0, data_len = 0;
	ip_addr.is_ipv6 = 0xff;
	if (lua_isstring(L, 2))
	{
		data_len = 0;
		data = luaL_checklstring(L, 2, &data_len);
	}
	else
	{
		buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
		data = (const char *)buff->addr;
		data_len = buff->used;
	}
	if (lua_isinteger(L, 3))
	{
		ip_addr.is_ipv6 = 0;
		ip_addr.ipv4 = lua_tointeger(L, 3);
	}
	else if (lua_isstring(L, 3))
	{
		ip_len = 0;
	    ip = luaL_checklstring(L, 3, &ip_len);

		if (network_string_is_ipv4(ip, ip_len))
		{
			ip_addr.is_ipv6 = 0;
			ip_addr.ipv4 = network_string_to_ipv4(ip, ip_len);
		}
		else
		{
			char *name = luat_heap_malloc(ip_len + 1);
			memcpy(name, ip, ip_len);
			name[ip_len] = 0;
			network_string_to_ipv6(name, &ip_addr);
			free(name);
		}
	}
	uint32_t tx_len;
	int result = network_tx(l_ctrl->netc, (const uint8_t *)data, data_len, luaL_optinteger(L, 5, 0), (ip_addr.is_ipv6 != 0xff)?&ip_addr:NULL, luaL_optinteger(L, 4, 0), &tx_len, 0);
#endif
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, tx_len != data_len);
	lua_pushboolean(L, result == 0);
	return 3;
}

/*Receive the data sent by the peer. Note that the data has been cached at the bottom layer. Using this function only extracts it. In UDP mode, only one data packet will be extracted at a time.
@api socket.rx(ctrl, buff, flag, limit)
ctrl obtained by @user_data socket.create
@user_data zbuff stores the received data. If the buffer is not enough, it will automatically expand.
@int receives parameters, currently reserved, has no effect
@int Receive data length limit, if specified, only the first N bytes will be luat. 2024.1.5 New
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value, if false, follow up with close
@return int The length of data received this time
@return string Peer IP, only meaningful in UDP mode. Return nil in TCP mode. Pay attention to the returned format. If it is IPV4, 1byte 0x00 + 4byte address. If it is IPV6, 1byte 0x01 + 16byte address.
@return int Peer port, only meaningful in UDP mode, returns 0 in TCP mode
@usage
-- Read data from socket, ctrl is returned by socket.create, please check demo/socket
local buff = zbuff.create(2048)
local succ, data_len, remote_ip, remote_port = socket.rx(ctrl, buff)

-- Limit read length, added in 2024.1.5
-- Notice
-- If it is UDP data, if the limit is less than the UDP packet length, only the first limit bytes will be luat, and the remaining data will be discarded.
-- If it is TCP data, if there is remaining data, it will not be discarded and can continue to be read.
-- There will be new EVENT data only when new data arrives. Unfinished data will not trigger a new EVENT event.
local succ, data_len, remote_ip, remote_port = socket.rx(ctrl, buff, 1500)

-- Read buffer size, new in 2024.1.5. Note that if the old version of firmware does not pass the buff parameter, an error will be reported.
-- For TCP data, what is returned here is the total length of the data to be read.
-- For UDP data, what is returned here is the length of a single UDP data packet
local succ, data_len = socket.rx(ctrl)
if succ then
log.info("Length of data to be received", data_len)
end*/
static int l_socket_rx(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	luat_zbuff_t *buff = NULL;
	luat_ip_addr_t ip_addr = {0};
	uint8_t ip[17] = {0};
	uint16_t port = 0;
	// uint8_t new_flag = 0;
	uint32_t rx_len = 0;
	uint32_t total_len = 0;
	int result = network_rx(l_ctrl->netc, NULL, 0, 0, NULL, NULL, &total_len);
	if (result < 0)
	{
		lua_pushboolean(L, 0);
		lua_pushinteger(L, 0);
		return 2;
	}
	if (!total_len)
	{
		lua_pushboolean(L, 1);
		lua_pushinteger(L, 0);
		return 2;
	}
	// Is the zbuff passed in? If not, return the value length.
	if (lua_isuserdata(L, 2)) {
		buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
	}
	else {
		//return length only
		lua_pushboolean(L, 1);
		lua_pushinteger(L, total_len);
		return 2;
	}
	// Whether to limit the length of received data
	if (lua_gettop(L) >= 4 && lua_isinteger(L, 4)) {
		uint32_t limit = lua_tointeger(L, 4);
		if (total_len > limit)
		{
			LLOGD("Data length to be data %d Limit read length %d", total_len, limit);
			total_len = limit;
		}
	}

	if (1)
	{
		if ((buff->len - buff->used) < total_len)
		{
			result = __zbuff_resize(buff, total_len + buff->used);
			if (result < 0) {
				if (buff->len > buff->used) {
					LLOGW("zbuff automatic expansion failed, reducing the length of received data %d -> %d", total_len, buff->len - buff->used);
					total_len = buff->len - buff->used;
				}
				else {
					LLOGE("The zbuff automatic expansion failed, and the zbuff has no remaining space and cannot read the remaining data.");
					lua_pushboolean(L, 0);
					lua_pushinteger(L, 0);
					return 2;
				}
			}
		}
		result = network_rx(l_ctrl->netc, buff->addr + buff->used, total_len, 0, &ip_addr, &port, &rx_len);
		if (result < 0)
		{
			lua_pushboolean(L, 0);
			lua_pushinteger(L, 0);
			return 2;
		}
		else if (!rx_len)
		{
			lua_pushboolean(L, 1);
			lua_pushinteger(L, 0);
			return 2;
		}
		else
		{
			buff->used += rx_len;
			lua_pushboolean(L, 1);
			lua_pushinteger(L, rx_len);
			if (l_ctrl->netc->is_tcp)
			{
				lua_pushnil(L);
				lua_pushnil(L);
			}
			else
			{
#ifdef LUAT_USE_LWIP
#if LWIP_IPV6
				if (IPADDR_TYPE_V4 == ip_addr.type)
				{
					ip[0] = 0;
					memcpy(ip + 1, &ip_addr.u_addr.ip4.addr, 4);
					lua_pushlstring(L, (const char*)ip, 5);
				}
				else
				{
					ip[0] = 1;
					memcpy(ip + 1, ip_addr.u_addr.ip6.addr, 16);
					lua_pushlstring(L, (const char*)ip, 17);
				}
#else
				ip[0] = 0;
				memcpy(ip + 1, &ip_addr.addr, 4);
				lua_pushlstring(L, (const char*)ip, 5);
#endif
#else
				if (!ip_addr.is_ipv6)
				{
					ip[0] = 0;
					memcpy(ip + 1, &ip_addr.ipv4, 4);
					lua_pushlstring(L, (const char*)ip, 5);
				}
				else
				{
					ip[0] = 1;
					memcpy(ip + 1, &ip_addr.ipv6_u8_addr, 16);
					lua_pushlstring(L, (const char*)ip, 17);
				}
#endif
				lua_pushinteger(L, port);
			}
		}
	}
	return 4;
}

/*Read data (non-zbuff version)
@api socket.read(netc, len)
ctrl obtained by @userdata socket.create
@int limits the length of read data, optional, if not passed, all will be read
@return boolean whether the reading is successful or not
@return string The read data is only valid when the read is successful.
@return string The other party’s IP address, which is only valid when the read is successful and UDP communication occurs
@return int The other party's port, only valid when the reading is successful and UDP communication
@usage
-- This function was added in 2024.4.8 and is used to easily read small data.
-- Please use the socket.rx function first. This function is mainly used for alternative calls when the firmware does not contain the zbuff library.
local ok, data = socket.read(netc, 1500)
if ok and #data > 0 then
log.info("The data read", data)
end*/
static int l_socket_read(lua_State *L) {
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	luat_ip_addr_t ip_addr = {0};
	uint8_t ip[17] = {0};
	uint16_t port = 0;
	// uint8_t new_flag = 0;
	uint32_t rx_len = 0;
	uint32_t total_len = 0;
	int result = network_rx(l_ctrl->netc, NULL, 0, 0, NULL, NULL, &total_len);
	if (result < 0)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	if (!total_len)
	{
		lua_pushboolean(L, 1);
		lua_pushstring(L, "");
		return 2;
	}
	// Whether to limit the length of received data
	if (lua_gettop(L) >= 2 && lua_isinteger(L, 2)) {
		uint32_t limit = lua_tointeger(L, 2);
		if (limit > 0 && total_len > limit)
		{
			LLOGD("Data length to be data %d Limit read length %d", total_len, limit);
			total_len = limit;
		}
	}
	luaL_Buffer bf = {0};
	char* buff = luaL_buffinitsize(L, &bf, total_len);
	result = network_rx(l_ctrl->netc, (uint8_t*)buff, total_len, 0, &ip_addr, &port, &rx_len);
	if (result < 0)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	else if (!rx_len)
	{
		lua_pushboolean(L, 1);
		lua_pushstring(L, "");
		return 2;
	}
	else
	{
		lua_pushboolean(L, 1);
		luaL_pushresultsize(&bf, rx_len);
		if (l_ctrl->netc->is_tcp)
		{
			return 2;
		}
		else
		{
		#ifdef LUAT_USE_LWIP
			#if LWIP_IPV6
			if (IPADDR_TYPE_V4 == ip_addr.type)
			{
				ip[0] = 0;
				memcpy(ip + 1, &ip_addr.u_addr.ip4.addr, 4);
				lua_pushlstring(L, (const char*)ip, 5);
			}
			else
			{
				ip[0] = 1;
				memcpy(ip + 1, ip_addr.u_addr.ip6.addr, 16);
				lua_pushlstring(L, (const char*)ip, 17);
			}
			#else
			ip[0] = 0;
			memcpy(ip + 1, &ip_addr.addr, 4);
			lua_pushlstring(L, (const char*)ip, 5);
			#endif
		#else
			if (!ip_addr.is_ipv6)
			{
				ip[0] = 0;
				memcpy(ip + 1, &ip_addr.ipv4, 4);
				lua_pushlstring(L, (const char*)ip, 5);
			}
			else
			{
				ip[0] = 1;
				memcpy(ip + 1, &ip_addr.ipv6_u8_addr, 16);
				lua_pushlstring(L, (const char*)ip, 17);
			}
		#endif
			lua_pushinteger(L, port);
			return 4;
		}
	}
}

/*Wait for new socket messages. After the connection is successful and the data is sent successfully, use it once to transition the network state to receive new data.
@api socket.wait(ctrl)
ctrl obtained by @user_data socket.create
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value, if false, follow up with close
@return boolean true there is new data to be received, false there is no data, and socket.EVENT messages need to be received later.
@usage
local succ, result = socket.wait(ctrl)*/
static int l_socket_wait(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	int result = network_wait_event(l_ctrl->netc, NULL, 0, NULL);
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, result == 0);
	return 2;
}

/*Start listening as a server
@api socket.listen(ctrl)
ctrl obtained by @user_data socket.create
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value, if false, follow up with close
@return boolean true has been connected, false has not been connected, and then you need to receive socket.ON_LINE messages
@usage
local succ, result = socket.listen(ctrl)*/
static int l_socket_listen(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	int result = network_listen(l_ctrl->netc, 0);
	lua_pushboolean(L, (result < 0)?0:1);
	lua_pushboolean(L, result == 0);
	return 2;
}

/*As the server receives a new client, note that if the hardware protocol stack like W5500 does not support 1-to-many, the second parameter is not required.
@api socket.accept(ctrl)
The ctrl obtained by @user_data socket.create, here is the server side
@string or function or nil string is the taskName of the message notification, function is the callback function, consistent with the socket.create parameter
@return boolean true no exception occurred, false failed, if false there is no need to look at the next return value, if false, follow up with close
@return user_data or nil If 1-to-many is supported, a new ctrl will be returned and automatically created. If it is not supported, nil will be returned.
@usage
local succ, new_netc = socket.listen(ctrl, cb)*/
static int l_socket_accept(lua_State *L)
{
	luat_socket_ctrl_t *old_ctrl = l_get_ctrl(L, 1);
	if (!old_ctrl) return 0;
	if (network_accept_enable(old_ctrl->netc))
	{
		luat_socket_ctrl_t *new_ctrl = (luat_socket_ctrl_t *)lua_newuserdata(L, sizeof(luat_socket_ctrl_t));
		if (!new_ctrl)
		{
			lua_pushboolean(L, 0);
			lua_pushnil(L);
			return 2;
		}
		new_ctrl->adapter_index = old_ctrl->adapter_index;
		new_ctrl->netc = network_alloc_ctrl(old_ctrl->adapter_index);
		if (!new_ctrl->netc)
		{
			LLOGD("create fail");
			lua_pushboolean(L, 0);
			lua_pushnil(L);
			return 2;
		}
		network_init_ctrl(new_ctrl->netc, NULL, luat_lib_socket_callback, new_ctrl);
		if (lua_isfunction(L, 2))
		{
			lua_pushvalue(L, 2);
			new_ctrl->cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);
			new_ctrl->task_name = NULL;
		}
		else if (lua_isstring(L, 2))
		{
			new_ctrl->cb_ref = 0;
			size_t len;
			const char *buf;
			buf = lua_tolstring(L, 2, &len);//Get string data
			new_ctrl->task_name = luat_heap_malloc(len + 1);
			memset(new_ctrl->task_name, 0, len + 1);
			memcpy(new_ctrl->task_name, buf, len);
		}
		if (network_socket_accept(old_ctrl, new_ctrl))
		{
			lua_pushboolean(L, 0);
			lua_pushnil(L);
			return 2;
		}
		else
		{
			lua_pushboolean(L, 1);
			luaL_setmetatable(L, LUAT_NW_CTRL_TYPE);
			return 2;
		}

	}
	else
	{
		lua_pushboolean(L, !network_socket_accept(old_ctrl->netc, NULL));
		lua_pushnil(L);
		return 2;
	}
}

/*Get the current status of the socket
@api socket.state(ctrl)
ctrl obtained by @user_data socket.create
@return int or nil, if the input parameters are correct, return the value of the status, otherwise return nil
@return string or nil, if the input parameters are correct, return the Chinese description of the status, otherwise return nil
@usage
local state, str = socket.state(ctrl)
log.info("state", state, str)
state 0 "Hardware offline",
1 "Offline",
2 "Waiting for DNS",
3 "Connecting",
4 "TLS handshake in progress",
5 "online",
6 "Listening",
7 "Offline",
8 "unknown"*/
static int l_socket_state(lua_State *L)
{
	luat_socket_ctrl_t *l_ctrl = l_get_ctrl(L, 1);
	L_CTRL_CHECK;
	lua_pushinteger(L, l_ctrl->netc->state);
	lua_pushstring(L, network_ctrl_state_string(l_ctrl->netc->state));
	return 2;
}

/*Actively release network_ctrl
@api socket.release(ctrl)
ctrl obtained by @user_data socket.create
@usage
-- Once released, it can no longer be used.
socket.release(ctrl)*/
static int l_socket_release(lua_State *L)
{
	return l_socket_gc(L);
}

/*Set up DNS server
@api socket.setDNS(adapter_index, dns_index, ip)
@int Adapter serial number, it can only be socket.ETH0, socket.STA, socket.AP. If not filled in, the last registered adapter will be selected.
@int dns server serial number, starting from 1
@string or int dns, if it is IPV4, it can be an int value in big endian format
@return boolean returns true if successful, false if failed
@usage
--Set the DNS configuration of the default network adapter
socket.setDNS(nil, 1, "114.114.114.114")
--Set the DNS configuration of the network adapter
socket.setDNS(socket.ETH0, 1, "114.114.114.114")*/
static int l_socket_set_dns(lua_State *L)
{
#ifdef LUAT_USE_LWIP
	char ip_buf[68];
	int adapter_index = luaL_optinteger(L, 1, network_get_last_register_adapter());
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	int dns_index = luaL_optinteger(L, 2, 1);
	luat_ip_addr_t ip_addr;
	const char *ip;
	size_t ip_len;
	network_set_ip_invaild(&ip_addr);
	if (lua_isinteger(L, 3))
	{
		network_set_ip_ipv4(&ip_addr, lua_tointeger(L, 3));
		ip = NULL;
		ip_len = 0;
	}
	else
	{
		ip_len = 0;
	    ip = luaL_checklstring(L, 3, &ip_len);
	    memcpy(ip_buf, ip, ip_len);
	    ip_buf[ip_len] = 0;
	    ipaddr_aton(ip_buf, &ip_addr);
	}
#else
	int adapter_index = luaL_optinteger(L, 1, network_get_last_register_adapter());
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	int dns_index = luaL_optinteger(L, 2, 1);
	luat_ip_addr_t ip_addr;
	const char *ip;
	size_t ip_len;
	ip_addr.is_ipv6 = 0xff;
	if (lua_isinteger(L, 3))
	{
		ip_addr.is_ipv6 = 0;
		ip_addr.ipv4 = lua_tointeger(L, 3);
		ip = NULL;
		ip_len = 0;
	}
	else
	{
		ip_len = 0;
	    ip = luaL_checklstring(L, 3, &ip_len);
	    ip_addr.is_ipv6 = !network_string_is_ipv4(ip, ip_len);
	    if (ip_addr.is_ipv6)
	    {
	    	char *temp = luat_heap_malloc(ip_len + 1);
	    	memcpy(temp, ip, ip_len);
	    	temp[ip_len] = 0;
	    	network_string_to_ipv6(temp, &ip_addr);
	    	luat_heap_free(temp);
	    }
	    else
	    {
	    	ip_addr.ipv4 = network_string_to_ipv4(ip, ip_len);
	    }
	}
#endif
	network_set_dns_server(adapter_index, dns_index - 1, &ip_addr);
	lua_pushboolean(L, 1);
	return 1;
}

/*Set up SSL log registration
@api socket.sslLog(log_level)
@int mbedtls log level
@return nil no return value
@usage
--[[
SSL/TLS log level description
0 does not print
1 only print errors and warnings
2most info
Detailed debugging for 3 and above

Too much information can cause memory fragmentation
]]
--Print most info logs
socket.sslLog(2)*/
static int l_socket_set_ssl_log(lua_State *L)
{
	#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(luaL_optinteger(L, 1, 1));
	#endif
	return 0;
}


#ifdef LUAT_USE_SNTP
#include "luat_sntp.h"
#endif

/*Check the network status of the network card adapter
@api socket.adapter(index)
@int The serial number of the adapter to be checked. You can leave it blank and all network cards will be checked until IP READY is encountered. If the network card is specified, it can only be socket.ETH0 (external Ethernet), socket.LWIP_ETH (built-in Ethernet), socket .LWIP_STA (STA with built-in WIFI), socket.LWIP_AP (AP with built-in WIFI), socket.LWIP_GP (GPRS with built-in cellular network), socket.USB (external USB network card)
@return boolean Whether the adapter being checked is IP READY, true means it is ready for networking, false means it is temporarily unavailable for networking
@return int The last viewed adapter serial number
@usage
-- Check all network cards until you find one with IP READY
local isReady,index = socket.adapter() --If isReady is true, index is the network card adapter serial number of IP READY
--Check whether the external Ethernet (such as W5500) is IP READY
local isReady,default = socket.adapter(socket.ETH0)*/
static int l_socket_adapter(lua_State *L)
{
	int adapter_index = luaL_optinteger(L, 1, -1);
	if (adapter_index > NW_ADAPTER_INDEX_LWIP_NONE &&  adapter_index < NW_ADAPTER_QTY)
	{
		lua_pushboolean(L, network_check_ready(NULL, adapter_index));
		lua_pushinteger(L, adapter_index);
	}
	else
	{
		for(int i = NW_ADAPTER_INDEX_LWIP_GPRS; i < NW_ADAPTER_QTY; i++)
		{
			if (network_check_ready(NULL, i))
			{
				lua_pushboolean(L, 1);
				lua_pushinteger(L, i);
				return 2;
			}
		}
		lua_pushboolean(L, 0);
		lua_pushinteger(L, NW_ADAPTER_QTY - 1);
	}

	return 2;
}


/*Get peer ip
@api socket.remoteIP(ctrl)
ctrl obtained by @user_data socket.create
@return string IP1, if it is nil, it means that the IP address was not obtained
@return string IP2, if nil, it means there is no IP2
@return string IP3, if nil, it means there is no IP3
@return string IP4, if nil, it means there is no IP4
@usage
-- Note: It can only be obtained after receiving the socket.ON_LINE message. A maximum of 4 IPs are returned.
-- If remote_port is set to 0 in socket.connect, the socket.ON_LINE message will be returned when DNS is completed.
local ip1,ip2,ip3,ip4 = socket.remoteIP(ctrl)*/
static int l_socket_remote_ip(lua_State *L)
{
	luat_socket_ctrl_t *ctrl = l_get_ctrl(L, 1);
	PV_Union uPV;
	uint8_t i;
	uint8_t total;
	if (!ctrl)
	{
		goto NO_REMOTE_IP;
	}
	if (!ctrl->netc->dns_ip_nums || !ctrl->netc->dns_ip)
	{
		goto NO_REMOTE_IP;
	}
	total = (ctrl->netc->dns_ip_nums > 4)?4:ctrl->netc->dns_ip_nums;
	for(i = 0; i < total; i++)
	{
#ifdef LUAT_USE_LWIP
		lua_pushfstring(L, "%s", ipaddr_ntoa(&ctrl->netc->dns_ip[i].ip));
#else
		uPV.u32 = &ctrl->netc->dns_ip[i].ip.ipv4;
		lua_pushfstring(L, "%d.%d.%d.%d", uPV.u8[0], uPV.u8[1], uPV.u8[2], uPV.u8[3]);
#endif
	}
	if (total < 4)
	{
		for(i = total; i < 4; i++)
		{
			lua_pushnil(L);
		}
	}
	return 4;
NO_REMOTE_IP:
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushnil(L);
	return 4;
}

#include "rotable2.h"
static const rotable_Reg_t reg_socket_adapter[] =
{
	{"create",				ROREG_FUNC(l_socket_create)},
	{"debug",				ROREG_FUNC(l_socket_set_debug)},
	{"config",				ROREG_FUNC(l_socket_config)},
	{"linkup",				ROREG_FUNC(l_socket_linkup)},
	{"connect",				ROREG_FUNC(l_socket_connect)},
	{"listen",				ROREG_FUNC(l_socket_listen)},
	{"accept",				ROREG_FUNC(l_socket_accept)},
	{"discon",				ROREG_FUNC(l_socket_disconnect)},
	{"close",				ROREG_FUNC(l_socket_close)},
	{"tx",					ROREG_FUNC(l_socket_tx)},
	{"rx",					ROREG_FUNC(l_socket_rx)},
	{"read",				ROREG_FUNC(l_socket_read)},
	{"wait",				ROREG_FUNC(l_socket_wait)},
	{"state",				ROREG_FUNC(l_socket_state)},
	{"release",				ROREG_FUNC(l_socket_release)},
	{ "setDNS",           	ROREG_FUNC(l_socket_set_dns)},
	{ "sslLog",				ROREG_FUNC(l_socket_set_ssl_log)},
	{"localIP",         	ROREG_FUNC(l_socket_local_ip)},
	{"remoteIP",         	ROREG_FUNC(l_socket_remote_ip)},
	{"adapter",				ROREG_FUNC(l_socket_adapter)},
#ifdef LUAT_USE_SNTP
	{"sntp",         		ROREG_FUNC(l_sntp_get)},
	{"ntptm",           	ROREG_FUNC(l_sntp_tm)},
	{"sntp_port",         	ROREG_FUNC(l_sntp_port)},
#endif
	//@const ETH0 number ETH0 with hardware protocol stack, value is 5
    { "ETH0",           	ROREG_INT(NW_ADAPTER_INDEX_ETH0)},
	//@const LWIP_ETH number Ethernet card using LWIP protocol stack, the value is 4
	{ "LWIP_ETH",          	ROREG_INT(NW_ADAPTER_INDEX_LWIP_ETH)},
	//@const LWIP_STA number WIFI STA using LWIP protocol stack, the value is 2
	{ "LWIP_STA",          	ROREG_INT(NW_ADAPTER_INDEX_LWIP_WIFI_STA)},
	//@const LWIP_AP number WIFI AP using LWIP protocol stack, the value is 3
	{ "LWIP_AP",     		ROREG_INT(NW_ADAPTER_INDEX_LWIP_WIFI_AP)},
	//@const LWIP_GP number Mobile cellular Modules using LWIP protocol stack, value is 1
	{ "LWIP_GP",          	ROREG_INT(NW_ADAPTER_INDEX_LWIP_GPRS)},
	//@const USB number USB network card using LWIP protocol stack, the value is 6
	{ "USB",     			ROREG_INT(NW_ADAPTER_INDEX_USB)},
	//@const LINK number LINK event
    { "LINK",           	ROREG_INT(EV_NW_RESULT_LINK & 0x0fffffff)},
    //@const ON_LINE number ON_LINE事件
	{ "ON_LINE",          	ROREG_INT(EV_NW_RESULT_CONNECT & 0x0fffffff)},
    //@const EVENT number EVENT event
	{ "EVENT",          	ROREG_INT(EV_NW_RESULT_EVENT & 0x0fffffff)},
    //@const TX_OK number TX_OK event
	{ "TX_OK",     			ROREG_INT(EV_NW_RESULT_TX & 0x0fffffff)},
    //@const CLOSED number CLOSED事件
	{ "CLOSED",     		ROREG_INT(EV_NW_RESULT_CLOSE & 0x0fffffff)},
	{ NULL,            		ROREG_INT(0)}
};

LUAMOD_API int luaopen_socket_adapter( lua_State *L ) {
    luat_newlib2(L, reg_socket_adapter);
    luaL_newmetatable(L, LUAT_NW_CTRL_TYPE); /* create metatable for file handles */
    lua_pushcfunction(L, l_socket_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1); /* pop new metatable */
    return 1;
}
// #endif
