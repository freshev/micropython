/*@Modules  sfd
@summary SPI FLASH操作库
@version 1.0
@date    2021.05.18
@tag LUAT_USE_SFD*/
#include "luat_base.h"
#include "luat_spi.h"
#include "luat_sfd.h"

#define LUAT_LOG_TAG "sfd"
#include "luat_log.h"

extern const sdf_opts_t sfd_w25q_opts;
extern const sdf_opts_t sfd_mem_opts;
extern const sdf_opts_t sfd_onchip_opts;

/*Initialize spi flash
@api sfd.init(type, spi_id, spi_cs)
@string type, can be "spi", "zbuff", or "onchip"
@int SPI bus id, or zbuff instance
@int The GPIO corresponding to the chip select pin of SPI FLASH. It only needs to be passed when the type is spi.
@return userdata successfully returns a data structure, otherwise returns nil
@usage
local drv = sfd.init("spi", 0, 17)
if drv then
    log.info("sfd", "chip id", sfd.id(drv):toHex())
end
-- Firmware after 2023.01.15 supports onchip type, which supports direct reading and writing of a small area of   on-chip flash, usually 64k
-- This area is usually where the fdb/fskv library is located, so don't mix them.
local onchip = sfd.init("onchip")
local data = sfd.read(onchip, 0x100, 256)
sfd.erase(onchip, 0x100)
sfd.write(onchip, 0x100, data or "Hi")*/
static int l_sfd_init(lua_State *L) {

    const char* type = luaL_checkstring(L, 1);
    if (!strcmp("spi", type)) {
        
        int spi_id = luaL_checkinteger(L, 2);
        int spi_cs = luaL_checkinteger(L, 3);

        sfd_drv_t *drv = (sfd_drv_t *)lua_newuserdata(L, sizeof(sfd_drv_t));
        memset(drv, 0, sizeof(sfd_drv_t));
        drv->cfg.spi.id = spi_id;
        drv->cfg.spi.cs = spi_cs;
        drv->opts = &sfd_w25q_opts;
        drv->type = 0;

        int re = drv->opts->initialize(drv);
        if (re == 0) {
            return 1;
        }
        return 0;
    }
    if (!strcmp("zbuff", type)) {
        sfd_drv_t *drv = (sfd_drv_t *)lua_newuserdata(L, sizeof(sfd_drv_t));
        memset(drv, 0, sizeof(sfd_drv_t));
        drv->type = 1;
        drv->cfg.zbuff = luaL_checkudata(L, 2, "ZBUFF*");
        drv->opts = &sfd_mem_opts;
        drv->sector_count = drv->cfg.zbuff->len / 256;

        int re = drv->opts->initialize(drv);
        if (re == 0) {
            return 1;
        }
        return 0;
    }
    if (!strcmp("onchip", type)) {
        sfd_drv_t *drv = (sfd_drv_t *)lua_newuserdata(L, sizeof(sfd_drv_t));
        memset(drv, 0, sizeof(sfd_drv_t));
        drv->type = 3;
        drv->opts = &sfd_onchip_opts;
        int re = drv->opts->initialize(drv);
        if (re == 0) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/*Check spi flash status
@api sfd.status(drv)
@userdata The data structure returned by sfd.init
@return int status value, 0 is not initialized successfully, 1 is initialized successfully and is idle, 2 is busy
@usage
local drv = sfd.init("spi", 0, 17)
if drv then
    log.info("sfd", "status", sfd.status(drv))
end*/
static int l_sfd_status(lua_State *L) {
    sfd_drv_t *drv = (sfd_drv_t *) lua_touserdata(L, 1);
    lua_pushinteger(L, drv->opts->status(drv));
    return 1;
}

/*Read data
@api sfd.read(drv, offset, len)
@userdata The data structure returned by sfd.init
@int starting offset
@int read length, currently limited to 256
@return string data
@usage
local drv = sfd.init("spi", 0, 17)
if drv then
    log.info("sfd", "read", sfd.read(drv, 0x100, 256))
end*/
static int l_sfd_read(lua_State *L) {
    sfd_drv_t *drv = (sfd_drv_t *) lua_touserdata(L, 1);
    size_t offset = luaL_checkinteger(L, 2);
    size_t len = luaL_checkinteger(L, 3);
    luaL_Buffer buff;
    luaL_buffinitsize(L, &buff, len);
    drv->opts->read(drv, buff.b, offset, len);
    luaL_pushresult(&buff);
    return 1;
}

/*Write data
@api sfd.write(drv, offset, data)
@userdata The data structure returned by sfd.init
@int starting offset
@string The data to be written, currently supports 256 bytes and below
@return boolean returns true if successful, false if failed
@usage
local drv = sfd.init("spi", 0, 17)
if drv then
    log.info("sfd", "write", sfd.write(drv, 0x100, "hi,luatos"))
end*/
static int l_sfd_write(lua_State *L) {
    sfd_drv_t *drv = (sfd_drv_t *) lua_touserdata(L, 1);
    size_t offset = luaL_checkinteger(L,2);
    size_t len = 0;
    const char* buff = luaL_checklstring(L, 3, &len);
    int re = drv->opts->write(drv, buff, offset, len);
    lua_pushboolean(L, re == 0 ? 1 : 0);
    return 1;
}

/*Wipe data
@api sfd.erase(drv, offset)
@userdata The data structure returned by sfd.init
@int starting offset
@return boolean returns true if successful, false if failed
@usage
local drv = sfd.init("spi", 0, 17)
if drv then
    log.info("sfd", "write", sfd.erase(drv, 0x100))
end*/
static int l_sfd_erase(lua_State *L) {
    sfd_drv_t *drv = (sfd_drv_t *) lua_touserdata(L, 1);
    size_t offset = luaL_checkinteger(L, 2);
    size_t len = luaL_optinteger(L, 3, 4096);
    int re = drv->opts->erase(drv, offset, len);
    lua_pushboolean(L, re == 0 ? 1 : 0);
    return 1;
}

static int l_sfd_ioctl(lua_State *L) {
    return 0;
}

/*Chip unique id
@api sfd.id(drv)
@userdata The data structure returned by sfd.init
@return string 8-byte (64bit) chip ID
@usage
local drv = sfd.init("spi", 0, 17)
if drv then
    log.info("sfd", "chip id", sfd.id(drv))
end*/
static int l_sfd_id(lua_State *L) {
    sfd_drv_t *drv = (sfd_drv_t *) lua_touserdata(L, 1);
    lua_pushlstring(L, drv->chip_id, 8);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_sfd[] =
{
    { "init" ,             ROREG_FUNC(l_sfd_init)},
    { "status",            ROREG_FUNC(l_sfd_status)},
    { "read",              ROREG_FUNC(l_sfd_read)},
    { "write",             ROREG_FUNC(l_sfd_write)},
    { "erase",             ROREG_FUNC(l_sfd_erase)},
    { "ioctl",             ROREG_FUNC(l_sfd_ioctl)},
    { "id",                ROREG_FUNC(l_sfd_id)},
    { NULL,                ROREG_INT(0)}
};

LUAMOD_API int luaopen_sfd( lua_State *L ) {
    luat_newlib2(L, reg_sfd);
    return 1;
}
