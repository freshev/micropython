#ifndef LUAT_HTTP_H
#define LUAT_HTTP_H
#include "http_parser.h"
#include "common_api.h"
// #define HTTP_REQUEST_BUF_LEN_MAX 	(1024)
enum
{
	HTTP_STATE_IDLE,
	HTTP_STATE_CONNECT,
	HTTP_STATE_SEND_HEAD,
	HTTP_STATE_SEND_BODY_START,
	HTTP_STATE_SEND_BODY,
	HTTP_STATE_GET_HEAD,
	HTTP_STATE_GET_BODY,
	HTTP_STATE_DONE,
	HTTP_STATE_WAIT_CLOSE,
};

#define HTTP_GET_DATA 		(2)
#define HTTP_POST_DATA 		(1)
#define HTTP_OK 			(0)
#define HTTP_ERROR_STATE 	(-1)
#define HTTP_ERROR_HEADER 	(-2)
#define HTTP_ERROR_BODY 	(-3)
#define HTTP_ERROR_CONNECT 	(-4)
#define HTTP_ERROR_CLOSE 	(-5)
#define HTTP_ERROR_RX 		(-6)
#define HTTP_ERROR_DOWNLOAD (-7)
#define HTTP_ERROR_TIMEOUT  (-8)
#define HTTP_ERROR_FOTA  	(-9)

#define HTTP_CALLBACK 		(1)

#define HTTP_RE_REQUEST_MAX (3)


/**
 * @defgroup luatos_HTTP HTTP(S) related interface
 * @{*/

/** Callback function of http running process
 * status >=0 indicates running status. HTTP_STATE_XXX <0 indicates an error has occurred and stopped. ==0 indicates the end.
 * data In the response phase, the head data will be called back. If it is NULL, the head reception is completed. If data_mode is set, body data will also be called back directly.
 * data_len data length. When the head receives, one more \0 will be added to each line to facilitate string processing.
 * user_param user's own parameters*/
typedef void (*luat_http_cb)(int status, void *data, uint32_t data_len, void *user_param);

typedef struct{
	network_ctrl_t *netc;		/**< http netc*/
	uint8_t is_tls;             /**< Whether SSL*/
	const char *host; 			/**< http host, needs to be released*/
	uint16_t remote_port; 		/**<Remote port number*/
	const char* request_line;	/**< needs to be released, the first line of data in the http request*/
	Buffer_Struct request_head_buffer;	/**<Storage user-defined request head data*/
	uint8_t custom_host;        /**<Whether the Host has been customized?*/
	luat_http_cb http_cb;				/**< http lua callback function*/
	void *http_cb_userdata;				/**< http lua callback function user passes parameters*/
	//Analysis related
	http_parser  parser;
	Buffer_Struct response_head_buffer;	/**<The received head data cache is destroyed after being called back to the client.*/
	uint32_t timeout;
	void* timeout_timer;			/**< timeout_timer timer*/
	int error_code;
	Buffer_Struct response_cache;
	uint32_t total_len;
	uint32_t done_len;
	uint32_t offset;
	uint8_t retry_cnt_max;		/**<Maximum number of retries*/
	uint8_t retry_cnt;
	uint8_t state;
	uint8_t data_mode;
	uint8_t is_pause;
	uint8_t debug_onoff;
	uint8_t new_data;
	uint8_t is_post;
}luat_http_ctrl_t;



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
 * @param is_post whether it is a post request
 * @param ipv6 whether there is an IPV6 server
 * @param data_mode Big data mode must be turned on when the received data exceeds 1KB. After turning it on, "Accept: application/octet-stream\r\n" is automatically added to the request header.
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_start(luat_http_ctrl_t *http_ctrl, const char *url, uint8_t is_post, uint8_t ipv6, uint8_t continue_mode);
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

int luat_http_client_get_status_code(luat_http_ctrl_t *http_ctrl);

int luat_http_client_pause(luat_http_ctrl_t *http_ctrl, uint8_t is_pause);
/**
 * @brief GET request requires the server to start transmitting data from the offset position, use with caution
 *
 * @param http_ctrl client
 * @param offset offset position
 * @return Returns 0 on success, other values   fail*/
int luat_http_client_set_get_offset(luat_http_ctrl_t *http_ctrl, uint32_t offset);
/** @}*/
#endif
