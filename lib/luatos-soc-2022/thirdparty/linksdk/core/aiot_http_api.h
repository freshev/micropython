/**
 * @file aiot_http_api.h
 * @brief HTTP Modules header file, providing the ability to report data to the Alibaba Cloud IoT platform using the HTTP protocol
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 **/

#ifndef _AIOT_HTTP_API_H_
#define _AIOT_HTTP_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"

/**
 * @brief Business error code returned by the server
 *
 * @details
 *
 * Parse from the JSON message of the cloud platform's response to the reported message*/
typedef enum {
    /**
     * @brief 0, the server successfully received the reported message*/
    AIOT_HTTP_RSPCODE_SUCCESS                   = 0,
    /**
     * @brief 10000, the server returned an unknown error*/
    AIOT_HTTP_RSPCODE_COMMON_ERROR              = 10000,
    /**
     * @brief 10001, request parameter error*/
    AIOT_HTTP_RSPCODE_PARAM_ERROR               = 10001,
    /**
     * @brief 20001, the token has expired, please call @ref aiot_http_auth for authentication and obtain a new token*/
    AIOT_HTTP_RSPCODE_TOKEN_EXPIRED             = 20001,
    /**
     * @brief 20002, there is no token in the requested header to indicate that the device is legitimate, please call @ref aiot_http_auth for authentication and obtain a new token*/
    AIOT_HTTP_RSPCODE_TOKEN_NULL                = 20002,
    /**
     * @brief 20003, token error, please call @ref aiot_http_auth for authentication and obtain a new token*/
    AIOT_HTTP_RSPCODE_TOKEN_CHECK_ERROR         = 20003,
    /**
     * @brief 30001, message reporting failed*/
    AIOT_HTTP_RSPCODE_PUBLISH_MESSAGE_ERROR     = 30001,
    /**
     * @brief 40000, the device reports too frequently, triggering the server current limit*/
    AIOT_HTTP_RSPCODE_REQUEST_TOO_MANY          = 40000,
} aiot_http_response_code_t;

/**
 * @brief @ref aiot_http_setopt The option parameter of the function, for the data type in each option below, refers to the data type of the data parameter in @ref aiot_mqtt_setopt
 **/
typedef enum {
    /**
     * @brief HTTP server domain name address or IP address
     *
     * @details
     *
     * Alibaba Cloud IoT platform domain name address list: (tcp uses port 80, tls uses port 443)
     *
     * | Domain name address | Region | Port number |
     * |---------------------------------------|-------- -|--------|
     * | iot-as-http.cn-shanghai.aliyuncs.com | Shanghai | 443 |
     *
     * Data type: (char *)*/
    AIOT_HTTPOPT_HOST,
    /**
     * @brief HTTP server port number
     *
     * @details
     *
     * When connecting to Alibaba Cloud IoT platform:
     *
     * 1. If you are using tcp, set the port number to 80
     *
     * 2. If tls is used, the port number is set to 443
     *
     * Data type: (uint16_t *)*/
    AIOT_HTTPOPT_PORT,
    /**
     * @brief The security credentials used by the network when establishing HTTP connections
     *
     * @details
     *
     * This configuration item is used to configure @ref aiot_sysdep_network_cred_t security credential data for the underlying network
     *
     * 1. If this option is not configured, MQTT will establish the connection directly through tcp.
     *
     * 2. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_NONE, HTTP will establish the connection directly in tcp mode
     *
     * 3. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA, HTTP will establish the connection in tls mode
     *
     * Data type: (aiot_sysdep_network_cred_t *)*/
    AIOT_HTTPOPT_NETWORK_CRED,
    /**
     * @brief When establishing a HTTP connection, the timeout period for establishing a network connection
     *
     * @details
     *
     * refers to the timeout period for establishing a socket connection
     *
     * Data type: (uint32_t *) Default value: (5 *1000) ms
     **/
    AIOT_HTTPOPT_CONNECT_TIMEOUT_MS,
    /**
     * @brief When HTTP sends data, the longest time it takes in the protocol stack
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (5 * 1000) ms*/
    AIOT_HTTPOPT_SEND_TIMEOUT_MS,
    /**
     * @brief When HTTP receives data, the longest time it takes in the protocol stack
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (5 * 1000) ms*/
    AIOT_HTTPOPT_RECV_TIMEOUT_MS,
    /**
     * @brief When destroying the HTTP instance, the time to wait for other APIs to complete execution
     *
     * @details
     *
     * When calling @ref aiot_http_deinit to destroy the HTTP instance, if you continue to call other aiot_http_xxx API, the API will return STATE_USER_INPUT_EXEC_DISABLED error
     *
     * At this point, users should stop calling other aiot_http_xxx APIs
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_HTTPOPT_DEINIT_TIMEOUT_MS,
    /**
     * @brief When receiving the http message returned by the server, the maximum length of a single line http header
     *
     * @details
     *
     * When the single-line http header is set too short, @ref aiot_http_recv will return @ref STATE_HTTP_HEADER_BUFFER_TOO_SHORT status code
     *
     * Data type: (uint32_t *) Default value: 128*/
    AIOT_HTTPOPT_HEADER_BUFFER_LEN,
    /**
     * @brief When receiving the http message returned by the server, the maximum length of the body given in the @ref aiot_http_recv_handler_t callback function each time
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: 128*/
    AIOT_HTTPOPT_BODY_BUFFER_LEN,
    /**
     * @brief HTTP internal event callback function
     *
     * @details
     *
     * Data type: (aiot_http_event_handler_t)*/
    AIOT_HTTPOPT_EVENT_HANDLER,

    /*The data configured with the above options is shared with CORE_HTTPOPT_XXX*/

    /**
     * @brief User needs SDK temporary context
     *
     * @details
     *
     * 1. When HTTP data is received, the context will be given from the userdata parameter of @ref aiot_http_recv_handler_t
     *
     * 2. When an event occurs inside HTTP, the context will be given from the userdata parameter of @ref aiot_http_event_handler_t
     *
     * Data type: (void *)*/
    AIOT_HTTPOPT_USERDATA,
    /**
     * @brief HTTP data receiving callback function
     *
     * @details
     *
     * Data type: (aiot_http_recv_handler_t)*/
    AIOT_HTTPOPT_RECV_HANDLER,
    /**
     * @brief The product key of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_HTTPOPT_PRODUCT_KEY,
    /**
     * @brief The device name of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_HTTPOPT_DEVICE_NAME,
    /**
     * @brief The device secret of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_HTTPOPT_DEVICE_SECRET,
    /**
     * @brief reports extended information of the device, such as Modules vendor ID and Modules ID
     *
     * @details
     *
     * 1. Module vendor ID: The format is pid=xxx
     *
     * 2. Module ID: The format is mid=xxx
     *
     * If multiple information needs to be reported at the same time, use & to connect them, for example: pid=xxx&mid=xxx
     *
     * Data type: (char *)*/
    AIOT_HTTPOPT_EXTEND_DEVINFO,
    /**
     * @brief Use @ref aiot_http_auth to authenticate and obtain the token timeout
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (5 * 1000) ms*/
    AIOT_HTTPOPT_AUTH_TIMEOUT_MS,
    /**
     * @brief Whether to use http long connection
     *
     * @details
     *
     * If the value of this configuration is 0, each time @ref aiot_http_auth and @ref aiot_http_send are used, the SDK will re-establish an introduction with the HTTP server
     *
     * Data type: (uint8_t *) Default value: (5 * 1000) ms*/
    AIOT_HTTPOPT_LONG_CONNECTION,

    AIOT_HTTPOPT_MAX
} aiot_http_option_t;

/**
 * @brief SDK receives the HTTP message and describes the message type when passing it to the user data callback function.*/
typedef enum {
    /**
     * @brief Get HTTP Status Code*/
    AIOT_HTTPRECV_STATUS_CODE,
    /**
     * @brief Get the HTTP Header and return a set of key-value pairs in the Header each time*/
    AIOT_HTTPRECV_HEADER,
    /**
     * @brief Get the HTTP Body and return the complete Body content*/
    AIOT_HTTPRECV_BODY
} aiot_http_recv_type_t;

/**
 * @brief SDK receives the HTTP message and describes the content of the message when it is passed to the user data callback function.*/
typedef struct {
    /**
     * @brief HTTP message type, for more information please refer to @ref aiot_http_recv_type_t*/
    aiot_http_recv_type_t type;
    union {
        /**
         * @brief Data when the HTTP message type is @ref AIOT_HTTPRECV_STATUS_CODE*/
        struct {
            /**
             * @brief HTTP Status Code
             */
            uint32_t code;
        } status_code;
        /**
         * @brief Data when the HTTP message type is @ref AIOT_HTTPRECV_HEADER*/
        struct {
            /**
             * @brief single-line HTTP Header key*/
            char *key;
            /**
             * @brief Single-line HTTP Header value*/
            char *value;
        } header;
        /**
         * @brief Data when the HTTP message type is @ref AIOT_HTTPRECV_BODY*/
        struct {
            /**
             * @brief HTTP Body content*/
            uint8_t *buffer;
            /**
             * @brief HTTP Body length*/
            uint32_t len;
        } body;
    } data;
} aiot_http_recv_t;

/**
 * @brief HTTP message receiving callback function prototype, which can be specified through the @ref AIOT_HTTPOPT_RECV_HANDLER parameter of the @ref aiot_http_setopt interface
 *
 * @details
 *
 * When the SDK receives the response data from the HTTP server, it is output through this callback function
 *
 * @param[out] handle HTTP handle
 * @param[out] packet data received from HTTP server
 * @param[out] userdata context that the user passes to the SDK for temporary storage via @ref AIOT_HTTPOPT_USERDATA
 *
 * @return void*/
typedef void (*aiot_http_recv_handler_t)(void *handle, const aiot_http_recv_t *packet, void *userdata);

/**
 * @brief When a state change occurs within the SDK and the user is notified through the user event callback function, a description of the event type*/
typedef enum {
    /**
     * @brief token invalid event, at this time the user should call @ref aiot_http_auth to obtain a new token*/
    AIOT_HTTPEVT_TOKEN_INVALID
} aiot_http_event_type_t;

/**
 * @brief When a state change occurs within the SDK and the user is notified through the user event callback function, a description of the event content*/
typedef struct {
    aiot_http_event_type_t type;
} aiot_http_event_t;

/**
 * @brief HTTP event callback function prototype, which can be specified through the @ref AIOT_HTTPOPT_EVENT_HANDLER parameter of the @ref aiot_http_setopt interface
 *
 * @param[out] handle HTTP handle
 * @param[out] event event structure
 * @param[out] user_data pointer to user context data, set by the @ref AIOT_HTTPOPT_USERDATA option of @ref aiot_http_setopt*/
typedef void (* aiot_http_event_handler_t)(void *handle, const aiot_http_event_t *event, void *userdata);

/**
 * @brief Create an HTTP cloud instance
 *
 * @return void*
 *
 * @retval non-NULL, HTTP instance handle
 * @retval NULL, failed to initialize HTTP instance*/
void *aiot_http_init(void);

/**
 * @brief Set HTTP instance parameters
 *
 * @param[in] handle HTTP handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_http_option_t
 * @param[in] data configuration data, for more information please refer to @ref aiot_http_option_t
 *
 * @return int32_t
 *
 * @retval STATE_SUCCESS, successful
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE, user input parameters are invalid
 * @retval STATE_SYS_DEPEND_MALLOC_FAILED, memory allocation failed*/
int32_t aiot_http_setopt(void *handle, aiot_http_option_t option, void *data);

/**
 * @brief Send an authentication request to the server and obtain the token
 *
 * @param[in] handle HTTP handle
 *
 * @return int32_t
 *
 * @retval STATE_SUCCESS, authentication successful
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_MISSING_PRODUCT_KEY, the necessary option ProductKey is not set
 * @retval STATE_USER_INPUT_MISSING_DEVICE_NAME, necessary option DeviceName is not set
 * @retval STATE_USER_INPUT_MISSING_DEVICE_SECRET, the necessary option DeviceSecret is not set
 * @retval STATE_HTTP_TOKEN_LEN_ERROR, token length error
 * @retval STATE_HTTP_GET_TOKEN_FAILED, failed to obtain token*/
int32_t aiot_http_auth(void *handle);

/**
 * @brief Report data to the IoT platform
 *
 * @param[in] handle HTTP handle
 * @param[in] topic The target topic reported, the product details page controlled by the IoT platform has a complete topic list of the device
 * @param[in] payload pointer to the reported data
 * @param[in] payload_len The length of the reported data
 *
 * @return int32_t
 *
 * @retval STATE_SUCCESS, reported successfully
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE, user input parameters are invalid
 * @retval STATE_HTTP_NOT_AUTH, the device is not authenticated*/
int32_t aiot_http_send(void *handle, char *topic, uint8_t *payload, uint32_t payload_len);

/**
 * The server response data format is
 * {
 * "code": 0, // Business status code
 * "message": "success", // business information
 * "info": {
 * "messageId": 892687627916247040,
 * }
 * }*/

/**
 * @brief accepts HTTP response data, and the data will be output from the @ref aiot_http_event_handler_t callback function set by the user
 *
 * @param[in] handle HTTP handle
 *
 * @return int32_t
 *
 * @retval >= 0, received HTTP body data length
 * @retval STATE_HTTP_HANDLE_IS_NULL, HTTP handle is NULL
 * @retval STATE_USER_INPUT_NULL_POINTER, the user input parameter is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE, buffer_len is 0
 * @retval STATE_HTTP_RSP_MSG_ERROR, server response message error
 * @retval STATE_SYS_DEPEND_NWK_CLOSED, the network connection has been closed
 * @retval STATE_SYS_DEPEND_NWK_READ_OVERTIME, network reception timeout
 * @retval STATE_HTTP_RECV_LINE_TOO_LONG, HTTP single line data is too long and cannot be parsed internally
 * @retval STATE_HTTP_PARSE_STATUS_LINE_FAILED, status code cannot be parsed
 * @retval STATE_HTTP_GET_CONTENT_LEN_FAILED, failed to get Content-Length*/
int32_t aiot_http_recv(void *handle);

/**
 * @brief Destroy the HTTP instance specified by parameter p_handle
 *
 * @param[in] p_handle pointer to HTTP handle
 *
 * @return int32_t
 *
 * @retval STATE_SUCCESS successful
 * @retval STATE_USER_INPUT_NULL_POINTER The parameter p_handle is NULL or the handle pointed to by p_handle is NULL*/
int32_t aiot_http_deinit(void **p_handle);


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _AIOT_HTTP_API_H_ */

