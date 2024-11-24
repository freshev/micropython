/*@Modules libcoap
@summary coap data processing
@version 1.0
@date 2020.06.30
@demolibcoap
@tag LUAT_USE_COAP*/
#include "luat_base.h"
#include "luat_timer.h"
#include "luat_mem.h"
#include "luat_msgbus.h"

#define LUAT_LOG_TAG "libcoap"
#include "luat_log.h"

//---------------------------
#ifndef COAP_OPTION_IF_MATCH

#define COAP_OPTION_IF_MATCH        1 /* C, opaque, 0-8 B, (none) */
#define COAP_OPTION_URI_HOST        3 /* C, String, 1-255 B, destination address */
#define COAP_OPTION_ETAG            4 /* E, opaque, 1-8 B, (none) */
#define COAP_OPTION_IF_NONE_MATCH   5 /* empty, 0 B, (none) */
#define COAP_OPTION_URI_PORT        7 /* C, uint, 0-2 B, destination port */
#define COAP_OPTION_LOCATION_PATH   8 /* E, String, 0-255 B, - */
#define COAP_OPTION_URI_PATH       11 /* C, String, 0-255 B, (none) */
#define COAP_OPTION_CONTENT_FORMAT 12 /* E, uint, 0-2 B, (none) */
#define COAP_OPTION_CONTENT_TYPE COAP_OPTION_CONTENT_FORMAT
#define COAP_OPTION_MAXAGE         14 /* E, uint, 0--4 B, 60 Seconds */
#define COAP_OPTION_URI_QUERY      15 /* C, String, 1-255 B, (none) */
#define COAP_OPTION_ACCEPT         17 /* C, uint,   0-2 B, (none) */
#define COAP_OPTION_LOCATION_QUERY 20 /* E, String,   0-255 B, (none) */
#define COAP_OPTION_SIZE2          28 /* E, uint, 0-4 B, (none) */
#define COAP_OPTION_PROXY_URI      35 /* C, String, 1-1034 B, (none) */
#define COAP_OPTION_PROXY_SCHEME   39 /* C, String, 1-255 B, (none) */
#define COAP_OPTION_SIZE1          60 /* E, uint, 0-4 B, (none) */

#define COAP_MEDIATYPE_TEXT_PLAIN                 0 /* text/plain (UTF-8) */
#define COAP_MEDIATYPE_APPLICATION_LINK_FORMAT   40 /* application/link-format */
#define COAP_MEDIATYPE_APPLICATION_XML           41 /* application/xml */
#define COAP_MEDIATYPE_APPLICATION_OCTET_STREAM  42 /* application/octet-stream */
#define COAP_MEDIATYPE_APPLICATION_RDF_XML       43 /* application/rdf+xml */
#define COAP_MEDIATYPE_APPLICATION_EXI           47 /* application/exi  */
#define COAP_MEDIATYPE_APPLICATION_JSON          50 /* application/json  */
#define COAP_MEDIATYPE_APPLICATION_CBOR          60 /* application/cbor  */

#endif
//---------------------------


typedef struct libcoap_header
{
    unsigned int tokenLen: 4 ;
    unsigned int type    : 2 ; // 0 - CON, 1 - NON, 2 - ACK, 3 - RST
    unsigned int version : 2 ;
    uint8_t code;
    uint16_t     msgid;
}libcoap_header_t;

typedef struct libcoap_opt_header
{
    unsigned int opt_len   : 4 ;
    unsigned int opt_delta : 4 ;
}libcoap_opt_header_t;

typedef struct luat_lib_libcoap
{
    libcoap_header_t header;
    char token[9];
    size_t optSize;
    size_t dataSize;
    char opt[256];
    char data[512 - 128];
}luat_lib_libcoap_t;

static uint16_t c_msgid = 1;

#define LUAT_COAP_HANDLE "COAP*"

//-----------------------------------------------------------------
static void addopt(luat_lib_libcoap_t* _coap, uint8_t opt_type, const char* value, size_t len) {
    if (_coap->optSize > 0) { // Check the required opt before

    }
    int cur_opt = opt_type;

    // LLOGD("opt type=%d value len=%d", opt_type, len);
    // LLOGD("opt optSize cur=%d", _coap->optSize);
    size_t idx = _coap->optSize;
    if (len <= 12) {
        _coap->opt[idx++] = (cur_opt << 4) + (len & 0xF);
    }
    else if (len <= 268) {
        _coap->opt[idx++] = (cur_opt << 4) + (0x0D);
        _coap->opt[idx++] = len - 0x0D;
    } else {
        LLOGW("coap opt is too large!!! ignore!!!");
        return;
    }
    for (size_t i = 0; i < len; i++)
    {
        _coap->opt[idx++] = *(value + i);
    }

    _coap->optSize += idx;
}

/**
Create a coap packet
@api libcoap.new(code, uri, headers, payload)
@int coap code, such as libcoap.GET/libcoap.POST/libcoap.PUT/libcoap.DELETE
@string target URI, must be filled in, no need to start with /
@table request headers, similar to http headers, optional
@string request body, similar to http body, optional
@return userdata coap data packet
@usage
--Create a packet requesting server time
local coapdata = libcoap.new(libcoap.GET, "time")
local data = coapdata:rawdata()*/
static int l_libcoap_new(lua_State* L) {
    luat_lib_libcoap_t* _coap;

    // Generate coap structure
    _coap = (luat_lib_libcoap_t*)lua_newuserdata(L, sizeof(luat_lib_libcoap_t));
    if (_coap == NULL)
    {
        LLOGW("out of memory!!!");
        return 0;
    }

    memset(_coap, 0, sizeof(luat_lib_libcoap_t));
    _coap->header.version = 0x01;
    _coap->header.tokenLen = 0;
    _coap->header.msgid = c_msgid ++; //Set msgid
    luaL_setmetatable(L, LUAT_COAP_HANDLE);

    // Then set according to user parameters

    // First, it is code, the first parameter
    _coap->header.code = luaL_checkinteger(L, 1);

    // You must set the URI, the second parameter
    size_t len = 0;
    const char* uri = luaL_checklstring(L, 2, &len);
    addopt(_coap, 11, uri, len); // Uri-Path = 11

    //Set other OPT, the third parameter
    // TODO is a table, right?

    //The last is data, the fourth parameter
    if (lua_isstring(L, 4)) {
        const char* payload = luaL_checklstring(L, 4, &len);
        if (len > 512) {
            LLOGE("data limit to 512 bytes");
            lua_pushstring(L, "data limit to 512 bytes");
            lua_error(L);
        }
        _coap->dataSize = len;
        memcpy(_coap->data, payload, len);
    }

    return 1;
}

/**
Parse coap packets
@api libcoap.parse(str)
@string coap packet
@return userdata coap data packet, if the parsing fails, nil will be returned
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:hcode(), coapdata:data())*/
static int l_libcoap_parse(lua_State* L) {
    size_t len;
    const char* buff = luaL_checklstring(L, 1, &len);
    if (len < 4) {
        return 0; // It must be at least 4 bytes
    }
    libcoap_header_t *header = (libcoap_header_t *)buff; // Fuck the head
    if (header->version != 0x01) {
        LLOGW("bad coap version");
        return 0;
    }
    luat_lib_libcoap_t* _coap;

    // Generate coap structure
    _coap = (luat_lib_libcoap_t*)lua_newuserdata(L, sizeof(luat_lib_libcoap_t));
    if (_coap == NULL)
    {
        LLOGE("out of memory at libcoap.parse");
        return 0;
    }
    memset(_coap, 0, sizeof(luat_lib_libcoap_t));
    memcpy(_coap, buff, 4);
    luaL_setmetatable(L, LUAT_COAP_HANDLE);
    int idx = 4;

    // Analyze the token
    if (_coap->header.tokenLen > 0) {
        memcpy(_coap->token, buff+len, _coap->header.tokenLen);
        idx += _coap->header.tokenLen;
    }
    // Prepare the pointer
    char* ptr = (char*)buff;
    ptr += 4; // skip header
    ptr += _coap->header.tokenLen;

    // analyze opt
    if (idx < len) {
        while (idx < len && *ptr != 0xFF) {
            libcoap_opt_header_t *opt_header = (libcoap_opt_header_t *)ptr;
            if (opt_header->opt_delta == 0xF)
                break;
            LLOGD("found opt %d %d", opt_header->opt_delta, opt_header->opt_len);
            if (opt_header->opt_len <= 12) {
                // nop
            }
            else if (opt_header->opt_len == 13) {
                opt_header->opt_len += (unsigned int)(*(ptr+1));
            }

            ptr += opt_header->opt_len + 1;
            idx += opt_header->opt_len + 1;
            _coap->optSize += opt_header->opt_len + 1;
        }
        LLOGD("opt size=%d", _coap->optSize);
        memcpy(_coap->opt, ptr - _coap->optSize, _coap->optSize);
    }


    //Analyze the data
    if (idx < len) {
        _coap->dataSize = len - idx - 1;
        LLOGD("data size=%d", _coap->dataSize);
        memcpy(_coap->data, ptr+1, _coap->dataSize);
    }

    return 1;
}

//---------------------------------------------
#define tocoap(L)	((luat_lib_libcoap_t *)luaL_checkudata(L, 1, LUAT_COAP_HANDLE))

/**
Get the msgid of the coap packet
@api coapdata:msgid()
@return int msgid of coap packet
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:msgid())*/
static int libcoap_msgid(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    lua_pushinteger(L, _coap->header.msgid);
    return 1;
}

/**
Get the token of the coap packet
@api coapdata:token()
@return string token of coap packet
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:token())*/
static int libcoap_token(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    _coap->token[8] = 0x00; // Make sure there is a terminator
    lua_pushstring(L, _coap->token);
    return 1;
}

/**
Get the binary data of the coap packet for sending to the server
@api coapdata:rawdata()
@return string binary data of coap packet
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.new(libcoap.GET, "time")
netc:send(coapdata:rawdata())*/
static int libcoap_rawdata(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    // Let’s start synthesizing data
    size_t len = 0;
    char buff[512] = {0}; //The maximum length is tentatively 512

    // First, copy the header
    _coap->header.version = 0x01;
    _coap->header.tokenLen =  strlen(_coap->token);
    memcpy(buff, _coap, 4); // The header is fixed at 4 bytes
    len += 4;

    //LLOGD("libcoap header len=%d", len);

    // Is there a token?
    if (_coap->header.tokenLen > 0) {
        memcpy((char*)(buff) + len, _coap->token, _coap->header.tokenLen);
        len += _coap->header.tokenLen;
        //LLOGD("libcoap add token %ld", _coap->header.tokenLen);
    }

    // Then process opt
    if (_coap->optSize > 0) {
        memcpy((char*)(buff) + len, _coap->opt, _coap->optSize);
        len += _coap->optSize;
        //LLOGD("libcoap add opt %ld ,first=%d", _coap->optSize, _coap->opt[0]);
    }

    //Finally add data
    if (_coap->dataSize > 0) {
        buff[len] = 0xFF;
        len ++;
        memcpy((char*)(buff) + len, _coap->data, _coap->dataSize);
        len += _coap->dataSize;
        //LLOGD("libcoap add data %ld", _coap->dataSize);
    }

    lua_pushlstring(L, buff, len);
    return 1;
}

/**
Get the code of the coap packet
@api coapdata:code()
@return int code of coap packet
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:code())*/
static int libcoap_code(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    lua_pushinteger(L, _coap->header.code);
    return 1;
}

/**
Get the http code of the coap packet, which is more friendly than the original code of coap
@api coapdata:hcode()
@return int http code of coap packet, for example 200,205,404
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:hcode())*/
static int libcoap_httpcode(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    lua_pushinteger(L, (_coap->header.code >> 5) * 100 + (_coap->header.code & 0xF));
    return 1;
}

/**
Get the type of coap packet, such as libcoap.CON/NON/ACK/RST
@api coapdata:type(t)
@int new type value, optional
@return int type of coap packet
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:type())*/
static int libcoap_type(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    if (lua_isinteger(L, 1)) {
        _coap->header.type = luaL_checkinteger(L, 1);
    }
    lua_pushinteger(L, _coap->header.type);
    return 1;
}

/**
Get the data of the coap packet
@api coapdata:data()
@return string data of coap packet
@usage
-- Parse incoming data packets from the server
local coapdata = libcoap.parse(indata)
log.info("coapdata", coapdata:data())*/
static int libcoap_data(lua_State *L) {
    luat_lib_libcoap_t *_coap = tocoap(L);
    if (_coap == NULL) {
        LLOGE("coap sturt is NULL");
        lua_pushstring(L, "coap sturt is NULL");
        lua_error(L);
        return 0;
    }
    lua_pushlstring(L, _coap->data, _coap->dataSize);
    return 1;
}

static const luaL_Reg lib_libcoap[] = {
    {"tp",          libcoap_type},
    {"msgid",       libcoap_msgid},
    {"token",       libcoap_token},
    {"code",        libcoap_code},
    {"hcode",        libcoap_httpcode},
    {"rawdata",     libcoap_rawdata},
    {"data",        libcoap_data},
    //{"__gc",        libcoap_gc},
    //{"__tostring",  libcoap_tostring},
    {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, LUAT_COAP_HANDLE);  /* create metatable for file handles */
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_setfuncs(L, lib_libcoap, 0);  /* add file methods to new metatable */
  lua_pop(L, 1);  /* pop new metatable */
}


#include "rotable2.h"
#define CODE(H,L) (H << 5 + L)
static const rotable_Reg_t reg_libcoap[] =
{
    { "new",   ROREG_FUNC(l_libcoap_new)},
    { "parse", ROREG_FUNC(l_libcoap_parse)},

    // ----- type constant
    { "CON",        ROREG_INT(0)},
    { "NON",        ROREG_INT(1)},
    { "ACK",        ROREG_INT(2)},
    { "RST",        ROREG_INT(3)},

    // request class)
    { "NONE",       ROREG_INT(0)},
    { "GET",        ROREG_INT(1)},
    { "POST",       ROREG_INT(2)},
    { "PUT",        ROREG_INT(3)},
    { "DELETE",     ROREG_INT(4)},

    //Response class
    // { "Created",    NULL,   CODE(2,1)},
    // { "Deleted",    NULL,   CODE(2,2)},
    // { "Valid",      NULL,   CODE(2,3)},
    // { "Changed",    NULL,   CODE(2,4)},
    // { "Content",    NULL,   CODE(2,5)},

    // { "Bad Request",NULL,   CODE(4,0)},
    // { "Unauthorized",NULL,   CODE(4,1)},
    // { "Bad Option", NULL,   CODE(4,2)},
    // { "Forbidden",  NULL,   CODE(4,3)},
    // { "Not Found",  NULL,   CODE(4,4)},
    // { "Method Not Allowed",NULL,   CODE(4,5)},
    // { "Not Acceptable",    NULL,   CODE(4,6)},
    // { "Precondition Failed",    NULL,   CODE(4,12)},
    // { "Request Entity Too Large",    NULL,   CODE(4,13)},
    // { "Unsupported Content-Format",    NULL,   CODE(4,15)},

    // { "Internal Server Error",    NULL,   CODE(5,0)},
    // { "Not Implemented",    NULL,   CODE(5,1)},
    // { "Bad Gateway",    NULL,   CODE(5,2)},
    // { "Service Unavailable",    NULL,   CODE(5,3)},
    // { "Gateway Timeout",    NULL,   CODE(5,4)},
    // { "Proxying Not Supported",    NULL,   CODE(5,5)},
	{ NULL,  ROREG_INT(0) }
};

LUAMOD_API int luaopen_libcoap( lua_State *L ) {
    luat_newlib2(L, reg_libcoap);
    createmeta(L);
    return 1;
}
