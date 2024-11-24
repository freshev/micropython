
/*@Modules gmssl
@summary National Secret Algorithm (SM2/SM3/SM4)
@version 1.1
@date 2023.03.02
@author chenxudong1208
@demo gmssl
@tag LUAT_USE_GMSSL
@usage
-- This library supports three algorithms SM2 SM3 SM4
-- In theory, it can be expanded to support SM9 algorithm
-- SM1 is not supported because it is a hardware algorithm and not implemented by software.*/
#include "luat_base.h"
#include "luat_mem.h"
#include "luat_str.h"
#include <time.h>
#include "luat_zbuff.h"
#include "gmssl/sm2.h"
#include "gmssl/sm3.h"
#include "gmssl/sm4.h"
// #include "mbedtls/hmac_drbg.h"

#define LUAT_LOG_TAG "sm"
#include "luat_log.h"
#define SM3_DIGEST_LENGTH    32
#define SM4_BLOCK_LEN 16
#define SM2_STR_LEN 300
#define HEX_CODE 16

extern void luat_str_fromhex(const char* str, size_t len, char* buff);

static void DeletePaddingBuf(luaL_Buffer *B, const char *pPadding, size_t nBufLen, uint8_t *pBuf, uint8_t pPaddLen)
{
    uint8_t nPadLen;
    if((strcmp(pPadding, "PKCS5")==0) || (strcmp(pPadding, "PKCS7")==0))
    {
        nPadLen = *(pBuf+nBufLen-1);
        //printf("aes DeletePaddingBuf length=%d\n", nPadLen);
        if((pPaddLen-nPadLen) >= 0)
        {
            luaL_addlstring(B, (char*)pBuf, nBufLen-nPadLen);
        }
    }
    else if(strcmp(pPadding, "ZERO")==0)
    {                        
        uint8_t *pEnd = pBuf+nBufLen-1;
        nPadLen = 0;
        while(1)
        {
            if(*pEnd == 0)
            {
                nPadLen++;
                if(nPadLen == pPaddLen)
                {
                    break;
                }
                pEnd--;
            }
            else
            {
                break;
            }
        }
        //printf("aes DeletePaddingBuf length=%d\n", nPadLen);
        if((pPaddLen-nPadLen) >= 0)
        {
            luaL_addlstring(B, (char*)pBuf, nBufLen-nPadLen);
        }
    }
    else
    {
        luaL_addlstring(B, (char*)pBuf, nBufLen);
    }
}


/*sm2 algorithm encryption
@api sm.sm2encrypt(pkx,pky,data, mode, mode2)
@string public key x, required. HEX string
@string public key y, required. HEX string
@string Data to be calculated, required, up to 32 bytes, non-HEX string
@boolean output mode, default false. false-GMSSL default format DER, true-website compatibility mode
@boolean standard version, default false. false-C1C3C2 new international, true-C1C2C3 old international
@return string The encrypted string is output as is without HEX conversion. If the encryption fails, nil or an empty string will be returned.
@usage
-- Tips that the mode/mode2 parameter is new on 2023.10.17
-- Since the implementation of SM2 on each platform is different, be sure to refer to the demo for usage.*/
static int l_sm2_encrypt(lua_State *L)
{    
    // size_t randLen = 0;
    size_t pkxLen = 0;
    size_t pkyLen = 0;
    size_t pBufLen = 0;
    const char *pkx = lua_tolstring(L, 1,&pkxLen);
    const char *pky = lua_tolstring(L, 2,&pkyLen);
    const char *pBuf = lua_tolstring(L, 3,&pBufLen);
    int ret = 0;

    //Check parameter validity
    if((pkxLen!=64))
    {
        LLOGE("invalid pkx password length=%d", pkxLen);
        return 0;
    }
    if((pkyLen!=64))
    {
        LLOGE("invalid pky password length=%d", pkyLen);
        return 0;
    }
    if (pBufLen > SM2_MAX_PLAINTEXT_SIZE) {
        LLOGD("data too large max %d but %d", SM2_MAX_PLAINTEXT_SIZE, pBufLen);
        return 0;
    }

    int mode = 0;
    if (lua_isboolean(L, 4)) {
        mode = lua_toboolean(L, 4);
    }
    int mode2 = 0;
    if (lua_isboolean(L, 5)) {
        mode2 = lua_toboolean(L, 5);
    }

    SM2_KEY sm2 = {0};
    SM2_POINT point = {0};
    luat_str_fromhex(pkx, 64, (char*)point.x);
    luat_str_fromhex(pky, 64, (char*)point.y);
    ret = sm2_key_set_public_key(&sm2, (const SM2_POINT*)&point);
    if (ret != 1) {
        LLOGD("sm2_key_set_public_key %d", ret);
        return 0;
    }

    uint8_t out[SM2_MAX_CIPHERTEXT_SIZE] = {0};
    size_t olen = 0;
    if (mode == 1) {
        SM2_CIPHERTEXT C = {0};
        ret = sm2_do_encrypt(&sm2, (const uint8_t *)pBuf, pBufLen, &C);
        if (ret == 1) {
            if (mode2 == 0) {
                memcpy(out, &C.point.x, 32);
                memcpy(out + 32, &C.point.y, 32);
                memcpy(out + 64, C.hash, 32);
                memcpy(out + 96, C.ciphertext, C.ciphertext_size);
                olen = 96 + C.ciphertext_size;
            }
            else {
                out[0] = 0x04;
                memcpy(out + 1, &C.point.x, 32);
                memcpy(out + 32 + 1, &C.point.y, 32);
                memcpy(out + 64 + 1, C.ciphertext, C.ciphertext_size);
                memcpy(out + 64 + C.ciphertext_size + 1, C.hash, 32);
                olen = 96 + C.ciphertext_size + 1;
            }
        }
    }
    else {
        ret = sm2_encrypt(&sm2, (const uint8_t *)pBuf, pBufLen, out, &olen);
    }
    if (ret != 1) {
        LLOGD("sm2_encrypt ret %d", ret);
        return 0;
    }
    lua_pushlstring(L, (char*)out, olen);
    return 1;
}

/*sm2 algorithm decryption
@api sm.sm2decrypt(private,data,mode,mode2)
@string private key, required, HEX string
@string Data to be calculated, required, original data, non-HEX string
@boolean output mode, default false. false-GMSSL default format DER, true-website compatibility mode
@boolean standard version, default false. false-C1C3C2 new international, true-C1C2C3 old international
@return string The decrypted string, without HEX conversion. If decryption fails, nil or empty string will be returned.
@usage
-- Tips that the mode/mode2 parameter is new on 2023.10.17
-- Since the implementation of SM2 on each platform is different, be sure to refer to the demo for usage.*/
static int l_sm2_decrypt(lua_State *L)
{    
    size_t privateLen = 0;
    size_t pBufLen = 0;
    const char *private = lua_tolstring(L, 1,&privateLen);
    const char *pBuf = lua_tolstring(L, 2,&pBufLen);
    int ret = 0;

    int mode = 0;
    if (lua_isboolean(L, 3)) {
        mode = lua_toboolean(L, 3);
    }
    int mode2 = 0;
    if (lua_isboolean(L, 4)) {
        mode2 = lua_toboolean(L, 4);
    }


    //Check parameter validity
    if((privateLen!=64))
    {
        LLOGE("invalid private password length=%d", privateLen);
        return 0;
    }
    if (pBufLen < 97) {
        LLOGE("The data is too short, it should be more than 97 bytes");
        return 0;
    }
    
    SM2_KEY sm2 = {0};
    char out[512] = {0};
    size_t olen = 0;
    luat_str_fromhex(private, 64, (char*)sm2.private_key);

    if (mode) {
        // LLOGD("Website compatibility mode");
        SM2_CIPHERTEXT C = {0};
        if (mode2 == 0) {
            // LLOGD("C1C3C2");
            C.ciphertext_size = (uint8_t)(pBufLen - 96);
            // LLOGD("pBufLen %d ciphertext_size %d", pBufLen, C.ciphertext_size);
            memcpy(&C.point.x, pBuf, 32);
            memcpy(&C.point.y, pBuf + 32, 32);
            memcpy(C.hash, pBuf + 64, 32);
            memcpy(C.ciphertext, pBuf + 96, C.ciphertext_size);
        }
        else {
            // LLOGD("C1C2C3");
            pBuf ++;
            pBufLen --;
            C.ciphertext_size = (uint8_t)(pBufLen - 96);
            // LLOGD("pBufLen %d ciphertext_size %d", pBufLen, C.ciphertext_size);
            memcpy(&C.point.x, pBuf, 32);
            memcpy(&C.point.y, pBuf + 32, 32);
            memcpy(C.ciphertext, pBuf + 64, C.ciphertext_size);
            memcpy(C.hash, pBuf + 64 + C.ciphertext_size, 32);
        }
        ret = sm2_do_decrypt(&sm2, &C, (uint8_t *)out, &olen);
    }
    else {
        // LLOGD("GMSSL default mode");
        ret = sm2_decrypt(&sm2, (uint8_t*)pBuf, pBufLen, (uint8_t*)out, &olen);
    }
    if (ret != 1) {
        LLOGD("sm2_decrypt ret %d", ret);
        return 0;
    }
    lua_pushlstring(L, (char*)out, olen);
    return 1;
}


/*sm3 algorithm, calculate HASH value
@api sm.sm3(data)
@string Data to be calculated, required
@return string corresponding hash value
@usage
local encodeStr = gmssl.sm3("lqlq666lqlq946")
log.info("testsm.sm3update",string.toHex(encodeStr))*/
static int l_sm3_update(lua_State *L)
{
    size_t inputLen = 0;
    uint8_t dgst[SM3_DIGEST_LENGTH];
    const char *inputData = lua_tolstring(L,1,&inputLen);
    sm3_digest((uint8_t*)inputData, inputLen, dgst);

    lua_pushlstring(L, (char*)dgst, SM3_DIGEST_LENGTH);   
    return 1;
}


/*sm3 algorithm, calculates HASH value, but with HMAC
@api sm.sm3hmac(data, key)
@string Data to be calculated, required
@string key
@return string corresponding hash value
@usage
local encodeStr = gmssl.sm3hmac("lqlq666lqlq946", "123")
log.info("testsm.sm3update",string.toHex(encodeStr))*/
static int l_sm3hmac_update(lua_State *L)
{
    size_t inputLen = 0;
    size_t keyLen = 0;
    uint8_t dgst[SM3_DIGEST_LENGTH];
    const char *inputData = lua_tolstring(L, 1, &inputLen);
    const char *keyData = lua_tolstring(L, 2, &keyLen);
    sm3_hmac((uint8_t*)keyData, keyLen, (uint8_t*)inputData, inputLen, dgst);

    lua_pushlstring(L, (char*)dgst, SM3_DIGEST_LENGTH);   
    return 1;
}

/*SM4 encryption algorithm
@api gmssl.sm4encrypt(mode,padding,originStr,password)
@string encryption mode, CBC or ECB
@string filling method, NONE/ZERO/PKCS5/PKCS7
@string encrypted string
@string key
@string offset
@return string encrypted data
@usage
local originStr = "SM4 ECB ZeroPadding test"
--Encryption mode: ECB; Padding method: ZeroPadding; Key: 1234567890123456; Key length: 128 bit
local encodeStr = gmssl.sm4encrypt("ECB","ZERO",originStr,"1234567890123456")
print(originStr,"encrypt",string.toHex(encodeStr))
log.info("testsm.decrypt",gmssl.sm4decrypt("ECB","ZERO",encodeStr,"1234567890123456"))

originStr = "SM4 ECB Pkcs5Padding test"
--Encryption mode: ECB; Padding method: Pkcs5Padding; Key: 1234567890123456; Key length: 128 bit
encodeStr = gmssl.sm4encrypt("ECB","PKCS5",originStr,"1234567890123456")
print(originStr,"encrypt",string.toHex(encodeStr))
log.info("testsm.decrypt",gmssl.sm4decrypt("ECB","PKCS5",encodeStr,"1234567890123456"))

originStr = "SM4 CBC Pkcs5Padding test"
--Encryption mode: CBC; Padding method: Pkcs5Padding; Key: 1234567890123456; Key length: 256 bit; Offset: 1234567890666666
encodeStr = gmssl.sm4encrypt("CBC","PKCS5",originStr,"1234567890123456","1234567890666666")
print(originStr,"encrypt",string.toHex(encodeStr))
log.info("testsm.decrypt",gmssl.sm4decrypt("CBC","PKCS5",encodeStr,"1234567890123456","1234567890666666"))*/
static int l_sm4_encrypt(lua_State *L)
{    
    const char *pMode = luaL_checkstring(L, 1);
    const char *pPadding = luaL_checkstring(L, 2);
    size_t nBufLen = 0;
    const char *pBuf = lua_tolstring(L, 3, &nBufLen);
    size_t nPswdLen = 0;
    const char *pPassword = lua_tolstring(L, 4, &nPswdLen);
    size_t nIVLen = 0;
    const char *pIV =  lua_tolstring(L, 5, &nIVLen);

    int nPadLen = SM4_BLOCK_LEN-(nBufLen%SM4_BLOCK_LEN);
    uint8_t pPadBuf[SM4_BLOCK_LEN] = {0};
    uint8_t *pInBuf = NULL;
	
    //Check parameter validity
    if((nPswdLen!=16))
    {
        return luaL_error(L, "invalid password length=%d, only support 128bit Password", nPswdLen);
    }
    if((strcmp(pMode, "ECB")!=0) && (strcmp(pMode, "CBC")!=0))
    {
        return luaL_error(L, "invalid mode=%s, only support ECB,CBC", pMode);
    }
    if((strcmp(pPadding, "NONE")!=0) && (strcmp(pPadding, "PKCS5")!=0) && (strcmp(pPadding, "PKCS7")!=0) && (strcmp((char*)pPadding, "ZERO")!=0))
    {
        return luaL_error(L, "invalid padding=%s, only support NONE,PKCS5,PKCS7,ZERO", pPadding);
    }
    if(((strcmp(pMode, "CBC")==0)) && (nIVLen!=16))
    {
        return luaL_error(L, "invalid iv length=%d, only support 128bit IV", nIVLen);
    }

    //Construct filling data
    if((strcmp(pPadding, "PKCS5")==0) || (strcmp(pPadding, "PKCS7")==0))
    {
        memset(pPadBuf, nPadLen, sizeof(pPadBuf));
    }
    else if(strcmp(pPadding, "ZERO")==0)
    {
        memset(pPadBuf, 0, sizeof(pPadBuf));
    }   
	else if(strcmp(pPadding, "NONE")==0)
    {
    	if((strcmp(pMode, "CBC")==0) || (strcmp(pMode, "ECB")==0)){
	        if(nBufLen%SM4_BLOCK_LEN != 0)
	        {
	            return luaL_error(L, "buf len should be multiple of 16, len=%d", nBufLen);
	        }
        }
        nPadLen = 0;
    }

    //encryption
    {       
        luaL_Buffer b;
        uint32_t nRmnLen;
        luaL_buffinit( L, &b );

         //The original data and filled data are spliced   together
        if (strcmp((char*)pPadding, "NONE")!=0)
        {
            pInBuf = luat_heap_malloc(nBufLen+nPadLen);
            if(pInBuf == NULL)
            {
                //LLOGD("aes_encrypt malloc error!!!\n");
                luaL_pushresult( &b );
                return 1;
            }
            memcpy(pInBuf, pBuf, nBufLen);
            memcpy(pInBuf+nBufLen, pPadBuf, nPadLen); 
            nBufLen += nPadLen;
            nRmnLen = nBufLen;
        }
        else
        {
            pInBuf =  luat_heap_malloc(nBufLen);
			nRmnLen = nBufLen;
            if(pInBuf == NULL)
            {
                //LLOGD("aes_encrypt malloc error!!!\n");
                luaL_pushresult( &b );
                return 1;
            }
            memcpy(pInBuf, pBuf, nBufLen);
        }

		SM4_KEY sm4_key;
        memset(&sm4_key,0,sizeof(SM4_KEY));
		sm4_set_encrypt_key(&sm4_key, (uint8_t*)pPassword);

        if(strcmp(pMode, "ECB") == 0)
        {
            //Start group encryption, each group of 16 bytes
            char out[SM4_BLOCK_LEN];
            while(nRmnLen>0)
            {
                sm4_encrypt(&sm4_key, (uint8_t*)(pInBuf+nBufLen-nRmnLen), (uint8_t*)out);
                luaL_addlstring(&b, out, SM4_BLOCK_LEN);
                nRmnLen -= SM4_BLOCK_LEN;
            }
        }
        else if((strcmp(pMode, "CBC") == 0))
        {
            //The data to be encrypted is passed in at once
            // sm4_cbc_encrypt(pInBuf,pInBuf,nBufLen,&sm4_key,pIV,1);
            char *out = luat_heap_malloc(nBufLen);
            sm4_cbc_encrypt(&sm4_key, (uint8_t*)pIV, pInBuf, nBufLen / SM4_BLOCK_LEN, (uint8_t*)out);
            luaL_addlstring(&b, out, nBufLen);
            luat_heap_free(out);
        }

        if(pInBuf != NULL)
        {
            luat_heap_free(pInBuf);
            pInBuf = NULL;
        }

        luaL_pushresult( &b );
        return 1;
    }   
}


/*SM4 decryption algorithm
@api gmssl.sm4decrypt(mode,padding,encodeStr,password)
@string encryption mode, CBC or ECB
@string filling method, NONE/ZERO/PKCS5/PKCS7
@string encrypted string
@string key
@string offset
@return string decrypted string
@usage
-- Refer to gmssl.sm4encrypt*/
static int l_sm4_decrypt(lua_State *L)
{    
    
    const char *pMode = luaL_checkstring(L, 1);
    const char *pPadding = luaL_checkstring(L, 2);
    size_t nBufLen = 0;
    const char *pBuf = lua_tolstring(L, 3, &nBufLen);
    size_t nPswdLen = 0;
    const char *pPassword = lua_tolstring(L, 4, &nPswdLen);
    size_t nIVLen = 0;
    const char *pIV =  lua_tolstring(L, 5, &nIVLen);
    char out[SM4_BLOCK_LEN];

    //Check parameter validity
    int isCBC = strcmp((char*)pMode, "CBC") == 0;
    int isECB = strcmp((char*)pMode, "ECB") == 0;
    if(isCBC || isECB){
	    if((nBufLen % 16) != 0){
			return luaL_error(L, "invalid BufLen length=%d, BufLen must be Integer multiples of 16", nBufLen);
		}
	}
    if((nPswdLen!=16))
    {
        return luaL_error(L, "invalid password length=%d, only support 128, 192, 256 bits", nPswdLen);
    }
    if(!isCBC && !isECB)
    {
        return luaL_error(L, "invalid mode=%s, only support ECB,CBC,CTR", pMode);
    }
    if((strcmp(pPadding, "NONE")!=0) && (strcmp(pPadding, "PKCS5")!=0) && (strcmp(pPadding, "PKCS7")!=0) && (strcmp((char*)pPadding, "ZERO")!=0))
    {
        return luaL_error(L, "invalid padding=%s, only support NONE,PKCS5,PKCS7,ZERO", pPadding);
    }
    if(isCBC && (nIVLen!=16)) 
    {
        return luaL_error(L, "invalid iv length=%d, only support 16", nIVLen);
    }    
    
    //Decrypt
    {       
        luaL_Buffer b;
        uint32_t nRmnLen;
        luaL_buffinit( L, &b );

        nRmnLen = nBufLen;
		SM4_KEY sm4_key;
        memset(&sm4_key,0,sizeof(SM4_KEY));
        sm4_set_decrypt_key(&sm4_key,(uint8_t*)pPassword);

        if(isECB)
        {
            //Start group decryption, each group of 16 bytes
            while(nRmnLen>0)
            {
                sm4_decrypt(&sm4_key,(uint8_t*)(pBuf+nBufLen-nRmnLen), (uint8_t*)out);
                //Delete padding data
                if(nRmnLen==SM4_BLOCK_LEN)
                {
                    DeletePaddingBuf(&b, pPadding, SM4_BLOCK_LEN, (uint8_t*)out, SM4_BLOCK_LEN);
                }
                else
                {
                    luaL_addlstring(&b, out, SM4_BLOCK_LEN);
                }
                nRmnLen -= SM4_BLOCK_LEN;
            }
        }
        else if (isCBC)
        {
            //The data to be decrypted is passed in at once
            if (nBufLen <= 1024) {
                char out[1024];
                sm4_cbc_decrypt(&sm4_key, (uint8_t*)pIV, (uint8_t*)pBuf, nBufLen/SM4_BLOCK_LEN, (uint8_t*)out);
                DeletePaddingBuf(&b, pPadding, nBufLen, (uint8_t*)out, SM4_BLOCK_LEN);
            }
            else {
                char *out = luat_heap_malloc(nBufLen);
                if (out == NULL) {
                    LLOGE("out of memory when malloc SM4 decrypt buff");
                    return 0;
                }
                sm4_cbc_decrypt(&sm4_key, (uint8_t*)pIV, (uint8_t*)pBuf, nBufLen/SM4_BLOCK_LEN, (uint8_t*)out);
                DeletePaddingBuf(&b, pPadding, nBufLen, (uint8_t*)out, SM4_BLOCK_LEN);
                luat_heap_free(out);
            }
        }
		
        luaL_pushresult( &b );
        return 1;
    }
}


/*sm2 algorithm signature
@api sm.sm2sign(private,data,id)
@string private key, required, HEX string
@string Data to be calculated, required, original data, non-HEX string
@string id value, non-HEX string, optional, default value "1234567812345678"
@return string The previous string has not been converted by HEX. If the signature fails, nil will be returned.
@usage
--This API was added on 2023.10.19
-- Please refer to the demo for specific usage.*/
static int l_sm2_sign(lua_State *L)
{
    int ret = 0;
    size_t pkLen = 0;
    size_t pBufLen = 0;
    size_t idLen = 0;
    // uint8_t sig[SM2_MAX_SIGNATURE_SIZE];
    // size_t siglen = 0;
    const char *pk =   luaL_checklstring(L, 1, &pkLen);
    const char *pBuf = luaL_checklstring(L, 2 ,&pBufLen);
    const char *id =   luaL_optlstring(L, 3, "1234567812345678", &idLen);

    SM2_SIGN_CTX ctx = {0};
    uint8_t dgst[SM3_DIGEST_SIZE];
    SM2_SIGNATURE sig;
    uint8_t pkey[32] = {0};
    if (pkLen != 64) {
        LLOGW("private key len must be 64 byte HEX string");
        return 0;
    }
    if (pBufLen < 1) {
        LLOGW("The data to be signed cannot be an empty string");
        return 0;
    }

    luat_str_fromhex(pk, 64, (char*)pkey);
    ret = sm2_key_set_private_key(&ctx.key, (const uint8_t*)pkey);
    if (ret != 1) {
        LLOGW("sm2_key_set_private_key %d", ret);
        return 0;
    }
    sm3_init(&ctx.sm3_ctx);
    if (id && idLen > 0) {
        uint8_t z[SM3_DIGEST_SIZE];
        sm2_compute_z(z, &ctx.key.public_key, id, idLen);
		sm3_update(&ctx.sm3_ctx, z, sizeof(z));
    }
    sm3_update(&ctx.sm3_ctx, (const uint8_t*)pBuf, pBufLen);
    sm3_finish(&ctx.sm3_ctx, dgst);
    ret = sm2_do_sign(&ctx.key, dgst, &sig);
    if (ret == 1) {
        lua_pushlstring(L, (const char*)sig.r, 64);
        return 1;
    }
    return 0;
}



/*sm2 algorithm signature verification
@api sm.sm2verify(pkx, pky, data, id, sig)
@string public key X, required, HEX string
@string Public key Y, required, HEX string
@string Data to be calculated, required, original data, non-HEX string
@string id value, non-HEX string, optional, default value "1234567812345678"
@string signature data, must be 64 bytes, non-HEX string
@return boolean Returns true if the verification is successful, otherwise returns nil
@usage
--This API was added on 2023.10.19
-- Please refer to the demo for specific usage.*/
static int l_sm2_verify(lua_State *L)
{    
    int ret = 0;
    size_t pkxLen = 0;
    size_t pkyLen = 0;
    size_t pBufLen = 0;
    size_t idLen = 0;
    size_t siglen = 0;
    const char *pkx =  luaL_checklstring(L, 1, &pkxLen);
    const char *pky =  luaL_checklstring(L, 2, &pkyLen);
    const char *pBuf = luaL_checklstring(L, 3, &pBufLen);
    const char *id =   luaL_optlstring(L, 4, "1234567812345678", &idLen);
    const char *sig = luaL_checklstring(L, 5, &siglen);

    if (pkxLen != 64 || pkyLen != 64) {
        LLOGW("public key x/y len must be 64 byte HEX string");
        return 0;
    }
    if (pBufLen < 1) {
        LLOGW("The data to be signed cannot be an empty string");
        return 0;
    }
    if (siglen != 64) {
        LLOGW("The sig data length should be 64 bytes");
        return 0;
    }

    SM2_SIGN_CTX ctx = {0};
    uint8_t dgst[SM3_DIGEST_SIZE];
    SM2_SIGNATURE sigT = {0};
    
    luat_str_fromhex(pkx, 64, (char*)ctx.key.public_key.x);
    luat_str_fromhex(pky, 64, (char*)ctx.key.public_key.y);
    memcpy(sigT.r, sig, 64);

    sm3_init(&ctx.sm3_ctx);
    if (id && idLen > 0) {
        uint8_t z[SM3_DIGEST_SIZE];
        sm2_compute_z(z, &ctx.key.public_key, id, idLen);
		sm3_update(&ctx.sm3_ctx, z, sizeof(z));
    }
    sm3_update(&ctx.sm3_ctx, (const uint8_t*)pBuf, pBufLen);
    sm3_finish(&ctx.sm3_ctx, dgst);
    ret = sm2_do_verify(&ctx.key, dgst, &sigT);
    lua_pushboolean(L, ret == 1 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_gmssl[] =
{
    { "sm2encrypt",      ROREG_FUNC(l_sm2_encrypt)},
    { "sm2decrypt",      ROREG_FUNC(l_sm2_decrypt)},
    { "sm3update",       ROREG_FUNC(l_sm3_update)},
    { "sm3",             ROREG_FUNC(l_sm3_update)},
    { "sm3hmac",         ROREG_FUNC(l_sm3hmac_update)},
    { "sm4encrypt",      ROREG_FUNC(l_sm4_encrypt)},
    { "sm4decrypt",      ROREG_FUNC(l_sm4_decrypt)},
    { "sm2sign",         ROREG_FUNC(l_sm2_sign)},
    { "sm2verify",       ROREG_FUNC(l_sm2_verify)},

	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_gmssl( lua_State *L ) {
    luat_newlib2(L, reg_gmssl);
    return 1;
}

