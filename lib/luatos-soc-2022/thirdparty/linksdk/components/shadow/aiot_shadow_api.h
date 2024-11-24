/**
 * @file aiot_shadow_api.h
 * @brief shadow Modules header file, providing the ability to update, delete, and obtain device shadows
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 * Please follow the following process to use the API
 *
 * 1. Before using the device shadow Modules, users should first create an MQTT instance
 *
 * 2. Call `aiot_shadow_init` to create a device shadow instance and save the instance handle
 *
 * 3. Call `aiot_shadow_setopt` to configure the `AIOT_SHADOWOPT_MQTT_HANDLE` option to set the MQTT handle. This option is a mandatory configuration option
 *
 * 4. Call `aiot_shadow_setopt` to configure the `AIOT_SHADOWOPT_RECV_HANDLER` and `AIOT_SHADOWOPT_USERDATA` options to register the data acceptance callback function and user context data pointer
 *
 * 5. Before using `aiot_shadow_send` to send messages, the connection establishment of the MQTT instance should be completed first
 *
 * 6. Mobilize `aiot_shadow_send` to send messages such as updating device shadow, obtaining device shadow or deleting device to the cloud platform, and handle various types of cloud response messages or active push-down messages in the registered callback function.
 **/
#ifndef __AIOT_SHADOW_API_H__
#define __AIOT_SHADOW_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x1300~-0x13FF expresses the status code of the SDK in the data-model Modules*/
#define STATE_SHADOW_BASE                                           (-0x1300)

/**
 * @brief The user did not call @ref aiot_shadow_setopt to configure the MQTT handle*/
#define STATE_SHADOW_MQTT_HANDLE_IS_NULL                            (-0x1301)

/**
 * @brief When the user reports the @ref AIOT_SHADOWMSG_UPDATE or @ref AIOT_SHADOWMSG_DELETE_REPORTED message, reported in the message structure is NULL
 **/
#define STATE_SHADOW_REPORTED_DATA_IS_NULL                          (-0x1302)

/**
 * @brief An error occurred while parsing the topic corresponding to the downstream data.*/
#define STATE_SHADOW_INTERNAL_TOPIC_ERROR                           (-0x1303)

/**
 * @brief Log status code when receiving server downlink message*/
#define STATE_SHADOW_LOG_RECV                                       (-0x1304)

/**
 * @brief Log status code when parsing server downlink messages fails*/
#define SATAE_SHADOW_LOG_PARSE_RECV_MSG_FAILED                      (-0x1305)


/**
 * @brief @ref aiot_shadow_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_shadow_setopt
 *
 * When the data type of data is char *, take configuring @ref AIOT_SHADOWOPT_PRODUCT_KEY as an example:
 *
 * char *product_key = "xxx";
 * aiot_shadow_setopt(shadow_handle, AIOT_SHADOWOPT_PRODUCT_KEY, product_key);
 **/
typedef enum {
    /**
     * @brief The MQTT handle that the Modules depends on
     *
     * @details
     *
     * The shadow Modules depends on the underlying MQTT Modules. Users must configure the correct MQTT handle, otherwise it will not work properly. The data type is (void *)*/
    AIOT_SHADOWOPT_MQTT_HANDLE,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message and informs the user that the data type is (aiot_shadow_recv_handler_t)*/
    AIOT_SHADOWOPT_RECV_HANDLER,

    /**
     * @brief Users need the SDK temporary context, the data type is (void *)
     *
     * @details This context pointer will be passed to the user by the SDK when the callback set by AIOT_SHADOWOPT_RECV_HANDLER is called.*/
    AIOT_SHADOWOPT_USERDATA,

    /**
     * @brief Maximum number of configuration options, cannot be used as configuration parameters*/
    AIOT_SHADOWOPT_MAX,
} aiot_shadow_option_t;

/**
 * @brief shadow Modules sends message type
 *
 * @details
 *
 * This enumeration type includes all data types supported by the shadow Modules. Different message types will have different message structures.
 **/
typedef enum {
    /**
     * @brief updates the reported value in the device shadow, message structure reference @ref aiot_shadow_msg_update_t
     **/
    AIOT_SHADOWMSG_UPDATE,

    /**
     * @brief Clear the desired value of the device shadow, message structure reference @ref aiot_shadow_msg_clean_desired_t
     **/
    AIOT_SHADOWMSG_CLEAN_DESIRED,

    /**
     * @brief Get device shadow, message structure reference @ref aiot_shadow_msg_get_t
     **/
    AIOT_SHADOWMSG_GET,

    /**
     * @brief Delete some or all reported values   of the device shadow, message structure reference @ref aiot_shadow_msg_delete_reported_t
     **/
    AIOT_SHADOWMSG_DELETE_REPORTED,

    /**
     * @brief Maximum number of messages, cannot be used as message type*/
    AIOT_SHADOWMSG_MAX,
} aiot_shadow_msg_type_t;

/**
 * @brief Message structure used to <b>update reported data in device shadow</b>*/
typedef struct {
    /**
     * @brief device shadow reported object string, <b>must be a string ending with the terminator '\0'</b>, such as "{\"LightSwitch\": 1}"*/
    char *reported;

    /**
     * @brief The target version of the device shadow, <b>must be greater than the current version of the device shadow</b>. If set to -1, the device shadow data will be cleared and the device shadow version will be updated to 0*/
    int64_t version;
} aiot_shadow_msg_update_t;

/**
 * @brief Message structure used to <b>clear desired data in device shadow</b>*/
typedef struct {
    /**
     * @brief The target version of the device shadow, <b>must be greater than the current version of the device shadow</b>*/
    int64_t version;
} aiot_shadow_msg_clean_desired_t;

/**
 * @brief Message structure used to <b>obtain device shadow</b>,*/
typedef struct {
    /**
     * @brief reserved fields*/
    uint32_t resevered;
} aiot_shadow_msg_get_t;

/**
 * @brief Message structure used to <b>delete reported data in device shadow</b>*/
typedef struct {
    /**
     * @brief The reported data that the user wants to delete, <b>must be a string ending with the terminator '\0'</b>. \n
     * If you want to delete all reported data, you should fill in the "\"null\"" string \n
     * If you want to delete part of the reported data, define the corresponding value as null. If you want to clear only the LightSwitch value, fill in "{\"LightSwitch\":\"null\"}"*/
    char *reported;

    /**
     * @brief The target version of the device shadow, <b>must be greater than the current version of the device shadow</b>*/
    int64_t version;
} aiot_shadow_msg_delete_reported_t;

/**
 * The message structure of the message sent by the @brief data-model Modules*/
typedef struct {
    /**
     * @brief The product_key of the device to which the message belongs. If it is NULL, the product_key configured through aiot_shadow_setopt will be used \n
     * In the scenario of gateway sub-device, the message of the sub-device can be sent to the cloud by specifying the product_key of the sub-device.*/
    char *product_key;
    /**
     * @brief The device_name of the device to which the message belongs. If it is NULL, the device_name configured through aiot_shadow_setopt will be used \n
     * In the scenario of gateway sub-device, the message of the sub-device can be sent to the cloud by specifying the product_key of the sub-device.*/
    char *device_name;
    /**
     * @brief message type, please refer to @ref aiot_shadow_msg_type_t*/
    aiot_shadow_msg_type_t type;
    /**
     * @brief message data union, different message types will use different message structures*/
    union {
        aiot_shadow_msg_update_t            update;
        aiot_shadow_msg_clean_desired_t     clean_desired;
        aiot_shadow_msg_get_t               get;
        aiot_shadow_msg_delete_reported_t   delete_reporte;
    } data;
} aiot_shadow_msg_t;


/**
 * When the @brief shadow Modules receives a message from the network, it notifies the user of the message type*/
typedef enum {
    /**
     * @brief After the device sends @ref AIOT_SHADOWMSG_UPDATE, @ref AIOT_SHADOWMSG_CLEAN_DESIRED or @ref AIOT_SHADOWMSG_DELETE_REPORTED, the response message returned by the cloud, \n
     * Message data structure reference @ref aiot_shadow_recv_generic_reply_t*/
    AIOT_SHADOWRECV_GENERIC_REPLY,

    /**
     * @brief When the device is online, the shadow content automatically delivered by the cloud, message data structure reference @ref aiot_shadow_recv_control_t*/
    AIOT_SHADOWRECV_CONTROL,

    /**
     * @brief Actively obtain the shadow content returned by the device shadow content cloud, message data structure reference @ref aiot_shadow_recv_get_reply_t*/
    AIOT_SHADOWRECV_GET_REPLY,
} aiot_shadow_recv_type_t;

/**
 * @brief After the device sends a @ref AIOT_SHADOWMSG_UPDATE, @ref AIOT_SHADOWMSG_CLEAN_DESIRED or @ref AIOT_SHADOWMSG_DELETE_REPORTED type message, the response message returned by the cloud*/
typedef struct {
    /**
     * @brief pointer to response data*/
    char *payload;

    /**
     * @brief response data length*/
    uint32_t payload_len;

    /**
     * @brief Response status string. If the cloud processing is successful, it will be <b>success</b>. If the message is sent incorrectly, it will be <b>error</b>. The error message and error code are placed in the payload.*/
    char *status;

    /**
     * @brief The timestamp corresponding to the response message*/
    uint64_t timestamp;
} aiot_shadow_recv_generic_reply_t;

/**
 * @brief If the device is online, the user application calls the cloud API <a href="https://help.aliyun.com/document_detail/69954.html#doc-api-Iot-UpdateDeviceShadow">UpdateDeviceShadow</a> and downloads it from the cloud push message*/
typedef struct {
    /**
     * @brief pointer to device shadow data*/
    char *payload;

    /**
     * @brief device shadow data length*/
    uint32_t payload_len;

    /**
     * @brief device shadow version*/
    uint64_t version;
} aiot_shadow_recv_control_t;

/**
 * @brief After the device sends the @ref AIOT_SHADOWMSG_GET type message, the device shadow data returned by the cloud*/
typedef struct {
    /**
     * @brief pointer to device shadow data*/
    char *payload;

    /**
     * @brief device shadow data length*/
    uint32_t payload_len;

    /**
     * @brief device shadow version number*/
    uint64_t version;
} aiot_shadow_recv_get_reply_t;

/**
 * When the @brief shadow Modules receives a message from the network, it notifies the user of the message content*/
typedef struct {
    /**
     * @brief product_key of the device to which the message belongs*/
    char *product_key;
    /**
     * @brief device_name of the device to which the message belongs*/
    char *device_name;
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_shadow_recv_type_t*/
    aiot_shadow_recv_type_t  type;
    /**
     * @brief message data union, different message types will use different message structures*/
    union {
        aiot_shadow_recv_generic_reply_t generic_reply;
        aiot_shadow_recv_control_t control;
        aiot_shadow_recv_get_reply_t get_reply;
    } data;
} aiot_shadow_recv_t;


/**
 * When the @brief shadow Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle shadow session handle
 * @param[in] recv shadow accepts the message structure, which stores the content of the received shadow message.
 * @param[in] userdata pointer to user context data, this pointer is set by the user by calling @ref aiot_shadow_setopt to configure the @ref AIOT_SHADOWOPT_USERDATA option
 *
 * @return void*/
typedef void (* aiot_shadow_recv_handler_t)(void *handle,
        const aiot_shadow_recv_t *recv, void *userdata);

/**
 * @brief creates a shadow session instance and configures session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL shadow instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_shadow_init(void);

/**
 * @brief configure shadow session
 *
 * @param[in] handle shadow session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_shadow_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_shadow_option_t
 *
 * @return int32_t
 * @retval STATE_SUCCESS parameter configuration successful
 * @retval STATE_USER_INPUT_NULL_POINTER The input parameter handle or data is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE The enumeration value of the input parameter optioin>=AIOT_SHADOWOPT_MAX
 * @retval others refer to @ref aiot_state_api.h
 **/
int32_t aiot_shadow_setopt(void *handle, aiot_shadow_option_t option, void *data);

/**
 * @brief Send shadow message request to the server
 *
 * @param[in] handle shadow session handle
 * @param[in] msg message structure, which can specify the device sending the message <i>productKey</i>, <i>deviceName</i>; message type, message data, etc. For more information, please refer to @ref aiot_shadow_msg_t
 *
 * @return int32_t
 * @retval STATE_SUCCESS request sent successfully
 * @retval STATE_USER_INPUT_NULL_POINTER input parameter <i>handle</i> or <i>msg</i> is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE The structure member <i>type</i> of the input parameter <i>msg</i> >= AIOT_SHADOWMSG_MAX
 * @retval STATE_SYS_DEPEND_MALLOC_FAILED Memory allocation failed
 * @retval STATE_SHADOW_MQTT_HANDLE_IS_NULL The user did not call @ref aiot_shadow_setopt to configure the MQTT handle
 * @retval others refer to the corresponding error code description in @ref aiot_state_api.h or @ref STATE_SHADOW_BASE
 **/
int32_t aiot_shadow_send(void *handle, aiot_shadow_msg_t *msg);

/**
 * @brief End the shadow session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to shadow session handle
 *
 * @return int32_t
 * @retval STATE_SUCCESS execution successful
 * @retval <STATE_SUCCESS execution failed
 **/
int32_t aiot_shadow_deinit(void **handle);


#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_SHADOW_API_H__ */

