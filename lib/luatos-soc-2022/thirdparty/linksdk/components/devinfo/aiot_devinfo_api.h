/**
 * @file aiot_devinfo_api.h
 * @brief devinfo Modules header file, providing the ability to update and delete device labels
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 * The Devinfo Modules is used to update or delete device tags to the Alibaba Cloud IoT platform. The API usage process is as follows:
 *
 * 1. First refer to the instructions of @ref aiot_mqtt_api.h to ensure that the `MQTT` connection with the IoT platform is successfully established.
 *
 * 2. Call @ref aiot_devinfo_init to initialize the devinfo session and obtain the session handle
 *
 * 3. Call @ref aiot_devinfo_setopt to configure the parameters of the devinfo session. For common configuration items, see the description of @ref aiot_devinfo_setopt.
 *
 * 4. Call @ref aiot_devinfo_send to send a request for label change, such as update or delete
 *
 * 5. After the received response is processed by the SDK, the @ref AIOT_DEVINFOOPT_RECV_HANDLER callback function configured by @ref aiot_devinfo_setopt will be called to notify the user of the cloud response.
 **/
#ifndef __AIOT_DEVINFO_API_H__
#define __AIOT_DEVINFO_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x1200~-0x12FF expresses the status code of the SDK in the devinfo Modules*/
#define STATE_DEVINFO_BASE                                             (-0x1200)

/**
 * @brief MQTT session handle is not set, please set the MQTT session handle through @ref aiot_devinfo_setopt*/
#define STATE_DEVINFO_MISSING_MQTT_HANDLE                              (-0x1201)

/**
 * @brief devinfo Modules notifies the user of the message type when receiving a message from the network*/
typedef enum {
    AIOT_DEVINFORECV_GENERIC_REPLY,
} aiot_devinfo_recv_type_t;

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
} aiot_devinfo_recv_generic_reply_t;

/**
 * When the @brief devinfo Modules receives a message from the network, it notifies the user of the message content*/
typedef struct {
    char *product_key;
    char *device_name;
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_devinfo_recv_type_t*/
    aiot_devinfo_recv_type_t  type;
    union {
        /**
         * @brief Response received from the cloud to update or delete device tags*/
        aiot_devinfo_recv_generic_reply_t generic_reply;
    } data;
} aiot_devinfo_recv_t;

/**
 * When the @brief devinfo Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle devinfo session handle
 * @param[in] packet devinfo message structure, which stores the content of the received devinfo message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_devinfo_recv_handler_t)(void *handle, const aiot_devinfo_recv_t *packet, void *userdata);

/**
 * @brief devinfo The event type that notifies the user when a status change occurs inside the devinfo Modules that deserves the user's attention*/
typedef enum {
    /**
     * @brief The device information in the received response is illegal, and the product key and device name cannot be obtained.*/
    AIOT_DEVINFOEVT_INVALID_DEVINFO,
    /**
     * @brief The field in the received response is illegal*/
    AIOT_DEVINFOEVT_INVALID_RESPONSE,
    /**
     * @brief The field format in the received response is incorrect*/
    AIOT_DEVINFOEVT_INVALID_RESPONSE_FORMAT,
} aiot_devinfo_event_type_t;

/**
 * @brief devinfo Modules notifies the user of the event content when a status change that deserves the user's attention occurs.*/
typedef struct {
    /**
     * @brief The event type corresponding to the event content. For more information, please refer to @ref aiot_devinfo_event_type_t*/
    aiot_devinfo_event_type_t  type;
} aiot_devinfo_event_t;

/**
 * @brief devinfo Modules notifies the user of the event callback function called when a status change that deserves the user's attention occurs.
 *
 * @param[in] handle, devinfo session handle
 * @param[in] event, the content of the event that occurred in the devinfo Modules
 * @param[in] userdata, user context
 *
 * @return void*/
typedef void (*aiot_devinfo_event_handler_t)(void *handle, const aiot_devinfo_event_t *event, void *userdata);

/**
 * @brief @ref send message type in aiot_devinfo_msg_t
 *
 * @details
 *
 * There are two message types, namely update device label and delete device label*/
typedef enum {
    /**
     * @brief Update device label*/
    AIOT_DEVINFO_MSG_UPDATE,
    /**
     * @brief delete device label*/
    AIOT_DEVINFO_MSG_DELETE
} aiot_devinfo_msg_type_t;

/**
 * @brief Update or delete the params content of the device label*/
typedef struct {
    char *params;
} aiot_devinfo_msg_data_t;

typedef struct {
    /**
     * @brief product key of the device*/
    char *product_key;
    /**
     * @brief device name of the device*/
    char *device_name;
    /**
     * @brief message type, for more information please refer to @ref aiot_devinfo_msg_type_t*/
    aiot_devinfo_msg_type_t type;
    union {
        /**
         * @brief Update device label, format: "[{\"attrKey\":\"xxx\",\"attrValue\":\"yyy\"}]"
         *
         * @details
         *
         * As can be seen from the above format, the format for updating device tags is a JSON array. Multiple sets of device tags can be reported at one time by attrKey and attrValue.*/
        aiot_devinfo_msg_data_t update;
        /**
         * @brief Delete device tag, format: "[{\"attrKey\":\"xxx\"}]"
         *
         * @details
         *
         * As can be seen from the above format, the format for deleting device tags is a JSON array. Multiple sets of device tags can be deleted by pressing attrKey at one time.*/
        aiot_devinfo_msg_data_t delete;
    } data;
} aiot_devinfo_msg_t;

/**
 * @brief @ref aiot_devinfo_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_devinfo_setopt
 *
 * 1. When the data type of data is void *, take configuring @ref AIOT_DEVINFOOPT_MQTT_HANDLE as an example:
 *
 * void *mqtt_handle = aiot_mqtt_init();
 * aiot_devinfo_setopt(devinfo_handle, AIOT_DEVINFOOPT_MQTT_HANDLE, mqtt_handle);
 *
 * 2. When the data type of data is other data types, take configuring @ref AIOT_DEVINFOOPT_DEINIT_TIMEOUT_MS as an example:
 *
 * uint32_t deinit_timeout_ms = 443;
 * aiot_devinfo_setopt(devinfo_handle, AIOT_DEVINFOOPT_DEINIT_TIMEOUT_MS, (void *)&deinit_timeout_ms);*/
typedef enum {
    /**
     * @brief devinfo session requires an MQTT handle. You need to establish an MQTT connection first, and then set the MQTT handle.*/
    AIOT_DEVINFOOPT_MQTT_HANDLE,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message to notify the user
     *
     * @details
     *
     * Data type: ( @ref aiot_devinfo_recv_handler_t)*/
    AIOT_DEVINFOOPT_RECV_HANDLER,

    /**
     * @brief sets a callback, which is called when the internal status of the SDK changes to notify the user
     *
     * @details
     *
     * Data type: ( @ref aiot_devinfo_event_handler_t)*/
    AIOT_DEVINFOOPT_EVENT_HANDLER,

    /**
     * @brief Users need the SDK temporary context, the data type is (void *)
     *
     * @details This context pointer will be passed to the user by the SDK when the callbacks set by AIOT_DEVINFOOPT_RECV_HANDLER and AIOT_DEVINFOOPT_EVENT_HANDLER are called.*/
    AIOT_DEVINFOOPT_USERDATA,

    /**
     * @brief When destroying the devinfo instance, the time to wait for other APIs to complete execution
     *
     * @details
     *
     * When calling @ref aiot_devinfo_deinit to destroy the devinfo instance, if you continue to call other aiot_devinfo_xxx API, the API will return @ref STATE_USER_INPUT_EXEC_DISABLED error
     *
     * At this point, users should stop calling other aiot_devinfo_xxx APIs
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_DEVINFOOPT_DEINIT_TIMEOUT_MS,
    AIOT_DEVINFOOPT_MAX
} aiot_devinfo_option_t;

/**
 * @brief Create a devinfo session instance and configure session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL devinfo instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_devinfo_init(void);

/**
 * @brief configure devinfo session
 *
 * @param[in] handle devinfo session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_devinfo_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_devinfo_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval >=STATE_SUCCESS Parameter configuration successful
 **/
int32_t aiot_devinfo_setopt(void *handle, aiot_devinfo_option_t option, void *data);

/**
 * @brief End the devinfo session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to devinfo session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_devinfo_deinit(void **handle);

/**
 * @brief Send devinfo message request to devinfo server
 *
 * @param handle devinfo session handle
 * @param msg devinfo message sent to the cloud to delete/update device label information
 *
 * @return int32_t
 * @retval <STATE_SUCCESS request failed to send
 * @retval >=STATE_SUCCESS The request was sent successfully*/
int32_t aiot_devinfo_send(void *handle, aiot_devinfo_msg_t *msg);

#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_DEVINFO_API_H__ */

