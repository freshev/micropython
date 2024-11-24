
/*@Modulesymodem
@summary ymodem protocol
@version 1.0
@date 2022.09.30
@author Dozingfiretruck
@tag LUAT_USE_YMODEM
@usage
-- The purpose of this library is to receive data. If you need to send files, it is recommended to use the xmodem library
local handler = ymodem.create("/")
uart.setup(1, 115200)
uart.on(1, "receive", function(id, len)
while 1 do
local data = uart.read(id, 512)
if not data or #data == 0 then
break
end
ymodem.receive(handler, data)
end
end)*/

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_ymodem.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "ymodem"
#include "luat_log.h"

typedef struct
{
	void *ctrl;
}ymodem_handler;

/*Create a ymodem handler
@api ymodem.create(dir_path,file_path)
@string Saved folder path, default is "/"
@string The absolute file path to force saving. The default is empty. If set, it will be saved directly in the file.
@return boolean true on success, false on failure
@usage
local handler = ymodem.create("/")*/
static int l_ymodem_create(lua_State *L){
	ymodem_handler *handler = (ymodem_handler *)lua_newuserdata(L, sizeof(ymodem_handler));
	size_t len;
	const char *dir_path,*file_path;
	if (lua_isstring(L, 1))
	{
		dir_path = lua_tolstring(L, 1, &len);

	}
	else
	{
		dir_path = NULL;
	}
	if (lua_isstring(L, 2))
	{
		file_path = lua_tolstring(L, 2, &len);

	}
	else
	{
		file_path = NULL;
	}
	handler->ctrl = luat_ymodem_create_handler(dir_path?dir_path:"/", file_path);
	lua_pushlightuserdata(L, handler);
    return 1;
}

/*ymodem receives file data and saves it
@api ymodem.receive(handler, data)
@userdata ymodem processing handle
@zbuff/string input data
@return boolean true on success, false on failure
@return int ack value, needs to be returned to the sender through serial port/network etc.
@return int flag value, it needs to be returned to the sender through serial port/network, etc. If there is an ack value, the flag will not be sent.
@return boolean, true when a file is received, false during transmission
@return boolean, true if the entire transfer is completed, otherwise false
@usage
-- Note that the data source is not limited, usually uart.read gets the data
no_error,ack,flag,file_done,all_done = ymodem.receive(handler, data)*/

static int l_ymodem_receive(lua_State *L){
	ymodem_handler *handler = (ymodem_handler *)lua_touserdata(L, 1);
	int result;
	size_t len;
	uint8_t ack, flag, file_ok, all_done;
	const char *data;
	if (handler && handler->ctrl)
	{
		if (lua_isstring(L, 2))
		{
			data = lua_tolstring(L, 1, &len);
		}
		else if(lua_isuserdata(L, 2))
	    {
	        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
	        len = buff->used;
	        data = (const char *)(buff->addr);

	    }
		else
		{
			data = NULL;
			len = 0;
		}
		result = luat_ymodem_receive(handler->ctrl, (uint8_t*)data, len, &ack, &flag, &file_ok, &all_done);
		lua_pushboolean(L, !result);
		lua_pushinteger(L, ack);
		if (flag)
		{
			lua_pushinteger(L, flag);
		}
		else
		{
			lua_pushnil(L);
		}
		lua_pushboolean(L, file_ok);
		lua_pushboolean(L, all_done);
	}
	else
	{
		LLOGE("%x,%x", handler, handler->ctrl);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushboolean(L, 0);
		lua_pushboolean(L, 0);
	}
	return 5;
}

/*Reset ymodem process
@api ymodem.reset(handler)
@userdata ymodem processing handle
@usage
--Restore to the initial state, generally used to reset after a reception error, so as to perform the next reception
ymodem.reset(handler)*/
static int l_ymodem_reset(lua_State *L){
	ymodem_handler *handler = (ymodem_handler *)lua_touserdata(L, 1);
	if (handler && handler->ctrl) luat_ymodem_reset(handler->ctrl);
    return 0;
}

/*Release ymodem handler
@api ymodem.release(handler)
@userdata handler
@usage
ymodem.release(handler)*/

static int l_ymodem_release(lua_State *L){
	ymodem_handler *handler = (ymodem_handler *)lua_touserdata(L, 1);
	if (handler && handler->ctrl) {
		luat_ymodem_release(handler->ctrl);
		handler->ctrl = NULL;
	}
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_ymodem[] =
{
    { "create",           	ROREG_FUNC(l_ymodem_create)},
    { "receive",   			ROREG_FUNC(l_ymodem_receive)},
    { "reset",      		ROREG_FUNC(l_ymodem_reset)},
    { "release",			ROREG_FUNC(l_ymodem_release)},
	{ NULL,             ROREG_INT(0)}
};

LUAMOD_API int luaopen_ymodem( lua_State *L ) {
    luat_newlib2(L, reg_ymodem);
    return 1;
}
