#ifndef LUAT_CRYPTO_H
#define LUAT_CRYPTO_H
#include "luat_base.h"

#define LUAT_CRYPTO_AES_ECB 1
#define LUAT_CRYPTO_AES_CBC 2
#define LUAT_CRYPTO_AES_CTR 3
#define LUAT_CRYPTO_AES_CFB 4
#define LUAT_CRYPTO_AES_OFB 5

#define LUAT_CRYPTO_AES_PAD_ZERO 1
#define LUAT_CRYPTO_AES_PAD_5 2
#define LUAT_CRYPTO_AES_PAD_7 3

typedef struct
{
    size_t result_size;
    size_t key_len;
	void* ctx;
}luat_crypt_stream_t;
/**
 * @defgroup luatos_crypto crypto data encryption
 * @{*/
/**
 * @brief Generate random numbers
 *
 * @param buff random number saved in memory
 * @param len length
 * @return int*/
int luat_crypto_trng(char* buff, size_t len);
/// @brief Calculate md5 value
/// @param str The string to be calculated
/// @param str_size The length of the string to be calculated
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_md5_simple(const char* str, size_t str_size, void* out_ptr);
/// @brief Calculate hmac_md5 value
/// @param str The string to be calculated
/// @param str_size string length
/// @param mac key
/// @param mac_size the length of the key
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_hmac_md5_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr);

/// @brief Calculate sha1 value
/// @param str The string to be calculated
/// @param str_size The length of the string to be calculated
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_sha1_simple(const char* str, size_t str_size, void* out_ptr);
/// @brief Calculate hmac_sha1 value
/// @param str The string to be calculated
/// @param str_size string length
/// @param mac key
/// @param mac_size the length of the key
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_hmac_sha1_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr);
/// @brief Calculate sha256 value
/// @param str The string to be calculated
/// @param str_size The length of the string to be calculated
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_sha256_simple(const char* str, size_t str_size, void* out_ptr);
/// @brief Calculate hmac_sha256 value
/// @param str The string to be calculated
/// @param str_size string length
/// @param mac key
/// @param mac_size the length of the key
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_hmac_sha256_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr) ;
/// @brief Calculate sha512 value
/// @param str The string to be calculated
/// @param str_size The length of the string to be calculated
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_sha512_simple(const char* str, size_t str_size, void* out_ptr) ;
/// @brief Calculate hmac_sha512 value
/// @param str The string to be calculated
/// @param str_size string length
/// @param mac key
/// @param mac_size the length of the key
/// @param out_ptr output
/// @return 0 on success, -1 on failure
int luat_crypto_hmac_sha512_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr) ;
/**
 * @brief BASE64 encryption
 * @param dst buffer
 * @param dlen buffer length
 * @param olen The number of bytes written
 * @param src encryption key
 * @param slen encryption key length
 * @return 0 success*/
int luat_crypto_base64_encode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen ) ;
/**
 * @brief BASE64 decryption
 * @param dst buffer
 * @param dlen buffer length
 * @param olen The number of bytes written
 * @param src key
 * @param slen key length
 * @return 0 success*/
int luat_crypto_base64_decode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen ) ;
/**@}*/ 
int luat_crypto_cipher_list(const char** list, size_t* len);
int luat_crypto_cipher_suites(const char** list, size_t* len);

int luat_crypto_md(const char* md, const char* str, size_t str_size, void* out_ptr, const char* key, size_t key_len);
int luat_crypto_md_file(const char* md, void* out_ptr, const char* key, size_t key_len, const char* path);

int luat_crypto_md_init(const char* md, const char* key, luat_crypt_stream_t *stream);
int luat_crypto_md_update(const char* str, size_t str_size, luat_crypt_stream_t *stream);
int luat_crypto_md_finish(void* out_ptr, luat_crypt_stream_t *stream);

typedef struct luat_crypto_cipher_ctx
{
    const char* cipher;
    const char* pad;
    const char* str;
    const char* key;
    const char* iv;
    
    size_t cipher_size;
    size_t pad_size;
    size_t str_size;
    size_t key_size;
    size_t iv_size;
    char* outbuff;
    size_t outlen;
    uint8_t flags;
}luat_crypto_cipher_ctx_t;

int luat_crypto_cipher_xxx(luat_crypto_cipher_ctx_t* cctx);

#endif
