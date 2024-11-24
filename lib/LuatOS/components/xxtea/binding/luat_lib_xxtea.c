
/*@Modulesxxtea
@summary xxtea encryption and decryption
@version 1.0
@date 2023.12.22
@author wendal
@tag LUAT_USE_XXTEA
@usage
-- This library is a package of https://github.com/xxtea/xxtea-c. Thanks again to the author of xxtea

local text = "Hello World!"
local key = "07946"
local encrypt_data = xxtea.encrypt(text, key)
log.info("testCrypto.xxteaTest","xxtea_encrypt:"..encrypt_data)
local decrypt_data = xxtea.decrypt(encrypt_data, key)
log.info("testCrypto.xxteaTest","decrypt_data:"..decrypt_data)*/

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "xxtea"
#include "luat_log.h"

#include "xxtea.h"

/*encryption
@api xxtea.encrypt(data, key)
@string data data to be encrypted
@string key The key used for encryption
@return string encrypted data, returns nil on failure*/
static int l_xxtea_encrypt(lua_State* L) {
    size_t len;
    size_t out_len;
    const char* data = luaL_checklstring(L, 1, &len);
    const char* key = luaL_checkstring(L, 2);

    char* result = xxtea_encrypt(data, len, key, &out_len);
    if (result) {
        lua_pushlstring(L, result, out_len);
        luat_heap_free(result);
        return 1;
    }

    return 0;
}

/*Decrypt
@api xxtea.decrypt(data, key)
@string data data to be decrypted
@string key The key used for decryption
@return string decrypted data, returns nil on failure*/
static int l_xxtea_decrypt(lua_State* L) {
    size_t len;
    size_t out_len;
    const char* data = luaL_checklstring(L, 1, &len);
    const char* key = luaL_checkstring(L, 2);

    char* result = xxtea_decrypt(data, len, key, &out_len);
    if (result) {
        lua_pushlstring(L, result, out_len);
        luat_heap_free(result);
        return 1;
    }

    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_xxtea[] =
{
    { "encrypt",           	ROREG_FUNC(l_xxtea_encrypt)},
    { "decrypt",           	ROREG_FUNC(l_xxtea_decrypt)},
	{ NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_xxtea( lua_State *L ) {
    luat_newlib2(L, reg_xxtea);
    return 1;
}
