/*@Modulesrsa
@summary RSA encryption and decryption
@version 1.0
@date 2022.11.03
@demorsa
@tagLUAT_USE_RSA
@usage
-- Please generate the private key and public key on your computer. The current maximum support is 4096bit. Generally speaking, 2048bit is enough.
-- openssl genrsa -out privkey.pem 2048
-- openssl rsa -in privkey.pem -pubout -out public.pem
-- privkey.pem is the private key, public.pem is the public key
--Private key is used for encryption and signature, usually kept confidential and placed on the server side
-- The public key is used for decryption and signature verification. It can generally be made public and placed on the device side.

-- In order to demonstrate API usage, the private key is also placed on the device.

local res = rsa.encrypt((io.readFile("/luadb/public.pem")), "abc")
--Print results
log.info("rsa", "encrypt", res and #res or 0, res and res:toHex() or "")

--The following is decryption, which is usually not performed on the device side. It is mainly used to demonstrate usage and will be very slow.
if res then
    -- Read the private key, then decode the data
    local dst = rsa.decrypt((io.readFile("/luadb/privkey.pem")), res, "")
    log.info("rsa", "decrypt", dst and #dst or 0, dst and dst:toHex() or "")
end

-- Demonstrate signature and verification
local hash = crypto.sha1("1234567890"):fromHex()
-- Signing is usually slow, usually done by the server
local sig = rsa.sign((io.readFile("/luadb/privkey.pem")), rsa.MD_SHA1, hash, "")
log.info("rsa", "sign", sig and #sig or 0, sig and sig:toHex() or "")
if sig then
    --Verification is very fast
    local ret = rsa.verify((io.readFile("/luadb/public.pem")), rsa.MD_SHA1, hash, sig)
    log.info("rsa", "verify", ret)
end*/

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_crypto.h"
#include "luat_fs.h"

#define LUAT_LOG_TAG "rs"
#include "luat_log.h"

#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"

static int myrand( void *rng_state, unsigned char *output, size_t len ) {
    (void)rng_state;
    luat_crypto_trng((char*)output, len);
    return 0;
}

/*RSA encryption
@api rsa.encrypt(key, data)
@string public key data, only supports PEM format
@string The data to be encrypted cannot exceed half the number of digits in the public key. For example, a 2048-bit public key can only encrypt 128 bytes of data.
@return string The data after successful encryption. If it fails, nil will be returned.
@usage
-- "abc" in the code below is the data to be encrypted
local res = rsa.encrypt((io.readFile("/luadb/public.pem")), "abc")
--Print results
log.info("rsa", "encrypt", res and #res or 0, res and res:toHex() or "")*/
static int l_rsa_encrypt(lua_State* L) {
    int ret = 0;
    size_t ilen = 0;
    size_t keylen = 0;
    size_t olen = 0;
    char buff[1024];
    const char* key = luaL_checklstring(L, 1, &keylen);
    const char* data = luaL_checklstring(L, 2, &ilen);

    mbedtls_pk_context ctx_pk;
    mbedtls_pk_init(&ctx_pk);
    ret = mbedtls_pk_parse_public_key(&ctx_pk, (const unsigned char*)key, keylen + 1);
    if (ret) {
        mbedtls_pk_free(&ctx_pk);
        LLOGW("bad public key %04X", -ret);
        return 0;
    }

    ret = mbedtls_pk_encrypt(&ctx_pk, (const unsigned char*)data, ilen, (unsigned char*)buff, &olen, 1024, myrand, NULL);
    mbedtls_pk_free(&ctx_pk);

    if (ret) {
        LLOGW("mbedtls_rsa_pkcs1_encrypt %04X", -ret);
        return 0;
    }
    lua_pushlstring(L, buff, olen);
    return 1;
}

/*RSA decryption
@api rsa.decrypt(key, data, pwd)
@string private key data, only supports PEM format
@string data to be decrypted
@string Password of private key, optional
@return string The data after successful decryption. If it fails, nil will be returned.
@usage
-- Note that decryption is usually very slow, it is recommended to do it on the server side
-- res is the data to be decrypted
local dst = rsa.decrypt((io.readFile("/luadb/privkey.pem")), res, "")
log.info("rsa", "decrypt", dst and #dst or 0, dst and dst:toHex() or "")*/
static int l_rsa_decrypt(lua_State* L) {
    int ret = 0;
    size_t ilen = 0;
    size_t keylen = 0;
    size_t rsa_len = 0;
    size_t olen = 0;
    size_t pwdlen = 0;
    char buff[1024];
    const char* key = luaL_checklstring(L, 1, &keylen);
    const char* data = luaL_checklstring(L, 2, &ilen);
    const char* pwd = luaL_optlstring(L, 3, "", &pwdlen);

    mbedtls_pk_context ctx_pk;
    mbedtls_pk_init(&ctx_pk);
    ret = mbedtls_pk_parse_key(&ctx_pk, (const unsigned char*)key, keylen + 1, (const unsigned char*)pwd, pwdlen
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    , myrand, NULL
#endif
    );
    if (ret) {
        mbedtls_pk_free(&ctx_pk);
        LLOGW("bad private key %04X", -ret);
        return 0;
    }
    rsa_len = (mbedtls_pk_get_bitlen(&ctx_pk) + 7 ) / 8;
    if (rsa_len != ilen) {
        mbedtls_pk_free(&ctx_pk);
        LLOGW("data len NOT match expect %d but %d", rsa_len, ilen);
        return 0;
    }
    ret = mbedtls_pk_decrypt(&ctx_pk, (const unsigned char*)data, ilen, (unsigned char*)buff, &olen, 1024, myrand, NULL);
    mbedtls_pk_free(&ctx_pk);

    if (ret) {
        LLOGW("mbedtls_rsa_pkcs1_decrypt %04X", -ret);
        return 0;
    }
    lua_pushlstring(L, buff, olen);
    return 1;
}

/*RSA signature verification
@api rsa.verify(key, md, hash, sig)
@string public key data, only supports PEM format
@int signature mode, such as rsa.MD_SHA1, rsa.MD_SHA256
@string hash data, if it is a HEX string, remember to convert fromHex to binary data
@string sig data, if it is a HEX string, remember to convert fromHex to binary data
@return bool Returns true if valid, false otherwise, nil if error occurs
@usage
local ret = rsa.verify((io.readFile("/luadb/public.pem")), rsa.MD_SHA1, hash, sig)
log.info("rsa", "verify", ret)*/
static int l_rsa_verify(lua_State* L) {
    int ret = 0;
    size_t hash_len = 0;
    size_t sig_len = 0;
    size_t keylen = 0;

    const char* key =  luaL_checklstring(L,  1, &keylen);
    mbedtls_md_type_t md = luaL_checkinteger(L, 2);
    const char* hash = luaL_checklstring(L, 3, &hash_len);
    const char* sig =  luaL_checklstring(L,  4, &sig_len);

    mbedtls_pk_context ctx_pk;
    mbedtls_pk_init(&ctx_pk);
    ret = mbedtls_pk_parse_public_key(&ctx_pk, (const unsigned char*)key, keylen + 1);
    if (ret) {
        mbedtls_pk_free(&ctx_pk);
        LLOGW("bad public key %04X", -ret);
        return 0;
    }

    ret = mbedtls_pk_verify(&ctx_pk, md, (const unsigned char*)hash, hash_len, (const unsigned char*)sig, sig_len);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}


/*RSA signature
@api rsa.sign(key, md, hash, pwd)
@string private key data, only supports PEM format
@int signature mode, such as rsa.MD_SHA1, rsa.MD_SHA256
@string hash data, if it is a HEX string, remember to convert fromHex to binary data
@string private key password, optional
@return string returns sig data successfully, otherwise returns nil
@usage
local sig = rsa.sign((io.readFile("/luadb/privkey.pem")), rsa.MD_SHA1, hash, "")
log.info("rsa", "sign", sig and #sig or 0, sig and sig:toHex() or "")*/
static int l_rsa_sign(lua_State* L) {
    int ret = 0;
    size_t pwdlen = 0;
    size_t hash_len = 0;
    size_t sig_len = 0;
    size_t keylen = 0;

    char sig[MBEDTLS_PK_SIGNATURE_MAX_SIZE] = {0};

    const char* key =  luaL_checklstring(L,  1, &keylen);
    mbedtls_md_type_t md = luaL_checkinteger(L, 2);
    const char* hash = luaL_checklstring(L, 3, &hash_len);
    const char* pwd = luaL_optlstring(L, 4, "", &pwdlen);

    mbedtls_pk_context ctx_pk;
    mbedtls_pk_init(&ctx_pk);
    ret = mbedtls_pk_parse_key(&ctx_pk, (const unsigned char*)key, keylen + 1, (const unsigned char*)pwd, pwdlen
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    , myrand, NULL
#endif
    );
    if (ret) {
        mbedtls_pk_free(&ctx_pk);
        LLOGW("bad private key %04X", -ret);
        return 0;
    }
    ret = mbedtls_pk_sign(&ctx_pk, md, (const unsigned char*)hash, hash_len, (unsigned char*)sig, 
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    MBEDTLS_PK_SIGNATURE_MAX_SIZE,
#endif 
    &sig_len, myrand, NULL);
    mbedtls_pk_free(&ctx_pk);

    if (ret) {
        LLOGW("mbedtls_pk_sign %04X", -ret);
        return 0;
    }
    lua_pushlstring(L, sig, sig_len);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_rsa[] =
{
    { "encrypt" ,        ROREG_FUNC(l_rsa_encrypt)},
    { "decrypt" ,        ROREG_FUNC(l_rsa_decrypt)},

    { "verify",          ROREG_FUNC(l_rsa_verify)},
    { "sign",            ROREG_FUNC(l_rsa_sign)},

    //@const MD_MD5 MD5 mode, used for signature and verification
    { "MD_MD5",          ROREG_INT(MBEDTLS_MD_MD5)},
    //@const MD_SHA1 SHA1 mode, used for signature and verification
    { "MD_SHA1",         ROREG_INT(MBEDTLS_MD_SHA1)},
    //@const MD_SHA224 SHA224 mode, used for signature and verification
    { "MD_SHA224",       ROREG_INT(MBEDTLS_MD_SHA224)},
    //@const MD_SHA256 SHA256 mode, used for signature and verification
    { "MD_SHA256",       ROREG_INT(MBEDTLS_MD_SHA256)},
	{ NULL,              ROREG_INT(0) }
};

LUAMOD_API int luaopen_rsa( lua_State *L ) {
    luat_newlib2(L, reg_rsa);
    return 1;
}
