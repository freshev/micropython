#ifndef LUAT_HTTP_H
#define LUAT_HTTP_H

#ifdef __LUATOS__
#include "luat_zbuff.h"
#else
typedef enum {
    LUAT_HEAP_AUTO_DUMMY,
    LUAT_HEAP_SRAM_DUMMY,
    LUAT_HEAP_PSRAM_DUMMY,
} LUAT_HEAP_TYPE;//Just for placeholder, don't use it
typedef struct luat_zbuff {
	LUAT_HEAP_TYPE type; //memory type
    uint8_t* addr;      //Data storage address
    size_t len;       //The length of the actual allocated space
    union {
    	size_t cursor;    //The current pointer position indicates how much data has been processed
    	size_t used;	//The amount of data that has been saved indicates how much data is stored.
    };

    uint32_t width; //width
    uint32_t height;//high
    uint8_t bit;    //color depth
} luat_zbuff_t;
#endif


#if defined(AIR101) || defined(AIR103)
// #define HTTP_REQ_HEADER_MAX_SIZE 	(2048)
#define HTTP_RESP_BUFF_SIZE 		(2048)
#else
// #define HTTP_REQ_HEADER_MAX_SIZE 	(8192)
#define HTTP_RESP_BUFF_SIZE 		(8192)
#endif
#define HTTP_HEADER_BASE_SIZE 	(1024)
#include "http_parser.h"

#define HTTP_CALLBACK 		(1)
#define HTTP_RE_REQUEST_MAX (3)
#define HTTP_TIMEOUT 		(10*60*1000) // 10 minutes

/**
 * @defgroup luatos_HTTP HTTP(S) related interface
 * @{*/
enum{
	HTTP_STATE_IDLE,
	HTTP_STATE_CONNECT,
	HTTP_STATE_SEND_HEAD,
	HTTP_STATE_SEND_BODY_START,
	HTTP_STATE_SEND_BODY,
	HTTP_STATE_GET_HEAD,
    HTTP_STATE_GET_HEAD_DONE,
	HTTP_STATE_GET_BODY,
    HTTP_STATE_GET_BODY_DONE,
	HTTP_STATE_DONE,
	HTTP_STATE_WAIT_CLOSE,
};

enum{
	HTTP_OK = 0,
    HTTP_ERROR_STATE 	= -1,
    HTTP_ERROR_HEADER 	= -2,
    HTTP_ERROR_BODY 	= -3,
    HTTP_ERROR_CONNECT 	= -4,
    HTTP_ERROR_CLOSE 	= -5,
    HTTP_ERROR_RX 		= -6,
    HTTP_ERROR_DOWNLOAD = -7,
    HTTP_ERROR_TIMEOUT  = -8,
    HTTP_ERROR_FOTA  	= -9,
};



/** Callback function of http running process
 * status >=0 indicates running status. HTTP_STATE_XXX <0 indicates an error has occurred and stopped. ==0 indicates the end.
 * data In the response phase, the head data will be called back. If it is NULL, the head reception is completed. If data_mode is set, body data will also be called back directly.
 * data_len data length. When the head receives, one more \0 will be added to each line to facilitate string processing.
 * user_param user's own parameters*/
typedef void (*luat_http_cb)(int status, void *data, uint32_t data_len, void *user_param);

#define HTTP_GET_DATA 		(2)
#define HTTP_POST_DATA 		(1)


typedef struct{
	network_ctrl_t *netc;		// http netc
	http_parser  parser;	    //Analysis related
	char *host; 			/**< http host, needs to be released*/
	char* request_line;	/**< needs to be released, the first line of data in the http request*/
	uint16_t remote_port; 		/**<Remote port number*/
	uint8_t is_tls;             // Whether SSL
	uint8_t custom_host;        /**<Whether the Host has been customized?*/
	uint8_t is_post;
	uint8_t re_request_count;
	void* timeout_timer;			/**< timeout_timer timer*/
	uint32_t timeout;
	uint32_t tx_offset;
	// Send related
	char *req_header;
	char *req_body;				//Send body
	size_t req_body_len;		//Send body length
    char *req_auth;
	void* http_cb;				/**< http callback function*/
	void* http_cb_userdata;				/**< http callback function user-passed parameters*/
	uint8_t is_pause;
	uint8_t debug_onoff;
    uint8_t headers_complete;
    uint8_t close_state;
	char resp_buff[HTTP_RESP_BUFF_SIZE];
	size_t resp_buff_offset;
	size_t resp_headers_done;
	uint32_t body_len;			//body cache length

#ifdef LUAT_USE_FOTA
	//OTA related
	uint8_t isfota;				//Whether it is ota download
	uint32_t address;			
	uint32_t length;		
	luat_spi_device_t* spi_device;
#endif
	//Download related
	uint8_t is_download;		//Whether to download
	char *dst;			//Download path
	// http_parser_settings parser_settings;
	char* headers;
	uint32_t headers_len;		//headers cache length
	char* body;

	//Response related
	int32_t resp_content_len;	//content length
	FILE* fd;					//Download FILE
	luat_ip_addr_t ip_addr;		// http ip
	uint64_t idp;
	luat_zbuff_t *zbuff_body;

	Buffer_Struct request_head_buffer;	/**<Storage user-defined request head data*/
	Buffer_Struct response_head_buffer;	/**<The received head data cache is destroyed after being called back to the client.*/
	int error_code;
	uint32_t offset;
	uint32_t context_len;
	uint8_t retry_cnt_max;		/**<Maximum number of retries*/
	uint8_t state;
	uint8_t data_mode;
	uint8_t new_data;
	uint8_t context_len_vaild;
	uint8_t luatos_mode;

}luat_http_ctrl_t;

//The following 2 APIs are used internally by luatos and not used by csdk
int luat_http_client_init(luat_http_ctrl_t* http, int ipv6);
int luat_http_client_start_luatos(luat_http_ctrl_t* http);

/**
 * @brief Create an http client
 *
 * @param cb http running process callback function
 * @param user_param User’s own parameters during callback
 * @param adapter_index network card adapter, write -1 if unclear, the system automatically assigns it
 * @return Returns the client address successfully, returns NULL on failure*/
luat_http_ctrl_t* luat_http_client_create(luat_http_cb cb, void *user_param, int adapter_index);
/**
 * @brief The general configuration of the http client. There is already a default configuration when creating the client. It does not need to be configured.
 *
 * @param http_ctrl client
 * @param timeout single data transmission timeout, unit ms
 * @param debug_onoff Whether to enable debugging printing, which will occupy some system resources.
 * @param retry_cnt The maximum number of retransmissions due to transmission exceptions
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_base_config(luat_http_ctrl_t* http_ctrl, uint32_t timeout, uint8_t debug_onoff, uint8_t retry_cnt);
/**
 * @brief Client SSL configuration, configuration is only required when accessing https
 *
 * @param http_ctrl client
 * @param mode <0 turns off the SSL function and ignores subsequent parameters; 0 ignores the certificate verification process, most https applications can be configured in this way, and subsequent certificate configurations can be written with NULL and 0; 2 forces certificate verification, and subsequent certificate related parameters must write correctly
 * @param server_cert Server certificate string, there must be 0 at the end. If certificate verification is not ignored, this must be
 * @param server_cert_len Server certificate data length, the length includes the trailing 0, that is, strlen(server_cert) + 1
 * @param client_cert Client certificate string, there must be 0 at the end, only for two-way authentication, which may be used in the general financial industry
 * @param client_cert_len client certificate data length, including the trailing 0
 * @param client_cert_key Client certificate private key string, which must have 0 at the end. It is only available for two-way authentication. It may be used in the general financial industry.
 * @param client_cert_key_len Client certificate private key data length, the length includes the trailing 0
 * @param client_cert_key_password The client certificate private key password string, which must have 0 at the end, is only available for two-way authentication. If the private key is not password protected, it is not required.
 * @param client_cert_key_password_len The length of the client certificate private key password data, the length includes the trailing 0
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_ssl_config(luat_http_ctrl_t* http_ctrl, int mode, const char *server_cert, uint32_t server_cert_len,
		const char *client_cert, uint32_t client_cert_len,
		const char *client_cert_key, uint32_t client_cert_key_len,
		const char *client_cert_key_password, uint32_t client_cert_key_password_len);

/**
 * @brief Clear the POST data and request head parameters set by the user
 *
 * @param http_ctrl client
 * @return Returns 0 on success, other values   fail*/

int luat_http_client_clear(luat_http_ctrl_t *http_ctrl);

/**
 * @brief Set a user's request head parameter. Content-Length is generally not needed. It is automatically generated when setting the POST body.
 *
 * @param http_ctrl client
 * @param name The name of the head parameter
 * @param value The value of the head parameter
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_set_user_head(luat_http_ctrl_t *http_ctrl, const char *name, const char *value);


/**
 * @brief starts an http request
 *
 * @param http_ctrl client
 * @param url http requests the complete url. If there are escape characters, they need to be escaped in advance.
 * @param type request type, 0 get 1 post 2 put 3 delete
 * @param ipv6 whether there is an IPV6 server
 * @param data_mode Big data mode must be turned on when the received data exceeds 1KB. After turning it on, "Accept: application/octet-stream\r\n" is automatically added to the request header.
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_start(luat_http_ctrl_t *http_ctrl, const char *url, uint8_t type, uint8_t ipv6, uint8_t continue_mode);
/**
 * @brief Stop the current http request, there will be no http callback after the call
 *
 * @param http_ctrl client
 * @return Returns 0 on success, other values   fail*/

int luat_http_client_close(luat_http_ctrl_t *http_ctrl);
/**
 * @brief Completely release the current http client
 *
 * @param p_http_ctrl The address of the client pointer
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_destroy(luat_http_ctrl_t **p_http_ctrl);

/**
 * @brief The body data is sent during the POST request. If the amount of data is relatively large, it can be sent in batches in the HTTP_STATE_SEND_BODY callback.
 *
 * @param http_ctrl client
 * @param data body data
 * @param len body data length
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_post_body(luat_http_ctrl_t *http_ctrl, void *data, uint32_t len);
/**
 * @brief http gets status code
 *
 * @param http_ctrl client
 * @return status code*/
int luat_http_client_get_status_code(luat_http_ctrl_t *http_ctrl);
/**
 * @brief http client settings pause
 *
 * @param http_ctrl client
 * @param is_pause whether to pause
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_pause(luat_http_ctrl_t *http_ctrl, uint8_t is_pause);
/**
 * @brief GET request requires the server to start transmitting data from the offset position, use with caution
 *
 * @param http_ctrl client
 * @param offset offset position
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_set_get_offset(luat_http_ctrl_t *http_ctrl, uint32_t offset);
/**
 * @brief Get context length
 *
 * @param http_ctrl client
 * @param len context length value
 * @return Returns 0 successfully, other values   fail or are chunk encoding*/
int luat_http_client_get_context_len(luat_http_ctrl_t *http_ctrl, uint32_t *len);
/** @}*/
#endif

