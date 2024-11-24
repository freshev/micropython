
/*@Modules  otp
@summary OTP操作库
@version 1.0
@date    2021.12.08
@tag LUAT_USE_OTP
@demo    otp*/
#include "luat_base.h"
#include "luat_otp.h"

#define LUAT_LOG_TAG "otp"
#include "luat_log.h"

/*Read the specified OTP area to read data
@api otp.read(zone, offset, len)
@int area, usually 0/1/2/3, related to specific hardware
@int offset
@int Read length, unit byte, must be a multiple of 4, cannot exceed 4096 bytes
@return string returns string successfully, otherwise returns nil
@usage

local otpdata = otp.read(0, 0, 64)
if otpdata then
    log.info("otp", otpdata:toHex())
end*/
static int l_otp_read(lua_State *L) {
    int zone;
    int offset;
    int len;

    zone = luaL_checkinteger(L, 1);
    offset = luaL_checkinteger(L, 2);
    len = luaL_checkinteger(L, 3);

    if (zone < 0 || zone > 16) {
        return 0;
    }
    if (offset < 0 || offset > 4*1024) {
        return 0;
    }
    if (len < 0 || len > 4*1024) {
        return 0;
    }
    if (offset + len > 4*1024) {
        return 0;
    }
    luaL_Buffer buff;
    luaL_buffinitsize(L, &buff, len);
    memset(buff.b, 0, len);
    int ret = luat_otp_read(zone, buff.b, (size_t)offset, (size_t)len);
    if (ret >= 0) {
        lua_pushlstring(L, buff.b, ret);
        return 1;
    }
    else {
        return 0;
    }
};

/*Write data to the specified OTP area
@api otp.write(zone, data, offset)
@int area, usually 0/1/2/3, related to specific hardware
@string data, the length must be a multiple of 4
@int offset
@return booL returns true if successful, otherwise returns false*/
static int l_otp_write(lua_State *L) {
    int zone;
    int offset;
    size_t len;
    const char* data;

    zone = luaL_checkinteger(L, 1);
    data = luaL_checklstring(L, 2, &len);
    offset = luaL_checkinteger(L, 3);

    if (zone < 0 || zone > 16) {
        return 0;
    }
    if (offset < 0 || offset > 4*1024) {
        return 0;
    }
    if (len > 4*1024) {
        return 0;
    }
    if (offset + len > 4*1024) {
        return 0;
    }
    int ret = luat_otp_write(zone, (char*)data, (size_t)offset, len);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
};

/*Erase specified OTP area
@api otp.erase(zone)
@int area, usually 0/1/2/3, related to specific hardware
@return bool returns true if successful, otherwise returns false*/
static int l_otp_erase(lua_State *L) {
    int zone;
    zone = luaL_checkinteger(L, 1);
    int ret = luat_otp_erase(zone, 0, 1024);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
};

/*Lock the OTP area. Special attention! Once locked, it cannot be unlocked and the OTP becomes read-only!!!
@api otp.lock(zone)
@return bool returns true if successful, otherwise returns false*/
static int l_otp_lock(lua_State *L) {
    int zone = luaL_optinteger(L, 1, 0);
    int ret = luat_otp_lock(zone);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
};


#include "rotable2.h"
static const rotable_Reg_t reg_otp[] =
{
    {"read",    ROREG_FUNC(l_otp_read)},
    {"write",   ROREG_FUNC(l_otp_write)},
    {"erase",   ROREG_FUNC(l_otp_erase)},
    {"lock",    ROREG_FUNC(l_otp_lock)},
	{ NULL,     ROREG_INT(0) }
};

LUAMOD_API int luaopen_otp( lua_State *L ) {
    luat_newlib2(L, reg_otp);
    return 1;
}
