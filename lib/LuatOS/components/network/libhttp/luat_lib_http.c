/*@Modules http
@summary http client
@version 1.0
@date 2022.09.05
@demo http
@tagLUAT_USE_NETWORK
@usage
--To use the http library, you need to Introduction the sysplus library and use it within a task.
require "sys"
require "sysplus"

sys.taskInit(function()
sys.wait(1000)
local code,headers,body = http.request("GET", "http://www.example.com/abc").wait()
log.info("http", code, body)
end)*/

#include "luat_base.h"
#include "luat_spi.h"
#include "luat_network_adapter.h"
#include "luat_rtos.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "luat_mem.h"
#include "http_parser.h"
#include "luat_http.h"

#define LUAT_LOG_TAG "http"
#include "luat_log.h"


#ifndef LUAT_HTTP_DEBUG
#define LUAT_HTTP_DEBUG 0
#endif
#if LUAT_HTTP_DEBUG == 0
#undef LLOGD
#define LLOGD(...)
#endif

int http_close(luat_http_ctrl_t *http_ctrl);
int http_set_url(luat_http_ctrl_t *http_ctrl, const char* url, const char* method);

static int http_add_header(luat_http_ctrl_t *http_ctrl, const char* name, const char* value){
	if (name == NULL || value == NULL || strlen(name) == 0 || strlen(value) == 0) {
		return -1;
	}
	size_t len = strlen(name) + strlen(value) + 4;
	if (http_ctrl->req_header == NULL) {
		http_ctrl->req_header = luat_heap_malloc(len + 1);
		if (http_ctrl->req_header == NULL) {
			LLOGE("out of memory when malloc custom headers");
			return -1;
		}
		http_ctrl->req_header[0] = 0;
	}
	else {
		void *ptr = luat_heap_realloc(http_ctrl->req_header, strlen(http_ctrl->req_header) + len + 1);
		if (ptr == NULL) {
			LLOGE("out of memory when malloc custom headers");
			return -1;
		}
		http_ctrl->req_header = ptr;
	}
	memcpy(http_ctrl->req_header + strlen(http_ctrl->req_header), name, strlen(name) + 1);
	memcpy(http_ctrl->req_header + strlen(http_ctrl->req_header), ": ", 3);
	memcpy(http_ctrl->req_header + strlen(http_ctrl->req_header), value, strlen(value) + 1);
	memcpy(http_ctrl->req_header + strlen(http_ctrl->req_header), "\r\n", 3);
	return 0;
}

/*http client
@api http.request(method,url,headers,body,opts,ca_file,client_ca, client_key, client_password)
@string request method, supports legal HTTP methods such as GET/POST
@string url address, supports http and https, supports domain name, supports custom port
@tabal request header optional For example {["Content-Type"] = "application/x-www-form-urlencoded"}
@string/zbuff body optional
@table Additional configuration optional includes timeout: timeout unit ms is optional, the default is 10 minutes, write 0 to wait forever dst: download path, optional adapter: choose to use the network card, optional debug: whether to turn on debug information, optional, ipv6: Whether it is ipv6. The default is not. Optional callback: download callback function. Parameter content_len: total length. body_len: parameter passed by the user in the form of download length userdata. Optional userdata: callback custom parameter passed.
@string server ca certificate data, optional, generally not needed
@string client ca certificate data, optional, generally not needed, only required for two-way https authentication
@string Client private key encrypted data, optional, generally not required, only required for two-way https authentication
@string Client private key password data, optional, generally not required, only required for two-way https authentication
@return int code, the value returned by the server is >=100, the most common is 200. If it is a low-level error, such as connection failure, the return value is less than 0
@return tabal headers When code>100, it represents the header data returned by the server
@return string/int body The content string of the server response. If it is download mode, the file size is returned.
@usage

--[[
Code error message list:
-1 HTTP_ERROR_STATE Error status, usually underlying exception, please report an issue
-2 HTTP_ERROR_HEADER Wrong response header, usually a server problem
-3 HTTP_ERROR_BODY Wrong response body, usually a server problem
-4 HTTP_ERROR_CONNECT Failed to connect to the server, not connected to the Internet, wrong address, wrong domain name
-5 HTTP_ERROR_CLOSE The connection was disconnected early, network or server problem
-6 HTTP_ERROR_RX Error in receiving data, network problem
-7 HTTP_ERROR_DOWNLOAD An error occurred during file downloading, network problem or download path problem
-8 HTTP_ERROR_TIMEOUT timeout, including connection timeout and data reading timeout
-9 HTTP_ERROR_FOTA The fota function reports an error, usually the update package is illegal.
]]

-- GET request
local code, headers, body = http.request("GET","http://site0.cn/api/httptest/simple/time").wait()
log.info("http.get", code, headers, body)
-- POST request
local code, headers, body = http.request("POST","http://httpbin.com/post", {}, "abc=123").wait()
log.info("http.post", code, headers, body)

-- GET request, but download to file
local code, headers, body = http.request("GET","http://httpbin.com/", {}, "", {dst="/data.bin"}).wait()
log.info("http.get", code, headers, body)

-- Custom timeout, 5000ms
http.request("GET","http://httpbin.com/", nil, nil, {timeout=5000}).wait()*/
static int l_http_request(lua_State *L) {
	size_t server_cert_len = 0,client_cert_len = 0, client_key_len = 0, client_password_len = 0,len = 0;
	const char *server_cert = NULL;
	const char *client_cert = NULL;
	const char *client_key = NULL;
	const char *client_password = NULL;
	int adapter_index = -1;
	char body_len[16] = {0};
	// mbedtls_debug_set_threshold(4);

	luat_http_ctrl_t *http_ctrl = (luat_http_ctrl_t *)luat_heap_malloc(sizeof(luat_http_ctrl_t));
	if (!http_ctrl){
		LLOGE("out of memory when malloc http_ctrl");
        lua_pushinteger(L,HTTP_ERROR_CONNECT);
		luat_pushcwait_error(L,1);
		return 1;
	}
	memset(http_ctrl, 0, sizeof(luat_http_ctrl_t));

	http_ctrl->timeout = HTTP_TIMEOUT;
	int use_ipv6 = 0;
	int is_debug = 0;

	if (lua_istable(L, 5)){
		lua_pushstring(L, "adapter");
		if (LUA_TNUMBER == lua_gettable(L, 5)) {
			adapter_index = luaL_optinteger(L, -1, network_get_last_register_adapter());
		}else{
			adapter_index = network_get_last_register_adapter();
		}
		lua_pop(L, 1);

		lua_pushstring(L, "timeout");
		if (LUA_TNUMBER == lua_gettable(L, 5)) {
			http_ctrl->timeout = luaL_optinteger(L, -1, HTTP_TIMEOUT);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "dst");
		if (LUA_TSTRING == lua_gettable(L, 5)) {
			const char *dst = luaL_checklstring(L, -1, &len);
			http_ctrl->dst = luat_heap_malloc(len + 1);
			memset(http_ctrl->dst, 0, len + 1);
			memcpy(http_ctrl->dst, dst, len);
			http_ctrl->is_download = 1;
		}
		lua_pop(L, 1);

		lua_pushstring(L, "debug");
		if (LUA_TBOOLEAN == lua_gettable(L, 5)) {
			is_debug = lua_toboolean(L, -1);
		}
		lua_pop(L, 1);

#ifdef LUAT_USE_FOTA
		http_ctrl->address = 0xffffffff;
		http_ctrl->length = 0;
		lua_pushstring(L, "fota");
		int type = lua_gettable(L, 5);
		if (LUA_TBOOLEAN == type) {
			http_ctrl->isfota = lua_toboolean(L, -1);
		}else if (LUA_TTABLE == type) {
			http_ctrl->isfota = 1;
			lua_pushstring(L, "address");
			if (LUA_TNUMBER == lua_gettable(L, -2)) {
				http_ctrl->address = luaL_checkinteger(L, -1);
			}
			lua_pop(L, 1);
			lua_pushstring(L, "length");
			if (LUA_TNUMBER == lua_gettable(L, -2)) {
				http_ctrl->length = luaL_checkinteger(L, -1);
			}
			lua_pop(L, 1);
			lua_pushstring(L, "param1");
			if (LUA_TUSERDATA == lua_gettable(L, -2)) {
				http_ctrl->spi_device = (luat_spi_device_t*)lua_touserdata(L, -1);
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
#endif
		lua_pushstring(L, "ipv6");
		if (LUA_TBOOLEAN == lua_gettable(L, 5) && lua_toboolean(L, -1)) {
			use_ipv6 = 1;
		}
		lua_pop(L, 1);

		lua_pushstring(L, "callback");
		if (LUA_TFUNCTION == lua_gettable(L, 5)) {
			http_ctrl->http_cb = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		if (http_ctrl->http_cb){
			lua_pushstring(L, "userdata");
			lua_gettable(L, 5);
			http_ctrl->http_cb_userdata = luaL_ref(L, LUA_REGISTRYINDEX);
		}
	}else{
		adapter_index = network_get_last_register_adapter();
	}
#ifdef LUAT_USE_FOTA
	if (http_ctrl->isfota == 1 && http_ctrl->is_download == 1){
		LLOGE("Only one can be selected for FOTA and Download");
		goto error;
	}
#endif
	if (adapter_index < 0 || adapter_index >= NW_ADAPTER_QTY){
		LLOGE("bad network adapter index %d", adapter_index);
		goto error;
	}

	http_ctrl->netc = network_alloc_ctrl((uint8_t)adapter_index);
	if (!http_ctrl->netc){
		LLOGE("netc create fail");
		goto error;
	}

    luat_http_client_init(http_ctrl, use_ipv6);
	http_ctrl->netc->is_debug = (uint8_t)is_debug;
	http_ctrl->debug_onoff = (uint8_t)is_debug;
	const char *method = luaL_optlstring(L, 1, "GET", &len);
	if (len > 11) {
		LLOGE("method is too long %s", method);
		goto error;
	}
	// memcpy(http_ctrl->method, method, len + 1);
	// LLOGD("method:%s",http_ctrl->method);

	if (strcmp("POST", method) == 0 || strcmp("PUT", method) == 0){
		http_ctrl->is_post = 1;
	}
	
	const char *url = luaL_checklstring(L, 2, &len);
	// http_ctrl->url = luat_heap_malloc(len + 1);
	// memset(http_ctrl->url, 0, len + 1);
	// memcpy(http_ctrl->url, url, len);
    

	int ret = http_set_url(http_ctrl, url, method);
	if (ret){
		goto error;
	}

	// LLOGD("http_ctrl->url:%s",http_ctrl->url);
#ifndef LUAT_USE_TLS
		if (http_ctrl->is_tls){
			LLOGE("NOT SUPPORT TLS");
			goto error;
		}
#endif

	if (lua_istable(L, 3)) {
		lua_pushnil(L);
		while (lua_next(L, 3) != 0) {
			const char *name = lua_tostring(L, -2);
			const char *value = lua_tostring(L, -1);
			if (!strcmp("Host", name) || !strcmp("host", name)) {
				http_ctrl->custom_host = 1;
			}
			if (strcmp("Content-Length", name)) {
				http_add_header(http_ctrl,name,value);
			}
			lua_pop(L, 1);
		}
	}
	if (lua_isstring(L, 4)) {
		const char *body = luaL_checklstring(L, 4, &(http_ctrl->req_body_len));
		http_ctrl->req_body = luat_heap_malloc((http_ctrl->req_body_len) + 1);
		// TODO checks whether req_body is NULL
		memset(http_ctrl->req_body, 0, (http_ctrl->req_body_len) + 1);
		memcpy(http_ctrl->req_body, body, (http_ctrl->req_body_len));
		snprintf_(body_len, 16,"%d",(http_ctrl->req_body_len));
		http_add_header(http_ctrl,"Content-Length",body_len);
	}else if(lua_isuserdata(L, 4)){//zbuff
		http_ctrl->zbuff_body = ((luat_zbuff_t *)luaL_checkudata(L, 4, LUAT_ZBUFF_TYPE));
		if (http_ctrl->is_post){
			snprintf_(body_len, 16,"%d",(http_ctrl->zbuff_body->used));
			http_add_header(http_ctrl,"Content-Length",body_len);
		}
	}

    // TODO realloc req_header

	if (http_ctrl->is_tls){
		if (lua_isstring(L, 6)){
			server_cert = luaL_checklstring(L, 6, &server_cert_len);
		}
		if (lua_isstring(L, 7)){
			client_cert = luaL_checklstring(L, 7, &client_cert_len);
		}
		if (lua_isstring(L, 8)){
			client_key = luaL_checklstring(L, 8, &client_key_len);
		}
		if (lua_isstring(L, 9)){
			client_password = luaL_checklstring(L, 9, &client_password_len);
		}
		network_init_tls(http_ctrl->netc, (server_cert || client_cert)?2:0);
		if (server_cert){
			network_set_server_cert(http_ctrl->netc, (const unsigned char *)server_cert, server_cert_len+1);
		}
		if (client_cert){
			network_set_client_cert(http_ctrl->netc, (const unsigned char *)client_cert, client_cert_len+1,
					(const unsigned char *)client_key, client_key_len+1,
					(const unsigned char *)client_password, client_password_len+1);
		}
	}else{
		network_deinit_tls(http_ctrl->netc);
	}

	network_set_ip_invaild(&http_ctrl->ip_addr);
	http_ctrl->idp = luat_pushcwait(L);

    if (luat_http_client_start_luatos(http_ctrl)) {
        goto error;
    }
    return 1;
error:
	// if (http_ctrl->timeout_timer){
	// 	luat_stop_rtos_timer(http_ctrl->timeout_timer);
	// }
	http_close(http_ctrl);
    lua_pushinteger(L,HTTP_ERROR_CONNECT);
	luat_pushcwait_error(L,1);
	return 1;
}

#include "rotable2.h"
const rotable_Reg_t reg_http[] =
{
	{"request",			ROREG_FUNC(l_http_request)},
	{ NULL,             ROREG_INT(0)}
};

const rotable_Reg_t reg_http_emtry[] =
{
	{ NULL,             ROREG_INT(0)}
};

LUAMOD_API int luaopen_http( lua_State *L ) {
#ifdef LUAT_USE_NETWORK
    luat_newlib2(L, reg_http);
#else
    luat_newlib2(L, reg_http_emtry);
	LLOGE("reg_http require network enable!!");
#endif
    lua_pushvalue(L, -1);
    lua_setglobal(L, "http2"); 
    return 1;
}

//------------------------------------------------------
int32_t l_http_callback(lua_State *L, void* ptr){
	(void)ptr;
	char* temp;
	char* header;
	char* value;
	uint16_t header_len = 0,value_len = 0;

    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)msg->ptr;
	uint64_t idp = http_ctrl->idp;
	if (http_ctrl->timeout_timer){
		luat_stop_rtos_timer(http_ctrl->timeout_timer);
		luat_release_rtos_timer(http_ctrl->timeout_timer);
		http_ctrl->timeout_timer = NULL;
	}
	LLOGD("l_http_callback arg1:%d is_download:%d idp:%d",msg->arg1,http_ctrl->is_download,idp);
	if (msg->arg1!=0 && msg->arg1!=HTTP_ERROR_FOTA ){
		if (msg->arg1 == HTTP_CALLBACK){
			lua_geti(L, LUA_REGISTRYINDEX, http_ctrl->http_cb);
			// int userdata_type = lua_type(L, -2);
			if (lua_isfunction(L, -1)) {
				lua_pushinteger(L, http_ctrl->resp_content_len);
				lua_pushinteger(L, msg->arg2);
				if (http_ctrl->http_cb_userdata){
					lua_geti(L, LUA_REGISTRYINDEX, http_ctrl->http_cb_userdata);
					lua_call(L, 3, 0);
				}else{
					lua_call(L, 2, 0);
				}
			}
			return 0;
		}else{
			lua_pushinteger(L, msg->arg1); //Return the error code
			luat_cbcwait(L, idp, 1);
			goto exit;
		}
	}
	
	lua_pushinteger(L, msg->arg1==HTTP_ERROR_FOTA?HTTP_ERROR_FOTA:http_ctrl->parser.status_code);
	lua_newtable(L);
	// LLOGD("http_ctrl->headers:%.*s",http_ctrl->headers_len,http_ctrl->headers);
	header = http_ctrl->headers;
	while ( (http_ctrl->headers_len)>0 ){
		value = strstr(header,":")+1;
		if (value[1]==' '){
			value++;
		}
		temp = strstr(value,"\r\n")+2;
		header_len = (uint16_t)(value-header)-1;
		value_len = (uint16_t)(temp-value)-2;
		LLOGD("header:%.*s",header_len,header);
		LLOGD("value:%.*s",value_len,value);
		lua_pushlstring(L, header,header_len);
		lua_pushlstring(L, value,value_len);
		lua_settable(L, -3);
		http_ctrl->headers_len -= temp-header;
		header = temp;
	}
	// LLOGD("http_ctrl->body:%.*s len:%d",http_ctrl->body_len,http_ctrl->body,http_ctrl->body_len);
	// When processing the body, we need to distinguish between download mode and non-download mode.
	if (http_ctrl->is_download) {
		// Download mode
		if (http_ctrl->fd == NULL) {
			// The download operation is all normal, and the length is returned
			lua_pushinteger(L, http_ctrl->body_len);
			luat_cbcwait(L, idp, 3); // code, headers, body
			goto exit;
		}else if (http_ctrl->fd != NULL) {
			//Download interrupted!!
			luat_fs_fclose(http_ctrl->fd);
			luat_fs_remove(http_ctrl->dst); //remove file
		}
		// Download failed, return error code
		lua_pushinteger(L, -1);
		luat_cbcwait(L, idp, 3); // code, headers, body
		goto exit;
	}
#ifdef LUAT_USE_FOTA
	else if(http_ctrl->isfota && http_ctrl->parser.status_code == 200){
		lua_pushinteger(L, http_ctrl->body_len);
		luat_cbcwait(L, idp, 3); // code, headers, body
	}
#endif
	else {
		// non-download mode
		lua_pushlstring(L, http_ctrl->body, http_ctrl->body_len);
		luat_cbcwait(L, idp, 3); // code, headers, body
	}
exit:
	if (http_ctrl->http_cb){
		luaL_unref(L, LUA_REGISTRYINDEX, http_ctrl->http_cb);
		http_ctrl->http_cb = 0;
		if (http_ctrl->http_cb_userdata){
			luaL_unref(L, LUA_REGISTRYINDEX, http_ctrl->http_cb_userdata);
			http_ctrl->http_cb_userdata = 0;
		}
	}
	http_close(http_ctrl);
	return 0;
}

void luat_http_client_onevent(luat_http_ctrl_t *http_ctrl, int error_code, int arg) {
	// network_close(http_ctrl->netc, 0);
	rtos_msg_t msg = {0};
	msg.handler = l_http_callback;
	msg.ptr = http_ctrl;
	msg.arg1 = error_code;
	msg.arg2 = arg;
	luat_msgbus_put(&msg, 0);
}
