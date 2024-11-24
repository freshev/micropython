#include "luat_base.h"
#include "luat_network_adapter.h"
#include "luat_rtos.h"
// #include "luat_msgbus.h"
#include "http_parser.h"
#include "luat_http.h"
#include "common_api.h"
#define HTTP_HEADER_BASE_SIZE 	(1024)
#define HTTP_RESP_BUFF_SIZE 		(4096)

static void http_network_error(luat_http_ctrl_t *http_ctrl);
static void http_send_message(luat_http_ctrl_t *http_ctrl);
static int32_t luat_lib_http_callback(void *data, void *param);

static void luat_http_dummy_cb(int status, void *data, uint32_t data_len, void *user_param) {;}

static void http_network_close(luat_http_ctrl_t *http_ctrl)
{
	http_ctrl->state = HTTP_STATE_WAIT_CLOSE;
	luat_rtos_timer_stop(http_ctrl->timeout_timer);
	if (!network_close(http_ctrl->netc, 0))
	{
		http_network_error(http_ctrl);
	}
}

static void http_network_error(luat_http_ctrl_t *http_ctrl)
{
	if (++(http_ctrl->retry_cnt))
	{
		if (http_ctrl->retry_cnt >= http_ctrl->retry_cnt_max)
		{
			if (http_ctrl->error_code > 0)
			{
				http_ctrl->error_code = HTTP_ERROR_STATE;
			}
			http_ctrl->http_cb(http_ctrl->error_code, NULL, 0, http_ctrl->http_cb_userdata);
			return;
		}
	}
	if (http_ctrl->debug_onoff)
	{
		DBG("retry %d", http_ctrl->retry_cnt);
	}
	http_ctrl->state = HTTP_STATE_CONNECT;
	if (http_ctrl->timeout)
	{
		luat_start_rtos_timer(http_ctrl->timeout_timer, http_ctrl->timeout, 1);
	}
	else
	{
		luat_stop_rtos_timer(http_ctrl->timeout_timer);
	}

	if (network_connect(http_ctrl->netc, http_ctrl->host, strlen(http_ctrl->host), NULL, http_ctrl->remote_port, 0) < 0)
	{
		DBG("http can not connect!");
		http_ctrl->state = HTTP_STATE_IDLE;
		http_ctrl->error_code = HTTP_ERROR_CONNECT;
		network_close(http_ctrl->netc, 0);
		http_ctrl->http_cb(http_ctrl->error_code, NULL, 0, http_ctrl->http_cb_userdata);
	}
}



static int on_header_field(http_parser* parser, const char *at, size_t length){
	char temp[16] = {':'};
	luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)parser->data;
	if (http_ctrl->state != HTTP_STATE_GET_HEAD)
	{
		DBG("!");
		return 0;
	}
	http_ctrl->response_head_buffer.Pos = 0;
	OS_BufferWrite(&http_ctrl->response_head_buffer, at, length);
	OS_BufferWrite(&http_ctrl->response_head_buffer, temp, 1);
    return 0;
}
	
static int on_header_value(http_parser* parser, const char *at, size_t length){
	char tmp[16] = {0};
	luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)parser->data;
	if (http_ctrl->state != HTTP_STATE_GET_HEAD){
		DBG("http state error %d", http_ctrl->state);
		return 0;
	}
	OS_BufferWrite(&http_ctrl->response_head_buffer, at, length);
	OS_BufferWrite(&http_ctrl->response_head_buffer, tmp, 1);
	http_ctrl->http_cb(HTTP_STATE_GET_HEAD, http_ctrl->response_head_buffer.Data, http_ctrl->response_head_buffer.Pos, http_ctrl->http_cb_userdata);
    return 0;
}

static int on_headers_complete(http_parser* parser){
	luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)parser->data;
	if (http_ctrl->state != HTTP_STATE_GET_HEAD){
		DBG("http state error %d", http_ctrl->state);
		return 0;
	}

	if (!http_ctrl->total_len)
	{
		if (http_ctrl->parser.content_length != -1)
		{
			http_ctrl->total_len = http_ctrl->parser.content_length;
		}
		else
		{
			DBG("no content lenght, maybe error!");
		}
	}

	if (http_ctrl->debug_onoff)
	{
		DBG("total %u done %u", http_ctrl->total_len, http_ctrl->done_len);
	}

	http_ctrl->http_cb(HTTP_STATE_GET_HEAD, NULL, 0, http_ctrl->http_cb_userdata);
	http_ctrl->state = HTTP_STATE_GET_BODY;
    return 0;
}

static int on_body(http_parser* parser, const char *at, size_t length){
	// DBG("on_body:%.*s",length,at);
	luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)parser->data;
	if (http_ctrl->state != HTTP_STATE_GET_BODY){
		DBG("http state error %d", http_ctrl->state);
		return 0;
	}
	http_ctrl->done_len += length;
	http_ctrl->http_cb(HTTP_STATE_GET_BODY, at, length, http_ctrl->http_cb_userdata);
    return 0;
}

static int on_message_complete(http_parser* parser){
	luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)parser->data;
	// http_ctrl->body[http_ctrl->body_len] = 0x00;
	if (http_ctrl->done_len != http_ctrl->total_len)
	{
		DBG("http rx body len error %u,%u", http_ctrl->done_len, http_ctrl->total_len);
	}
	http_ctrl->http_cb(HTTP_STATE_GET_BODY, NULL, 0, http_ctrl->http_cb_userdata);
	http_ctrl->error_code = 0;
	http_ctrl->state = HTTP_STATE_DONE;
	luat_rtos_timer_stop(http_ctrl->timeout_timer);
    return 0;
}

static int on_chunk_header(http_parser* parser){
    return 0;
}

static const http_parser_settings parser_settings = {
	.on_header_field = on_header_field,
	.on_header_value = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_body = on_body,
	.on_message_complete = on_message_complete,
	.on_chunk_header = on_chunk_header
};

static uint32_t http_send(luat_http_ctrl_t *http_ctrl, uint8_t* data, size_t len) {
	if (len == 0)
		return 0;
	uint32_t tx_len = 0;
	network_tx(http_ctrl->netc, data, len, 0, NULL, 0, &tx_len, 0);
	return tx_len;
}

static void http_send_message(luat_http_ctrl_t *http_ctrl){
	uint32_t tx_len = 0;
	int result;
	const char line[] = "Accept: application/octet-stream\r\n";
	uint8_t *temp = calloc(1, 320);
	http_ctrl->state = HTTP_STATE_SEND_HEAD;
	//Send request line, mainly, buff is borrowed here, but this will not conflict with resp
	http_send(http_ctrl, http_ctrl->request_line, strlen((char*)http_ctrl->request_line));
	// Determine whether custom headers have a host
	if (http_ctrl->custom_host == 0) {
		result = snprintf_((char*)temp, 320,  "Host: %s\r\n", http_ctrl->host);
		http_send(http_ctrl, temp, result);
	}

	if (http_ctrl->data_mode && (http_ctrl->offset || http_ctrl->done_len)){
		result = snprintf_(temp, 320,  "Range: bytes=%u-\r\n", (http_ctrl->offset + http_ctrl->done_len));
		if (http_ctrl->debug_onoff)
		{
			DBG("get offset %u+%u", http_ctrl->offset, http_ctrl->done_len);
		}
		http_send(http_ctrl, temp, result);
	}

	//Send custom header
	if (http_ctrl->request_head_buffer.Data && http_ctrl->request_head_buffer.Pos){
		http_send(http_ctrl, http_ctrl->request_head_buffer.Data, http_ctrl->request_head_buffer.Pos);
	}
	if (http_ctrl->data_mode)
	{
		http_send(http_ctrl, line, sizeof(line) - 1);
	}
	// end header
	http_send(http_ctrl, (uint8_t*)"\r\n", 2);
	// send body
	free(temp);
	http_ctrl->state = HTTP_STATE_GET_HEAD;

	if (http_ctrl->is_post)
	{
		http_ctrl->http_cb(HTTP_STATE_SEND_BODY_START, NULL, 0, http_ctrl->http_cb_userdata);
	}
}

static LUAT_RT_RET_TYPE luat_http_timer_callback(LUAT_RT_CB_PARAM){
	luat_http_ctrl_t * http_ctrl = (luat_http_ctrl_t *)param;
	if (http_ctrl->new_data)
	{
		http_ctrl->new_data = 0;
	}
	else
	{
		DBG("http timeout error!");
		http_ctrl->error_code = HTTP_ERROR_TIMEOUT;
		http_network_close(http_ctrl);
	}
}

static int32_t luat_lib_http_callback(void *data, void *param){
	OS_EVENT *event = (OS_EVENT *)data;
	luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)param;
	if (HTTP_STATE_IDLE == http_ctrl->state)
	{
		DBG("http state error %d", http_ctrl->state);
		return 0;
	}
	int ret = 0;
	size_t nParseBytes;
	uint32_t rx_len = 0;
	if (event->Param1){
		DBG("http network error!");
		http_ctrl->error_code = HTTP_ERROR_STATE;
		http_network_close(http_ctrl);
		return -1;
	}

	switch(event->ID)
	{
	case EV_NW_RESULT_EVENT:
		http_ctrl->new_data = 1;
		if (http_ctrl->is_pause)
		{
			if (http_ctrl->debug_onoff)
			{
				DBG("rx pause");
			}
			break;
		}
		while (1)
		{
			int result = network_rx(http_ctrl->netc, http_ctrl->response_cache.Data + http_ctrl->response_cache.Pos, http_ctrl->response_cache.MaxLen - http_ctrl->response_cache.Pos, 0, NULL, NULL, &rx_len);
			if (result)
			{
				http_ctrl->error_code = HTTP_ERROR_RX;
				http_network_close(http_ctrl);
			}
			if (rx_len > 0)
			{
				http_ctrl->response_cache.Pos += rx_len;
				nParseBytes = http_parser_execute(&http_ctrl->parser, &parser_settings, http_ctrl->response_cache.Data, http_ctrl->response_cache.Pos);
				OS_BufferRemove(&http_ctrl->response_cache, nParseBytes);
			}
			else
			{
				break;
			}
		}
		break;
	case EV_NW_RESULT_CONNECT:
		http_ctrl->response_cache.Pos = 0;
		http_parser_init(&http_ctrl->parser, HTTP_RESPONSE);
		http_ctrl->parser.data = http_ctrl;
		http_send_message(http_ctrl);
		break;
	case EV_NW_RESULT_TX:
		if (http_ctrl->is_post)
		{
			http_ctrl->http_cb(HTTP_STATE_SEND_BODY, NULL, 0, http_ctrl->http_cb_userdata);
		}
		http_ctrl->state = HTTP_STATE_GET_HEAD;
		break;
	case EV_NW_RESULT_CLOSE:
		if (http_ctrl->error_code && (http_ctrl->state != HTTP_STATE_DONE))
		{
			if (http_ctrl->debug_onoff)
			{
				DBG("http network closed");
			}
			http_network_error(http_ctrl);
		}
		else
		{
			http_ctrl->state = HTTP_STATE_IDLE;
			http_ctrl->http_cb(http_ctrl->state, NULL, 0, http_ctrl->http_cb_userdata);
		}
		return 0;
	case EV_NW_RESULT_LINK:
		return 0;
	}
	ret = network_wait_event(http_ctrl->netc, NULL, 0, NULL);

	if (ret < 0){
		http_ctrl->error_code = HTTP_ERROR_STATE;
		http_network_close(http_ctrl);
		return -1;
	}
    return 0;
}

luat_http_ctrl_t* luat_http_client_create(luat_http_cb cb, void *user_param, int adapter_index)
{
	luat_http_ctrl_t *http_ctrl = calloc(1, sizeof(luat_http_ctrl_t));
	if (!http_ctrl) return NULL;

	http_ctrl->timeout_timer = luat_create_rtos_timer(luat_http_timer_callback, http_ctrl, NULL);
	if (!http_ctrl->timeout_timer)
	{
		free(http_ctrl);
		DBG("no more timer");
		return NULL;
	}

	if (adapter_index >= 0)
	{
		http_ctrl->netc = network_alloc_ctrl(adapter_index);
	}
	else
	{
		http_ctrl->netc = network_alloc_ctrl(network_get_last_register_adapter());
	}
	if (!http_ctrl->netc)
	{
		luat_release_rtos_timer(http_ctrl->timeout_timer);
		free(http_ctrl);
		DBG("no more network ctrl");
		return NULL;
	}

	network_init_ctrl(http_ctrl->netc, NULL, luat_lib_http_callback, http_ctrl);
	network_set_base_mode(http_ctrl->netc, 1, 10000, 0, 0, 0, 0);
	network_set_local_port(http_ctrl->netc, 0);
	http_ctrl->http_cb = cb?cb:luat_http_dummy_cb;
	http_ctrl->http_cb_userdata = user_param;
	http_ctrl->timeout = 15000;
	http_ctrl->retry_cnt_max = 3;
	http_ctrl->state = HTTP_STATE_IDLE;
	http_ctrl->debug_onoff = 0;
	http_ctrl->netc->is_debug = 0;
	return http_ctrl;
}

int luat_http_client_base_config(luat_http_ctrl_t* http_ctrl, uint32_t timeout, uint8_t debug_onoff, uint8_t retry_cnt)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state)
	{
		if (http_ctrl->debug_onoff)
		{
			DBG("http running, please stop and set");
		}
		return -ERROR_PERMISSION_DENIED;
	}
	http_ctrl->timeout = timeout;
	http_ctrl->debug_onoff = debug_onoff;
	http_ctrl->netc->is_debug = debug_onoff;
	http_ctrl->retry_cnt_max = retry_cnt;
	http_ctrl->netc->is_debug = debug_onoff;
}

int luat_http_client_ssl_config(luat_http_ctrl_t* http_ctrl, int mode, const char *server_cert, uint32_t server_cert_len,
		const char *client_cert, uint32_t client_cert_len,
		const char *client_cert_key, uint32_t client_cert_key_len,
		const char *client_cert_key_password, uint32_t client_cert_key_password_len)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state)
	{
		if (http_ctrl->debug_onoff)
		{
			DBG("http running, please stop and set");
		}
		return -ERROR_PERMISSION_DENIED;
	}

	if (mode < 0)
	{
		network_deinit_tls(http_ctrl->netc);
		return 0;
	}
	if (mode > 2)
	{
		return -ERROR_PARAM_INVALID;
	}
	int result;
	network_init_tls(http_ctrl->netc, (server_cert || client_cert)?2:0);

	if (server_cert){
		result = network_set_server_cert(http_ctrl->netc, (const unsigned char *)server_cert, server_cert_len);
		if (result)
		{
			DBG("set server cert failed %d", result);
			return -ERROR_OPERATION_FAILED;
		}
	}
	if (client_cert){
		result = network_set_client_cert(http_ctrl->netc, (const unsigned char *)client_cert, client_cert_len,
				(const unsigned char *)client_cert_key, client_cert_key_len,
				(const unsigned char *)client_cert_key_password, client_cert_key_password_len);
		if (result)
		{
			DBG("set client cert failed %d", result);
			return -ERROR_OPERATION_FAILED;
		}
	}
	return 0;
}



int luat_http_client_clear(luat_http_ctrl_t *http_ctrl)
{
	OS_DeInitBuffer(&http_ctrl->request_head_buffer);
}


int luat_http_client_set_user_head(luat_http_ctrl_t *http_ctrl, const char *name, const char *value)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state)
	{
		if (http_ctrl->debug_onoff)
		{
			DBG("http running, please stop and set");
		}
		return -ERROR_PERMISSION_DENIED;
	}

	if (!http_ctrl->request_head_buffer.Data)
	{
		OS_InitBuffer(&http_ctrl->request_head_buffer, HTTP_RESP_BUFF_SIZE);
	}

	int ret = sprintf_(http_ctrl->request_head_buffer.Data + http_ctrl->request_head_buffer.Pos, "%s:%s\r\n", name, value);
	if (ret > 0)
	{
		http_ctrl->request_head_buffer.Pos += ret;
		if (!strcmp("Host", name) || !strcmp("host", name))
		{
			http_ctrl->custom_host = 1;
		}
		return 0;
	}
	else
	{
		return -ERROR_OPERATION_FAILED;
	}
}

int luat_http_client_start(luat_http_ctrl_t *http_ctrl, const char *url, uint8_t is_post, uint8_t ipv6, uint8_t data_mode)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state)
	{
		if (http_ctrl->debug_onoff)
		{
			DBG("http running, please stop and start");
		}
		return -ERROR_PERMISSION_DENIED;
	}
	http_ctrl->is_post = is_post;
	http_ctrl->data_mode = data_mode;
	http_ctrl->retry_cnt = 0;
	http_ctrl->total_len = 0;
	http_ctrl->done_len = 0;
	http_ctrl->remote_port = 0;
	http_ctrl->parser.status_code = 0;
	OS_ReInitBuffer(&http_ctrl->response_head_buffer, HTTP_HEADER_BASE_SIZE);
	OS_ReInitBuffer(&http_ctrl->response_cache, HTTP_RESP_BUFF_SIZE);
	network_connect_ipv6_domain(http_ctrl->netc, ipv6);

    if (http_ctrl->host)
    {
    	free(http_ctrl->host);
    	http_ctrl->host = NULL;
    }

    if (http_ctrl->request_line)
    {
    	free(http_ctrl->request_line);
    	http_ctrl->request_line = NULL;
    }

	char *tmp = url;
    if (!strncmp("https://", url, strlen("https://"))) {
        http_ctrl->is_tls = 1;
        tmp += strlen("https://");
    }
    else if (!strncmp("http://", url, strlen("http://"))) {
        http_ctrl->is_tls = 0;
        tmp += strlen("http://");
    }
    else {
        DBG("only http/https supported %s", url);
        return -ERROR_PARAM_INVALID;
    }

	int tmplen = strlen(tmp);
	if (tmplen < 5) {
        DBG("url too short %s", url);
        return -ERROR_PARAM_INVALID;
    }
	char tmphost[256] = {0};
    char *tmpuri = NULL;
    for (size_t i = 0; i < tmplen; i++){
        if (tmp[i] == '/') {
			if (i > 255) {
				DBG("host too long %s", url);
				return -ERROR_PARAM_INVALID;
			}
            tmpuri = tmp + i;
            break;
        }else if(i == tmplen-1){
			tmphost[i+2] = '/';
			tmpuri = tmp + i+1;
		}
		tmphost[i] = tmp[i];
    }
	if (strlen(tmphost) < 1) {
        DBG("host not found %s", url);
        return -ERROR_PARAM_INVALID;
    }
    if (strlen(tmpuri) == 0) {
        tmpuri = "/";
    }
    // DBG("tmphost:%s",tmphost);
	// DBG("tmpuri:%s",tmpuri);
    for (size_t i = 1; i < strlen(tmphost); i++){
		if (tmphost[i] == ':') {
			tmphost[i] = '\0';
			http_ctrl->remote_port = atoi(tmphost + i + 1);
			break;
		}
    }
    if (http_ctrl->remote_port <= 0) {
        if (http_ctrl->is_tls)
            http_ctrl->remote_port = 443;
        else
            http_ctrl->remote_port = 80;
    }

    http_ctrl->host = malloc(strlen(tmphost) + 1);
    if (http_ctrl->host == NULL) {
        DBG("out of memory when malloc host");
        return -ERROR_NO_MEMORY;
    }
    memcpy(http_ctrl->host, tmphost, strlen(tmphost) + 1);

	size_t linelen = strlen((char*)tmpuri) + 32;
    http_ctrl->request_line = malloc(linelen);
    if (http_ctrl->request_line == NULL) {
        DBG("out of memory when malloc url/request_line");
        return -ERROR_NO_MEMORY;
    }


	snprintf_((char*)http_ctrl->request_line, 8192, "%s %s HTTP/1.1\r\n", is_post?"POST":"GET", tmpuri);

	if (http_ctrl->timeout)
	{
		luat_start_rtos_timer(http_ctrl->timeout_timer, http_ctrl->timeout, 1);
	}
	else
	{
		luat_stop_rtos_timer(http_ctrl->timeout_timer);
	}

	http_ctrl->state = HTTP_STATE_CONNECT;
    if (http_ctrl->debug_onoff)
    {
    	DBG("http connect %s:%d", http_ctrl->host, http_ctrl->remote_port);
    }

    http_ctrl->error_code = HTTP_ERROR_CONNECT;
	if (network_connect(http_ctrl->netc, http_ctrl->host, strlen(http_ctrl->host), NULL, http_ctrl->remote_port, 0) < 0)
	{
		DBG("http can not connect!");
		network_close(http_ctrl->netc, 0);
		http_ctrl->state = HTTP_STATE_IDLE;
		return -1;
	}
	return 0;
}

int luat_http_client_close(luat_http_ctrl_t *http_ctrl)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;

	if (http_ctrl->debug_onoff)
	{
		DBG("user close http!");
	}

	http_ctrl->state = HTTP_STATE_WAIT_CLOSE;
	http_ctrl->retry_cnt = http_ctrl->retry_cnt_max;
	network_force_close_socket(http_ctrl->netc);
	luat_rtos_timer_stop(http_ctrl->timeout_timer);
	http_ctrl->state = HTTP_STATE_IDLE;
	http_ctrl->offset = 0;
	return 0;
}

int luat_http_client_destroy(luat_http_ctrl_t **p_http_ctrl)
{
	if (!p_http_ctrl) return -ERROR_PARAM_INVALID;
	luat_http_ctrl_t *http_ctrl = *p_http_ctrl;
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->debug_onoff)
	{
		DBG("user destroy http!");
	}
	http_ctrl->state = HTTP_STATE_WAIT_CLOSE;

	network_force_close_socket(http_ctrl->netc);
	luat_rtos_timer_stop(http_ctrl->timeout_timer);
	luat_release_rtos_timer(http_ctrl->timeout_timer);
	network_release_ctrl(http_ctrl->netc);

	if (http_ctrl->host){
		free(http_ctrl->host);
	}
	if (http_ctrl->request_line){
		free(http_ctrl->request_line);
	}
	OS_DeInitBuffer(&http_ctrl->request_head_buffer);
	OS_DeInitBuffer(&http_ctrl->response_head_buffer);
	OS_DeInitBuffer(&http_ctrl->response_cache);

	free(http_ctrl);
	*p_http_ctrl = NULL;
	return 0;
}

int luat_http_client_get_status_code(luat_http_ctrl_t *http_ctrl)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	return http_ctrl->parser.status_code;
}

int luat_http_client_set_get_offset(luat_http_ctrl_t *http_ctrl, uint32_t offset)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state)
	{
		if (http_ctrl->debug_onoff)
		{
			DBG("http running, stop and set!");
		}
		return -ERROR_PERMISSION_DENIED;
	}
	http_ctrl->offset = offset;
	return 0;
}

int luat_http_client_pause(luat_http_ctrl_t *http_ctrl, uint8_t is_pause)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state != HTTP_STATE_GET_BODY)
	{
		if (http_ctrl->debug_onoff)
		{
			DBG("http not recv body data, no use!");
		}
		return -ERROR_PERMISSION_DENIED;
	}
	if (http_ctrl->debug_onoff)
	{
		DBG("http pause state %d!", is_pause);
	}
	http_ctrl->is_pause = is_pause;
	if (!http_ctrl->is_pause)
	{
		OS_EVENT event = {EV_NW_RESULT_EVENT, 0, 0, 0};
		luat_lib_http_callback(&event, http_ctrl);
	}
}

int luat_http_client_post_body(luat_http_ctrl_t *http_ctrl, void *data, uint32_t len)
{
	if (!http_ctrl) return -ERROR_PARAM_INVALID;
	if (http_ctrl->state != HTTP_STATE_GET_HEAD)
	{
		return -ERROR_PERMISSION_DENIED;
	}
	http_send(http_ctrl, data, len);
}
