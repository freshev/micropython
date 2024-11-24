
/*@Modules fota
@summary Underlying firmware upgrade
@version core V0007
@date 2022.05.26
@demo fota
@tag LUAT_USE_FOTA
@usage
-- If you get the upgrade package from http, just look at demo/fota.
--The following is the basic logic of calling this fota library after obtaining the update package from other channels.

-- Pass it in piece by piece
sys.taskInit(function()
    fota.init()
    while 1 do
        local buf = xxx -- Here is the upgrade package fragment obtained from other channels
        -- buf can be zbuff or string
        --The maximum length of data written each time should not exceed 4k
        local result, isDone, cache = fota.run(buf)
        if not result then
            log.info("fota", "something went wrong")
            break
        end
        if isDone then
            while true do
                local succ,fotaDone = fota.isDone()
                if not succ then
                    log.info("fota", "something went wrong")
                    break
                end
                if fotaDone then
                    log.info("fota", "Completed")
                    break
                end
                sys.wait(100)
            end
            break
        end
        sys.wait(100)
    end
end)

--Use a file to pass in one time
sys.taskInit(function()
    fota.init()
    fota.file("/xxx") -- Pass in the specific path
end)*/
#include "luat_base.h"
#include "luat_fota.h"
#include "luat_zbuff.h"
#include "luat_spi.h"
#include "luat_fs.h"
#include "luat_mem.h"

#define LUAT_LOG_TAG "fota"
#include "luat_log.h"

/**
Initialize fota process
@api fota.init(storge_location, len, param1, param2)
@int/string The starting position of fota data storage<br>If it is int, it is determined by the chip platform<br>If it is string, it is stored in the file system<br>If it is nil, the storage is determined by the bottom layer Location
@int Maximum space for data storage
@userdata param1, if the data is stored in spiflash, it is spi_device
@int param2, currently only used for external flash update, spiflash power control pin
@return boolean returns true on success, false on failure
@usage
--Initialize fota process
local result = fota.init(0, 0x00300000, spi_device) --Since the flash of 105 starts from 0x01000000, 0 is the external spiflash
local result = fota.init() --ec618 series/EC7XX series use fixed internal addresses, so no parameters are needed
local result = fota.init(0xe0000000, 0, spi_device, 27) --EC7XX series allows the use of external flash update, but the address must be added with an offset of 0xe0000000*/
static int l_fota_init(lua_State* L)
{
	uint32_t address = 0xffffffff;
    size_t len = 0;
    uint32_t length;
    const char *buf = NULL;
    luat_spi_device_t* spi_device = NULL;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
    	buf = lua_tolstring(L, 1, &len);//Get string data
    }
    else
    {
    	address = luaL_optinteger(L, 1, 0xffffffff);
    }
    length = luaL_optinteger(L, 2, 0);
    if (lua_isuserdata(L, 3))
    {
    	spi_device = (luat_spi_device_t*)lua_touserdata(L, 3);
    }
    uint8_t power_pin = luaL_optinteger(L, 4, 0xffffffff);
    if (spi_device)
    {
    	spi_device->user_data = &power_pin;
    }

	lua_pushboolean(L, !luat_fota_init(address, length, spi_device, buf, len));
	return 1;
}

/**
Wait for the underlying fota process to be ready
@api fota.wait()
@boolean Whether the process has been completed completely, true means the process has been completed correctly
@return boolean ready to return true
@usage
local isDone = fota.wait()*/
static int l_fota_wait(lua_State* L)
{
    lua_pushboolean(L, luat_fota_wait_ready());
	return 1;
}

/**
Write fota data
@api fota.run(buff, offset, len)
@zbuff/string fota data, try to use zbuff
@int starting offset, valid when passing in zbuff, default is 0
@int writing length, valid when passing in zbuff, the default is zbuff:used()
@return boolean Returns false if there is an exception, returns true if there is no exception
@return boolean returns true after receiving the last block
@return int The amount of data that has not been written yet. If it exceeds 64K, you must wait.
@usage
local result, isDone, cache = fota.run(buf) -- write fota process

-- Tip: If the incoming zbuff is, after the writing is successful, please clear the data in the zbuff yourself.

--Added offset and len parameters in 2024.4.3, only valid for zbuff
fota.run(buff, 0, 1024)*/
static int l_fota_write(lua_State* L)
{
	int result = 0;
    size_t len = 0;
    const char *buf = NULL;
    if(lua_isuserdata(L, 1))
    {
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 1, LUAT_ZBUFF_TYPE));
        size_t offset = luaL_optinteger(L, 2, 0);
        len = luaL_optinteger(L, 3, buff->used - offset);
        if (len + offset > buff->len) {
            LLOGE("len too long %d > %d", len, buff->len);
            result = -1;
        }
        else {
            result = luat_fota_write(buff->addr + offset, len);
        }
    }
    else
    {
        buf = lua_tolstring(L, 1, &len);//Get string data
        result = luat_fota_write((uint8_t*)buf, len);
    }
    if (result > 0)
    {
    	lua_pushboolean(L, 1);
    	lua_pushboolean(L, 0);
    }
    else if (result == 0)
    {
    	lua_pushboolean(L, 1);
    	lua_pushboolean(L, 1);
    }
    else
    {
    	lua_pushboolean(L, 0);
    	lua_pushboolean(L, 1);
    }
    lua_pushinteger(L, result);
	return 3;
}

/**
Read fota data from the specified file
@api fota.file(path)
@string file path
@return boolean Returns false if there is an exception, returns true if there is no exception
@return boolean returns true after receiving the last block
@return int The amount of data that has not been written yet. If it exceeds 64K, you must wait.
@usage
local result, isDone, cache = fota.file("/xxx.bin") -- write fota process
-- This API was added on 2023.03.23*/
static int l_fota_file(lua_State* L)
{
    int result = 0;
	const char *path = luaL_checkstring(L, 1);
    FILE* fd = luat_fs_fopen(path, "rb");
    if (fd == NULL) {
        LLOGE("no such file for FOTA %s", path);
        lua_pushboolean(L, 0);
    	lua_pushboolean(L, 0);
        lua_pushinteger(L, 0);
        return 3;
    }
    #define BUFF_SIZE (4096)
    char *buff = luat_heap_malloc(BUFF_SIZE);
    if (buff == NULL) {
        luat_fs_fclose(fd);
        LLOGE("out of memory when reading file %s", path);
        lua_pushboolean(L, 0);
    	lua_pushboolean(L, 0);
        lua_pushinteger(L, 0);
        return 3;
    }
    int len  = 0;
    while (1) {
        len = luat_fs_fread(buff , BUFF_SIZE, 1, fd);
        if (len < 1) {
            // EOF ended
            break;
        }
        result = luat_fota_write((uint8_t*)buff, len);
        if (result < 0) {
            break;
        }
    }
    luat_heap_free(buff);
    luat_fs_fclose(fd);

    if (result > 0)
    {
    	lua_pushboolean(L, 1);
    	lua_pushboolean(L, 0);
    }
    else if (result == 0)
    {
    	lua_pushboolean(L, 1);
    	lua_pushboolean(L, 1);
    }
    else
    {
    	lua_pushboolean(L, 0);
    	lua_pushboolean(L, 1);
    }
    lua_pushinteger(L, result);
	return 3;
}

/**
Wait for the underlying fota process to complete
@api fota.isDone()
@return boolean Returns false if there is an exception, returns true if there is no exception
@return boolean returns true when writing to the last block
@usage
local result, isDone = fota.isDone()*/
static int l_fota_done(lua_State* L)
{
	int result = luat_fota_done();
    if (result > 0)
    {
    	lua_pushboolean(L, 1);
    	lua_pushboolean(L, 0);
    }
    else if (result == 0)
    {
    	lua_pushboolean(L, 1);
    	lua_pushboolean(L, 1);
    }
    else
    {
    	lua_pushboolean(L, 0);
    	lua_pushboolean(L, 1);
    }
	return 2;
}

/**
End fota process
@api fota.finish(is_ok)
@boolean Whether the process has been completed completely, true means the process has been completed correctly
@return boolean returns true on success, false on failure
@usage
-- End the fota process
local result = fota.finish(true)*/
static int l_fota_end(lua_State* L)
{
	lua_pushboolean(L, !luat_fota_end(lua_toboolean(L, 1)));
	return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_fota[] =
{
	{ "init",		ROREG_FUNC(l_fota_init)},
	{ "wait",		ROREG_FUNC(l_fota_wait)},
	{ "run",		ROREG_FUNC(l_fota_write)},
	{ "isDone",		ROREG_FUNC(l_fota_done)},
	{ "finish",		ROREG_FUNC(l_fota_end)},
    { "file",       ROREG_FUNC(l_fota_file)},
	{ NULL,         ROREG_INT(0) }
};

LUAMOD_API int luaopen_fota( lua_State *L ) {
    luat_newlib2(L, reg_fota);
    return 1;
}
