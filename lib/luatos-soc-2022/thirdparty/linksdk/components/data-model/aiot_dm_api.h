/**
 * @file aiot_dm_api.h
 * @brief The header file of the data model Modules provides the ability to upload the object model data format to the cloud, including attributes, events, services and data upstream and downstream capabilities in the binary format of the object model.
 * @date 2020-01-20
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 * Please follow the following process to use the API
 *
 * 1. Before using the object model Modules, users should first create an MQTT instance
 *
 * 2. Call `aiot_dm_init` to create an object model instance and save the instance handle
 *
 * 3. Call `aiot_dm_setopt` to configure the `AIOT_DMOPT_MQTT_HANDLE` option to set the MQTT handle. This option is a mandatory configuration option
 *
 * 4. Call `aiot_dm_setopt` to configure the `AIOT_DMOPT_RECV_HANDLER` and `AIOT_DMOPT_USERDATA` options to register the data acceptance callback function and user context data pointer
 *
 * 5. Before using `aiot_dm_send` to send messages, the connection establishment of the MQTT instance should be completed first.
 *
 * 6. Mobilize `aiot_dm_send` to send different types of messages to the cloud platform, and process various types of cloud platform downlink messages in the registered callback function
 **/

#ifndef __AIOT_DM_API_H__
#define __AIOT_DM_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x0A00~-0x0AFF expresses the status code of the SDK in the data-model Modules*/
#define STATE_DM_BASE                                           (-0x0A00)

/**
 * @brief When the user sends the @ref AIOT_DMMSG_EVENT_POST message, the event_id in the message data is NULL*/
#define STATE_DM_EVENT_ID_IS_NULL                               (-0x0A01)

/**
 * @brief When a user sends a @ref AIOT_DMMSG_ASYNC_SERVICE_REPLY or @ref AIOT_DMMSG_SYNC_SERVICE_REPLY message, the event_id in the message data is NULL*/
#define STATE_DM_SERVICE_ID_IS_NULL                             (-0x0A02)

/**
 * @brief When the user sends the @ref AIOT_DMMSG_SYNC_SERVICE_REPLY message, the rrpc_id in the message data is NULL*/
#define STATE_DM_RRPC_ID_IS_NULL                                (-0x0A03)

/**
 * @brief When the user sends a request message, the param in the message data is NULL*/
#define STATE_DM_MSG_PARAMS_IS_NULL                             (-0X0A04)

/**
 * @brief When the user sends a response message, the data in the message data is NULL*/
#define STATE_DM_MSG_DATA_IS_NULL                               (-0X0A05)

/**
 * @brief An error occurred while parsing the topic corresponding to the downstream data.*/
#define STATE_DM_INTERNAL_TOPIC_ERROR                           (-0x0A06)

/**
 * @brief The user did not call @ref aiot_dm_setopt to configure the MQTT handle*/
#define STATE_DM_MQTT_HANDLE_IS_NULL                            (-0x0A07)

/**
 * @brief Log status code when receiving server downlink message*/
#define STATE_DM_LOG_RECV                                       (-0x0A08)

/**
 * @brief Log status code when parsing server downlink messages fails*/
#define SATAE_DM_LOG_PARSE_RECV_MSG_FAILED                      (-0x0A09)


/**
 * @brief data-model Modules configuration option enumeration type definition. @ref aiot_dm_setopt The data type of the function input data varies according to different options
 **/
typedef enum {
    /**
     * @brief The MQTT handle that the Modules depends on
     *
     * @details
     *
     * The data-model Modules depends on the underlying MQTT Modules. Users must configure the correct MQTT handle, otherwise it will not work properly.
     *
     * Data type: (void *)*/
    AIOT_DMOPT_MQTT_HANDLE,

    /**
     * @brief Data receiving callback function, data-model calls this callback function after receiving the downlink message from the IoT platform
     *
     * @details
     *
     * Data type: (aiot_dm_recv_handler_t), see @ref aiot_dm_recv_handler_t callback function prototype for details*/
    AIOT_DMOPT_RECV_HANDLER,

    /**
     * @brief pointer to user context data
     *
     * @details
     *
     * Return this pointer to the user through the userdata parameter in the @ref aiot_dm_recv_handler_t data receiving callback function registered by the user.
     *
     * Data type: (void *)*/
    AIOT_DMOPT_USERDATA,

    /**
     * @brief Whether the user wants to receive a reply after the post message
     *
     * @details
     *
     * Whether to receive reply messages from the cloud. 1 means to receive it, 0 means not to.
     *
     * Data type: (uint8_t) *)*/
    AIOT_DMOPT_POST_REPLY,

    /**
     * @brief Maximum number of configuration options, cannot be used as configuration parameters*/
    AIOT_DMOPT_MAX,
} aiot_dm_option_t;

/**
 * @brief data-model Modules sends message type
 *
 * @details
 *
 * This enumeration type includes all data types that the dm Modules supports sending. Different message types will have different message structures.
 * Users can view the web document <a href="https://help.aliyun.com/document_detail/89301.html">Device Properties/Events/Services</a> to learn more about various data types
 **/
typedef enum {
    /**
     * @brief attribute reporting, message structure reference @ref aiot_dm_msg_property_post_t \n
     * After successfully sending this message, a response message of type @ref AIOT_DMRECV_GENERIC_REPLY will be received*/
    AIOT_DMMSG_PROPERTY_POST,

    /**
     * @brief event reporting, message structure reference @ref aiot_dm_msg_event_post_t \n
     * After successfully sending this message, a response message of type @ref AIOT_DMRECV_GENERIC_REPLY will be received*/
    AIOT_DMMSG_EVENT_POST,

    /**
     * @brief property setting response, message structure reference @ref aiot_dm_msg_property_set_reply_t*/
    AIOT_DMMSG_PROPERTY_SET_REPLY,

    /**
     * @brief asynchronous service response, message structure reference @ref aiot_dm_msg_async_service_reply_t*/
    AIOT_DMMSG_ASYNC_SERVICE_REPLY,

    /**
     * @brief Synchronization service response, message structure reference @ref aiot_dm_msg_sync_service_reply_t*/
    AIOT_DMMSG_SYNC_SERVICE_REPLY,

    /**
     * @brief object model upstream data in binary format, message structure reference @ref aiot_dm_msg_raw_data_t*/
    AIOT_DMMSG_RAW_DATA,

    /**
     * @brief Synchronization service response in binary format, message structure reference @ref aiot_dm_msg_raw_service_reply_t*/
    AIOT_DMMSG_RAW_SERVICE_REPLY,

    /**
     * @brief Get the desired attribute value. For the message structure, please refer to @ref aiot_dm_msg_get_desired_t, \n
     * After successfully sending this message, a response message of type @ref AIOT_DMRECV_GENERIC_REPLY will be received*/
    AIOT_DMMSG_GET_DESIRED,

    /**
     * @brief Clear the specified desired value, please refer to @ref aiot_dm_msg_delete_desired_t for the message structure \n
     * After successfully sending this message, a response message of type @ref AIOT_DMRECV_GENERIC_REPLY will be received*/
    AIOT_DMMSG_DELETE_DESIRED,

    /**
     * @brief Clear the specified desired value, please refer to @ref aiot_dm_msg_delete_desired_t for the message structure \n
     * After successfully sending this message, a response message of type @ref AIOT_DMRECV_GENERIC_REPLY will be received*/
    AIOT_DMMSG_PROPERTY_BATCH_POST,

    /**
     * @brief Maximum number of messages, cannot be used as message type*/
    AIOT_DMMSG_MAX,
} aiot_dm_msg_type_t;

/**
 * @brief <b>Object model attribute reporting</b> message structure*/
typedef struct {
    /**
     * @brief JSON structure in string form, <b>must end with the terminator '\0'</b>. Contains attribute data to be reported by the user, such as <i>"{\"LightSwitch\":0} "</i>*/
    char *params;
} aiot_dm_msg_property_post_t;

/**
 * @brief <b>Object model event reporting</b> message structure*/
typedef struct {
    /**
     * @brief event identifier, <b>must be a string ending with the terminator '\0'</b>*/
    char *event_id;
    /**
     * @brief JSON structure in string form, <b>must end with the terminator '\0'</b>. Contains event data to be reported by the user, such as <i>"{\"ErrorNum\":0} "</i>*/
    char *params;
} aiot_dm_msg_event_post_t;

/**
 * @brief <b>Property setting response</b> message structure, the user can send this message to reply after receiving the @ref AIOT_DMRECV_PROPERTY_SET type of property setting.*/
typedef struct {
    /**
     * @brief message identifier, an integer of uint64_t type, <b>must be consistent with the message identifier set by the property</b>*/
    uint64_t msg_id;
    /**
     * @brief Device-side status code, 200-Request successful, for more status codes, see <a href="https://help.aliyun.com/document_detail/89309.html">Device-side general code</a>*/
    uint32_t code;
    /**
     * @brief Device-side response data, a JSON structure in the form of a string, <b>must end with the terminator '\0'</b>, such as <i>"{}"</i> indicating that the response data is empty*/
    char    *data;
} aiot_dm_msg_property_set_reply_t;

/**
 * @brief <b>Asynchronous service response</b> message structure. After receiving an asynchronous service call message of @ref AIOT_DMRECV_ASYNC_SERVICE_INVOKE type, the user should send this message to respond.*/
typedef struct {
    /**
     * @brief message identifier, an integer of uint64_t type, <b>must be consistent with the message identifier of the asynchronous service call</b>*/
    uint64_t msg_id;
    /**
     * @brief service identifier, identifying the service to be responded to*/
    char    *service_id;
    /**
     * @brief Device-side status code, 200-Request successful, for more status codes, see <a href="https://help.aliyun.com/document_detail/89309.html">Device-side general code</a>*/
    uint32_t code;
    /**
     * @brief Device-side response data, a JSON structure in the form of a string, <b>must end with the terminator '\0'</b>, such as <i>"{}"</i> indicating that the response data is empty*/
    char    *data;
} aiot_dm_msg_async_service_reply_t;

/**
 * @brief <b>Synchronization service response</b> message structure. After receiving the @ref AIOT_DMRECV_SYNC_SERVICE_INVOKE type of synchronization service call message, the user should respond within the timeout period (default 7s)*/
typedef struct {
    /**
     * @brief message identifier, an integer of uint64_t type, <b>must be consistent with the message identifier of the synchronization service call</b>*/
    uint64_t msg_id;
    /**
     * @brief RRPC identifier, a string used to uniquely identify each synchronization service, <b>must be consistent with the RRPC identifier of the synchronization service call message</b>*/
    char    *rrpc_id;
    /**
     * @brief service identifier, identifying the service to be responded to*/
    char    *service_id;
    /**
     * @brief Device-side status code, 200-Request successful, for more status codes, see <a href="https://help.aliyun.com/document_detail/89309.html">Device-side general code</a>*/
    uint32_t code;
    /**
     * @brief Device-side response data, a JSON structure in the form of a string, <b>must end with the terminator '\0'</b>, such as <i>"{}"</i> indicating that the response data is empty*/
    char    *data;
} aiot_dm_msg_sync_service_reply_t;

/**
 * @brief <b>Object model binary data</b> message structure. The sent binary data will be converted into JSON format data through the JavaScript script of the Internet of Things platform. Users should ensure that the cloud parsing script has been correctly enabled before sending this message.*/
typedef struct {
    /**
     * @brief pointer to the binary data to be sent*/
    uint8_t *data;
    /**
     * @brief The length of the data to be sent*/
    uint32_t data_len;
} aiot_dm_msg_raw_data_t;

/**
 * @brief <b>Synchronization service response in binary format</b> message structure. After receiving the @ref AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE type message, the user should respond within the timeout period (default 7s)\n
 * Users should ensure that the cloud parsing script is enabled and the script is working properly before using this message*/
typedef struct {
    /**
     * @brief RRPC identifier, special string, <b>must be consistent with the RRPC identifier of the synchronization service call message</b>*/
    char    *rrpc_id;
    /**
     * @brief pointer to the binary data to be sent*/
    uint8_t *data;
    /**
     * @brief The length of the data to be sent*/
    uint32_t data_len;
} aiot_dm_msg_raw_service_reply_t;

/**
 * @brief <b>Get the expected attribute value</b> message structure, send*/
typedef struct {
    /**
     * @brief JSON<b>array</b> in string form, <b>must end with the terminator '\0'</b>. It should contain the ID of the desired attribute that the user wants to obtain, such as <i>" [\"LightSwitch\"]"</i>*/
    char *params;
} aiot_dm_msg_get_desired_t;

/**
 * @brief <b>Delete the specified expected value</b> message structure*/
typedef struct {
    /**
     * @brief JSON structure in the form of a string, <b>must end with the terminator '\0'</b>. It should contain the ID and expected value version number of the desired attribute that the user wants to delete, such as <i>"{\ "LightSwitch\":{\"version\":1},\"Color\":{}}"</i>*/
    char *params;
} aiot_dm_msg_delete_desired_t;


/**
 * @brief <b>Object model attribute reporting</b> message structure*/
typedef struct {
    /**
     * @brief JSON structure in string form, <b>must end with the terminator '\0'</b>. Contains properties and event data that users want to report in batches, such as {"properties":{"Power": [ { "value": "on", "time": 1524448722000 },
     * { "value": "off", "time": 1524448722001 } ], "WF": [ { "value": 3, "time": 1524448722000 }]}, "events": {"alarmEvent": [{ "value": { "Power": "on", "WF": "2"},
     * "time": 1524448722000}]}}*/
    char *params;
} aiot_dm_msg_property_batch_post_t;

/**
 * The message structure of the message sent by the @brief data-model Modules*/
typedef struct {
    /**
     * @brief The product_key of the device to which the message belongs. If it is NULL, the product_key configured through aiot_dm_setopt is used\n
     * In the scenario of gateway sub-device, the message of the sub-device can be sent to the cloud by specifying the product_key of the sub-device.*/
    char *product_key;
    /**
     * @brief The device_name of the device to which the message belongs. If it is NULL, use the device_name configured through aiot_dm_setopt\n
     * In the scenario of gateway sub-device, the message of the sub-device can be sent to the cloud by specifying the product_key of the sub-device.*/
    char *device_name;
    /**
     * @brief message type, please refer to @ref aiot_dm_msg_type_t*/
    aiot_dm_msg_type_t type;
    /**
     * @brief message data union, different message types will use different message structures*/
    union {
        aiot_dm_msg_property_post_t         property_post;
        aiot_dm_msg_event_post_t            event_post;
        aiot_dm_msg_property_set_reply_t    property_set_reply;
        aiot_dm_msg_sync_service_reply_t    sync_service_reply;
        aiot_dm_msg_async_service_reply_t   async_service_reply;
        aiot_dm_msg_raw_data_t              raw_data;
        aiot_dm_msg_raw_service_reply_t     raw_service_reply;
        aiot_dm_msg_get_desired_t           get_desired;
        aiot_dm_msg_delete_desired_t        delete_desired;
    } data;
} aiot_dm_msg_t;


/**
 * @brief data-model Modules accepts message type enumeration
 *
 * @details
 *
 * This enumeration type includes all data types that the dm Modules supports receiving. Different message types will have different message structures.
 * Users can view the web document <a href="https://help.aliyun.com/document_detail/89301.html">Device Properties/Events/Services</a> to learn more about various data types
 **/
typedef enum {
    /**
     * @brief Report attribute/response message returned by the server after practice, message data structure reference @ref aiot_dm_recv_generic_reply_t*/
    AIOT_DMRECV_GENERIC_REPLY,

    /**
     * @brief The property setting message sent by the server, the message data structure refers to @ref aiot_dm_recv_property_set_t*/
    AIOT_DMRECV_PROPERTY_SET,

    /**
     * @brief Asynchronous service invocation message sent by the server, message data structure reference @ref aiot_dm_recv_async_service_invoke_t*/
    AIOT_DMRECV_ASYNC_SERVICE_INVOKE,

    /**
     * @brief Synchronization service invocation message issued by the server, message data structure reference @ref aiot_dm_recv_sync_service_invoke_t*/
    AIOT_DMRECV_SYNC_SERVICE_INVOKE,

    /**
     * @brief The binary data response reported by the server to the device, message data structure reference @ref aiot_dm_recv_raw_data_t*/
    AIOT_DMRECV_RAW_DATA_REPLY,

    /**
     * @brief object model binary data issued by the server, message data structure reference @ref aiot_dm_recv_raw_data_t*/
    AIOT_DMRECV_RAW_DATA,

    /**
     * @brief Synchronous service invocation message in binary format issued by the server, message data structure reference @ref aiot_dm_recv_raw_service_invoke_t*/
    AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE,

    /**
     * @brief Maximum number of messages, cannot be used as message type*/
    AIOT_DMRECV_MAX,
} aiot_dm_recv_type_t;

/**
 * @brief <b>Cloud Universal Response</b> message structure. After the device reports @ref AIOT_DMMSG_PROPERTY_POST, @ref AIOT_DMMSG_EVENT_POST or @ref AIOT_DMMSG_GET_DESIRED, the server will respond to this message.*/
typedef struct {
    /**
     * @brief message identifier, an integer of uint64_t type, consistent with the message identifier reported by attributes or events*/
    uint32_t msg_id;
    /**
     * @brief Device-side error code, 200-Request successful, for more error codes, see <a href="https://help.aliyun.com/document_detail/120329.html">Device-side error code</a>*/
    uint32_t code;
    /**
     * @brief pointer to the cloud response data*/
    char *data;
    /**
     * @brief The length of the cloud response data*/
    uint32_t data_len;
    /**
     * @brief Pointer to the status message string. When the device reports that the request is successful, the corresponding response message is "success". If the request fails, the response message contains error information.*/
    char *message;
    /**
     * @brief The length of the message string*/
    uint32_t message_len;
} aiot_dm_recv_generic_reply_t;

/**
 * @brief <b>Property settings</b> message structure*/
typedef struct {
    /**
     * @brief message identifier, integer of type uint64_t*/
    uint64_t    msg_id;
    /**
     * @brief The attribute data sent by the server is a JSON structure in the form of a string. This string <b>does</b> end with the terminator '\0', such as <i>"{\"LightSwitch\" :0}"</i>*/
    char       *params;
    /**
     * @brief The string length of the attribute data*/
    uint32_t    params_len;
} aiot_dm_recv_property_set_t;

/**
 * @brief <b>Synchronization service call</b> message structure. After receiving the synchronization service, the user must respond within the timeout period (default 7s)*/
typedef struct {
    /**
     * @brief message identifier, integer of type uint64_t*/
    uint64_t    msg_id;
    /**
     * @brief RRPC identifier, a special string used to uniquely identify each synchronization service*/
    char       *rrpc_id;
    /**
     * @brief service identifier, the content of the string is determined by the user-defined object model*/
    char       *service_id;
    /**
     * @brief The input parameter data of the service call is a JSON structure in the form of a string. This string <b>does</b> end with the terminator '\0', such as <i>"{\"LightSwitch\" :0}"</i>*/
    char       *params;
    /**
     * @brief The string length of the input parameter*/
    uint32_t    params_len;
} aiot_dm_recv_sync_service_invoke_t;

/**
 * @brief <b>Synchronous service call</b> message structure*/
typedef struct {
    /**
     * @brief message identifier, integer of type uint64_t*/
    uint64_t    msg_id;
    /**
     * @brief service identifier, the content of the string is determined by the user-defined object model*/
    char       *service_id;
    /**
     * @brief The input parameter data of the service call is a JSON structure in the form of a string. This string <b>does</b> end with the terminator '\0', such as <i>"{\"LightSwitch\" :0}"</i>*/
    char       *params;
    /**
     * @brief The string length of the input parameter*/
    uint32_t    params_len;
} aiot_dm_recv_async_service_invoke_t;

/**
 * @brief <b>Object model binary data</b> message structure. The JSON format object model data of the server will be converted into binary data through the JavaScript script of the IoT platform. Users should ensure that the cloud has been correctly enabled before receiving this message. parse script*/
typedef struct {
    /**
     * @brief Pointer to the receiving data buffer*/
    uint8_t    *data;
    /**
     * @brief The length of binary data*/
    uint32_t    data_len;
} aiot_dm_recv_raw_data_t;

/**
 * @brief <b>Synchronization service call of binary data</b> message structure. The JSON format object model data of the server will be converted into binary data through the JavaScript script of the IoT platform. The user should ensure that it is correct before receiving this message. Enable cloud parsing script*/
typedef struct {
    /**
     * @brief RRPC identifier, a special string used to uniquely identify each synchronization service*/
    char       *rrpc_id;
    /**
     * @brief Pointer to the receiving data buffer*/
    uint8_t    *data;
    /**
     * @brief The length of binary data*/
    uint32_t    data_len;
} aiot_dm_recv_raw_service_invoke_t;

/**
 * @brief data-model Modules receives the message structure*/
typedef struct {
    /**
     * @brief The product_key of the device to which the message belongs. If not configured, the product_key configured by the MQTT Modules will be used by default.*/
    char *product_key;
    /**
     * @brief The device_name of the device to which the message belongs. If not configured, the device_name configured by the MQTT Modules will be used by default.*/
    char *device_name;
    /**
     * @brief The type of message received, please refer to @ref aiot_dm_recv_type_t*/
    aiot_dm_recv_type_t type;
    /**
     * @brief message data union, different message types will use different message structures*/
    union {
        aiot_dm_recv_generic_reply_t        generic_reply;
        aiot_dm_recv_property_set_t         property_set;
        aiot_dm_recv_async_service_invoke_t async_service_invoke;
        aiot_dm_recv_sync_service_invoke_t  sync_service_invoke;
        aiot_dm_recv_raw_data_t             raw_data;
        aiot_dm_recv_raw_service_invoke_t   raw_service_invoke;
    } data;
} aiot_dm_recv_t;

/**
 * The function prototype definition of the message receiving callback function of the @brief data-model Modules. When the Modules receives the downlink data from the server, it will call this callback function and input the message data to the user through the <i>recv</i> parameter, \n
 * At the same time, the user context data pointer is returned to the user through the <i>userdata</i> parameter.
 *
 * @param[in] handle data-model instance handle
 * @param[in] recv message data sent by the service. <b>All data pointers in the message structure will become invalid after leaving the callback function. Memory copying must be used to save message data</b>
 * @param[in] userdata pointer to user context data, this pointer is set by the user by calling @ref aiot_dm_setopt to configure the @ref AIOT_DMOPT_USERDATA option
 *
 * @return void*/
typedef void (*aiot_dm_recv_handler_t)(void *handle, const aiot_dm_recv_t *recv, void *userdata);

/**
 * @brief initialize data-model instance
 *
 * @return void*
 * @retval non-NULL data-model instance handle
 * @retval NULL initialization failed, usually caused by memory allocation failure*/
void *aiot_dm_init(void);

/**
 * @brief Set data-model parameters
 *
 * @param[in] handle data-model instance handle
 * @param[in] option configuration option, for more information please see @ref aiot_dm_option_t
 * @param[in] data configuration data, please see @ref aiot_dm_option_t for more information
 *
 * @return int32_t
 * @retval STATE_SUCCESS parameter configuration successful
 * @retval STATE_USER_INPUT_NULL_POINTER The input parameter handle or data is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE The enumeration value of the input parameter optioin>=AIOT_DMOPT_MAX
 * @retval others refer to @ref aiot_state_api.h
 **/
int32_t aiot_dm_setopt(void *handle, aiot_dm_option_t option, void *data);

/**
 * @brief Send a data-model message to the IoT platform. The message type and message data are determined by the msg input parameter.
 *
 * @param[in] handle data-model instance handle
 * @param[in] msg message structure, which can specify the device sending the message <i>productKey</i>, <i>deviceName</i>; message type, message data, etc. For more information, please refer to @ref aiot_dm_msg_t
 *
 * @return int32_t
 * @retval >=STATE_SUCCESS message sent successfully, for @ref AIOT_DMMSG_PROPERTY_POST, @ref AIOT_DMMSG_EVENT_POST, @ref AIOT_DMMSG_GET_DESIRED and @ref AIOT_DMMSG_DELETE_DESIRED messages, \n
 * The message identifier <i>msg_id</i> value returned after successful sending is >STATE_SUCCESS
 * @retval STATE_USER_INPUT_NULL_POINTER input parameter <i>handle</i> or <i>msg</i> is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE The structure member <i>type</i> of the input parameter <i>msg</i> >= AIOT_DMMSG_MAX
 * @retval STATE_SYS_DEPEND_MALLOC_FAILED Memory allocation failed
 * @retval STATE_DM_MQTT_HANDLE_IS_NULL The user did not call @ref aiot_dm_setopt to configure the MQTT handle
 * @retval others refer to the corresponding error code description in @ref aiot_state_api.h or @ref STATE_DM_BASE
 **/
int32_t aiot_dm_send(void *handle, const aiot_dm_msg_t *msg);

/**
 * @brief Destroy the data-model instance and release resources
 *
 * @param[in] p_handle pointer to the data-model instance handle
 * @return int32_t
 * @retval STATE_SUCCESS execution successful
 * @retval <STATE_SUCCESS execution failed
 **/
int32_t aiot_dm_deinit(void **p_handle);


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef __AIOT_DM_API_H__ */

