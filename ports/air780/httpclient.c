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

#include <stdio.h>
#include <string.h>

#include "netdb.h"
#include "sockets.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "ec_sslcmd_api.h"
//#include "cmsis_os2.h"
#include "ssl_config.h"
#include "httpclient.h"
#include "luat_debug.h"

// #include DEBUG_LOG_HEADER_FILE
#define HTTP_CONNECT_TIMEOUT_EN


#define CHUNK_SIZE         (1501)
#define CHUNK_SIZE_OUT     (2 * CHUNK_SIZE)
#define MAXHOST_SIZE       (128)
#define TEMPBUF_SIZE       (512)
#define HTTP_HEAD_BUF_SIZE (800)

#define MAX_TIMEOUT  (10 * 60)  //10 min

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#define CHECK_CONN_ERR(ret) \
  do{ \
    if(ret) { \
      return HTTP_CONN; \
    } \
  } while(0)

#define CHECK_ERR(ret) \
        do{ \
          if(ret != HTTP_OK && ret != HTTP_CONN) { \
            return ret; \
          } \
        } while(0)

#define PRTCL_ERR() \
  do{ \
    return HTTP_PRTCL; \
  } while(0)

#define OVERFLOW_ERR(ret) \
    do{ \
      if(ret) { \
        return HTTP_OVERFLOW; \
      } \
    } while(0)

#define DEBUG_LEVEL 3

/* void httpMemDebug(const char *topic) {
    char mess[100];
    char mess2[10];
    int total, used, max_used;
    luat_meminfo_sys(&total, &used, &max_used);
    mess[0] = '\0';
    strcat(mess, topic);
    strcat(mess, ":");
    itoa(total - used, mess2, 10);
    strcat(mess, mess2);
    LUAT_DEBUG_PRINT("%s", mess);
}*/

// ---------------------------------------------------------------------
// mbedtls 3.xx wrappers
// ---------------------------------------------------------------------
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 ) {
    mbedtls_sha256_starts(ctx, is224);
}
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen ) {
    mbedtls_sha256_update(ctx, input, ilen);
}
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32]) {
    mbedtls_sha256_finish(ctx, output);
}
#endif
// ---------------------------------------------------------------------
// net_sockets.c start
// ---------------------------------------------------------------------
static int net_would_block(const mbedtls_net_context *ctx)
{
    int err = errno;
    if ((fcntl(ctx->fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK) {
        errno = err;
        return 0;
    }

    switch (errno = err) {
#if defined EAGAIN
        case EAGAIN:
#endif
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
    return 1;
    }
    return 0;
}

static int check_fd(int fd, int for_select)
{
    if (fd < 0) return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    /* A limitation of select() is that it only works with file descriptors
     * that are strictly less than FD_SETSIZE. This is a limitation of the
     * fd_set type. Error out early, because attempting to call FD_SET on a
     * large file descriptor is a buffer overflow on typical platforms. */
    if (for_select && fd >= FD_SETSIZE) return MBEDTLS_ERR_NET_POLL_FAILED;
    return 0;
}

int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf, size_t len, uint32_t timeout) {
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    struct timeval tv;
    fd_set read_fds;
    int fd = ((mbedtls_net_context *) ctx)->fd;

    ret = check_fd(fd, 1);
    if (ret != 0) return ret;

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    ret = select(fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);
    /* Zero fds ready means we timed out */
    if (ret == 0) return MBEDTLS_ERR_SSL_TIMEOUT;
    if (ret < 0) {
        if (errno == EINTR) return MBEDTLS_ERR_SSL_WANT_READ;
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
    /* This call will not block */
    return mbedtls_net_recv(ctx, buf, len);
}

void mbedtls_net_init(mbedtls_net_context *ctx) {
    ctx->fd = -1;
}

static int net_prepare(void) {
    return 0;
}

int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto) {

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    struct addrinfo hints, *addr_list, *cur;

    if ((ret = net_prepare()) != 0) return ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;
    if (getaddrinfo(host, port, &hints, &addr_list) != 0) return MBEDTLS_ERR_NET_UNKNOWN_HOST;

    /* Try the sockaddrs until a connection succeeds */
    ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
    for (cur = addr_list; cur != NULL; cur = cur->ai_next) {
        ctx->fd = (int) socket(cur->ai_family, cur->ai_socktype,
                               cur->ai_protocol);
        if (ctx->fd < 0) {
            ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }
        if (connect(ctx->fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            ret = 0;
            break;
        }
        mbedtls_net_close(ctx);
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    }
    freeaddrinfo(addr_list);
    return ret;
}

int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int fd = ((mbedtls_net_context *) ctx)->fd;
    ret = check_fd(fd, 0);
    if (ret != 0) return ret;
    ret = (int) read(fd, buf, len);
    if (ret < 0) {
        if (net_would_block(ctx) != 0) return MBEDTLS_ERR_SSL_WANT_READ;
        if (errno == EPIPE || errno == ECONNRESET) return MBEDTLS_ERR_NET_CONN_RESET;
        if (errno == EINTR) return MBEDTLS_ERR_SSL_WANT_READ;
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
    return ret;
}

int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len) {
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int fd = ((mbedtls_net_context *) ctx)->fd;
    ret = check_fd(fd, 0);
    if (ret != 0) return ret;
    ret = (int) write(fd, buf, len);
    if (ret < 0) {
        if (net_would_block(ctx) != 0) return MBEDTLS_ERR_SSL_WANT_WRITE;
        if (errno == EPIPE || errno == ECONNRESET) return MBEDTLS_ERR_NET_CONN_RESET;
        if (errno == EINTR) return MBEDTLS_ERR_SSL_WANT_WRITE;
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }
    return ret;
}

void mbedtls_net_close(mbedtls_net_context *ctx) {
    if (ctx->fd == -1) return;
    close(ctx->fd);
    ctx->fd = -1;
}
void mbedtls_net_free(mbedtls_net_context *ctx) {
    if (ctx == NULL || ctx->fd == -1) return;
    shutdown(ctx->fd, 2);
    close(ctx->fd);
    ctx->fd = -1;
}

// ---------------------------------------------------------------------
// net_sockets.c end
// ---------------------------------------------------------------------
static char *httpSendBuf = NULL;
static char *httpSendBufTemp = NULL;
uint8_t ec_mbedtls_check_crt_valid;
extern sslSessionContext * gSslSessionContext;

static HTTPResult parseURL(const char* url, char* scheme, int32_t maxSchemeLen, char* host, int32_t maxHostLen, uint16_t* port, char* path, int32_t maxPathLen) //Parse URL
{
    // DBG("url=%s", (uint8_t*)url);
    char* schemePtr = (char*) url;
    char* hostPtr = (char*) strstr(url, "://");
    if (hostPtr == NULL) {
        DBG("Could not find host");
        return HTTP_PARSE; //URL is invalid
    }

    if ( (uint16_t)maxSchemeLen < hostPtr - schemePtr + 1 ) { //including NULL-terminating char
        DBG("Scheme str is too small (%d >= %d)", maxSchemeLen, hostPtr - schemePtr + 1);
        return HTTP_PARSE;
    }
    memcpy(scheme, schemePtr, hostPtr - schemePtr);
    scheme[hostPtr - schemePtr] = '\0';

    hostPtr += 3;

    int32_t hostLen = 0;

    char* portPtr = strchr(hostPtr, ':');
    if( portPtr != NULL ) {
        hostLen = portPtr - hostPtr;
        portPtr++;
        if( sscanf(portPtr, "%hu", port) != 1) {
            DBG("Could not find port");
            return HTTP_PARSE;
        }
        // DBG("has port=%d, hostLen= %d", *port,hostLen);
    } else {
        hostLen = strlen(hostPtr);
        // DBG("no port, hostLen=%d", hostLen);
        *port=0;
    }
    char* pathPtr = strchr(hostPtr, '/');
    if( pathPtr != 0 && portPtr == 0) {
        hostLen = pathPtr - hostPtr;
        // DBG("has path, hostLen=%d", hostLen);
    }
    if( maxHostLen < hostLen + 1 ) { //including NULL-terminating char
        DBG("Host str is too small (%d >= %d)", maxHostLen, hostLen + 1);
        return HTTP_PARSE;
    }
    memcpy(host, hostPtr, hostLen);
    host[hostLen] = '\0';
    // DBG("host=%s", (uint8_t*)host);

    int32_t pathLen;
    char* fragmentPtr = strchr(hostPtr, '#');
    if(fragmentPtr != NULL) {
        pathLen = fragmentPtr - pathPtr;
    } else {
        if(pathPtr != NULL){
            pathLen = strlen(pathPtr);
        } else {
            pathLen = 0;
        }
    }

    if( maxPathLen < pathLen + 1 ) { //including NULL-terminating char
        DBG("Path str is too small (%d >= %d)", maxPathLen, pathLen + 1);
        return HTTP_PARSE;
    }
    if (pathPtr!= NULL && pathLen > 0) {
        memcpy(path, pathPtr, pathLen);
        path[pathLen] = '\0';
    }
    // DBG("path=%s", (uint8_t*)path);
    // DBG("parseURL{url(%s),host(%s),maxHostLen(%d),port(%d),path(%s),maxPathLen(%d)}\r\n", url, host, maxHostLen, *port, path, maxPathLen);

    return HTTP_OK;
}

// Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
static int base64enc(const char *input, unsigned int length, char *output, int len)
{
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int c, c1, c2, c3;

    if ((uint16_t)len < ((((length-1)/3)+1)<<2)) return -1;
    for(unsigned int i = 0, j = 0; i<length; i+=3,j+=4) {
        c1 = ((((unsigned char)*((unsigned char *)&input[i]))));
        c2 = (length>i+1)?((((unsigned char)*((unsigned char *)&input[i+1])))):0;
        c3 = (length>i+2)?((((unsigned char)*((unsigned char *)&input[i+2])))):0;

        c = ((c1 & 0xFC) >> 2);
        output[j+0] = base64[c];
        c = ((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4);
        output[j+1] = base64[c];
        c = ((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6);
        output[j+2] = (length>i+1)?base64[c]:'=';
        c = (c3 & 0x3F);
        output[j+3] = (length>i+2)?base64[c]:'=';
    }
    output[(((length-1)/3)+1)<<2] = '\0';
    return 0;
}

static void createauth (const char *user, const char *pwd, char *buf, int len)
{
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "%s:%s", user, pwd);
    base64enc(tmp, strlen(tmp), &buf[strlen(buf)], len - strlen(buf));
}

static HTTPResult httpSslClose(HttpClientContext* context)
{
    HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
    /*context->clientCert = NULL;
    context->caCert = NULL;
    context->clientPk = NULL; let up level free it*/    

    if(ssl == NULL) return HTTP_MBEDTLS_ERR;
    if(&ssl->netContext) mbedtls_net_free(&(ssl->netContext));
    if(&ssl->pkContext) mbedtls_pk_free(&(ssl->pkContext));
    if(&ssl->caCert) mbedtls_x509_crt_free(&(ssl->caCert));
    if(&ssl->clientCert) mbedtls_x509_crt_free(&(ssl->clientCert));
    if(&ssl->ctrDrbgContext) mbedtls_ctr_drbg_free(&(ssl->ctrDrbgContext));
    if(&ssl->entropyContext) mbedtls_entropy_free(&(ssl->entropyContext));
    if(&ssl->sslConfig) mbedtls_ssl_config_free(&(ssl->sslConfig));
    if(&ssl->sslContext) mbedtls_ssl_close_notify(&(ssl->sslContext));
    if(&ssl->sslContext) mbedtls_ssl_free(&(ssl->sslContext));
    free(ssl);

    context->ssl = NULL;
    return HTTP_OK;
}

static HTTPResult httpSendInter(HttpClientContext* context, const char* buf, uint16_t len) //0 on success, err code on failure
{
    int32_t waitToSend = len;
    int32_t hasSend = 0;
    fd_set writeFs;
    struct timeval tv;
    uint32_t preSelTime = 0,passedTime=0;
    uint8_t ret=0;

    tv.tv_sec = 0;
    tv.tv_usec =context->timeout_s * 1000000;
    
    FD_ZERO(&writeFs);
    if(context && context->socket >= 0)
    FD_SET(context->socket, &writeFs);
    
    do {
        tv.tv_usec -= (passedTime * 1000);
        // DBG("passedTime: %d,tv_usec %d", passedTime,tv.tv_usec);
        // preSelTime = osKernelGetTickCount()/portTICK_PERIOD_MS;
        ret = select(context->socket + 1, NULL, &writeFs, NULL, &tv);
        if(ret > 0) {
            hasSend = send(context->socket, (buf + len - waitToSend), waitToSend, MSG_DONTWAIT);
            // DBG("%d bytes data has sent to server", hasSend);
            if(hasSend > 0) waitToSend -= hasSend;
            else if(hasSend == 0) return HTTP_OK;
            else {
                DBG("send failed");
                return HTTP_CONN;
            }
            // DBG("preSelTime: %d,current tick %d", preSelTime,osKernelGetTickCount());
            // passedTime = (osKernelGetTickCount()-preSelTime)>0?(osKernelGetTickCount() - preSelTime)/portTICK_PERIOD_MS:(0xFFFFFFFF - preSelTime + osKernelGetTickCount())/portTICK_PERIOD_MS;
        } else return HTTP_CONN; //select returns <=0 select timeout or error
    } while(waitToSend > 0);
    return HTTP_OK;
}


static HTTPResult httpSslSend(mbedtls_ssl_context* sslContext, const char* buf, uint16_t len) {
    int32_t waitToSend = len;
    int32_t hasSend = 0;    

    do {
        hasSend = mbedtls_ssl_write(sslContext, (unsigned char *)(buf + len - waitToSend), waitToSend);
        if(hasSend > 0) waitToSend -= hasSend;
        else if(hasSend == 0) return HTTP_OK;
        else {
            DBG("http_client(ssl): send failed \n");
            return HTTP_CONN;
        }
    } while(waitToSend>0);
    return HTTP_OK;
}

static int httpSslNonblockRecv(void *netContext, uint8_t *buf, size_t len) {
    int ret;
    int fd = ((mbedtls_net_context *)netContext)->fd;
    if(fd < 0) return HTTP_MBEDTLS_ERR;
    ret = (int)recv(fd, buf, len, MSG_DONTWAIT);
    if(ret<0){
        if( errno == EPIPE || errno == ECONNRESET) return (MBEDTLS_ERR_NET_CONN_RESET);
        if( errno == EINTR ) return (MBEDTLS_ERR_SSL_WANT_READ);
        if(ret == -1 && errno == EWOULDBLOCK) return ret;
        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }
    return (ret);
}

HTTPResult httpSend(HttpClientContext* context, const char* buf, uint16_t len) { //0 on success, err code on failure
    uint8_t ret=0;
    if(context->isHttps) {
        HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
        ret = httpSslSend(&(ssl->sslContext), buf, len);
    } else ret = httpSendInter(context, buf, len);
    return ret;
}


HTTPResult httpRecv(HttpClientContext* context, char* buf, int32_t minLen, int32_t maxLen, int32_t* pReadLen) { //0 on success, err code on failure

    // DBG("Trying to read between %d and %d bytes", minLen, maxLen);
    int32_t readLen = 0;

    int ret;
    while (readLen < maxLen) {
        if(!context->isHttps) {
            if (readLen < minLen) {
                ret = recv(context->socket, buf+readLen, minLen-readLen, 0);
                // DBG("recv [blocking] return:%d", ret);
                if(ret == 0)
                {
                    int mErr = sock_get_errno(context->socket);
                    if(socket_error_is_fatal(mErr)) { //maybe closed or reset by peer
                       // DBG("recv return 0 fatal error return HTTP_CLOSED");
                       return HTTP_CLOSED;
                    } else { 
                       // DBG("recv return 0 connect error");
                       return HTTP_CONN;
                    }
                }
            } else {
                ret = recv(context->socket, buf+readLen, maxLen-readLen, MSG_DONTWAIT);
                // DBG("recv [not blocking] return:%d", ret);
                if(ret == -1 && errno == EWOULDBLOCK) {
                    // DBG("recv [not blocking] errno == EWOULDBLOCK");
                    break;
                }
            }
        } else {
            HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
            if (readLen < minLen) {
                mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), (mbedtls_ssl_send_t*)mbedtls_net_send, (mbedtls_ssl_recv_t*)mbedtls_net_recv, NULL);
                ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char *)(buf+readLen), minLen-readLen);
                if(ret < 0) {
                    // DBG("mbedtls_ssl_read [blocking] return:-0x%x", -ret);
                }
                if(ret == 0) {
                    // DBG("mbedtls_ssl_read [blocking] return 0 connect error");
                    return HTTP_CONN;
                }
            } else {
                mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), (mbedtls_ssl_send_t*)mbedtls_net_send, (mbedtls_ssl_recv_t*)httpSslNonblockRecv, NULL);
                ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char*)(buf+readLen), maxLen-readLen);
                
                if(ret < 0) {
                    // DBG("mbedtls_ssl_read [not blocking] return:-0x%x", -ret);
                }
                if(ret == -1 && errno == EWOULDBLOCK) {
                    // DBG("mbedtls_ssl_read [not blocking] errno == EWOULDBLOCK");
                    break;
                }
            }
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) return HTTP_CLOSED;
        }

        if (ret > 0)  readLen += ret;
        else if ( ret == 0 ) break;
        else {
            // DBG("Connection error (recv returned -0x%x)", -ret);
            *pReadLen = readLen;
            if(context->isHttps) return HTTP_MBEDTLS_ERR;
            else {
                int mErr = sock_get_errno(context->socket);
                if(socket_error_is_fatal(mErr)) { //maybe closed or reset by peer
                   // DBG("recv return 0 fatal error return HTTP_CLOSED");
                   return HTTP_CLOSED;
                } else{ 
                   // DBG("recv return 0 connect error");
                   return HTTP_CONN;
                }
            }
        }
    }
    // DBG("Read %d bytes", readLen);
    buf[readLen] = '\0';    // DS makes it easier to see what's new.
    *pReadLen = readLen;
    
    return HTTP_OK;
}

static HTTPResult prepareBuffer(HttpClientContext* context, char* sendBuf, int* cursor, char* buf, int len) {
    int copyLen;
    int sendbufCursor = *cursor;
    
    if(len == 0) len = strlen(buf);

    do {
        if((CHUNK_SIZE - sendbufCursor) >= len) copyLen = len;
        else return HTTP_OVERFLOW;

        memcpy(sendBuf + sendbufCursor, buf, copyLen);
        sendbufCursor += copyLen;
        len -= copyLen;
    } while(len);    
    *cursor = sendbufCursor;
    return HTTP_OK;
}

void httpFreeBuff(HTTPResult ret)
{
    if(ret != 0) {
        if(httpSendBuf != NULL) {
            // DBG("Free httpSendBuf");
            free(httpSendBuf);
            httpSendBuf = NULL;
        }
        if(httpSendBufTemp != NULL) {
            // DBG("Free httpSendBufTemp");
            free(httpSendBufTemp);
            httpSendBufTemp = NULL;
        }
    }
}

static HTTPResult httpSendHeader(HttpClientContext* context, const char * url, HTTP_METH method, HttpClientData * data) {
    char scheme[8];
    uint16_t port;
    char host[MAXHOST_SIZE];
    char path[MAXPATH_SIZE];
    HTTPResult ret = HTTP_OK;
    int bufCursor = 0;
    memset(host, 0, MAXHOST_SIZE);
    memset(path, 0, MAXPATH_SIZE);
    context->method = method;

    HTTPResult res = parseURL(url, scheme, sizeof(scheme), host, sizeof(host), &port, path, sizeof(path));
    if(res != HTTP_OK) {
        DBG("parseURL returned %d", res);
        return res;
    }
    
    //DBG("Malloc httpSendBuf");
    httpSendBuf = malloc(CHUNK_SIZE);
    memset(httpSendBuf, 0, CHUNK_SIZE);
    
    //DBG("Malloc httpSendBufTemp");
    httpSendBufTemp = malloc(TEMPBUF_SIZE);
    
    const char* meth = (method==HTTP_GET)?"GET":(method==HTTP_POST)?"POST":(method==HTTP_PUT)?"PUT":(method==HTTP_HEAD)?"HEAD":(method==HTTP_DELETE)?"DELETE":"";
    snprintf(httpSendBufTemp, TEMPBUF_SIZE, "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); //Write request
    ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
    httpFreeBuff(ret);
    OVERFLOW_ERR(ret);
    
    // send authorization
    if (context->basicAuthUser && context->basicAuthPassword) {
        memset(httpSendBufTemp, 0, TEMPBUF_SIZE);
        // DBG("send auth (if defined)");
        strcpy(httpSendBufTemp, "Authorization: Basic ");
        createauth(context->basicAuthUser, context->basicAuthPassword, httpSendBufTemp+strlen(httpSendBufTemp), TEMPBUF_SIZE-strlen(httpSendBufTemp));
        strcat(httpSendBufTemp, "\r\n");
        // DBG(" (%s,%s) => (%s)", context->basicAuthUser, context->basicAuthPassword, httpSendBufTemp);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);
    }
    
    // Send custom header
    if(context->custHeader) {
        snprintf(httpSendBufTemp, TEMPBUF_SIZE, "%s\r\n", context->custHeader);
        // DBG("httpSendHeader custheader:{%s}", httpSendBufTemp);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);
    }

    // set range
    if(data->isRange) {
       // DBG("Range:bytes=%d-%d",data->rangeHead, data->rangeTail);
       if(data->rangeTail == -1){
            snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Range:bytes=%d-\r\n", data->rangeHead);
        }else{
           snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Range:bytes=%d-%d\r\n", data->rangeHead, data->rangeTail);
       }
       ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
       httpFreeBuff(ret);
       OVERFLOW_ERR(ret);
    }
    
    //Send default headers
    strcpy(httpSendBufTemp, "Connection: Keep-Alive\r\n");
    ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
    httpFreeBuff(ret);
    OVERFLOW_ERR(ret);
    
    snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Content-Length: %d\r\n", data->postBufLen);
    ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
    httpFreeBuff(ret);
    OVERFLOW_ERR(ret);

    if(data->postContentType != NULL) {
        snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Content-Type: %s\r\n", data->postContentType);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);
    }
    
    //Close headers
    ret = prepareBuffer(context, httpSendBuf, &bufCursor, "\r\n", strlen("\r\n"));
    httpFreeBuff(ret);
    OVERFLOW_ERR(ret);
    // DBG("httpSendHeader send head:%s, headlen:%d", httpSendBuf,strlen(httpSendBuf));

    if(context->isHttps){
        HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
        ret = httpSslSend(&(ssl->sslContext), httpSendBuf, strlen(httpSendBuf));
        httpFreeBuff(ret);
        CHECK_CONN_ERR(ret);
    }
    else {
        ret = httpSendInter(context, httpSendBuf, strlen(httpSendBuf));
        httpFreeBuff(ret);
        CHECK_CONN_ERR(ret);
    }
    httpFreeBuff((HTTPResult)1);
    return ret;
}

static HTTPResult httpSendUserdata(HttpClientContext* context, HttpClientData * data) 
{
    HTTPResult ret = HTTP_OK;
    // DBG("begin send content");
    if(data->postBuf && data->postBufLen) {
        // DBG("data->postBufLen=%d",data->postBufLen);
        if(context->isHttps) {
            HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
            ret = httpSslSend(&(ssl->sslContext), data->postBuf, data->postBufLen);
            CHECK_CONN_ERR(ret);
        } else {
            // DBG("data->postBuf=%s",(uint8_t*)data->postBuf);
            ret = httpSendInter(context, data->postBuf, data->postBufLen);
            CHECK_CONN_ERR(ret);
        }
    }
    return ret;
}

static HTTPResult check_timeout_ret(HTTPResult ret){
    static uint8_t count = 0;
    if(ret == HTTP_OK){
        count = 0;
    }else if(ret == HTTP_CONN ){
        if(count < 3){
            DBG("wait %d x 20s", count);
            count += 1;
        }else{
            DBG("give up return HTTP_TIMEOUT");
            ret = HTTP_REQ_TIMEOUT;
        }
    }
    return ret; 
}

static HTTPResult httpParseContent(HttpClientContext* context, char * buf, int32_t trfLen, HttpClientData * data) {

    int32_t crlfPos = 0;
    HTTPResult ret = HTTP_OK;
    int maxlen;
    int total = 0;
    int templen = 0;
    static int seqNum = 0;

    data->isMoreContent = TRUE;
    if(data->recvContentLength == -1 && data->isChunked == FALSE) {
        while(true) {
            if(total + trfLen < data->respBufLen - 1) {
                memcpy(data->respBuf + total, buf, trfLen);
                total += trfLen;
                data->respBuf[total] = '\0';
                // DBG("all data are here");
            }
            else
            {
                memcpy(data->respBuf + total, buf, data->respBufLen - 1 - total);
                data->respBuf[data->respBufLen-1] = '\0';
                data->blockContentLen = data->respBufLen-1;
                // DBG("still has more data on the way");
                return HTTP_MOREDATA;
            }

            maxlen = MIN(CHUNK_SIZE - 1, data->respBufLen - 1 - total);
            ret = httpRecv(context, buf, 1, maxlen, &trfLen);

            // DBG("receive data len:%d, total:%d", trfLen, total);

            if(ret != HTTP_OK) {
                data->blockContentLen = total;
                data->isMoreContent = false;
                // DBG("ret:%d", ret);
                return ret;
            }
            if(trfLen == 0) {
                // DBG("no more data read");
                data->isMoreContent = false;
                return HTTP_OK;
            }
        }
    }
    
    while(true) {
        int32_t readLen = 0;
    
        if( data->isChunked && data->needObtainLen <= 0) {//content is chunked code and first package
            // DBG("content is chunked code");
            //Read chunk header
            bool foundCrlf;
            do {
                foundCrlf = false;
                crlfPos=0;
                buf[trfLen]=0;
                if(trfLen >= 2) {
                    for(; crlfPos < trfLen - 2; crlfPos++) {
                        if( buf[crlfPos] == '\r' && buf[crlfPos + 1] == '\n' ) {
                            foundCrlf = true;
                            break;
                        }
                    }
                }
                if(!foundCrlf) { //Try to read more
                    DBG("no find crlf to read more");
                    maxlen = MIN(CHUNK_SIZE-trfLen-1, data->respBufLen-1-total);
                    if( trfLen < maxlen ) {
                        int32_t newTrfLen = 0;
                        ret = httpRecv(context, buf + trfLen, 0, maxlen, &newTrfLen);
                        trfLen += newTrfLen;
                        CHECK_CONN_ERR(ret);
                        continue;
                    } else {
                        PRTCL_ERR();
                    }
                }
            } while(!foundCrlf);
            buf[crlfPos] = '\0';
            int n = sscanf(buf, "%x", (unsigned int*)&readLen);
            data->needObtainLen = readLen;
            data->recvContentLength += readLen;
            if(n != 1) {
                DBG("Could not read chunk length");
                PRTCL_ERR();
            }
    
            memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2)); //Not need to move NULL-terminating char any more
            trfLen -= (crlfPos + 2);
    
            if( readLen == 0 ) {
                //Last chunk
                data->isMoreContent = false;
                break;
            }
        } else {
            readLen = data->needObtainLen;
        }
    
        // DBG("need to obtaining %d bytes trfLen=%d", readLen,trfLen);
        do {
            // DBG("trfLen=%d, readLen=%d", trfLen, readLen);
            templen = MIN(trfLen, readLen);
            if(total+templen < data->respBufLen - 1){
                memcpy(data->respBuf+total, buf, templen);
                total += templen;
                data->respBuf[total] = '\0';
                data->needObtainLen -= templen;
                // DBG("templen=%d data->needObtainLen=%d", templen,data->needObtainLen);
            } else {
                if(data->respBufLen -1 < trfLen){
                    // DBG("data->respBufLen=%d is too small, data has overflowed", data->respBufLen);
                }
                memcpy(data->respBuf + total, buf, data->respBufLen - 1 - total);
                data->respBuf[data->respBufLen - 1] = '\0';
                data->needObtainLen -= data->respBufLen - 1 - total;
                // DBG("data->needObtainLen=%d total=%d", data->needObtainLen,total);
                if(readLen > trfLen){
                    data->blockContentLen = data->respBufLen -1;
                    seqNum += 1;
                    // DBG("return 12 moredata data->blockContentLen=%d, seqNum=%d", data->blockContentLen, seqNum);
                    return HTTP_MOREDATA;
                } else {
                    total += templen;
                    // DBG("templen=%d total=%d", templen,total);
                }
            }
            
            if( trfLen >= readLen ) {
                memmove(buf, &buf[readLen], trfLen - readLen);
                trfLen -= readLen;
                readLen = 0;
                data->needObtainLen = 0;
                // DBG("trfLen=%d data->needObtainLen and readLen set 0", trfLen);
            } else {
                readLen -= trfLen;
                // DBG("readLen=%d", readLen);
            }
    
            if(readLen) {
                maxlen = MIN(MIN(CHUNK_SIZE-1, data->respBufLen-1-total),readLen);
                // DBG("to read maxlen=%d", maxlen);
                ret = httpRecv(context, buf, 1, maxlen, &trfLen);
                // DBG("httpRecv return: %d,trfLen:%d", ret,trfLen);
                ret = check_timeout_ret(ret);
                CHECK_ERR(ret);
            }
        } while(readLen);
    
        if( data->isChunked ) {
            if(trfLen < 2) {
                int32_t newTrfLen;
                //Read missing chars to find end of chunk
                // DBG("search end of chunk");
                maxlen = MIN(CHUNK_SIZE-trfLen-1, data->respBufLen-1-total);
                ret = httpRecv(context, buf + trfLen, 2 - trfLen, maxlen, &newTrfLen);
                CHECK_CONN_ERR(ret);
                trfLen += newTrfLen;
            }
            if( (buf[0] != '\r') || (buf[1] != '\n') ) {
                DBG("Chunk format error");
                PRTCL_ERR();
            }
            memmove(buf, &buf[2], trfLen - 2);
            trfLen -= 2;
        } else {
            // DBG("no more content");
            data->isMoreContent = false;
            break;
        }
    }
    // DBG("all content over, seqNum=%d", seqNum);
    data->blockContentLen = total;
    return ret;
}

static HTTPResult httpParseHeader(HttpClientContext* context, char* buf, int32_t trfLen, HttpClientData* data) {

    HTTPResult ret;
    int32_t crlfPos = 0;
    int temp1 = 0, temp2 = 0;
    int headerBufLen = data->headerBufLen;
    char *headerBuf = data->headerBuf;
    
    memset(headerBuf, 0, headerBufLen);
    
    data->recvContentLength = -1;
    data->recvRemaining = -1;
    data->inflatedContentLength = 0;
    data->isChunked = false;
    
    char* crlfPtr = strstr(buf, "\r\n");
    if( crlfPtr == NULL) {
        PRTCL_ERR();
    }

    crlfPos = crlfPtr - buf;
    memcpy(headerBuf, buf, crlfPos + 2);                                
    headerBuf += crlfPos + 2;
    buf[crlfPos] = '\0';

    //Parse HTTP response
    if( sscanf(buf, "HTTP/%*d.%*d %d %*[^\r\n]", &(context->httpResponseCode)) != 1 ) {
        DBG("Not a correct HTTP answer");
        PRTCL_ERR();
    }

    if( (context->httpResponseCode < 200) || (context->httpResponseCode >= 400) ) {
        DBG("Response code %d", context->httpResponseCode);
    }

    memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
    trfLen -= (crlfPos + 2);

    while(true) {
        char *colonPtr, *keyPtr, *valuePtr;
        int keyLen, valueLen;

        crlfPtr = strstr(buf, "\r\n");
        if(crlfPtr == NULL) {
            if( trfLen < (CHUNK_SIZE - 1) )  {
                int32_t newTrfLen = 0;
                ret = httpRecv(context, buf + trfLen, 1, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                trfLen += newTrfLen;
                buf[trfLen] = '\0';
                CHECK_ERR(ret);
                continue;
            } else {
                PRTCL_ERR();
            }
        }
        crlfPos = crlfPtr - buf;

        if(crlfPos == 0) { //End of headers
            memmove(buf, &buf[2], trfLen - 2 + 1); //Be sure to move NULL-terminating char as well
            trfLen -= 2;
            break;
        }
        
        colonPtr = strstr(buf, ": ");        
        if (colonPtr) {             
            if (headerBufLen >= crlfPos + 2) {
                memcpy(headerBuf, buf, crlfPos + 2);                                
                headerBuf += crlfPos + 2;
                headerBufLen -= crlfPos + 2;
            }
            
            keyLen = colonPtr - buf;
            valueLen = crlfPtr - colonPtr - strlen(": ");            
            keyPtr = buf;
            valuePtr = colonPtr + strlen(": ");

            // DBG("Read header : %.*s: %.*s", keyLen, keyPtr, valueLen, valuePtr);
            if (0 == strncasecmp(keyPtr, "Content-Length", keyLen)) {
                sscanf(valuePtr, "%d[^\r]", &(data->recvContentLength));                
                data->needObtainLen = data->recvContentLength;
                data->recvRemaining = data->recvContentLength;
            } else if (0 == strncasecmp(keyPtr, "Transfer-Encoding", keyLen)) {
                if (0 == strncasecmp(valuePtr, "Chunked", valueLen)) {
                    data->isChunked = true;
                    data->recvContentLength = 0;
                    data->recvRemaining = 0;
                    data->needObtainLen = 0;
                }
            } else if (0 == strncasecmp(keyPtr, "Content-Range", keyLen)) {
                sscanf(valuePtr, "%*[^/]/%d[^\r]", &(data->contentRange));                
                sscanf(valuePtr, "%*[^ ] %d-[^\\-]", &(temp1));                
                sscanf(valuePtr, "%*[^\\-]-%d[^/]", &(temp2));                                
            } else if (0 == strncasecmp(keyPtr, "Content-Encoding", keyLen)) {
                if (0 == strncasecmp(valuePtr, "deflate", valueLen)) {
                    data->deflated = true;                    
                }
            }
           
            memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
            trfLen -= (crlfPos + 2);
        } else {
            DBG("Could not parse header");
            PRTCL_ERR();
        }
    }
    
    if(context->method == HTTP_HEAD) return HTTP_OK;
    else return httpParseContent(context, buf, trfLen, data);
}

static HTTPResult httpConnectTimeout(int32_t connectFd, uint32_t timeout) {
    fd_set writeSet;
    fd_set errorSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(connectFd,&writeSet);
    FD_SET(connectFd,&errorSet);
    struct timeval tv;
    tv.tv_sec  = timeout;
    tv.tv_usec = 0;
    INT32 ret;

    ret = select(connectFd+1, NULL, &writeSet, &errorSet, &tv);
    
    if(ret < 0) return HTTP_CONN;
    else if(ret == 0) return HTTP_TIMEOUT;
    else {
        if(FD_ISSET(connectFd, &errorSet)){
            int mErr = sock_get_errno(connectFd);
            DBG("select error fd set get errno=%d", mErr);
            if(mErr) {
                return HTTP_CONN;
            }
        }
        else if(FD_ISSET(connectFd, &writeSet)){
            DBG("errno=115(EINPROGRESS) connect success in time(10s)");
        }
    }
    return HTTP_OK;
}

static HTTPResult httpConnectSocket(int32_t socket,struct sockaddr *addr, int32_t addrlen) {

    HTTPResult ret = HTTP_OK;
    int32_t errCode;

    if(connect(socket,addr,addrlen) == 0) {                                         
        // DBG("httpConnectSocket connect success");
    } else {
        errCode = sock_get_errno(socket);
        if(errCode == EINPROGRESS) {
            DBG("httpConnectSocket connect is ongoing");
            ret = httpConnectTimeout(socket, 25); //from 10s to 25s 
            if(ret == 0) {
                DBG("httpConnectSocket connect success");
            } else {
                DBG("httpConnectSocket connect fail,error code %d", errCode);
                if(socket_error_is_fatal(errCode)) {
                    ret = HTTP_CLOSED;
                }
            }
        }
        else {
            DBG("httpConnectSocket connect fail %d",errCode);
            ret = HTTP_CONN;
        }
    }    
    return ret;
}

static HTTPResult httpConn(HttpClientContext* context, char* host) {
    HTTPResult ret=HTTP_OK;
    //struct timeval timeout_send;
    struct timeval timeout_recv;
    struct addrinfo hints, *addr_list, *p;
    char port[10] = {0};
    int retVal = 0;

    //timeout_send.tv_sec = context->timeout_s > MAX_TIMEOUT ? MAX_TIMEOUT : context->timeout_s;
    //timeout_send.tv_usec = 0;
    timeout_recv.tv_sec = context->timeout_r > MAX_TIMEOUT ? MAX_TIMEOUT : context->timeout_r;
    timeout_recv.tv_usec = 0;

    memset( &hints, 0, sizeof( hints ) );
    //hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;


    snprintf(port, sizeof(port), "%d", context->port) ;
    
    if (getaddrinfo( host, port , &hints, &addr_list ) != 0 ) {
        DBG("HTTP connect unresolved dns");
        return HTTP_DNS;
    }

    //try address one by one until sucess
    for ( p = addr_list; p != NULL; p = p->ai_next ) {
        context->socket = (int) socket( p->ai_family, p->ai_socktype,p->ai_protocol);
        if ( context->socket < 0 ) {
            ret = HTTP_SOCKET_FAIL;
            continue;//try new one
        }

        retVal = bind_cid(context->socket, context->pdpId);
        if(retVal == -1){
            ret = HTTP_BIND_FAIL;
            continue;//try new one
        }
        /* set timeout for both tx removed since lwip not support tx timeout */
        //if ( context->timeout_s > 0) {
        //    setsockopt(context->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout_send, sizeof(timeout_send));
        //}

        /* set timeout for both rx */
        if ( context->timeout_r > 0) {
            setsockopt(context->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_recv, sizeof(timeout_recv));;
        }

        #ifdef HTTP_CONNECT_TIMEOUT_EN

        int32_t flags = fcntl( context->socket, F_GETFL, 0);
        if(flags < 0) {
            DBG("httpCreateSocket get file cntrl flags fail");
            close(context->socket);
            context->socket = -1;
            continue;//try new one
        }
        
        fcntl(context->socket, F_SETFL, flags|O_NONBLOCK); //set socket as nonblock for connect timeout
        
        if (httpConnectSocket( context->socket, p->ai_addr, (int32_t)p->ai_addrlen ) == HTTP_OK) {
            // DBG("HTTP connect success");
            ret = HTTP_OK;
            fcntl(context->socket, F_SETFL, flags&~O_NONBLOCK); //connect success recover to block mode
            break;
        }

        fcntl(context->socket, F_SETFL, flags&~O_NONBLOCK); //connect fail recover to block mode

        #else
        if (connect( context->socket, p->ai_addr, (int)p->ai_addrlen ) == 0) {
            ret = HTTP_OK;
            break;
        }
        #endif
        
        close( context->socket );
        context->socket = -1;
        ret = HTTP_SOCKET_FAIL;
    }

    freeaddrinfo( addr_list );
    return ret;
}


static int sslRandom(void *p_rng, unsigned char *output, size_t output_len)
{
    uint32_t rnglen = output_len;
    uint8_t   rngoffset = 0;

    while (rnglen > 0) {
        *(output + rngoffset) = (unsigned char)rand();
        rngoffset++;
        rnglen--;
    }
    return 0;
}

static void sslDebug(void *ctx, int level, const char *file, int line, const char *str) {
    DBG("%s(%d):%s", file, line, str);
}

static int tls_random( void *p_rng, unsigned char *output, size_t output_len) {
    luat_crypto_trng((char*)output, output_len);
    return 0;
}


static HTTPResult httpSslConn(HttpClientContext* context, char* host) {
    int value;
    HttpClientSsl *ssl = NULL;
    const char *custom = "https";
    char port[10] = {0};
    int authmode = MBEDTLS_SSL_VERIFY_NONE;
    uint32_t flag = 0;    
    
    mbedtls_ssl_session* pSavedSession = NULL;
    bool bDirectSaveSession = FALSE;

    context->ssl = malloc(sizeof(HttpClientSsl));
    memset(context->ssl, 0, sizeof(HttpClientSsl));
    ssl = context->ssl;    

    /*
     * 0. Initialize the RNG and the session data
     */
#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold((int)DEBUG_LEVEL);
#endif    
    mbedtls_net_init(&ssl->netContext);
    mbedtls_ssl_init(&ssl->sslContext);    
    mbedtls_ssl_config_init(&ssl->sslConfig);    
    mbedtls_x509_crt_init(&ssl->caCert);
    mbedtls_x509_crt_init(&ssl->clientCert);
    mbedtls_pk_init(&ssl->pkContext);
    mbedtls_ctr_drbg_init(&ssl->ctrDrbgContext);
    mbedtls_entropy_init(&ssl->entropyContext);

    if((value = mbedtls_ctr_drbg_seed(&ssl->ctrDrbgContext,
                             mbedtls_entropy_func,
                             &ssl->entropyContext,
                             (const unsigned char*)custom,
                             strlen(custom))) != 0) {
        DBG("mbedtls_ctr_drbg_seed failed, value:-0x%x.", -value);
        return HTTP_MBEDTLS_ERR;
    }

    /*
     * 0. Initialize certificates
     */
    if(context->seclevel != 0){
        if (NULL != context->caCert) {
            // DBG("STEP 0. Loading the CA root certificate ...");
            authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
            if (0 != (value = mbedtls_x509_crt_parse(&(ssl->caCert), (const unsigned char *)context->caCert, context->caCertLen))) {
                DBG("failed ! value:-0x%x", -value);
                return HTTP_MBEDTLS_ERR;
            }else if(context->saveMem == 1){
                free(context->caCert);
                context->caCert = NULL;
            }
        }
    }
    /* Setup Client Cert/Key */
    if(context->seclevel == 2){
        if (context->clientCert != NULL && context->clientPk != NULL) {
            // DBG("STEP 0. start prepare client cert ...");
            value = mbedtls_x509_crt_parse(&(ssl->clientCert), (const unsigned char *) context->clientCert, context->clientCertLen);
            if (value != 0) {
                DBG("failed!  mbedtls_x509_crt_parse returned -0x%x\n", -value);
                return HTTP_MBEDTLS_ERR;
            }else if(context->saveMem == 1){
                free(context->clientCert);
                context->clientCert = NULL;
                flag = 1;
            }
            // DBG("context->clientPkLen=%d", context->clientPkLen);
            
            #if MBEDTLS_VERSION_NUMBER >= 0x03000000
                value = mbedtls_pk_parse_key(&ssl->pkContext, (const unsigned char *) context->clientPk, context->clientPkLen, NULL, 0, tls_random, NULL);
            #else
                value = mbedtls_pk_parse_key(&ssl->pkContext, (const unsigned char *) context->clientPk, context->clientPkLen, NULL, 0);
            #endif
    
            if (value != 0) {
                DBG("failed !  mbedtls_pk_parse_key returned -0x%x\n", -value);
                return HTTP_MBEDTLS_ERR;
            }else if(context->saveMem == 1){
                free(context->clientPk);
                context->clientPk = NULL;
            }
        }
    }
    if(context->seclevel == 0) {
        // DBG("user set verify none");
        authmode = MBEDTLS_SSL_VERIFY_NONE;
    }

    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
    #else
    if(context->sni == 1) {
        // DBG("set sni");
        ssl->sslContext.sni = 1;
    }
    #endif
    
    if(context->ignore == 0) {
        // DBG("not ignore the crt's valid time");
        ec_mbedtls_check_crt_valid = 1;
    } else {
        // DBG("ignore the crt's valid time");
        ec_mbedtls_check_crt_valid = 0;
    }

    /*
     * 1. Start the connection
     */
    snprintf(port, sizeof(port), "%d", context->port);
    // DBG("STEP 1. Connecting to PORT:%d",context->port);    

    #if MBEDTLS_VERSION_NUMBER >= 0x03000000    
    if (0 != (value = mbedtls_net_connect(&ssl->netContext, host, port, MBEDTLS_NET_PROTO_TCP))) {
    #else
    if (0 != (value = mbedtls_net_connect(&ssl->netContext, host, port, MBEDTLS_NET_PROTO_TCP, context->pdpId))) {
    #endif
        DBG(" failed ! mbedtls_net_connect returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }
    
    /*
     * 2. Setup stuff
     */
    // DBG("STEP 2. Setting up the SSL/TLS structure...");
    if ((value = mbedtls_ssl_config_defaults(&(ssl->sslConfig), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        DBG(" failed! mbedtls_ssl_config_defaults returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }

    mbedtls_ssl_conf_max_version(&ssl->sslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl->sslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    // memcpy(&(ssl->crtProfile), ssl->sslConfig.cert_profile, sizeof(mbedtls_x509_crt_profile));
    mbedtls_ssl_conf_authmode(&(ssl->sslConfig), authmode);

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    if ((value = mbedtls_ssl_conf_max_frag_len(&(ssl->sslConfig), MBEDTLS_SSL_MAX_FRAG_LEN_4096)) != 0) {
        DBG(" mbedtls_ssl_conf_max_frag_len returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    // mbedtls_ssl_conf_cert_profile(&ssl->sslConfig, &ssl->crtProfile);
    mbedtls_ssl_conf_ca_chain(&(ssl->sslConfig), &(ssl->caCert), NULL);
    if(flag == 1) {
        if ((value = mbedtls_ssl_conf_own_cert(&(ssl->sslConfig), &(ssl->clientCert), &(ssl->pkContext))) != 0) {
            DBG("  failed! mbedtls_ssl_conf_own_cert returned -0x%x", -value);
            return HTTP_MBEDTLS_ERR;
        }
    }
#endif

    mbedtls_ssl_conf_session_tickets(&(ssl->sslConfig), MBEDTLS_SSL_SESSION_TICKETS_ENABLED);

    if(context->ciphersuite[0] != 0xFFFF){
        mbedtls_ssl_conf_ciphersuites(&(ssl->sslConfig), (const int *)(context->ciphersuite));
        DBG("conf ciphersuite 0x%x", context->ciphersuite[0]);
    }

    mbedtls_ssl_conf_rng(&(ssl->sslConfig), sslRandom, &(ssl->ctrDrbgContext));
    mbedtls_ssl_conf_dbg(&(ssl->sslConfig), sslDebug, NULL);

#if defined(MBEDTLS_SSL_ALPN)
    const char *alpn_list[] = { "http/1.1", NULL };
    mbedtls_ssl_conf_alpn_protocols(&(ssl->sslConfig),alpn_list);
#endif

    if(context->timeout_r > 0) {
        uint32_t recvTimeout;
        recvTimeout = context->timeout_r > MAX_TIMEOUT ? MAX_TIMEOUT * 1000 : context->timeout_r * 1000;
        mbedtls_ssl_conf_read_timeout(&(ssl->sslConfig), recvTimeout);
    }
    if ((value = mbedtls_ssl_setup(&(ssl->sslContext), &(ssl->sslConfig))) != 0) {
        DBG(" failed! mbedtls_ssl_setup returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }
    mbedtls_ssl_set_hostname(&(ssl->sslContext), host);
    mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), (mbedtls_ssl_send_t*)mbedtls_net_send, (mbedtls_ssl_recv_t*)mbedtls_net_recv, (mbedtls_ssl_recv_timeout_t*)mbedtls_net_recv_timeout);
    
    /*
     * 3. session resumption
     */
    if(context->cache == 1 && gSslSessionContext != NULL){
        // DBG("has set cache and saved session");
        if(strncasecmp((const CHAR*)gSslSessionContext->server, host, strlen(host)) == 0 && gSslSessionContext->port == context->port) {
            // DBG("Malloc pSavedSession");
            pSavedSession = malloc(sizeof(mbedtls_ssl_session));
            configASSERT(pSavedSession != NULL);

            if( ( value = mbedtls_ssl_session_load( pSavedSession, (const unsigned char*)gSslSessionContext->session, gSslSessionContext->session_len ) ) == 0) {
                // DBG("session_load success");
                if( ( value = mbedtls_ssl_set_session( &(ssl->sslContext), pSavedSession ) ) != 0 ) {
                    DBG(" failed! mbedtls_ssl_set_session returned -0x%x", -value );
                }
            } else{
                DBG(" failed! mbedtls_ssl_session_load returned -0x%x no session resumption", -value );
            }
        }
    }

    /*
     * 4. Handshake
     */
    // DBG("STEP 3. Performing the SSL/TLS handshake...");
    
    while ((value = mbedtls_ssl_handshake(&(ssl->sslContext))) != 0) {
        if ((value != MBEDTLS_ERR_SSL_WANT_READ) && (value != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            DBG("failed  ! mbedtls_ssl_handshake returned -0x%x", -value);
            return HTTP_MBEDTLS_ERR;
        }
    }

    // DBG("after handshake:%d", xBytesTaskMalloced);
    /*
     * 5. Verify the server certificate
     */
    // DBG("STEP 4. Verifying peer X.509 certificate..");
    flag = mbedtls_ssl_get_verify_result(&(ssl->sslContext));
    if (flag != 0) {
        DBG(" failed  ! verify result not confirmed.");
        return HTTP_MBEDTLS_ERR;
    }

    /*
     * 6. Save session for session resumption
     */
    /* get size of the buffer needed */
    if(context->cache == 1) {
        if(gSslSessionContext == NULL){//not save it yet
            // DBG("Malloc sslSessionContext");
            gSslSessionContext = (sslSessionContext*)malloc(sizeof(sslSessionContext));
            configASSERT(gSslSessionContext != NULL);
            memset(gSslSessionContext, 0, sizeof(sslSessionContext));
            bDirectSaveSession = TRUE;
        } else {//has saved session, to check whether the session is same with current session to be establish
            if(strncasecmp((const CHAR*)gSslSessionContext->server, host, strlen(host)) == 0 && gSslSessionContext->port == context->port){
                /*DBG("current connect is the same");
                if(ssl->sslContext.session->ticket_len != pSavedSession->ticket_len
                    || memcmp(ssl->sslContext.session->ticket,pSavedSession->ticket, ssl->sslContext.session->ticket_len) != 0){
                    DBG("server has give a new ticket, save it");
                    bDirectSaveSession = TRUE;
                }*/
            } else {
                // DBG("current connect is diff save new session");
                bDirectSaveSession = TRUE;
            }
        }
        if(bDirectSaveSession == TRUE){
            /*mbedtls_ssl_session_save( mbedtls_ssl_get_session_pointer(&(ssl->sslContext)),NULL, 0, &(gSslSessionContext->session_len) );
            DBG("new session need %d bytes", gSslSessionContext->session_len);
            if(gSslSessionContext->session_len < SSL_SESSION_MAX_LEN){//this session can save in cache so save it. else not save it
                if( (value = mbedtls_ssl_session_save( mbedtls_ssl_get_session_pointer(&(ssl->sslContext)),
                                                      gSslSessionContext->session, gSslSessionContext->session_len,
                                                      &gSslSessionContext->session_len ) ) != 0 ){
                    DBG("failed ! mbedtls_ssl_session_saved returned -0x%x", -value );
                }else{
                    DBG("new session get success");
                    strcpy(gSslSessionContext->server, host);
                    gSslSessionContext->port = context->port;
                    sslSaveSession();
                }
            }*/
        }
    }
    if(pSavedSession != NULL) {
        // DBG("Free pSavedSession");
        mbedtls_ssl_session_free(pSavedSession);
        free(pSavedSession);
    }
    return HTTP_OK;
}

void basicAuth(HttpClientContext* context, const char* user, const char* password) //Basic Authentification
{
    if (context->basicAuthUser) free(context->basicAuthUser);
    context->basicAuthUser = (char *)malloc(strlen(user)+1);
    strcpy(context->basicAuthUser, user);
    if (context->basicAuthPassword) free(context->basicAuthPassword);
    context->basicAuthPassword = (char *)malloc(strlen(password) + 1);
    strcpy(context->basicAuthPassword, password); //not free yet!!!
}

void custHeader(HttpClientContext* context, char *header) {
    context->custHeader = header;
}

HTTPResult httpConnect(HttpClientContext* context, const char* url) {
    HTTPResult ret;

    char scheme[8] = {0};
    uint16_t port;
    char host[MAXHOST_SIZE] = {0};
    char path[MAXPATH_SIZE] = {0};
    
    // DBG("httpConnect parse url: [%s]", url);
    HTTPResult res = parseURL(url, scheme, sizeof(scheme), host, sizeof(host), &port, path, sizeof(path));
    if(res != HTTP_OK) {
        DBG("parseURL returned %d", res);
        return res;
    }

    if(strncasecmp((const char*)scheme, "https", strlen("https")) == 0) context->isHttps = TRUE;
    else context->isHttps = FALSE;
    if(port == 0) { 
        if(context->isHttps) port = 443;
        else port = 80;
    }
    context->port = port;

    if(context->isHttps) {
        ret = httpSslConn(context, host);
        if(HTTP_OK == ret) {
            HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
            context->socket = ssl->netContext.fd;
        }
    } else {
        ret = httpConn(context, host);
    }
    return ret;
}

HTTPResult httpSendRequest(HttpClientContext* context, const char* url, HTTP_METH method,  HttpClientData * data) {
    HTTPResult ret = HTTP_CONN;
    if(context->socket < 0) return ret;

    ret = httpSendHeader(context, url, method, data);
    if(ret != HTTP_OK) return ret;
    if(method == HTTP_GET || method == HTTP_POST) {
        ret = httpSendUserdata(context, data);
    }
    // HTTPDBG("httpSendRequest ret:%d",ret);
    return ret;
}

HTTPResult httpClose(HttpClientContext* context) {
    
    HTTPResult ret = HTTP_OK;

    if(context->isHttps) {
        // DBG("httpSslClose");
        ret = httpSslClose(context);
    } else {
        if(context->socket >= 0) close(context->socket);
    }
    if(context->basicAuthUser) free(context->basicAuthUser);
    if(context->basicAuthPassword) free(context->basicAuthPassword);
    context->socket = -1;
    // HTTPDBG("httpClose");
    // DBG("httpClose,ret=%d",ret);
    return ret;
}
char *httpRecvRespBuf = NULL;

HTTPResult httpRecvResponse(HttpClientContext* context, HttpClientData * data) {

    HTTPResult ret = HTTP_CONN;
    int32_t trfLen = 0;

    if(context->socket < 0) return ret;
    //DBG("Malloc httpRecvRespBuf");
    httpRecvRespBuf = malloc(CHUNK_SIZE);

    if(httpRecvRespBuf == NULL) return HTTP_NO_MEMORY;
    memset(httpRecvRespBuf, 0, (CHUNK_SIZE));
   
    if(data->isMoreContent) {
        // DBG("data->isMoreContent is true continue parseContent");
        data->respBuf[0] = '\0';
        ret = httpParseContent(context, httpRecvRespBuf, trfLen, data);
    } else {
         ret = httpRecv(context, httpRecvRespBuf, 1, CHUNK_SIZE - 1, &trfLen);    // recommended by Rob Noble to avoid timeout wait
         if(ret != HTTP_OK) {      
            if(httpRecvRespBuf != NULL) {
                free(httpRecvRespBuf);
                httpRecvRespBuf = NULL;
            }
            return ret;
         }
         // DBG("has read %d bytes", trfLen);
         httpRecvRespBuf[trfLen] = '\0';
         if(trfLen) {
             // DBG("Received \r\n(%s\r\n)", httpRecvRespBuf);
             ret = httpParseHeader(context, httpRecvRespBuf, trfLen, data);
         }
     }
     if(httpRecvRespBuf != NULL) {
         //DBG("Free httpRecvRespBuf");
         free(httpRecvRespBuf);
         httpRecvRespBuf = NULL;
     }
     return ret;
}

int httpGetData(HttpClientContext* context, char *getUrl, char *buf, uint32_t len, int *stepLen, int *totalLen) {

    HTTPResult result = HTTP_INTERNAL;
    HttpClientData clientData = {0};
    uint32_t count = 0;
    uint16_t headerLen = 0;
    int result1 = 0;

    // DBG("Malloc headerBuf");
    clientData.headerBuf = malloc(HTTP_HEAD_BUF_SIZE);
    clientData.headerBufLen = HTTP_HEAD_BUF_SIZE;
    clientData.respBuf = buf;
    clientData.respBufLen = len;
    if(*stepLen != 0) {
        clientData.isRange = true;
        clientData.rangeHead = *stepLen;
        clientData.rangeTail = -1;
    }
    result = httpSendRequest(context, getUrl, HTTP_GET, &clientData);
    // DBG("send request result=%d", result);

    if (result != HTTP_OK) { 
        free(clientData.headerBuf);
        return result;
    }
    
    clientData.zOutBuff = malloc(CHUNK_SIZE_OUT);
    if(!clientData.zOutBuff) {
        free(clientData.headerBuf);
        DBG("inflateInit failed (NO_MEM_OUT)");
        return HTTP_NO_MEMORY;
    }

    memset(&clientData.stream, 0, sizeof(clientData.stream));
    clientData.stream.next_in = (uint8_t*)clientData.respBuf;
    clientData.stream.avail_in = 0;
    clientData.stream.next_out = (uint8_t*)clientData.zOutBuff;
    clientData.stream.avail_out = CHUNK_SIZE_OUT;
    
    if (mz_inflateInit(&clientData.stream) != MZ_OK) {
        free(clientData.headerBuf);
        DBG("inflateInit failed");
        return HTTP_INFLATE;
    }

    do {
        memset(clientData.headerBuf, 0, clientData.headerBufLen);
        memset(clientData.respBuf, 0, clientData.respBufLen);
        result = httpRecvResponse(context, &clientData);
        // DBG("result: %d", result);
        
        if(result == HTTP_OK || result == HTTP_MOREDATA){
            headerLen = strlen(clientData.headerBuf);
            if(headerLen > 0) {
                if(*stepLen == 0) {
                    *totalLen = clientData.recvContentLength;
                }
                // DBG("total content length=%d", clientData.recvContentLength);
            }
            
            if(context->recv_cb) {

                // DBG("recv %d bytes (%s)", clientData.blockContentLen, clientData.deflated ? "deflated" : "raw");
                // luat_debug_dump((uint8_t*)clientData.respBuf, MIN(clientData.blockContentLen, 16));

                if(clientData.deflated) {
                    // deflate it
                    if (!clientData.stream.avail_in) {
                        uint n = MIN(clientData.blockContentLen, clientData.recvRemaining);
                        clientData.stream.next_in = (uint8_t*)clientData.respBuf;
                        clientData.stream.avail_in = n;
                        clientData.recvRemaining -= n;
                        // luat_debug_dump((uint8_t*)clientData.respBuf, 16);
                    }
                    int status = mz_inflate(&clientData.stream, MZ_SYNC_FLUSH);
                    // DBG("status = %d, avail_in = %d, avail_out = %d", status, clientData.stream.avail_in, clientData.stream.avail_out);

                    uint n = CHUNK_SIZE_OUT - clientData.stream.avail_out;
                    result1 = (*context->recv_cb)(clientData.zOutBuff, n);
                    clientData.stream.next_out = (uint8_t*)clientData.zOutBuff;
                    clientData.stream.avail_out = CHUNK_SIZE_OUT;
                    clientData.inflatedContentLength += n;
                    
                    if (status == MZ_STREAM_END) {
                        // DBG("inflate EOS");
                        *stepLen += clientData.blockContentLen;
                        count += clientData.blockContentLen;
                        break;
                    } else if (status != MZ_OK) {
                        DBG("inflate failed with status %i!", status);
                        result = HTTP_INFLATE;
                        break;
                    }
                } else {
                    result1 = (*context->recv_cb)(clientData.respBuf, clientData.blockContentLen);
                }
                if (!result1) {
                    result = HTTP_CALLBACK;
                    break;
                }
            }
            
            *stepLen += clientData.blockContentLen;
            count += clientData.blockContentLen;
            // DBG("has recv=%d", count);
        }
    } while (result == HTTP_MOREDATA || result == HTTP_CONN);

    if (clientData.deflated) {
        // deflate last portion
        int status = mz_inflate(&clientData.stream, MZ_SYNC_FLUSH); // it's NOT reentrant if output buffer is small !
        // DBG("status = %d, avail_in = %d, avail_out = %d", status, clientData.stream.avail_in, clientData.stream.avail_out);
        uint32_t n = CHUNK_SIZE_OUT - clientData.stream.avail_out;
        result1 = (*context->recv_cb)(clientData.zOutBuff, n);
        if (!result1) result = HTTP_CALLBACK;
        clientData.inflatedContentLength += n;
    }
    
    if(mz_inflateEnd(&clientData.stream) != MZ_OK) {
        DBG("inflateEnd failed");
        result = HTTP_INFLATE;
    }
    if(clientData.zOutBuff) free(clientData.zOutBuff);

    // DBG("result=%d", result);
    if (context->httpResponseCode < 200 || context->httpResponseCode > 404) {
        DBG("invalid http response code = %d", context->httpResponseCode);
    } else if (count == 0 || count != clientData.recvContentLength) {
        DBG("data receive not completed");
    } else {
        if(!clientData.deflated)  DBG("received %d bytes", clientData.recvContentLength);
        else DBG("received %d bytes (inflated %d bytes)", clientData.recvContentLength, clientData.inflatedContentLength);
    }
    free(clientData.headerBuf);
    return result;
}


HTTPResult httpGet(HttpClientContext* context, const char* url,  HttpClientData * data,  int timeout) { // Blocking

    context->timeout_s = timeout;
    HTTPResult ret = HTTP_CONN;
    ret = httpConnect(context, url);
    if(ret == HTTP_OK) {
        ret = httpSendRequest(context, url, HTTP_GET, data);
        if(ret == HTTP_OK) {
            ret = httpRecvResponse(context, data);
        }
    }
    return ret;
}

HTTPResult httpPostURL(HttpClientContext* context, const char* url,  HttpClientData * data,  int timeout) { //Blocking

    context->timeout_s = timeout;
    HTTPResult ret = HTTP_CONN;
    ret = httpConnect(context, url);
    if(ret == HTTP_OK) {
        ret = httpSendRequest(context, url, HTTP_POST, data);
        if(ret == HTTP_OK) {
            ret = httpRecvResponse(context, data);
        }
    }
    return ret;
}

int httpInit(HttpClientContext* context, http_client_recv_callback_t recv_cb) {
    context->timeout_s = 2;
    context->timeout_r = 20;
    context->seclevel = 1;
    context->ciphersuite[0] = 0xFFFF;
    context->ignore = 1;
    context->recv_cb = recv_cb;
}

// --------------------------------------------------------------
// miniz.c
// --------------------------------------------------------------


#define MZ_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MZ_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MZ_CLEAR_OBJ(obj) memset(&(obj), 0, sizeof(obj))
#define MZ_CLEAR_ARR(obj) memset((obj), 0, sizeof(obj))
#define MZ_CLEAR_PTR(obj) memset((obj), 0, sizeof(*obj))
#define MZ_MACRO_END while (0)
#define MZ_ASSERT(x) assert(x)

//#if !defined(MINIZ_USE_UNALIGNED_LOADS_AND_STORES)
//#if MINIZ_X86_OR_X64_CPU
/* Set MINIZ_USE_UNALIGNED_LOADS_AND_STORES to 1 on CPU's that permit efficient integer loads and stores from unaligned addresses. */
#define MINIZ_LITTLE_ENDIAN 1
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#define MINIZ_UNALIGNED_USE_MEMCPY
//#else
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
//#endif
//#endif

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
#define MZ_READ_LE16(p) *((const mz_uint16 *)(p))
#define MZ_READ_LE32(p) *((const mz_uint32 *)(p))
#else
#define MZ_READ_LE16(p) ((mz_uint32)(((const mz_uint8 *)(p))[0]) | ((mz_uint32)(((const mz_uint8 *)(p))[1]) << 8U))
#define MZ_READ_LE32(p) ((mz_uint32)(((const mz_uint8 *)(p))[0]) | ((mz_uint32)(((const mz_uint8 *)(p))[1]) << 8U) | ((mz_uint32)(((const mz_uint8 *)(p))[2]) << 16U) | ((mz_uint32)(((const mz_uint8 *)(p))[3]) << 24U))
#endif


typedef struct {
    tinfl_decompressor m_decomp;
    mz_uint m_dict_ofs, m_dict_avail, m_first_call, m_has_flushed;
    int m_window_bits;
    mz_uint8 m_dict[TINFL_LZ_DICT_SIZE];
    tinfl_status m_last_status;
} inflate_state;

MINIZ_EXPORT void *miniz_def_alloc_func(void *opaque, size_t items, size_t size) {
    (void)opaque, (void)items, (void)size;
    void *ptr = MZ_MALLOC(items * size);
    // DBG("MZ_MALLOC %d bytes (%p)", items * size, ptr);
    return ptr;
}
MINIZ_EXPORT void miniz_def_free_func(void *opaque, void *address) {
    (void)opaque, (void)address;
    // DBG("MZ_FREE %p", address);
    MZ_FREE(address);
    address = NULL;
}
MINIZ_EXPORT void *miniz_def_realloc_func(void *opaque, void *address, size_t items, size_t size) {
    (void)opaque, (void)address, (void)items, (void)size;
    void *ptr = MZ_REALLOC(address, items * size);
    // DBG("MZ_REALLOC %d bytes (%p)", items * size, ptr);
    return ptr;
}


int mz_inflateInit(mz_streamp pStream) {
    int window_bits = MZ_DEFAULT_WINDOW_BITS;
    inflate_state *pDecomp;
    if (!pStream)
        return MZ_STREAM_ERROR;
    if ((window_bits != MZ_DEFAULT_WINDOW_BITS) && (-window_bits != MZ_DEFAULT_WINDOW_BITS))
        return MZ_PARAM_ERROR;

    pStream->data_type = 0;
    pStream->adler = 0;
    pStream->msg = NULL;
    pStream->total_in = 0;
    pStream->total_out = 0;
    pStream->reserved = 0;
    if (!pStream->zalloc)
        pStream->zalloc = miniz_def_alloc_func;
    if (!pStream->zfree)
        pStream->zfree = miniz_def_free_func;

    pDecomp = (inflate_state *)pStream->zalloc(pStream->opaque, 1, sizeof(inflate_state));
    if (!pDecomp) {
        DBG("inflate init inflate_state (NO_MEM)");
        return MZ_MEM_ERROR;
    }

    pStream->state = (struct mz_internal_state *)pDecomp;

    tinfl_init(&pDecomp->m_decomp);
    pDecomp->m_dict_ofs = 0;
    pDecomp->m_dict_avail = 0;
    pDecomp->m_last_status = TINFL_STATUS_NEEDS_MORE_INPUT;
    pDecomp->m_first_call = 1;
    pDecomp->m_has_flushed = 0;
    pDecomp->m_window_bits = window_bits;

    return MZ_OK;
}


int mz_inflateReset(mz_streamp pStream) {
    inflate_state *pDecomp;
    if (!pStream)
        return MZ_STREAM_ERROR;

    pStream->data_type = 0;
    pStream->adler = 0;
    pStream->msg = NULL;
    pStream->total_in = 0;
    pStream->total_out = 0;
    pStream->reserved = 0;

    pDecomp = (inflate_state *)pStream->state;

    tinfl_init(&pDecomp->m_decomp);
    pDecomp->m_dict_ofs = 0;
    pDecomp->m_dict_avail = 0;
    pDecomp->m_last_status = TINFL_STATUS_NEEDS_MORE_INPUT;
    pDecomp->m_first_call = 1;
    pDecomp->m_has_flushed = 0;
    /* pDecomp->m_window_bits = window_bits */;

    return MZ_OK;
}

int mz_inflate(mz_streamp pStream, int flush) {
    inflate_state *pState;
    mz_uint n, first_call, decomp_flags = TINFL_FLAG_COMPUTE_ADLER32;
    size_t in_bytes, out_bytes, orig_avail_in;
    tinfl_status status;

    if ((!pStream) || (!pStream->state))
        return MZ_STREAM_ERROR;
    if (flush == MZ_PARTIAL_FLUSH)
        flush = MZ_SYNC_FLUSH;
    if ((flush) && (flush != MZ_SYNC_FLUSH) && (flush != MZ_FINISH))
        return MZ_STREAM_ERROR;

    pState = (inflate_state *)pStream->state;
    if (pState->m_window_bits > 0)
        decomp_flags |= TINFL_FLAG_PARSE_ZLIB_HEADER;
    orig_avail_in = pStream->avail_in;

    first_call = pState->m_first_call;
    pState->m_first_call = 0;
    if (pState->m_last_status < 0)
        return MZ_DATA_ERROR;

    if (pState->m_has_flushed && (flush != MZ_FINISH))
        return MZ_STREAM_ERROR;
    pState->m_has_flushed |= (flush == MZ_FINISH);

    if ((flush == MZ_FINISH) && (first_call)) {
        // MZ_FINISH on the first call implies that the input and output buffers are large enough to hold the entire compressed/decompressed file. 
        decomp_flags |= TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF;
        in_bytes = pStream->avail_in;
        out_bytes = pStream->avail_out;
        status = tinfl_decompress(&pState->m_decomp, pStream->next_in, &in_bytes, pStream->next_out, pStream->next_out, &out_bytes, decomp_flags);
        pState->m_last_status = status;
        pStream->next_in += (mz_uint)in_bytes;
        pStream->avail_in -= (mz_uint)in_bytes;
        pStream->total_in += (mz_uint)in_bytes;
        // pStream->adler = tinfl_get_adler32(&pState->m_decomp);
        pStream->next_out += (mz_uint)out_bytes;
        pStream->avail_out -= (mz_uint)out_bytes;
        pStream->total_out += (mz_uint)out_bytes;

        if (status < 0) return MZ_DATA_ERROR;
        else if (status != TINFL_STATUS_DONE) {
            pState->m_last_status = TINFL_STATUS_FAILED;
            return MZ_BUF_ERROR;
        }
        return MZ_STREAM_END;
    }
    // flush != MZ_FINISH then we must assume there's more input. 
    if (flush != MZ_FINISH) decomp_flags |= TINFL_FLAG_HAS_MORE_INPUT;

    if (pState->m_dict_avail) {
        n = MZ_MIN(pState->m_dict_avail, pStream->avail_out);
        memcpy(pStream->next_out, pState->m_dict + pState->m_dict_ofs, n);
        pStream->next_out += n;
        pStream->avail_out -= n;
        pStream->total_out += n;
        pState->m_dict_avail -= n;
        pState->m_dict_ofs = (pState->m_dict_ofs + n) & (TINFL_LZ_DICT_SIZE - 1);
        return ((pState->m_last_status == TINFL_STATUS_DONE) && (!pState->m_dict_avail)) ? MZ_STREAM_END : MZ_OK;
    }

    for (;;) {
        in_bytes = pStream->avail_in;
        out_bytes = TINFL_LZ_DICT_SIZE - pState->m_dict_ofs;

        status = tinfl_decompress(&pState->m_decomp, pStream->next_in, &in_bytes, pState->m_dict, pState->m_dict + pState->m_dict_ofs, &out_bytes, decomp_flags);
        pState->m_last_status = status;

        pStream->next_in += (mz_uint)in_bytes;
        pStream->avail_in -= (mz_uint)in_bytes;
        pStream->total_in += (mz_uint)in_bytes;
        // pStream->adler = tinfl_get_adler32(&pState->m_decomp);

        pState->m_dict_avail = (mz_uint)out_bytes;

        n = MZ_MIN(pState->m_dict_avail, pStream->avail_out);
        memcpy(pStream->next_out, pState->m_dict + pState->m_dict_ofs, n);
        pStream->next_out += n;
        pStream->avail_out -= n;
        pStream->total_out += n;
        pState->m_dict_avail -= n;
        pState->m_dict_ofs = (pState->m_dict_ofs + n) & (TINFL_LZ_DICT_SIZE - 1);

        if (status < 0)
            return MZ_DATA_ERROR; // Stream is corrupted (there could be some uncompressed data left in the output dictionary - oh well). 
        else if ((status == TINFL_STATUS_NEEDS_MORE_INPUT) && (!orig_avail_in)) {
            return MZ_BUF_ERROR; // Signal caller that we can't make forward progress without supplying more input or by setting flush to MZ_FINISH. 
        }
        else if (flush == MZ_FINISH)
        {
            // The output buffer MUST be large to hold the remaining uncompressed data when flush==MZ_FINISH. 
            if (status == TINFL_STATUS_DONE) {
                return pState->m_dict_avail ? MZ_BUF_ERROR : MZ_STREAM_END;
            }
            // status here must be TINFL_STATUS_HAS_MORE_OUTPUT, which means there's at least 1 more byte on the way. If there's no more room left in the output buffer then something is wrong. 
            else if (!pStream->avail_out) {
                return MZ_BUF_ERROR;
            }
        }
        else if ((status == TINFL_STATUS_DONE) || (!pStream->avail_in) || (!pStream->avail_out) || (pState->m_dict_avail))
            break;
    }

    return ((status == TINFL_STATUS_DONE) && (!pState->m_dict_avail)) ? MZ_STREAM_END : MZ_OK;
}

int mz_inflateEnd(mz_streamp pStream)
{
    if (!pStream)
        return MZ_STREAM_ERROR;
    if (pStream->state)
    {
        pStream->zfree(pStream->opaque, pStream->state);
        pStream->state = NULL;
    }
    return MZ_OK;
}


#define TINFL_MEMCPY(d, s, l) memcpy(d, s, l)
#define TINFL_MEMSET(p, c, l) memset(p, c, l)

#define TINFL_CR_BEGIN  \
    switch (r->m_state) \
    {                   \
        case 0:
#define TINFL_CR_RETURN(state_index, result) \
    do                                       \
    {                                        \
        status = result;                     \
        r->m_state = state_index;            \
        goto common_exit;                    \
        case state_index:;                   \
    }                                        \
    MZ_MACRO_END
#define TINFL_CR_RETURN_FOREVER(state_index, result) \
    do                                               \
    {                                                \
        for (;;)                                     \
        {                                            \
            TINFL_CR_RETURN(state_index, result);    \
        }                                            \
    }                                                \
    MZ_MACRO_END
#define TINFL_CR_FINISH }

#define TINFL_GET_BYTE(state_index, c)                                                                                                                           \
    do                                                                                                                                                           \
    {                                                                                                                                                            \
        while (pIn_buf_cur >= pIn_buf_end)                                                                                                                       \
        {                                                                                                                                                        \
            TINFL_CR_RETURN(state_index, (decomp_flags & TINFL_FLAG_HAS_MORE_INPUT) ? TINFL_STATUS_NEEDS_MORE_INPUT : TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS); \
        }                                                                                                                                                        \
        c = *pIn_buf_cur++;                                                                                                                                      \
    }                                                                                                                                                            \
    MZ_MACRO_END

#define TINFL_NEED_BITS(state_index, n)                \
    do                                                 \
    {                                                  \
        mz_uint c;                                     \
        TINFL_GET_BYTE(state_index, c);                \
        bit_buf |= (((tinfl_bit_buf_t)c) << num_bits); \
        num_bits += 8;                                 \
    } while (num_bits < (mz_uint)(n))
#define TINFL_SKIP_BITS(state_index, n)      \
    do                                       \
    {                                        \
        if (num_bits < (mz_uint)(n))         \
        {                                    \
            TINFL_NEED_BITS(state_index, n); \
        }                                    \
        bit_buf >>= (n);                     \
        num_bits -= (n);                     \
    }                                        \
    MZ_MACRO_END
#define TINFL_GET_BITS(state_index, b, n)    \
    do                                       \
    {                                        \
        if (num_bits < (mz_uint)(n))         \
        {                                    \
            TINFL_NEED_BITS(state_index, n); \
        }                                    \
        b = bit_buf & ((1 << (n)) - 1);      \
        bit_buf >>= (n);                     \
        num_bits -= (n);                     \
    }                                        \
    MZ_MACRO_END

/* TINFL_HUFF_BITBUF_FILL() is only used rarely, when the number of bytes remaining in the input buffer falls below 2. */
/* It reads just enough bytes from the input stream that are needed to decode the next Huffman code (and absolutely no more). It works by trying to fully decode a */
/* Huffman code by using whatever bits are currently present in the bit buffer. If this fails, it reads another byte, and tries again until it succeeds or until the */
/* bit buffer contains >=15 bits (deflate's max. Huffman code size). */
#define TINFL_HUFF_BITBUF_FILL(state_index, pLookUp, pTree)                    \
    do                                                                         \
    {                                                                          \
        temp = pLookUp[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)];                \
        if (temp >= 0)                                                         \
        {                                                                      \
            code_len = temp >> 9;                                              \
            if ((code_len) && (num_bits >= code_len))                          \
                break;                                                         \
        }                                                                      \
        else if (num_bits > TINFL_FAST_LOOKUP_BITS)                            \
        {                                                                      \
            code_len = TINFL_FAST_LOOKUP_BITS;                                 \
            do                                                                 \
            {                                                                  \
                temp = pTree[~temp + ((bit_buf >> code_len++) & 1)];           \
            } while ((temp < 0) && (num_bits >= (code_len + 1)));              \
            if (temp >= 0)                                                     \
                break;                                                         \
        }                                                                      \
        TINFL_GET_BYTE(state_index, c);                                        \
        bit_buf |= (((tinfl_bit_buf_t)c) << num_bits);                         \
        num_bits += 8;                                                         \
    } while (num_bits < 15);

/* TINFL_HUFF_DECODE() decodes the next Huffman coded symbol. It's more complex than you would initially expect because the zlib API expects the decompressor to never read */
/* beyond the final byte of the deflate stream. (In other words, when this macro wants to read another byte from the input, it REALLY needs another byte in order to fully */
/* decode the next Huffman code.) Handling this properly is particularly important on raw deflate (non-zlib) streams, which aren't followed by a byte aligned adler-32. */
/* The slow path is only executed at the very end of the input buffer. */
/* v1.16: The original macro handled the case at the very end of the passed-in input buffer, but we also need to handle the case where the user passes in 1+zillion bytes */
/* following the deflate data and our non-conservative read-ahead path won't kick in here on this code. This is much trickier. */
#define TINFL_HUFF_DECODE(state_index, sym, pLookUp, pTree)                                                                         \
    do                                                                                                                              \
    {                                                                                                                               \
        int temp;                                                                                                                   \
        mz_uint code_len, c;                                                                                                        \
        if (num_bits < 15)                                                                                                          \
        {                                                                                                                           \
            if ((pIn_buf_end - pIn_buf_cur) < 2)                                                                                    \
            {                                                                                                                       \
                TINFL_HUFF_BITBUF_FILL(state_index, pLookUp, pTree);                                                                \
            }                                                                                                                       \
            else                                                                                                                    \
            {                                                                                                                       \
                bit_buf |= (((tinfl_bit_buf_t)pIn_buf_cur[0]) << num_bits) | (((tinfl_bit_buf_t)pIn_buf_cur[1]) << (num_bits + 8)); \
                pIn_buf_cur += 2;                                                                                                   \
                num_bits += 16;                                                                                                     \
            }                                                                                                                       \
        }                                                                                                                           \
        if ((temp = pLookUp[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)                                                          \
            code_len = temp >> 9, temp &= 511;                                                                                      \
        else                                                                                                                        \
        {                                                                                                                           \
            code_len = TINFL_FAST_LOOKUP_BITS;                                                                                      \
            do                                                                                                                      \
            {                                                                                                                       \
                temp = pTree[~temp + ((bit_buf >> code_len++) & 1)];                                                                \
            } while (temp < 0);                                                                                                     \
        }                                                                                                                           \
        sym = temp;                                                                                                                 \
        bit_buf >>= code_len;                                                                                                       \
        num_bits -= code_len;                                                                                                       \
    }                                                                                                                               \
    MZ_MACRO_END

static void tinfl_clear_tree(tinfl_decompressor *r) {
    if (r->m_type == 0) MZ_CLEAR_ARR(r->m_tree_0);
    else if (r->m_type == 1) MZ_CLEAR_ARR(r->m_tree_1);
    else MZ_CLEAR_ARR(r->m_tree_2);
}

tinfl_status tinfl_decompress(tinfl_decompressor *r, const mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, mz_uint8 *pOut_buf_start, mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const mz_uint32 decomp_flags) {

    static const mz_uint16 s_length_base[31] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0 };
    static const mz_uint8 s_length_extra[31] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0 };
    static const mz_uint16 s_dist_base[32] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0 };
    static const mz_uint8 s_dist_extra[32] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
    static const mz_uint8 s_length_dezigzag[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
    static const mz_uint16 s_min_table_sizes[3] = { 257, 1, 4 };

    mz_int16 *pTrees[3];
    mz_uint8 *pCode_sizes[3];

    tinfl_status status = TINFL_STATUS_FAILED;
    mz_uint32 num_bits, dist, counter, num_extra;
    tinfl_bit_buf_t bit_buf;
    const mz_uint8 *pIn_buf_cur = pIn_buf_next, *const pIn_buf_end = pIn_buf_next + *pIn_buf_size;
    mz_uint8 *pOut_buf_cur = pOut_buf_next, *const pOut_buf_end = pOut_buf_next ? pOut_buf_next + *pOut_buf_size : NULL;
    size_t out_buf_size_mask = (decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF) ? (size_t)-1 : ((pOut_buf_next - pOut_buf_start) + *pOut_buf_size) - 1, dist_from_out_buf_start;

    /* Ensure the output buffer's size is a power of 2, unless the output buffer is large enough to hold the entire output file (in which case it doesn't matter). */
    if (((out_buf_size_mask + 1) & out_buf_size_mask) || (pOut_buf_next < pOut_buf_start))
    {
        *pIn_buf_size = *pOut_buf_size = 0;
        return TINFL_STATUS_BAD_PARAM;
    }

    pTrees[0] = r->m_tree_0;
    pTrees[1] = r->m_tree_1;
    pTrees[2] = r->m_tree_2;
    pCode_sizes[0] = r->m_code_size_0;
    pCode_sizes[1] = r->m_code_size_1;
    pCode_sizes[2] = r->m_code_size_2;

    num_bits = r->m_num_bits;
    bit_buf = r->m_bit_buf;
    dist = r->m_dist;
    counter = r->m_counter;
    num_extra = r->m_num_extra;
    dist_from_out_buf_start = r->m_dist_from_out_buf_start;
    TINFL_CR_BEGIN

    bit_buf = num_bits = dist = counter = num_extra = r->m_zhdr0 = r->m_zhdr1 = 0;
    r->m_z_adler32 = r->m_check_adler32 = 1;
    if (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER)
    {
        TINFL_GET_BYTE(1, r->m_zhdr0);
        TINFL_GET_BYTE(2, r->m_zhdr1);
        counter = (((r->m_zhdr0 * 256 + r->m_zhdr1) % 31 != 0) || (r->m_zhdr1 & 32) || ((r->m_zhdr0 & 15) != 8));
        if (!(decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF))
            counter |= (((1U << (8U + (r->m_zhdr0 >> 4))) > 32768U) || ((out_buf_size_mask + 1) < (size_t)((size_t)1 << (8U + (r->m_zhdr0 >> 4)))));
        if (counter)
        {
            TINFL_CR_RETURN_FOREVER(36, TINFL_STATUS_FAILED);
        }
    }

    do
    {
        TINFL_GET_BITS(3, r->m_final, 3);
        r->m_type = r->m_final >> 1;
        if (r->m_type == 0)
        {
            TINFL_SKIP_BITS(5, num_bits & 7);
            for (counter = 0; counter < 4; ++counter)
            {
                if (num_bits)
                    TINFL_GET_BITS(6, r->m_raw_header[counter], 8);
                else
                    TINFL_GET_BYTE(7, r->m_raw_header[counter]);
            }
            if ((counter = (r->m_raw_header[0] | (r->m_raw_header[1] << 8))) != (mz_uint)(0xFFFF ^ (r->m_raw_header[2] | (r->m_raw_header[3] << 8))))
            {
                TINFL_CR_RETURN_FOREVER(39, TINFL_STATUS_FAILED);
            }
            while ((counter) && (num_bits))
            {
                TINFL_GET_BITS(51, dist, 8);
                while (pOut_buf_cur >= pOut_buf_end)
                {
                    TINFL_CR_RETURN(52, TINFL_STATUS_HAS_MORE_OUTPUT);
                }
                *pOut_buf_cur++ = (mz_uint8)dist;
                counter--;
            }
            while (counter)
            {
                size_t n;
                while (pOut_buf_cur >= pOut_buf_end)
                {
                    TINFL_CR_RETURN(9, TINFL_STATUS_HAS_MORE_OUTPUT);
                }
                while (pIn_buf_cur >= pIn_buf_end)
                {
                    TINFL_CR_RETURN(38, (decomp_flags & TINFL_FLAG_HAS_MORE_INPUT) ? TINFL_STATUS_NEEDS_MORE_INPUT : TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS);
                }
                n = MZ_MIN(MZ_MIN((size_t)(pOut_buf_end - pOut_buf_cur), (size_t)(pIn_buf_end - pIn_buf_cur)), counter);
                TINFL_MEMCPY(pOut_buf_cur, pIn_buf_cur, n);
                pIn_buf_cur += n;
                pOut_buf_cur += n;
                counter -= (mz_uint)n;
            }
        }
        else if (r->m_type == 3)
        {
            TINFL_CR_RETURN_FOREVER(10, TINFL_STATUS_FAILED);
        }
        else
        {
            if (r->m_type == 1)
            {
                mz_uint8 *p = r->m_code_size_0;
                mz_uint i;
                r->m_table_sizes[0] = 288;
                r->m_table_sizes[1] = 32;
                TINFL_MEMSET(r->m_code_size_1, 5, 32);
                for (i = 0; i <= 143; ++i)
                    *p++ = 8;
                for (; i <= 255; ++i)
                    *p++ = 9;
                for (; i <= 279; ++i)
                    *p++ = 7;
                for (; i <= 287; ++i)
                    *p++ = 8;
            }
            else
            {
                for (counter = 0; counter < 3; counter++)
                {
                    TINFL_GET_BITS(11, r->m_table_sizes[counter], "\05\05\04"[counter]);
                    r->m_table_sizes[counter] += s_min_table_sizes[counter];
                }
                MZ_CLEAR_ARR(r->m_code_size_2);
                for (counter = 0; counter < r->m_table_sizes[2]; counter++)
                {
                    mz_uint s;
                    TINFL_GET_BITS(14, s, 3);
                    r->m_code_size_2[s_length_dezigzag[counter]] = (mz_uint8)s;
                }
                r->m_table_sizes[2] = 19;
            }
            for (; (int)r->m_type >= 0; r->m_type--)
            {
                int tree_next, tree_cur;
                mz_int16 *pLookUp;
                mz_int16 *pTree;
                mz_uint8 *pCode_size;
                mz_uint i, j, used_syms, total, sym_index, next_code[17], total_syms[16];
                pLookUp = r->m_look_up[r->m_type];
                pTree = pTrees[r->m_type];
                pCode_size = pCode_sizes[r->m_type];
                MZ_CLEAR_ARR(total_syms);
                TINFL_MEMSET(pLookUp, 0, sizeof(r->m_look_up[0]));
                tinfl_clear_tree(r);
                for (i = 0; i < r->m_table_sizes[r->m_type]; ++i)
                    total_syms[pCode_size[i]]++;
                used_syms = 0, total = 0;
                next_code[0] = next_code[1] = 0;
                for (i = 1; i <= 15; ++i)
                {
                    used_syms += total_syms[i];
                    next_code[i + 1] = (total = ((total + total_syms[i]) << 1));
                }
                if ((65536 != total) && (used_syms > 1))
                {
                    TINFL_CR_RETURN_FOREVER(35, TINFL_STATUS_FAILED);
                }
                for (tree_next = -1, sym_index = 0; sym_index < r->m_table_sizes[r->m_type]; ++sym_index)
                {
                    mz_uint rev_code = 0, l, cur_code, code_size = pCode_size[sym_index];
                    if (!code_size)
                        continue;
                    cur_code = next_code[code_size]++;
                    for (l = code_size; l > 0; l--, cur_code >>= 1)
                        rev_code = (rev_code << 1) | (cur_code & 1);
                    if (code_size <= TINFL_FAST_LOOKUP_BITS)
                    {
                        mz_int16 k = (mz_int16)((code_size << 9) | sym_index);
                        while (rev_code < TINFL_FAST_LOOKUP_SIZE)
                        {
                            pLookUp[rev_code] = k;
                            rev_code += (1 << code_size);
                        }
                        continue;
                    }
                    if (0 == (tree_cur = pLookUp[rev_code & (TINFL_FAST_LOOKUP_SIZE - 1)]))
                    {
                        pLookUp[rev_code & (TINFL_FAST_LOOKUP_SIZE - 1)] = (mz_int16)tree_next;
                        tree_cur = tree_next;
                        tree_next -= 2;
                    }
                    rev_code >>= (TINFL_FAST_LOOKUP_BITS - 1);
                    for (j = code_size; j > (TINFL_FAST_LOOKUP_BITS + 1); j--)
                    {
                        tree_cur -= ((rev_code >>= 1) & 1);
                        if (!pTree[-tree_cur - 1])
                        {
                            pTree[-tree_cur - 1] = (mz_int16)tree_next;
                            tree_cur = tree_next;
                            tree_next -= 2;
                        }
                        else
                            tree_cur = pTree[-tree_cur - 1];
                    }
                    tree_cur -= ((rev_code >>= 1) & 1);
                    pTree[-tree_cur - 1] = (mz_int16)sym_index;
                }
                if (r->m_type == 2)
                {
                    for (counter = 0; counter < (r->m_table_sizes[0] + r->m_table_sizes[1]);)
                    {
                        mz_uint s;
                        TINFL_HUFF_DECODE(16, dist, r->m_look_up[2], r->m_tree_2);
                        if (dist < 16)
                        {
                            r->m_len_codes[counter++] = (mz_uint8)dist;
                            continue;
                        }
                        if ((dist == 16) && (!counter))
                        {
                            TINFL_CR_RETURN_FOREVER(17, TINFL_STATUS_FAILED);
                        }
                        num_extra = "\02\03\07"[dist - 16];
                        TINFL_GET_BITS(18, s, num_extra);
                        s += "\03\03\013"[dist - 16];
                        TINFL_MEMSET(r->m_len_codes + counter, (dist == 16) ? r->m_len_codes[counter - 1] : 0, s);
                        counter += s;
                    }
                    if ((r->m_table_sizes[0] + r->m_table_sizes[1]) != counter)
                    {
                        TINFL_CR_RETURN_FOREVER(21, TINFL_STATUS_FAILED);
                    }
                    TINFL_MEMCPY(r->m_code_size_0, r->m_len_codes, r->m_table_sizes[0]);
                    TINFL_MEMCPY(r->m_code_size_1, r->m_len_codes + r->m_table_sizes[0], r->m_table_sizes[1]);
                }
            }
            for (;;)
            {
                mz_uint8 *pSrc;
                for (;;)
                {
                    if (((pIn_buf_end - pIn_buf_cur) < 4) || ((pOut_buf_end - pOut_buf_cur) < 2))
                    {
                        TINFL_HUFF_DECODE(23, counter, r->m_look_up[0], r->m_tree_0);
                        if (counter >= 256)
                            break;
                        while (pOut_buf_cur >= pOut_buf_end)
                        {
                            TINFL_CR_RETURN(24, TINFL_STATUS_HAS_MORE_OUTPUT);
                        }
                        *pOut_buf_cur++ = (mz_uint8)counter;
                    }
                    else
                    {
                        int sym2;
                        mz_uint code_len;
#if TINFL_USE_64BIT_BITBUF
                        if (num_bits < 30)
                        {
                            bit_buf |= (((tinfl_bit_buf_t)MZ_READ_LE32(pIn_buf_cur)) << num_bits);
                            pIn_buf_cur += 4;
                            num_bits += 32;
                        }
#else
                        if (num_bits < 15)
                        {
                            bit_buf |= (((tinfl_bit_buf_t)MZ_READ_LE16(pIn_buf_cur)) << num_bits);
                            pIn_buf_cur += 2;
                            num_bits += 16;
                        }
#endif
                        if ((sym2 = r->m_look_up[0][bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)
                            code_len = sym2 >> 9;
                        else
                        {
                            code_len = TINFL_FAST_LOOKUP_BITS;
                            do
                            {
                                sym2 = r->m_tree_0[~sym2 + ((bit_buf >> code_len++) & 1)];
                            } while (sym2 < 0);
                        }
                        counter = sym2;
                        bit_buf >>= code_len;
                        num_bits -= code_len;
                        if (counter & 256)
                            break;

#if !TINFL_USE_64BIT_BITBUF
                        if (num_bits < 15)
                        {
                            bit_buf |= (((tinfl_bit_buf_t)MZ_READ_LE16(pIn_buf_cur)) << num_bits);
                            pIn_buf_cur += 2;
                            num_bits += 16;
                        }
#endif
                        if ((sym2 = r->m_look_up[0][bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)
                            code_len = sym2 >> 9;
                        else
                        {
                            code_len = TINFL_FAST_LOOKUP_BITS;
                            do
                            {
                                sym2 = r->m_tree_0[~sym2 + ((bit_buf >> code_len++) & 1)];
                            } while (sym2 < 0);
                        }
                        bit_buf >>= code_len;
                        num_bits -= code_len;

                        pOut_buf_cur[0] = (mz_uint8)counter;
                        if (sym2 & 256)
                        {
                            pOut_buf_cur++;
                            counter = sym2;
                            break;
                        }
                        pOut_buf_cur[1] = (mz_uint8)sym2;
                        pOut_buf_cur += 2;
                    }
                }
                if ((counter &= 511) == 256)
                    break;

                num_extra = s_length_extra[counter - 257];
                counter = s_length_base[counter - 257];
                if (num_extra)
                {
                    mz_uint extra_bits;
                    TINFL_GET_BITS(25, extra_bits, num_extra);
                    counter += extra_bits;
                }

                TINFL_HUFF_DECODE(26, dist, r->m_look_up[1], r->m_tree_1);
                num_extra = s_dist_extra[dist];
                dist = s_dist_base[dist];
                if (num_extra)
                {
                    mz_uint extra_bits;
                    TINFL_GET_BITS(27, extra_bits, num_extra);
                    dist += extra_bits;
                }

                dist_from_out_buf_start = pOut_buf_cur - pOut_buf_start;
                if ((dist == 0 || dist > dist_from_out_buf_start || dist_from_out_buf_start == 0) && (decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF))
                {
                    TINFL_CR_RETURN_FOREVER(37, TINFL_STATUS_FAILED);
                }

                pSrc = pOut_buf_start + ((dist_from_out_buf_start - dist) & out_buf_size_mask);

                if ((MZ_MAX(pOut_buf_cur, pSrc) + counter) > pOut_buf_end)
                {
                    while (counter--)
                    {
                        while (pOut_buf_cur >= pOut_buf_end)
                        {
                            TINFL_CR_RETURN(53, TINFL_STATUS_HAS_MORE_OUTPUT);
                        }
                        *pOut_buf_cur++ = pOut_buf_start[(dist_from_out_buf_start++ - dist) & out_buf_size_mask];
                    }
                    continue;
                }
#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES
                else if ((counter >= 9) && (counter <= dist))
                {
                    const mz_uint8 *pSrc_end = pSrc + (counter & ~7);
                    do
                    {
#ifdef MINIZ_UNALIGNED_USE_MEMCPY
                        memcpy(pOut_buf_cur, pSrc, sizeof(mz_uint32)*2);
#else
                        ((mz_uint32 *)pOut_buf_cur)[0] = ((const mz_uint32 *)pSrc)[0];
                        ((mz_uint32 *)pOut_buf_cur)[1] = ((const mz_uint32 *)pSrc)[1];
#endif
                        pOut_buf_cur += 8;
                    } while ((pSrc += 8) < pSrc_end);
                    if ((counter &= 7) < 3)
                    {
                        if (counter)
                        {
                            pOut_buf_cur[0] = pSrc[0];
                            if (counter > 1)
                                pOut_buf_cur[1] = pSrc[1];
                            pOut_buf_cur += counter;
                        }
                        continue;
                    }
                }
#endif
                while(counter>2)
                {
                    pOut_buf_cur[0] = pSrc[0];
                    pOut_buf_cur[1] = pSrc[1];
                    pOut_buf_cur[2] = pSrc[2];
                    pOut_buf_cur += 3;
                    pSrc += 3;
                    counter -= 3;
                }
                if (counter > 0)
                {
                    pOut_buf_cur[0] = pSrc[0];
                    if (counter > 1)
                        pOut_buf_cur[1] = pSrc[1];
                    pOut_buf_cur += counter;
                }
            }
        }
    } while (!(r->m_final & 1));

    /* Ensure byte alignment and put back any bytes from the bitbuf if we've looked ahead too far on gzip, or other Deflate streams followed by arbitrary data. */
    /* I'm being super conservative here. A number of simplifications can be made to the byte alignment part, and the Adler32 check shouldn't ever need to worry about reading from the bitbuf now. */
    TINFL_SKIP_BITS(32, num_bits & 7);
    while ((pIn_buf_cur > pIn_buf_next) && (num_bits >= 8))
    {
        --pIn_buf_cur;
        num_bits -= 8;
    }
    bit_buf &= ~(~(tinfl_bit_buf_t)0 << num_bits);
    MZ_ASSERT(!num_bits); /* if this assert fires then we've read beyond the end of non-deflate/zlib streams with following data (such as gzip streams). */

    if (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER)
    {
        for (counter = 0; counter < 4; ++counter)
        {
            mz_uint s;
            if (num_bits)
                TINFL_GET_BITS(41, s, 8);
            else
                TINFL_GET_BYTE(42, s);
            r->m_z_adler32 = (r->m_z_adler32 << 8) | s;
        }
    }
    TINFL_CR_RETURN_FOREVER(34, TINFL_STATUS_DONE);

    TINFL_CR_FINISH

common_exit:
    /* As long as we aren't telling the caller that we NEED more input to make forward progress: */
    /* Put back any bytes from the bitbuf in case we've looked ahead too far on gzip, or other Deflate streams followed by arbitrary data. */
    /* We need to be very careful here to NOT push back any bytes we definitely know we need to make forward progress, though, or we'll lock the caller up into an inf loop. */
    if ((status != TINFL_STATUS_NEEDS_MORE_INPUT) && (status != TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS))
    {
        while ((pIn_buf_cur > pIn_buf_next) && (num_bits >= 8))
        {
            --pIn_buf_cur;
            num_bits -= 8;
        }
    }
    r->m_num_bits = num_bits;
    r->m_bit_buf = bit_buf & ~(~(tinfl_bit_buf_t)0 << num_bits);
    r->m_dist = dist;
    r->m_counter = counter;
    r->m_num_extra = num_extra;
    r->m_dist_from_out_buf_start = dist_from_out_buf_start;
    *pIn_buf_size = pIn_buf_cur - pIn_buf_next;
    *pOut_buf_size = pOut_buf_cur - pOut_buf_next;
    if ((decomp_flags & (TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_COMPUTE_ADLER32)) && (status >= 0))
    {
        const mz_uint8 *ptr = pOut_buf_next;
        size_t buf_len = *pOut_buf_size;
        mz_uint32 i, s1 = r->m_check_adler32 & 0xffff, s2 = r->m_check_adler32 >> 16;
        size_t block_len = buf_len % 5552;
        while (buf_len)
        {
            for (i = 0; i + 7 < block_len; i += 8, ptr += 8)
            {
                s1 += ptr[0], s2 += s1;
                s1 += ptr[1], s2 += s1;
                s1 += ptr[2], s2 += s1;
                s1 += ptr[3], s2 += s1;
                s1 += ptr[4], s2 += s1;
                s1 += ptr[5], s2 += s1;
                s1 += ptr[6], s2 += s1;
                s1 += ptr[7], s2 += s1;
            }
            for (; i < block_len; ++i)
                s1 += *ptr++, s2 += s1;
            s1 %= 65521U, s2 %= 65521U;
            buf_len -= block_len;
            block_len = 5552;
        }
        r->m_check_adler32 = (s2 << 16) + s1;
        if ((status == TINFL_STATUS_DONE) && (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER) && (r->m_check_adler32 != r->m_z_adler32))
            status = TINFL_STATUS_ADLER32_MISMATCH;
    }
    return status;
}
// --------------------------------------------------------------
// --------------------------------------------------------------