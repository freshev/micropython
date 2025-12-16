/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "commontypedef.h"
#include "common_api.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define MAXPATH_SIZE 260
#define HTTP_CLIENT_DEFAULT_TIMEOUT 15000

// --------------------------------------------------------------
// miniz.h
// --------------------------------------------------------------
#define MZ_VERSION "11.0.0"
#define MZ_VERNUM 0xB000
#define MZ_VER_MAJOR 11
#define MZ_VER_MINOR 0
#define MZ_VER_REVISION 0
#define MZ_VER_SUBREVISION 0
#define MZ_DEFLATED 8
#define MZ_ADLER32_INIT (1)
#define MZ_CRC32_INIT (0)
#define MINIZ_EXPORT
#define MZ_MALLOC(x) malloc(x)
#define MZ_FREE(x) free(x)
#define MZ_REALLOC(p, x) realloc(p, x)

typedef unsigned long mz_ulong;
MINIZ_EXPORT void mz_free(void *p);
MINIZ_EXPORT mz_ulong mz_adler32(mz_ulong adler, const unsigned char *ptr, size_t buf_len);
MINIZ_EXPORT mz_ulong mz_crc32(mz_ulong crc, const unsigned char *ptr, size_t buf_len);
typedef void *(*mz_alloc_func)(void *opaque, size_t items, size_t size);
typedef void (*mz_free_func)(void *opaque, void *address);
typedef void *(*mz_realloc_func)(void *opaque, void *address, size_t items, size_t size);
MINIZ_EXPORT void *miniz_def_alloc_func(void *opaque, size_t items, size_t size);
MINIZ_EXPORT void miniz_def_free_func(void *opaque, void *address);
MINIZ_EXPORT void *miniz_def_realloc_func(void *opaque, void *address, size_t items, size_t size);

enum {
    MZ_DEFAULT_STRATEGY = 0,
    MZ_FILTERED = 1,
    MZ_HUFFMAN_ONLY = 2,
    MZ_RLE = 3,
    MZ_FIXED = 4
};


enum {
    MZ_NO_COMPRESSION = 0,
    MZ_BEST_SPEED = 1,
    MZ_BEST_COMPRESSION = 9,
    MZ_UBER_COMPRESSION = 10,
    MZ_DEFAULT_LEVEL = 6,
    MZ_DEFAULT_COMPRESSION = -1
};

enum {
    MZ_NO_FLUSH = 0,
    MZ_PARTIAL_FLUSH = 1,
    MZ_SYNC_FLUSH = 2,
    MZ_FULL_FLUSH = 3,
    MZ_FINISH = 4,
    MZ_BLOCK = 5
};


enum {
    MZ_OK = 0,
    MZ_STREAM_END = 1,
    MZ_NEED_DICT = 2,
    MZ_ERRNO = -1,
    MZ_STREAM_ERROR = -2,
    MZ_DATA_ERROR = -3,
    MZ_MEM_ERROR = -4,
    MZ_BUF_ERROR = -5,
    MZ_VERSION_ERROR = -6,
    MZ_PARAM_ERROR = -10000
};

#define MZ_DEFAULT_WINDOW_BITS 15
#define TINFL_LZ_DICT_SIZE 32768

struct mz_internal_state;

/* Compression/decompression stream struct. */
typedef struct mz_stream_s
{
    const unsigned char *next_in; /* pointer to next byte to read */
    unsigned int avail_in;        /* number of bytes available at next_in */
    mz_ulong total_in;            /* total number of bytes consumed so far */

    unsigned char *next_out; /* pointer to next byte to write */
    unsigned int avail_out;  /* number of bytes that can be written to next_out */
    mz_ulong total_out;      /* total number of bytes produced so far */

    char *msg;                       /* error msg (unused) */
    struct mz_internal_state *state; /* internal state, allocated by zalloc/zfree */

    mz_alloc_func zalloc; /* optional heap allocation function (defaults to malloc) */
    mz_free_func zfree;   /* optional heap free function (defaults to free) */
    void *opaque;         /* heap alloc function user pointer */

    int data_type;     /* data_type (unused) */
    mz_ulong adler;    /* adler32 of the source or uncompressed data */
    mz_ulong reserved; /* not used */
} mz_stream;

typedef mz_stream *mz_streamp;
MINIZ_EXPORT int mz_deflateInit(mz_streamp pStream, int level);
MINIZ_EXPORT int mz_deflateInit2(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy);
MINIZ_EXPORT int mz_deflateReset(mz_streamp pStream);
MINIZ_EXPORT int mz_deflate(mz_streamp pStream, int flush);
MINIZ_EXPORT int mz_deflateEnd(mz_streamp pStream);
MINIZ_EXPORT mz_ulong mz_deflateBound(mz_streamp pStream, mz_ulong source_len);
MINIZ_EXPORT int mz_compress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
MINIZ_EXPORT int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);
MINIZ_EXPORT mz_ulong mz_compressBound(mz_ulong source_len);


#define deflateInit mz_deflateInit
#define deflateInit2 mz_deflateInit2
#define deflateReset mz_deflateReset
#define deflate mz_deflate
#define deflateEnd mz_deflateEnd
#define deflateBound mz_deflateBound
#define compress mz_compress
#define compress2 mz_compress2
#define compressBound mz_compressBound
#define crc32 mz_crc32
#define adler32 mz_adler32

typedef unsigned char mz_uint8;
typedef signed short mz_int16;
typedef unsigned short mz_uint16;
typedef unsigned int mz_uint32;
typedef unsigned int mz_uint;
typedef int64_t mz_int64;
typedef uint64_t mz_uint64;
typedef int mz_bool;
typedef mz_uint32 tinfl_bit_buf_t;

#define MZ_FALSE (0)
#define MZ_TRUE (1)

/* Internal/private bits follow. */
enum {
    TINFL_MAX_HUFF_TABLES = 3,
    TINFL_MAX_HUFF_SYMBOLS_0 = 288,
    TINFL_MAX_HUFF_SYMBOLS_1 = 32,
    TINFL_MAX_HUFF_SYMBOLS_2 = 19,
    TINFL_FAST_LOOKUP_BITS = 10,
    TINFL_FAST_LOOKUP_SIZE = 1 << TINFL_FAST_LOOKUP_BITS
};

struct tinfl_decompressor_tag {
    mz_uint32 m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final, m_type, m_check_adler32, m_dist, m_counter, m_num_extra, m_table_sizes[TINFL_MAX_HUFF_TABLES];
    tinfl_bit_buf_t m_bit_buf;
    size_t m_dist_from_out_buf_start;
    mz_int16 m_look_up[TINFL_MAX_HUFF_TABLES][TINFL_FAST_LOOKUP_SIZE];
    mz_int16 m_tree_0[TINFL_MAX_HUFF_SYMBOLS_0 * 2];
    mz_int16 m_tree_1[TINFL_MAX_HUFF_SYMBOLS_1 * 2];
    mz_int16 m_tree_2[TINFL_MAX_HUFF_SYMBOLS_2 * 2];
    mz_uint8 m_code_size_0[TINFL_MAX_HUFF_SYMBOLS_0];
    mz_uint8 m_code_size_1[TINFL_MAX_HUFF_SYMBOLS_1];
    mz_uint8 m_code_size_2[TINFL_MAX_HUFF_SYMBOLS_2];
    mz_uint8 m_raw_header[4], m_len_codes[TINFL_MAX_HUFF_SYMBOLS_0 + TINFL_MAX_HUFF_SYMBOLS_1 + 137];
};

typedef struct tinfl_decompressor_tag tinfl_decompressor;

typedef enum {
    TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS = -4,
    TINFL_STATUS_BAD_PARAM = -3,
    TINFL_STATUS_ADLER32_MISMATCH = -2,
    TINFL_STATUS_FAILED = -1,
    TINFL_STATUS_DONE = 0,
    TINFL_STATUS_NEEDS_MORE_INPUT = 1,
    TINFL_STATUS_HAS_MORE_OUTPUT = 2
} tinfl_status;

enum {
    TINFL_FLAG_PARSE_ZLIB_HEADER = 1,
    TINFL_FLAG_HAS_MORE_INPUT = 2,
    TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,
    TINFL_FLAG_COMPUTE_ADLER32 = 8
};



#define tinfl_init(r)     \
    do                    \
    {                     \
        (r)->m_state = 0; \
    }                     \
    while(0)
// #define tinfl_get_adler32(r) (r)->m_check_adler32

MINIZ_EXPORT tinfl_status tinfl_decompress(tinfl_decompressor *r, const mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, mz_uint8 *pOut_buf_start, mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const mz_uint32 decomp_flags);


// --------------------------------------------------------------
// --------------------------------------------------------------



typedef int (*http_client_recv_callback_t)(char* buf, uint32_t len);

///HTTP client results
typedef enum {
    HTTP_OK = 0,        /// 0  Success
    HTTP_PARSE,         /// 1  url Parse error
    HTTP_DNS,           /// 2  Could not resolve name
    HTTP_PRTCL,         /// 3  Protocol error
    HTTP_SOCKET_FAIL,   /// 4  create socket fail
    HTTP_BIND_FAIL,     /// 5  bind fail  
    HTTP_TIMEOUT,       /// 6  Connection timeout
    HTTP_CONN,          /// 7  Connection error
    HTTP_CLOSED,        /// 8  Connection was closed by remote host
    HTTP_MBEDTLS_ERR,   /// 9  meet ssl error
    HTTP_MOREDATA,      /// 10 Need get more data
    HTTP_OVERFLOW,      /// 11 Buffer overflow
    HTTP_REQ_TIMEOUT,   /// 12 HTTP request timeout waittime is 60s
    HTTP_NO_MEMORY,     /// 13 memory not enough
    HTTP_INTERNAL,      /// 14 Internal error
    HTTP_INFLATE,       /// 15 miniz inflate error
    HTTP_CALLBACK       /// 16 error in callback
} HTTPResult;


typedef enum  {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD
} HTTP_METH;

typedef struct HttpClientSslTag {
    mbedtls_ssl_context       sslContext;
    mbedtls_net_context       netContext;
    mbedtls_ssl_config        sslConfig;
    mbedtls_entropy_context   entropyContext;
    mbedtls_ctr_drbg_context  ctrDrbgContext;
    // mbedtls_x509_crt_profile  crtProfile;
    mbedtls_x509_crt          caCert;
    mbedtls_x509_crt          clientCert;
    mbedtls_pk_context        pkContext;
} HttpClientSsl;

typedef struct HttpClientContextTag {
    int socket;
    int timeout_s;
    int timeout_r;
    bool isHttps;
    int method;
    uint16_t port;
    char* basicAuthUser;
    char* basicAuthPassword;
    char* custHeader;
    int httpResponseCode;
    HttpClientSsl * ssl;
    char *caCert;
    char *clientCert;
    char *clientPk;
    int32_t caCertLen;
    int32_t clientCertLen;
    int32_t clientPkLen;
    uint8_t seclevel;//0:no verify; 1:verify server; 2:both verify
    int32_t ciphersuite[2];//just like 0x0035 TLS_RSA_WITH_AES_256_CBC_SHA,ciphersuite[1] must NULL
    uint8_t pdpId;//pdp context id--cid--0 is default
    uint8_t cache;//0:no session resumption; 1:session resumption
    uint8_t sni;//0:no sni; 1:has sni
    uint8_t ignore;//0:not ignore; 1:ignore
    uint8_t saveMem;//0:disable; 1:enable
    http_client_recv_callback_t recv_cb;
} HttpClientContext;

typedef struct HttpClientDataTag {
    char * postBuf;         //user data to be post
    int postBufLen;
    char * postContentType; //content type of the post data
    int recvContentLength;  //response content length
    int recvRemaining;      //response content length (used in inflate)
    int needObtainLen;      //content length hasn't get
    int blockContentLen;    //content length of one block
    bool isChunked;
    bool isMoreContent;
    char * respBuf;         //buffer to store the response body data
    char * headerBuf;       //buffer to store the response head data
    int respBufLen;
    int  headerBufLen;
    bool isRange;           //if get file by Range, each block (rangeTail-rangeHead+1) bytes 
    int rangeHead;
    int rangeTail;
    int contentRange;
    bool deflated;
    mz_stream stream;
    char * zOutBuff; // stream out buffer
    int inflatedContentLength;
} HttpClientData;

/// Get the text form of the error number
/// 
/// Gets a pointer to a text message that described the result code.
///
/// @param[in] res is the HTTPResult code to translate to text.
/// @returns a pointer to a text message.
///
const char * GetErrorMessage(HTTPResult res);

/**
Provides a basic authentification feature (Base64 encoded username and password)
Pass two NULL pointers to switch back to no authentication
@param[in] user username to use for authentication, must remain valid durlng the whole HTTP session
@param[in] user password to use for authentication, must remain valid durlng the whole HTTP session
*/
void basicAuth(HttpClientContext* context, const char* user, const char* password); //Basic Authentification

/**
Set custom headers for request.

Pass NULL, 0 to turn off custom headers.

@code
 const char * hdrs[] = 
        {
        "Connection", "keep-alive",
        "Accept", "text/html",
        "User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64)",
        "Accept-Encoding", "gzip,deflate,sdch",
        "Accept-Language", "en-US,en;q=0.8",
        };

    http.basicAuth("username", "password");
    http.customHeaders(hdrs, 5);
@endcode

@param[in] headers an array (size multiple of two) key-value pairs, must remain valid during the whole HTTP session
@param[in] pairs number of key-value pairs
*/
void customHeaders(HttpClientContext* context, char** headers, int pairs);

HTTPResult httpSendRequest(HttpClientContext* context, const char* url, HTTP_METH method,  HttpClientData * data); 
HTTPResult httpRecvResponse(HttpClientContext* context, HttpClientData * data); 

//High Level setup functions
/** Execute a GET request on the URL
Blocks until completion
@param[in] url : url on which to execute the request
@param[in,out] pDataIn : pointer to an IHTTPDataIn instance that will collect the data returned by the request, can be NULL
@param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
@return 0 on success, HTTP error (<0) on failure
*/
HTTPResult httpGet(HttpClientContext* context, const char* url,  HttpClientData * data,  int timeout);

HTTPResult httpPostURL(HttpClientContext* context, const char* url,  HttpClientData * data,  int timeout);

HTTPResult httpConnect(HttpClientContext* context, const char* url); //Execute request

HTTPResult httpClose(HttpClientContext* context);

HTTPResult httpRecv(HttpClientContext* context, char* buf, int32_t minLen, int32_t maxLen, int32_t* pReadLen);

HTTPResult httpSend(HttpClientContext* context, const char* buf, uint16_t len); //0 on success, err code on failure

HTTPResult httpParseURL(HttpClientContext* context, char* url);

#endif
