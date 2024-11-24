
/*@Modules sdio
@summarysdio
@version 1.0
@date 2021.09.02
@tag LUAT_USE_SDIO
@usage
-- The function of this sdio library to mount tf card to the file system has been replaced by the sdio mode of fatfs.
-- This sdio library only retains functions for directly reading and writing TF cards
-- For example, use sdio 0 to mount the TF card
fatfs.mount(fatfs.SDIO, "/sd", 0)

-- After the mounting is completed, just use the relevant functions of the io library to operate
local f = io.open("/sd/abc.txt")*/
#include "luat_base.h"
#include "luat_sdio.h"
#include "luat_mem.h"

#define SDIO_COUNT 2
static luat_sdio_t sdio_t[SDIO_COUNT];

/**
initializesdio
@api sdio.init(id)
@int channel id, related to the specific device, usually starts from 0, defaults to 0
@return boolean open results*/
static int l_sdio_init(lua_State *L) {
    if (luat_sdio_init(luaL_optinteger(L, 1, 0)) == 0) {
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

/*Directly read and write data on the sd card
@api sdio.sd_read(id, offset, len)
@int sdio bus id
@int offset, must be a multiple of 512
@int length, must be a multiple of 512
@return string If the reading is successful, return a string, otherwise return nil
@usage
--Initialize sdio and read sd card data directly
sdio.init(0)
local t = sdio.sd_read(0, 0, 1024)
if t then
    ---xxx
end*/
static int l_sdio_read(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int offset = luaL_checkinteger(L, 2);
    int len = luaL_checkinteger(L, 3);
    char* recv_buff = luat_heap_malloc(len);
    if(recv_buff == NULL)
        return 0;
    int ret = luat_sdio_sd_read(id, sdio_t[id].rca, recv_buff, offset, len);
    if (ret > 0) {
        lua_pushlstring(L, recv_buff, ret);
        luat_heap_free(recv_buff);
        return 1;
    }
    luat_heap_free(recv_buff);
    return 0;
}

/*Write directly to sd card
@api sdio.sd_write(id, data, offset)
@int sdio bus id
@string The data to be written, the length must be a multiple of 512
@int offset, must be a multiple of 512
@return bool If the reading is successful, return true, otherwise return false
@usage
--Initialize sdio and read sd card data directly
sdio.init(0)
local t = sdio.sd_write(0, data, 0)
if t then
    ---xxx
end*/
static int l_sdio_write(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    size_t len;
    const char* send_buff;
    send_buff = lua_tolstring(L, 2, &len);
    int offset = luaL_checkinteger(L, 3);
    int ret = luat_sdio_sd_write(id, sdio_t[id].rca, (char*)send_buff, offset, len);
    if (ret > 0) {
        lua_pushboolean(L, 1);
    }
    lua_pushboolean(L, 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_sdio[] =
{
    { "init" ,          ROREG_FUNC(l_sdio_init )},
    { "sd_read" ,       ROREG_FUNC(l_sdio_read )},
    { "sd_write" ,      ROREG_FUNC(l_sdio_write)},
    // { "sd_mount" ,      ROREG_FUNC(l_sdio_sd_mount)},
    // { "sd_umount" ,     ROREG_FUNC(l_sdio_sd_umount)},
    // { "sd_format" ,     ROREG_FUNC(l_sdio_sd_format)},
	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_sdio( lua_State *L ) {
    luat_newlib2(L, reg_sdio);
    return 1;
}
