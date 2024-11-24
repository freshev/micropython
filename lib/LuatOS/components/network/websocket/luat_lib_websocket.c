/*@Modules  websocket
@summary websocket客户端
@version 1.0
@date    2022.11.28
@demo    websocket
@tag LUAT_USE_WEBSOCKET
@usage
local wsc = nil
if websocket then
	wsc = websocket.create(nil, "ws://echo.airtun.air32.cn/ws/echo")
    wsc:autoreconn(true, 3000) -- 自动重连机制
    wsc:on(function(wsc, event, data)
        log.info("wsc", event, data)
        if event == "conack" then
            wsc:send((json.encode({action="echo", device_id=device_id})))
            sys.publish("wsc_conack")
        end
    end)
    wsc:connect()
    --sys.waitUntil("websocket_conack", 15000)
    while true do
        sys.wait(45000)
        if wsc:ready() then
        	wsc:send((json.encode({action="echo", msg=os.date()})))
		end
    end
    wsc:close()
    wsc = nil
end*/

#include "luat_base.h"

#include "luat_network_adapter.h"
#include "luat_rtos.h"
#include "luat_zbuff.h"
#include "luat_mem.h"
#include "luat_websocket.h"

#define LUAT_LOG_TAG "websocket"
#include "luat_log.h"

#define LUAT_WEBSOCKET_CTRL_TYPE "WS*"

static luat_websocket_ctrl_t *get_websocket_ctrl(lua_State *L)
{
	if (luaL_testudata(L, 1, LUAT_WEBSOCKET_CTRL_TYPE))
	{
		return ((luat_websocket_ctrl_t *)luaL_checkudata(L, 1, LUAT_WEBSOCKET_CTRL_TYPE));
	}
	else
	{
		return ((luat_websocket_ctrl_t *)lua_touserdata(L, 1));
	}
}

int l_websocket_callback(lua_State *L, void *ptr)
{
	(void)ptr;
	rtos_msg_t *msg = (rtos_msg_t *)lua_topointer(L, -1);
	luat_websocket_ctrl_t *websocket_ctrl = (luat_websocket_ctrl_t *)msg->ptr;
	luat_websocket_pkg_t pkg = {0};
	// size_t payload_size = 0;
	switch (msg->arg1)
	{
	case WEBSOCKET_MSG_TIMER_PING:
	{
		luat_websocket_ping(websocket_ctrl);
		break;
	}
	case WEBSOCKET_MSG_RECONNECT:
	{
		luat_websocket_reconnect(websocket_ctrl);
		break;
	}
	case WEBSOCKET_MSG_PUBLISH:
	{
		if (websocket_ctrl->websocket_cb_id)
		{
			lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_cb_id);
			if (lua_isfunction(L, -1))
			{
				lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_ref);
				lua_pushstring(L, "recv");
				luat_websocket_payload((char *)msg->arg2, &pkg, 64 * 1024);
				lua_pushlstring(L, pkg.payload, pkg.plen);
				lua_pushinteger(L, pkg.FIN);
				lua_pushinteger(L, pkg.OPT_CODE);
				lua_call(L, 5, 0);
			}
		}
		luat_heap_free((char *)msg->arg2);
		break;
	}
	case WEBSOCKET_MSG_CONNACK:
	{
		if (websocket_ctrl->websocket_cb_id)
		{
			lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_cb_id);
			if (lua_isfunction(L, -1))
			{
				lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_ref);
				lua_pushstring(L, "conack");
				lua_call(L, 2, 0);
			}
			lua_getglobal(L, "sys_pub");
			if (lua_isfunction(L, -1))
			{
				lua_pushstring(L, "WEBSOCKET_CONNACK");
				lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_ref);
				lua_call(L, 2, 0);
			}
		}
		break;
	}
	case WEBSOCKET_MSG_RELEASE:
	{
		if (websocket_ctrl->websocket_ref)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_ref);
			websocket_ctrl->websocket_ref = 0;
		}
		break;
	}
	case WEBSOCKET_MSG_SENT :
	{
		if (websocket_ctrl->websocket_cb_id)
		{
			lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_cb_id);
			if (lua_isfunction(L, -1))
			{
				lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_ref);
				lua_pushstring(L, "sent");
				lua_call(L, 2, 0);
			}
		}
		break;
	}
	case WEBSOCKET_MSG_DISCONNECT : 
	{
		if (websocket_ctrl->websocket_cb_id)
		{
			lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_cb_id);
			if (lua_isfunction(L, -1))
			{
				lua_geti(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_ref);
				lua_pushstring(L, "disconnect");
				lua_call(L, 2, 0);
			}
		}
		break;
	}
	default:
	{
		LLOGD("l_websocket_callback error arg1:%d", msg->arg1);
		break;
	}
	}
	// lua_pushinteger(L, 0);
	return 0;
}

int l_luat_websocket_msg_cb(luat_websocket_ctrl_t *websocket_ctrl, int arg1, int arg2)
{
	rtos_msg_t msg = {
		.handler = l_websocket_callback,
		.ptr = websocket_ctrl,
		.arg1 = arg1,
		.arg2 = arg2,
	};
	luat_msgbus_put(&msg, 0);
	return 0;
}

/*Configure whether to turn on debug information
@api wsc:debug(onoff)
@boolean whether to turn on the debug switch
@return nil no return value
@usage wsc:debug(true)*/
static int l_websocket_set_debug(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	if (lua_isboolean(L, 2))
	{
		websocket_ctrl->netc->is_debug = lua_toboolean(L, 2);
	}
	return 0;
}

/*websocket client creation
@api websocket.create(adapter, url, keepalive, use_ipv6)
@int adapter serial number, refer to the constants of the socket library, the default is nil, and the platform's own method will be selected.
@string connection string, refer to usage
@int Heartbeat interval, default 60 seconds. Newly added on 2024.4.28
@boolean Whether to use ipv6, default false. Added on 2024.6.17
@return userdata If successful, the websocket client instance will be returned, otherwise nil will be returned.
@usage
-- Ordinary TCP link
wsc = websocket.create(nil,"ws://air32.cn/abc")
-- Encrypted TCP link
wsc = websocket.create(nil,"wss://air32.cn/abc")*/
static int l_websocket_create(lua_State *L)
{
	int ret = 0;
	int adapter_index = luaL_optinteger(L, 1, network_get_last_register_adapter());
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY)
	{
		return 0;
	}
	//Related to connection parameters
	luat_websocket_connopts_t opts = {0};
	size_t ip_len = 0;
	opts.url = luaL_checklstring(L, 2, &ip_len);
	if (lua_isinteger(L, 3)) {
		opts.keepalive = luaL_checkinteger(L, 3);
	}
	if (lua_type(L, 4) == LUA_TBOOLEAN) {
		opts.use_ipv6 = lua_toboolean(L, 4);
	}

	luat_websocket_ctrl_t *websocket_ctrl = (luat_websocket_ctrl_t *)lua_newuserdata(L, sizeof(luat_websocket_ctrl_t));
	if (!websocket_ctrl)
	{
		LLOGE("out of memory when malloc websocket_ctrl");
		return 0;
	}

	ret = luat_websocket_init(websocket_ctrl, adapter_index);
	if (ret)
	{
		LLOGE("websocket init FAID ret %d", ret);
		return 0;
	}

	network_set_ip_invaild(&websocket_ctrl->ip_addr);
	ret = luat_websocket_set_connopts(websocket_ctrl, &opts);
	if (ret){
		luat_websocket_release_socket(websocket_ctrl);
		return 0;
	}
	
	// TODO determines ret, if initialization fails, it should terminate

	luaL_setmetatable(L, LUAT_WEBSOCKET_CTRL_TYPE);
	lua_pushvalue(L, -1);
	websocket_ctrl->websocket_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	return 1;
}

/*Register websocket callback
@api wsc:on(cb)
@function cb websocket callback, parameters include websocket_client, event, data, payload
@return nil no return value
@usage
wsc:on(function(websocket_client, event, data, payload)
--Print various events
log.info("websocket", "event", event, data, payload)
end)
--[[
The values   of event are:
conack successfully connected to the server, the websocket protocol header information has been received, and communication has been established.
recv receives the information sent by the server, data and payload are not nil
The message sent by the sent send function has been confirmed by the server at the TCP protocol layer.
disconnect The server connection has been disconnected

The sent/disconnect event was added on 2023.04.01
]]*/
static int l_websocket_on(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	if (websocket_ctrl->websocket_cb_id != 0)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_cb_id);
		websocket_ctrl->websocket_cb_id = 0;
	}
	if (lua_isfunction(L, 2))
	{
		lua_pushvalue(L, 2);
		websocket_ctrl->websocket_cb_id = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	return 0;
}

/*Connect to server
@api wsc:connect()
@return boolean Returns true if the initiation is successful, otherwise returns false
@usage
-- Start establishing connection
wsc:connect()
-- This function only represents successful initiation. Subsequently, it is still necessary to judge whether the websocket connection is normal based on the ready function.*/
static int l_websocket_connect(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	int ret = luat_websocket_connect(websocket_ctrl);
	if (ret)
	{
		LLOGE("socket connect ret=%d\n", ret);
		luat_websocket_close_socket(websocket_ctrl);
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	return 1;
}

/*Automatically reconnect
@api wsc:autoreconn(reconnect, reconnect_time)
@bool Whether to automatically reconnect
@int Automatic reconnection period unit ms Default 3000ms
@usage
wsc:autoreconn(true)*/
static int l_websocket_autoreconn(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
    luat_websocket_autoreconn(websocket_ctrl, lua_toboolean(L, 2),luaL_optinteger(L, 3, 3000));
	return 0;
}

/*Post a message
@api wsc:send(data, fin, opt)
@string Data to be sent, required
@int Whether it is the last frame, the default is 1, that is, it is set to the last frame immediately, that is, a single frame is sent
@int opcode, default is string frame 0, optional 1
@return bool returns true if successful, otherwise false or nil
@usage
--Simple sending data
wsc:send("123")
--Send data in segments, and use 1 at the end (that is, the end of the FIN frame)
wsc:send("123", 0)
wsc:send("456", 0)
wsc:send("789", 1)*/
static int l_websocket_send(lua_State *L)
{
	size_t payload_len = 0;
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	const char *payload = NULL;
	luat_zbuff_t *buff = NULL;
	int ret = 0;
	if (lua_isstring(L, 2))
	{
		payload = luaL_checklstring(L, 2, &payload_len);
	}
	else if (luaL_testudata(L, 2, LUAT_ZBUFF_TYPE))
	{
		buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
		payload = (const char *)buff->addr;
		payload_len = buff->used;
	}
	else
	{
		LLOGD("only support string or zbuff");
		return 0;
	}
	luat_websocket_pkg_t pkg = {
		.FIN = 1,
		.OPT_CODE = 0x01,
		.plen = payload_len,
		.payload = payload};
	if (lua_isinteger(L, 3) && lua_tointeger(L, 3) == 1) {
		pkg.OPT_CODE = 0x02;
	}
	
	if (websocket_ctrl->websocket_state != 1) {
		LLOGI("not ready yet");
		lua_pushboolean(L, 0);
		return 1;
	}
	websocket_ctrl->frame_wait ++;
	ret = luat_websocket_send_frame(websocket_ctrl, &pkg);
	if (ret < 1) {
		websocket_ctrl->frame_wait --;//Sending failed
	}
	lua_pushboolean(L, ret == 0 ? 1 : 0);
	return 1;
}

/*The websocket client is closed (resources are released and can no longer be used after closing)
@api wsc:close()
@usage
wsc:close()*/
static int l_websocket_close(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	// websocket_disconnect(&(websocket_ctrl->broker));
	luat_websocket_close_socket(websocket_ctrl);
	if (websocket_ctrl->websocket_cb_id != 0)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, websocket_ctrl->websocket_cb_id);
		websocket_ctrl->websocket_cb_id = 0;
	}
	luat_websocket_release_socket(websocket_ctrl);
	return 0;
}

/*Is the websocket client ready?
@api wsc:ready()
@return bool whether the client is ready
@usage
local stat = wsc:ready()*/
static int l_websocket_ready(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	lua_pushboolean(L, websocket_ctrl->websocket_state > 0 ? 1 : 0);
	return 1;
}

/*Set additional headers
@api wsc:headers(headers)
@table/string can be a table or a string
@return bool whether the client is ready
@usage
-- table format
wsc:headers({
Auth="Basic ABCDEFGG"
})
-- string form
wsc:headers("Auth: Basic ABCDERG\r\n")*/
static int l_websocket_headers(lua_State *L)
{
	luat_websocket_ctrl_t *websocket_ctrl = get_websocket_ctrl(L);
	if (!lua_istable(L, 2) && !lua_isstring(L, 2)) {
		return  0;
	}
	#define WS_HEADER_MAX (1024)
	char* buff = luat_heap_malloc(WS_HEADER_MAX);
	memset(buff, 0, WS_HEADER_MAX);
	if (lua_istable(L, 2)) {
		size_t name_sz = 0;
		size_t value_sz = 0;
		lua_pushnil(L);
		while (lua_next(L, 2) != 0) {
			const char *name = lua_tolstring(L, -2, &name_sz);
			const char *value = lua_tolstring(L, -1, &value_sz);
			if (name_sz == 0 || value_sz == 0 || name_sz + value_sz > 256) {
				LLOGW("bad header %s %s", name, value);
				luat_heap_free(buff);
				return 0;
			}
			memcpy(buff + strlen(buff), name, name_sz);
			memcpy(buff + strlen(buff), ":", 1);
			if (WS_HEADER_MAX - strlen(buff) < value_sz * 2) {
				LLOGW("bad header %s %s, too large", name, value);
				luat_heap_free(buff);
				return 0;
			}
			for (size_t i = 0; i < value_sz; i++)
			{
				switch (value[i])
				{
				case '*':
				case '-':
				case '.':
				case '_':
				case ' ':
					sprintf_(buff + strlen(buff), "%%%02X", value[i]);
					break;
				default:
					buff[strlen(buff)] = value[i];
					break;
				}
			}
			lua_pop(L, 1);
			memcpy(buff + strlen(buff), "\r\n", 2);
		}
	}
	else {
		size_t len = 0;
		const char* data = luaL_checklstring(L, 2, &len);
		if (len > 1023) {
			LLOGW("headers too large size %d", len);
			luat_heap_free(buff);
			return 0;
		}
		memcpy(buff, data, len);
	}
	luat_websocket_set_headers(websocket_ctrl, buff);
	lua_pushboolean(L, 1);
	return 1;
}

static int _websocket_struct_newindex(lua_State *L);

void luat_websocket_struct_init(lua_State *L)
{
	luaL_newmetatable(L, LUAT_WEBSOCKET_CTRL_TYPE);
	lua_pushcfunction(L, _websocket_struct_newindex);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}

#include "rotable2.h"
const rotable_Reg_t reg_websocket[] =
	{
		{"create", 			ROREG_FUNC(l_websocket_create)},
		{"on", 				ROREG_FUNC(l_websocket_on)},
		{"connect", 		ROREG_FUNC(l_websocket_connect)},
		{"autoreconn", 		ROREG_FUNC(l_websocket_autoreconn)},
		{"send", 			ROREG_FUNC(l_websocket_send)},
		{"close", 			ROREG_FUNC(l_websocket_close)},
		{"ready", 			ROREG_FUNC(l_websocket_ready)},
		{"headers", 		ROREG_FUNC(l_websocket_headers)},
		{"debug",           ROREG_FUNC(l_websocket_set_debug)},
		{NULL, 				ROREG_INT(0)}
};

int _websocket_struct_newindex(lua_State *L)
{
	const rotable_Reg_t *reg = reg_websocket;
	const char *key = luaL_checkstring(L, 2);
	while (1)
	{
		if (reg->name == NULL)
			return 0;
		if (!strcmp(reg->name, key))
		{
			lua_pushcfunction(L, reg->value.value.func);
			return 1;
		}
		reg++;
	}
	// return 0;
}
#ifndef LUAT_USE_NETWORK
static const rotable_Reg_t reg_websocket_emtry[] = {
		{NULL, ROREG_INT(0)}
};
#endif

LUAMOD_API int luaopen_websocket(lua_State *L)
{

#ifdef LUAT_USE_NETWORK
	luat_newlib2(L, reg_websocket);
	luat_websocket_struct_init(L);
	return 1;
#else
	LLOGE("websocket require network enable!!");
	luat_newlib2(L, reg_websocket_emtry);
	return 1;
#endif
}
