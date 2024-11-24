
/*@Modules crypto
@summary Encryption, decryption and hash functions
@version 1.0
@date 2020.07.03
@demo crypto
@tag LUAT_USE_CRYPTO*/
#include "luat_base.h"
#include "luat_crypto.h"
#include "luat_mem.h"
#include "luat_str.h"
#include <time.h>
#include "luat_zbuff.h"
#include "mbedtls/md.h"

#define LUAT_LOG_TAG "crypto"
#define LUAT_CRYPTO_TYPE "crypto"
#include "luat_log.h"

static const unsigned char hexchars[] = "0123456789ABCDEF";
static void fixhex(const char* source, char* dst, size_t len) {
    for (size_t i = 0; i < len; i++)
    {
        char ch = *(source+i);
        dst[i*2] = hexchars[(unsigned char)ch >> 4];
        dst[i*2+1] = hexchars[(unsigned char)ch & 0xF];
    }
}

/**
Calculate md5 value
@api crypto.md5(str)
@string the string to be calculated
@return string hex string of calculated md5 value
@usage
-- Calculate the md5 of the string "abc"
log.info("md5", crypto.md5("abc"))*/
static int l_crypto_md5(lua_State *L) {
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);
    char tmp[32] = {0};
    char dst[32] = {0};
    if (luat_crypto_md5_simple(str, size, tmp) == 0) {
        fixhex(tmp, dst, 16);
        lua_pushlstring(L, dst, 32);
        return 1;
    }
    return 0;
}

/**
Calculate hmac_md5 value
@api crypto.hmac_md5(str, key)
@string the string to be calculated
@string key
@return string hex string of calculated hmac_md5 value
@usage
-- Calculate hmac_md5 of string "abc"
log.info("hmac_md5", crypto.hmac_md5("abc", "1234567890"))*/
static int l_crypto_hmac_md5(lua_State *L) {
    size_t str_size = 0;
    size_t key_size = 0;
    const char* str = luaL_checklstring(L, 1, &str_size);
    const char* key = luaL_checklstring(L, 2, &key_size);
    char tmp[32] = {0};
    char dst[32] = {0};
    if (luat_crypto_hmac_md5_simple(str, str_size, key, key_size, tmp) == 0) {
        fixhex(tmp, dst, 16);
        lua_pushlstring(L, dst, 32);
        return 1;
    }
    return 0;
}

/**
Calculate sha1 value
@api crypto.sha1(str)
@string the string to be calculated
@return string hex string of calculated sha1 value
@usage
-- Calculate the sha1 of the string "abc"
log.info("sha1", crypto.sha1("abc"))*/
static int l_crypto_sha1(lua_State *L) {
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);
    char tmp[40] = {0};
    char dst[40] = {0};
    if (luat_crypto_sha1_simple(str, size, tmp) == 0) {
        fixhex(tmp, dst, 20);
        lua_pushlstring(L, dst, 40);
        return 1;
    }
    return 0;
}

/**
Calculate hmac_sha1 value
@api crypto.hmac_sha1(str, key)
@string the string to be calculated
@string key
@return string hex string of calculated hmac_sha1 value
@usage
-- Calculate hmac_sha1 of string "abc"
log.info("hmac_sha1", crypto.hmac_sha1("abc", "1234567890"))*/
static int l_crypto_hmac_sha1(lua_State *L) {
    size_t str_size = 0;
    size_t key_size = 0;
    const char* str = luaL_checklstring(L, 1, &str_size);
    const char* key = luaL_checklstring(L, 2, &key_size);
    char tmp[40] = {0};
    char dst[40] = {0};
    if (luat_crypto_hmac_sha1_simple(str, str_size, key, key_size, tmp) == 0) {
        fixhex(tmp, dst, 20);
        lua_pushlstring(L, dst, 40);
        return 1;
    }
    return 0;
}


/**
Calculate sha256 value
@api crypto.sha256(str)
@string the string to be calculated
@return string hex string of calculated sha256 value
@usage
-- Calculate the sha256 of the string "abc"
log.info("sha256", crypto.sha256("abc"))*/
static int l_crypto_sha256(lua_State *L) {
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);
    char tmp[64] = {0};
    char dst[64] = {0};
    if (luat_crypto_sha256_simple(str, size, tmp) == 0) {
        fixhex(tmp, dst, 32);
        lua_pushlstring(L, dst, 64);
        return 1;
    }
    return 0;
}

/**
Calculate hmac_sha256 value
@api crypto.hmac_sha256(str, key)
@string the string to be calculated
@string key
@return string hex string of calculated hmac_sha256 value
@usage
-- Calculate hmac_sha256 of string "abc"
log.info("hmac_sha256", crypto.hmac_sha256("abc", "1234567890"))*/
static int l_crypto_hmac_sha256(lua_State *L) {
    size_t str_size = 0;
    size_t key_size = 0;
    const char* str = luaL_checklstring(L, 1, &str_size);
    const char* key = luaL_checklstring(L, 2, &key_size);
    char tmp[64] = {0};
    char dst[64] = {0};

    if (key_size > 64) {
        luat_crypto_sha256_simple(key, key_size, dst);
        key = (const char*)dst;
        key_size = 64;
    }

    if (luat_crypto_hmac_sha256_simple(str, str_size, key, key_size, tmp) == 0) {
        fixhex(tmp, dst, 32);
        lua_pushlstring(L, dst, 64);
        return 1;
    }
    return 0;
}

//---

/**
Calculate sha512 value
@api crypto.sha512(str)
@string the string to be calculated
@return string hex string of calculated sha512 value
@usage
-- Calculate the sha512 of the string "abc"
log.info("sha512", crypto.sha512("abc"))*/
static int l_crypto_sha512(lua_State *L) {
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);
    char tmp[128] = {0};
    char dst[128] = {0};
    if (luat_crypto_sha512_simple(str, size, tmp) == 0) {
        fixhex(tmp, dst, 64);
        lua_pushlstring(L, dst, 128);
        return 1;
    }
    return 0;
}

/**
Calculate hmac_sha512 value
@api crypto.hmac_sha512(str, key)
@string the string to be calculated
@string key
@return string hex string of calculated hmac_sha512 value
@usage
-- Calculate hmac_sha512 of string "abc"
log.info("hmac_sha512", crypto.hmac_sha512("abc", "1234567890"))*/
static int l_crypto_hmac_sha512(lua_State *L) {
    size_t str_size = 0;
    size_t key_size = 0;
    const char* str = luaL_checklstring(L, 1, &str_size);
    const char* key = luaL_checklstring(L, 2, &key_size);
    char tmp[128] = {0};
    char dst[128] = {0};

    if (key_size > 128) {
        luat_crypto_sha512_simple(key, key_size, dst);
        key = (const char*)dst;
        key_size = 128;
    }

    if (luat_crypto_hmac_sha512_simple(str, str_size, key, key_size, tmp) == 0) {
        fixhex(tmp, dst, 64);
        lua_pushlstring(L, dst, 128);
        return 1;
    }
    return 0;
}

int l_crypto_cipher_xxx(lua_State *L, uint8_t flags) {
    luat_crypto_cipher_ctx_t cctx = {0};
    cctx.cipher = luaL_optlstring(L, 1, "AES-128-ECB", &cctx.cipher_size);
    cctx.pad = luaL_optlstring(L, 2, "PKCS7", &cctx.pad_size);
    cctx.str = luaL_checklstring(L, 3, &cctx.str_size);
    cctx.key = luaL_checklstring(L, 4, &cctx.key_size);
    cctx.iv = luaL_optlstring(L, 5, "", &cctx.iv_size);
    cctx.flags = flags;

    luaL_Buffer buff;
    luaL_buffinitsize(L, &buff, cctx.str_size + 16);
    cctx.outbuff = buff.b;

    int ret = luat_crypto_cipher_xxx(&cctx);
    if (ret) {
        return 0;
    }
    luaL_pushresultsize(&buff, cctx.outlen);
    return 1;
}

/**
Symmetric encryption
@api crypto.cipher_encrypt(type, padding, str, key, iv)
@string algorithm name, such as AES-128-ECB/AES-128-CBC, please refer to crypto.cipher_list()
@string alignment, supports PKCS7/ZERO/ONE_AND_ZEROS/ZEROS_AND_LEN/NONE
@string Data to be encrypted
@string key, the key length of the corresponding algorithm is required
@string IV value, required by non-ECB algorithm
@return string encrypted string
@usage
-- Calculate AES
local data = crypto.cipher_encrypt("AES-128-ECB", "PKCS7", "1234567890123456", "1234567890123456")
local data2 = crypto.cipher_encrypt("AES-128-CBC", "PKCS7", "1234567890123456", "1234567890123456", "1234567890666666")*/
int l_crypto_cipher_encrypt(lua_State *L) {
    return l_crypto_cipher_xxx(L, 1);
}
/**
Symmetric decryption
@api crypto.cipher_decrypt(type, padding, str, key, iv)
@string algorithm name, such as AES-128-ECB/AES-128-CBC, please refer to crypto.cipher_list()
@string alignment, supports PKCS7/ZERO/ONE_AND_ZEROS/ZEROS_AND_LEN/NONE
@string data to be decrypted
@string key, the key length of the corresponding algorithm is required
@string IV value, required by non-ECB algorithm
@return string decrypted string
@usage
-- Encrypt with AES, then decrypt with AES
local data = crypto.cipher_encrypt("AES-128-ECB", "PKCS7", "1234567890123456", "1234567890123456")
local data2 = crypto.cipher_decrypt("AES-128-ECB", "PKCS7", data, "1234567890123456")
--The hex of data is 757CCD0CDC5C90EADBEEECF638DD0000
--The value of data2 is 1234567890123456*/
int l_crypto_cipher_decrypt(lua_State *L) {
    return l_crypto_cipher_xxx(L, 0);
}

#include "crc.h"

/**
Calculate CRC16
@api crypto.crc16(method, data, poly, initial, finally, inReversem outReverse)
@string CRC16 mode ("IBM", "MAXIM", "USB", "MODBUS", "CCITT", "CCITT-FALSE", "X25", "XMODEM", "DNP", "USER-DEFINED" )
@string string
@int polyvalue
@int initial value
@int finally value
@int Input to reverse, 1 to reverse, default 0 not to reverse
@int Input to reverse, 1 to reverse, default 0 not to reverse
@return int corresponding CRC16 value
@usage
-- Calculate CRC16
local crc = crypto.crc16("")*/
static int l_crypto_crc16(lua_State *L)
{
    size_t inputlen;
    const unsigned char *inputData;
    const char  *inputmethod = (const char*)luaL_checkstring(L, 1);
    if(lua_isuserdata(L, 2))
    {
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        inputlen = buff->len - buff->cursor;
        inputData = (const unsigned char *)(buff->addr + buff->cursor);
    }else{
        inputData = (const unsigned char*)lua_tolstring(L,2,&inputlen);
    }
    uint16_t poly = luaL_optnumber(L,3,0x0000);
    uint16_t initial = luaL_optnumber(L,4,0x0000);
    uint16_t finally = luaL_optnumber(L,5,0x0000);
    uint8_t inReverse = luaL_optnumber(L,6,0);
    uint8_t outReverse = luaL_optnumber(L,7,0);
    lua_pushinteger(L, calcCRC16(inputData, inputmethod,inputlen,poly,initial,finally,inReverse,outReverse));
    return 1;
}

/**
Directly calculate the crc16 value of modbus
@api crypto.crc16_modbus(data, start)
@string data
@int initialization value, default 0xFFFF
@return int corresponding CRC16 value
@usage
-- Calculate CRC16 modbus
local crc = crypto.crc16_modbus(data)
-- 2023.11.06 New initial value setting
crc = crypto.crc16_modbus(data, 0xFFFF)*/
static int l_crypto_crc16_modbus(lua_State *L)
{
    size_t len = 0;
    const unsigned char *inputData = (const unsigned char*)luaL_checklstring(L, 1, &len);
    uint16_t crc_init = luaL_optinteger(L, 2, 0xFFFF);

    lua_pushinteger(L, calcCRC16_modbus(inputData, len, crc_init));
    return 1;
}

/**
Calculate crc32 value
@api crypto.crc32(data)
@string data
@return int corresponding CRC32 value
@usage
-- Calculate CRC32
local crc = crypto.crc32(data)*/
static int l_crypto_crc32(lua_State *L)
{
    size_t len = 0;
    const unsigned char *inputData = (const unsigned char*)luaL_checklstring(L, 1, &len);

    lua_pushinteger(L, calcCRC32(inputData, len));
    return 1;
}

/**
Calculate crc8 value
@api crypto.crc8(data, poly, start, revert)
@string data
@int crc polynomial, optional, if not written, all parameters except data will be ignored
@int crc initial value, optional, default 0
@boolean Whether reverse order processing is required, default no
@return int corresponding CRC8 value
@usage
-- Calculate CRC8
local crc = crypto.crc8(data)
local crc = crypto.crc8(data, 0x31, 0xff, false)*/
static int l_crypto_crc8(lua_State *L)
{
    size_t len = 0;
    const unsigned char *inputData = (const unsigned char*)luaL_checklstring(L, 1, &len);
    if (!lua_isinteger(L, 2)) {
        lua_pushinteger(L, calcCRC8(inputData, len));
    } else {
    	uint8_t poly = lua_tointeger(L, 2);
    	uint8_t start = luaL_optinteger(L, 3, 0);
    	uint8_t is_rev = 0;
    	if (lua_isboolean(L, 4)) {
    		is_rev = lua_toboolean(L, 4);
    	}
    	uint8_t i;
    	uint8_t CRC8 = start;
		uint8_t *Src = (uint8_t *)inputData;
		if (is_rev)
		{
			poly = 0;
			for (i = 0; i < 8; i++)
			{
				if (start & (1 << (7 - i)))
				{
					poly |= 1 << i;
				}
			}
			while (len--)
			{

				CRC8 ^= *Src++;
				for (i = 0; i < 8; i++)
				{
					if ((CRC8 & 0x01))
					{
						CRC8 >>= 1;
						CRC8 ^= poly;
					}
					else
					{
						CRC8 >>= 1;
					}
				}
			}
		}
		else
		{
			while (len--)
			{

				CRC8 ^= *Src++;
				for (i = 8; i > 0; --i)
				{
					if ((CRC8 & 0x80))
					{
						CRC8 <<= 1;
						CRC8 ^= poly;
					}
					else
					{
						CRC8 <<= 1;
					}
				}
			}
		}
		lua_pushinteger(L, CRC8);
    }
    return 1;
}



static inline unsigned char crc7(const unsigned char* message, int length, unsigned char CRCPoly, unsigned char CRC)
{
    // unsigned char CRCPoly = 0xe5;
    unsigned char CRCTable[256];
    // unsigned char CRC = 0x00;
    for (int i = 0; i < 256; i++){
        CRCTable[i] = (i & 0x80) ? i ^ CRCPoly : i;
        for (int j = 1; j < 8; j++){
            CRCTable[i] <<= 1;
            if (CRCTable[i] & 0x80)
                CRCTable[i] ^= CRCPoly;
        }
    }
    for (int i = 0; i < length; i++)
        CRC = CRCTable[(CRC << 1) ^ message[i]];
    return CRC<< 1;
}

/**
Calculate crc7 value
@api crypto.crc7(data, poly, start)
@string data
@int crc polynomial, optional, default 0xE5
@int crc initial value, optional, default 0x00
@return int corresponding CRC7 value
@usage
-- Calculate CRC7, this API was added on 2023.10.07
local crc = crypto.crc7(data)
local crc = crypto.crc7(data, 0x31, 0xff)*/
static int l_crypto_crc7(lua_State* L) {
    size_t len = 0;
    const unsigned char *inputData = (const unsigned char*)luaL_checklstring(L, 1, &len);
    unsigned char poly = luaL_optinteger(L, 2, 0xe5);
    unsigned char start = luaL_optinteger(L, 3, 0);
    unsigned char result = crc7(inputData, len, poly, start);
    lua_pushinteger(L, result);
    return 1;
}

/**
Generate true random numbers
@api crypto.trng(len)
@int data length
@return string specifies a random number string
@usage
-- Generate a 32-bit random number ir
local r = crypto.trng(4)
local _, ir = pack.unpack(r, "I")*/
static int l_crypto_trng(lua_State *L) {
    int ret = 0;
    size_t len = luaL_checkinteger(L, 1);
    if (len < 1) {
        return 0;
    }
    if (len > 128)
        len = 128;
    char buff[128];
    ret = luat_crypto_trng(buff, len);
    if(ret ==0){
        lua_pushlstring(L, buff, len);
        return 1;
    }
    return 0;
}

/**
Calculation result of TOTP dynamic password
@api crypto.totp(secret,time)
@string The key provided by the website (which is the result of BASE32 encoding)
@int optional, timestamp, default current time
@return int The calculated six-digit result returns nil if the calculation fails.
@usage
--Calculated using current system time
local otp = crypto.totp("asdfassdfasdfass")*/
static int l_crypto_totp(lua_State *L) {
    size_t len = 0;
    const char* secret_base32 = luaL_checklstring(L,1,&len);

    char * secret = (char *)luat_heap_malloc(len+1);
    len = (size_t)luat_str_base32_decode((const uint8_t * )secret_base32,(uint8_t*)secret,len+1);

    uint64_t t = 0;
    if (lua_isinteger(L, 2)) {
        t = (uint64_t)(luaL_checkinteger(L, 2))/30;
    }
    else {
        t = (uint64_t)(time(NULL)/30);
    }
    uint8_t data[sizeof(uint64_t)] = {0};
    for(size_t i=0;i<sizeof(uint64_t);i++)
        data[sizeof(uint64_t)-1-i] = *(((uint8_t*)&t)+i);
    uint8_t hmac[20] = {0};
    int ret = luat_crypto_hmac_sha1_simple((const char *)data, sizeof(data), (const char *)secret, len, hmac);
    luat_heap_free(secret);
    if(ret == 0)
    {
        uint8_t offset = hmac[19] & 0x0f;
        uint32_t r = (
                        ((uint32_t)((hmac[offset + 0] & 0x7f)) << 24) |
                        ((uint32_t)((hmac[offset + 1] & 0xff)) << 16) |
                        ((uint32_t)((hmac[offset + 2] & 0xff)) << 8) |
                        ((uint32_t)(hmac[offset + 3] & 0xff))
                    ) % 1000000;
        lua_pushinteger(L,r);
        return 1;
    }
    return 0;
}

/**
Base64 encode the data
@api crypto.base64_encode(data)
@string data to be encoded
@return string encoded data
@usage
-- This function is the same as string.toBase64
local data = "123"
local bdata = crypto.base64_encode(data)
log.info("base64", "encode", data, bdata)
data = crypto.base64_decode(data)
log.info("base64", "decode", data, bdata)*/
int l_str_toBase64(lua_State *L);

/**
Base64 decode the data
@api crypto.base64_decode(data)
@string data to be decoded
@return string decoded data
@usage
-- This function is the same as string.fromBase64
local data = "123"
local bdata = crypto.base64_encode(data)
log.info("base64", "encode", data, bdata)
data = crypto.base64_decode(data)
log.info("base64", "decode", data, bdata)*/
int l_str_fromBase64(lua_State *L);

/**
Get the list of ciphers supported by the current firmware
@api crypto.cipher_list()
@return table cipher list supported by this firmware, string array
@usage
-- This API was added on 2022.07.27
local ciphers = crypto.cipher_list()
if ciphers then
    log.info("crypto", "ciphers list", json.encode(ciphers))
end*/
int l_crypto_cipher_list(lua_State *L) {
    const char* list[64] = {0};
    size_t len = 64;
    lua_newtable(L);
    int ret = luat_crypto_cipher_list(list, &len);
    if (ret == 0) {
        for (size_t i = 0; i < len; i++){
            lua_pushstring(L, list[i]);
            lua_seti(L, -2, i + 1);
        }
    }
    else {
        LLOGD("bsp not support cipher_list");
    }
    return 1;
}

/**
Get the list of cipher suites supported by the current firmware
@api crypto.cipher_suites()
@return table list of cipher suites supported by this firmware, string array
@usage
-- This API was added on 2022.11.16
local suites = crypto.cipher_suites()
if suites then
    log.info("crypto", "ciphers suites", json.encode(suites))
end*/
int l_crypto_cipher_suites(lua_State *L) {
    const char* list[128] = {0};
    size_t len = 128;
    lua_newtable(L);
    int ret = luat_crypto_cipher_suites(list, &len);
    if (ret == 0) {
        for (size_t i = 0; i < len; i++){
            lua_pushstring(L, list[i]);
            lua_seti(L, -2, i + 1);
        }
    }
    else {
        LLOGD("bsp not support cipher_suites");
    }
    return 1;
}

/**
Calculate the hash value of the file (md5/sha1/sha256 and hmac format)
@api crypto.md_file(tp, path, hmac)
@string hash type, upper and lower letters, such as "MD5" "SHA1" "SHA256"
@string file path, for example /luadb/logo.jpg
@string hmac value, optional
@return string HEX hash value, if it fails, there will be no return value.
@usage

-- No hmac hash value
log.info("md5", crypto.md_file("MD5", "/luadb/logo.jpg"))
log.info("sha1", crypto.md_file("SHA1", "/luadb/logo.jpg"))
log.info("sha256", crypto.md_file("SHA256", "/luadb/logo.jpg"))

-- Hash value with hmac
log.info("hmac_md5", crypto.md_file("MD5", "/luadb/logo.jpg", "123456"))
log.info("hmac_sha1", crypto.md_file("SHA1", "/luadb/logo.jpg", "123456"))
log.info("hmac_sha256", crypto.md_file("SHA256", "/luadb/logo.jpg", "123456"))*/
static int l_crypto_md_file(lua_State *L) {
    size_t key_len = 0;
    size_t path_size = 0;
    const char* key = NULL;
    const char *md = luaL_checkstring(L, 1);
    const char* path = luaL_checklstring(L, 2, &path_size);
    if (path_size < 2)
        return 0;
    if (lua_type(L, 3) == LUA_TSTRING) {
        key = luaL_checklstring(L, 3, &key_len);
    }
    char buff[128] = {0};
    char output[64];

    int ret = luat_crypto_md_file(md, output, key, key_len, path);
    if (ret < 1) {
        return 0;
    }

    fixhex(output, buff, ret);
    lua_pushlstring(L, buff, ret *2);
    return 1;
}

/**
Calculate the hash value of data (md5/sha1/sha256 and hmac format)
@api crypto.md(tp, data, hmac)
@string hash type, upper and lower letters, such as "MD5" "SHA1" "SHA256"
@string data to be processed
@string hmac value, optional
@return string HEX hash value, if it fails, there will be no return value.
@usage

-- No hmac hash value
log.info("md5", crypto.md("MD5", "1234567890"))
log.info("sha1", crypto.md("SHA1", "1234567890"))
log.info("sha256", crypto.md("SHA256", "1234567890"))

-- Hash value with hmac
log.info("hmac_md5", crypto.md("MD5", "1234567890", "123456"))
log.info("hmac_sha1", crypto.md("SHA1", "1234567890", "123456"))
log.info("hmac_sha256", crypto.md("SHA256", "1234567890", "123456"))*/
static int l_crypto_md(lua_State *L) {
    size_t key_len = 0;
    size_t data_size = 0;
    const char* key = NULL;
    const char *md = luaL_checkstring(L, 1);
    const char* data = luaL_checklstring(L, 2, &data_size);
    if (lua_type(L, 3) == LUA_TSTRING) {
        key = luaL_checklstring(L, 3, &key_len);
    }
    char buff[128] = {0};
    char output[64];

    int ret = luat_crypto_md(md, data, data_size, output, key, key_len);
    if (ret < 1) {
        return 0;
    }

    fixhex(output, buff, ret);
    lua_pushlstring(L, buff, ret *2);
    return 1;
}

/*Create a stream for streaming hashing
@api crypto.hash_init(tp)
@string hash type, uppercase letters, such as "MD5" "SHA1" "SHA256"
@string hmac value, optional
@return userdata successfully returns a data structure, otherwise returns nil
@usage
--hmac-free hash stream
local md5_stream = crypto.hash_init("MD5")
local sha1_stream = crypto.hash_init("SHA1")
local sha256_stream = crypto.hash_init("SHA256")

-- Hash stream with hmac
local md5_stream = crypto.hash_init("MD5", "123456")
local sha1_stream = crypto.hash_init("SHA1", "123456")
local sha256_stream = crypto.hash_init("SHA256", "123456")*/
static int l_crypt_hash_init(lua_State *L) {
    luat_crypt_stream_t *stream = (luat_crypt_stream_t *)lua_newuserdata(L, sizeof(luat_crypt_stream_t));
    if(stream == NULL) {
        return 0;
    } else {
        memset(stream, 0x00, sizeof(luat_crypt_stream_t));
        const char* key = NULL;
        const char* md = luaL_checkstring(L, 1);
        if(lua_type(L, 2) == LUA_TSTRING) {
            key = luaL_checklstring(L, 2, &(stream->key_len));
        }
        int ret = luat_crypto_md_init(md, key, stream);
        if (ret < 0) {
            return 0;
        } else {
            luaL_setmetatable(L, LUAT_CRYPTO_TYPE);
        }
    }
    return 1;
}

/*Streaming hash update data
@api crypto.hash_update(stream, data)
@userdata stream created by crypto.hash_init(), required
@string Data to be calculated, required
@return None
@usage
crypto.hash_update(stream, "OK")*/
static int l_crypt_hash_update(lua_State *L) {
    luat_crypt_stream_t *stream = (luat_crypt_stream_t *)luaL_checkudata(L, 1, LUAT_CRYPTO_TYPE);
    size_t data_len = 0;
    const char *data = luaL_checklstring(L, 2, &data_len);
    luat_crypto_md_update(data, data_len ,stream);
    return 0;
}

/*Get the streaming hash check value and release the created stream
@api crypto.hash_finish(stream)
Stream created by @userdata crypto.hash_init(), required
@return string Successfully returns the hex string of the calculated streaming hash value. Failure to return no return
@usage
local hashResult = crypto.hash_finish(stream)*/
static int l_crypt_hash_finish(lua_State *L) {
    luat_crypt_stream_t *stream = (luat_crypt_stream_t *)luaL_checkudata(L, 1, LUAT_CRYPTO_TYPE);
    char buff[128] = {0};
    char output[64];
    int ret = luat_crypto_md_finish(output, stream);
    //LLOGD("finish result %d", ret);
    if (ret < 1) {
        return 0;
    }
    fixhex(output, buff, ret);
    lua_pushlstring(L, buff, ret * 2);
    return 1;
}


/*Calculate checksum checksum
@api crypto.checksum(data, mode)
@string Data to be calculated, required
@int mode, accumulation mode, 0 - exclusive OR, 1 - accumulation, default is 0
@return int checksum value, checksum
@usage
-- This function was added on 2022.12.28
-- Simply calculate the checksum value
local ck = crypto.checksum("OK")
log.info("checksum", "ok", string.format("%02X", ck))
--The second parameter mode was added on May 23, 2023*/
static int l_crypt_checksum(lua_State *L) {
    size_t len = 0;
    uint8_t checksum = 0x00;
    uint8_t tmp = 0;
    const char* sentence = luaL_checklstring(L, 1, &len);
    int mode = luaL_optinteger(L, 2, 0);
    // LLOGD("mode %d", mode);
    for (size_t i = 0; i < len; i++)
    {
        tmp = *sentence;
        if (mode == 1) {
            checksum += tmp;
        }
        else {
            checksum ^= tmp;
        }
        // LLOGD("> %02X > %02X", checksum, tmp);
        sentence ++;
    }
    lua_pushinteger(L, checksum);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_crypto[] =
{
    { "md5" ,           ROREG_FUNC(l_crypto_md5            )},
    { "sha1" ,          ROREG_FUNC(l_crypto_sha1           )},
    { "sha256" ,        ROREG_FUNC(l_crypto_sha256         )},
    { "sha512" ,        ROREG_FUNC(l_crypto_sha512         )},
    { "hmac_md5" ,      ROREG_FUNC(l_crypto_hmac_md5       )},
    { "hmac_sha1" ,     ROREG_FUNC(l_crypto_hmac_sha1      )},
    { "hmac_sha256" ,   ROREG_FUNC(l_crypto_hmac_sha256    )},
    { "hmac_sha512" ,   ROREG_FUNC(l_crypto_hmac_sha512    )},
    { "cipher" ,        ROREG_FUNC(l_crypto_cipher_encrypt )},
    { "cipher_encrypt" ,ROREG_FUNC(l_crypto_cipher_encrypt )},
    { "cipher_decrypt" ,ROREG_FUNC(l_crypto_cipher_decrypt )},
    { "cipher_list" ,   ROREG_FUNC(l_crypto_cipher_list     )},
    { "cipher_suites",  ROREG_FUNC(l_crypto_cipher_suites)},
    { "crc16",          ROREG_FUNC(l_crypto_crc16          )},
    { "crc16_modbus",   ROREG_FUNC(l_crypto_crc16_modbus   )},
    { "crc32",          ROREG_FUNC(l_crypto_crc32          )},
    { "crc8",           ROREG_FUNC(l_crypto_crc8           )},
    { "crc7",           ROREG_FUNC(l_crypto_crc7           )},
    { "trng",           ROREG_FUNC(l_crypto_trng           )},
    { "totp",           ROREG_FUNC(l_crypto_totp           )},
    { "base64_encode",  ROREG_FUNC(l_str_toBase64)},
    { "base64_decode",  ROREG_FUNC(l_str_fromBase64)},
    { "md_file",        ROREG_FUNC(l_crypto_md_file)},
    { "md",             ROREG_FUNC(l_crypto_md)},
    { "checksum",       ROREG_FUNC(l_crypt_checksum)},
    { "hash_init",      ROREG_FUNC(l_crypt_hash_init)},
    { "hash_update",    ROREG_FUNC(l_crypt_hash_update)},
    { "hash_finish",    ROREG_FUNC(l_crypt_hash_finish)},

	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_crypto( lua_State *L ) {
    luat_newlib2(L, reg_crypto);
    luaL_newmetatable(L, LUAT_CRYPTO_TYPE);
    lua_pop(L, 1);
    return 1;
}

//Add several default implementations
#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK int luat_crypto_trng(char* buff, size_t len) {
    memset(buff, 0, len);
    return 0;
}

#endif
