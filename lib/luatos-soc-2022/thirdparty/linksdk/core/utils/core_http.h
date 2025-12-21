#ifndef _CORE_HTTP_H_
#define _CORE_HTTP_H_


#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "core_string.h"
#include "core_log.h"
#include "core_auth.h"
#include "aiot_http_api.h"

typedef enum {
    CORE_HTTP_SM_READ_HEADER,
    CORE_HTTP_SM_READ_BODY
} core_http_sm_t;

typedef struct {
    core_http_sm_t sm;
    uint32_t body_total_len;
    uint32_t body_read_len;
} core_http_session_t;

typedef struct {
    uint32_t code;
    uint8_t *content;
    uint32_t content_len;
    uint32_t content_total_len;
} core_http_response_t;

typedef struct {
    aiot_sysdep_portfile_t *sysdep;
    void *network_handle;
    char *host;
    uint16_t port;
    char *product_key;
    char *device_name;
    char *device_secret;
    char *extend_devinfo;
    uint32_t connect_timeout_ms;
    uint32_t send_timeout_ms;
    uint32_t recv_timeout_ms;
    uint32_t auth_timeout_ms;
    uint32_t deinit_timeout_ms;
    uint32_t header_line_max_len;
    uint32_t body_buffer_max_len;
    aiot_sysdep_network_cred_t *cred;
    char *token;
    uint8_t long_connection;
    uint8_t exec_enabled;
    uint32_t exec_count;
    uint8_t core_exec_enabled;
    uint32_t core_exec_count;
    void *data_mutex;
    void *send_mutex;
    void *recv_mutex;
    core_http_session_t session;
    aiot_http_event_handler_t event_handler;
    aiot_http_recv_handler_t recv_handler;
    aiot_http_recv_handler_t core_recv_handler;
    void *userdata;
    void *core_userdata;
} core_http_handle_t;

#define CORE_HTTP_MODULE_NAME "HTTP"
#define CORE_HTTP_DEINIT_INTERVAL_MS               (100)

#define CORE_HTTP_DEFAULT_CONNECT_TIMEOUT_MS       (10 * 1000)
#define CORE_HTTP_DEFAULT_AUTH_TIMEOUT_MS          (5 * 1000)
#define CORE_HTTP_DEFAULT_SEND_TIMEOUT_MS          (5 * 1000)
#define CORE_HTTP_DEFAULT_RECV_TIMEOUT_MS          (5 * 1000)
#define CORE_HTTP_DEFAULT_HEADER_LINE_MAX_LEN      (128)
#define CORE_HTTP_DEFAULT_BODY_MAX_LEN             (128)
#define CORE_HTTP_DEFAULT_DEINIT_TIMEOUT_MS        (2 * 1000)

typedef enum {
    CORE_HTTPOPT_HOST,                  /*Data type: (char *), server domain name, default value: iot-as-http.cn-shanghai.aliyuncs.com*/
    CORE_HTTPOPT_PORT,                  /*Data type: (uint16_t), server port number, default value: 443*/
    CORE_HTTPOPT_NETWORK_CRED,          /*Data type: (aiot_sysdep_network_cred_t *), network security credential, default value: NULL*/
    CORE_HTTPOPT_CONNECT_TIMEOUT_MS,    /*Data type: (uint32_t), timeout for establishing network connection*/
    CORE_HTTPOPT_SEND_TIMEOUT_MS,       /*Data type: (uint32_t), network transmission timeout (unit ms), default value: 5000ms*/
    CORE_HTTPOPT_RECV_TIMEOUT_MS,       /*Data type: (uint32_t), network acceptance timeout (unit ms), default value: 5000ms*/
    CORE_HTTPOPT_DEINIT_TIMEOUT_MS,     /*Data type: (uint32_t), when destroying the http instance, wait for the time when other APIs are completed*/
    CORE_HTTPOPT_HEADER_LINE_MAX_LEN,   /*Data type: (uint32_t), the maximum length of a single line header in the http protocol*/
    CORE_HTTPOPT_BODY_BUFFER_MAX_LEN,   /*Data type: (uint32_t), maximum length of body read each time*/
    CORE_HTTPOPT_EVENT_HANDLER,         /*Data type: (aiot_http_event_handler_t), user event callback function, default value: NULL*/
    /*The data configured with the above options is shared with AIOT_HTTPOPT_XXX*/
    CORE_HTTPOPT_USERDATA,              /*Data type: (void *), user context data pointer, default value: NULL*/
    CORE_HTTPOPT_RECV_HANDLER,          /*Data type: (aiot_http_event_handler_t), user data accepts callback function, default value: NULL*/
    CORE_HTTPOPT_MAX
} core_http_option_t;

typedef struct {
    char       *method;             /*HTTP request method, which can be "POST", "GET", etc.*/
    char       *path;               /*HTTP request path*/
    char       *header;             /*The header of the HTTP request must end with a single \r\n and does not need to contain Content-Length.*/
    uint8_t    *content;            /*Pointer to the content to be sent by the user*/
    uint32_t    content_len;        /*The length of the content to be sent by the user*/
} core_http_request_t;

/**
 * @brief Initialize an HTTP instance and return the instance handle
 *
 * @return void*
 * @retval NotNull core HTTP handle
 * @retval Null failed to initialize HTTP instance*/
void *core_http_init(void);

/**
 * @brief Set HTTP instance options
 *
 * @param[in] handle HTTP handle
 * @param option configuration option, you can view the enumeration type @ref core_http_option_t
 * @param[in] data configuration data, the data type corresponding to each option can be viewed @ref core_http_option_t
 * @return int32_t
 * @retval STATE_SUCCESS, successful
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE, user input parameters are invalid
 * @retval STATE_SYS_DEPEND_MALLOC_FAILED, memory allocation failed
 **/
int32_t core_http_setopt(void *handle, core_http_option_t option, void *data);

/**
 * @brief Establish network connection
 *
 * @param handle HTTP handle
 * @return int32_t
 * @retval STATE_SUCCESS The network connection is successfully established
 * @retval <STATE_SUCCESS Network connection establishment failed*/
int32_t core_http_connect(void *handle);

/**
 * @brief Send HTTP request
 *
 * @param[in] handle HTTP handle
 * @param request request structure, see @ref core_http_request_t
 * @return int32_t
 * @retval > 0, sent data length
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_NULL_POINTER, the user input parameter is NULL
 * @retval STATE_USER_INPUT_MISSING_HOST, the user has not configured the Host
 * @retval STATE_SYS_DEPEND_MALLOC_FAILED, memory allocation failed
 * @retval STATE_SYS_DEPEND_NWK_EST_FAILED, failed to establish network connection
 * @retval STATE_SYS_DEPEND_NWK_CLOSED, the network connection has been closed
 * @retval STATE_SYS_DEPEND_NWK_WRITE_LESSDATA, network send timeout*/
int32_t core_http_send(void *handle, const core_http_request_t *request);

/**
 * @brief accepts HTTP response data. It will parse the status code and header internally and notify the user through the callback function. If there is a body in the response, it will be saved in the user buffer.
 *
 * @param[in] handle HTTP handle
 * @param buffer points to the storage accept
 * @param buffer_len
 * @return int32_t
 * @retval >= 0, received HTTP body data length
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_NULL_POINTER, the user input parameter is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE, buffer_len is 0
 * @retval STATE_SYS_DEPEND_NWK_CLOSED, the network connection has been closed
 * @retval STATE_SYS_DEPEND_NWK_READ_OVERTIME, network reception timeout
 * @retval STATE_HTTP_RECV_LINE_TOO_LONG, HTTP single line data is too long and cannot be parsed internally
 * @retval STATE_HTTP_PARSE_STATUS_LINE_FAILED, status code cannot be parsed
 * @retval STATE_HTTP_GET_CONTENT_LEN_FAILED, failed to get Content-Length
 **/
int32_t core_http_recv(void *handle);

/**
 * @brief Destroy the HTTP instance specified by parameter p_handle
 *
 * @param[in] p_handle pointer to HTTP handle
 * @return int32_t
 * @retval STATE_SUCCESS successful
 * @retval STATE_USER_INPUT_NULL_POINTER The parameter p_handle is NULL or the handle pointed to by p_handle is NULL*/
int32_t core_http_deinit(void **p_handle);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _CORE_HTTP_H_ */

