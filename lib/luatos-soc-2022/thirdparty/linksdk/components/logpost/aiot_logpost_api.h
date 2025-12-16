/**
 * @file aiot_logpost_api.h
 * @brief logpost Modules header file, providing the ability to upload device-side logs to the cloud
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 * 1. Before using the log cloud Modules, users should first create an MQTT instance.
 *
 * 2. Call `aiot_logpost_init` to create a log cloud instance and save the instance handle
 *
 * 3. Call `aiot_logpost_setopt` to configure the `AIOT_LOGPOSTOPT_MQTT_HANDLE` option to set the MQTT handle. This option is a mandatory configuration option
 *
 * 4. Call `aiot_logpost_setopt` to configure the `AIOT_LOGPOSTOPT_EVENT_HANDLER` and `AIOT_LOGPOSTOPT_USER_DATA` options to register the event reception callback function and user context data pointer
 *
 * 5. Before using `aiot_logpost_send` to send log messages, the connection establishment of the MQTT instance should be completed first.
 **/
#ifndef __AIOT_LOGPOST_API_H__
#define __AIOT_LOGPOST_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x1500~-0x15FF expresses the status code of the SDK in the logpost Modules*/
#define STATE_LOGPOST_BASE                                              (-0x1500)

/**
 * @brief The user did not call @ref aiot_logpost_setopt to configure the MQTT handle*/
#define STATE_LOGPOST_MQTT_HANDLE_IS_NULL                               (-0x1501)

/**
 * @brief Log reporting is configured to be turned off by the cloud*/
#define STATE_LOGPOST_POST_TURN_OFF                                     (-0x1502)

/**
 * @brief The log message has the wrong log level*/
#define STATE_LOGPOST_LOGLEVEL_ERROR                                    (-0x1503)

/**
 * @brief The Modules name field of the log message is NULL*/
#define STATE_LOGPOST_LOG_MODULE_NAME_IS_NULL                           (-0x1504)

/**
 * @brief The log content field of the log message is NULL*/
#define STATE_LOGPOST_LOG_CONTENT_IS_NULL                               (-0x1505)

/**
 * @brief The log content field string length of the log message is greater than 4096 bytes*/
#define STATE_LOGPOST_LOG_CONTENT_TOO_LONG                              (-0x1506)

/**
 * @brief Internal log status code when receiving server downlink message*/
#define STATE_LOGPOST_LOG_RECV                                          (-0x1507)

/**
 * @brief Internal log status code when parsing server downlink message fails*/
#define SATAE_LOGPOST_LOG_PARSE_MSG_FAILED                              (-0x1508)


/**
 * @brief @ref aiot_logpost_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_logpost_setopt*/
typedef enum {
    /**
     * @brief The MQTT handle that the Modules depends on
     *
     * @details
     *
     * The LOGPOST Modules depends on the underlying MQTT Modules. Users must configure the correct MQTT handle, otherwise it will not work properly. The data type is (void *)*/
    AIOT_LOGPOSTOPT_MQTT_HANDLE,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message and informs the user that the data type is (aiot_logpost_recv_handler_t)*/
    AIOT_LOGPOSTOPT_EVENT_HANDLER,

    /**
     * @brief Users need the SDK temporary context, the data type is (void *)
     *
     * @details This context pointer will be passed to the user by the SDK when the callback set by AIOT_LOGPOSTOPT_RECV_HANDLER is called.*/
    AIOT_LOGPOSTOPT_USERDATA,

    /**
     * @brief System log switch.
     *
     * @detail Set to 1 to report system logs, and to 0 to not report system logs. The system logs here refer to the connection establishment time and network delay*/
    AIOT_LOGPOSTOPT_SYS_LOG,

    /**
     * @brief Maximum number of configuration options, cannot be used as configuration parameters*/
    AIOT_LOGPOSTOPT_MAX,

} aiot_logpost_option_t;


/**
 * @brief Log level enumeration type definition*/
typedef enum {
    AIOT_LOGPOST_LEVEL_FATAL,
    AIOT_LOGPOST_LEVEL_ERR,
    AIOT_LOGPOST_LEVEL_WARN,
    AIOT_LOGPOST_LEVEL_INFO,
    AIOT_LOGPOST_LEVEL_DEBUG,
} aiot_logpost_level_t;

/**
 * @brief log data structure definition
 **/
typedef struct {
    /**
     * @brief utc timestamp, unit is ms, this value will be displayed directly on the cloud console device log page*/
    uint64_t timestamp;

    /**
     * @brief log level, please see @ref aiot_logpost_level_t definition*/
    aiot_logpost_level_t loglevel;

    /**
     * @brief Modules name, <b>must be a string ending with the terminator '\0'</b>*/
    char *module_name;

    /**
     * @brief status code, which can be used to identify the status corresponding to the log*/
    int32_t code;

    /**
     * @brief message identifier, used to identify cloud downstream messages. The corresponding identifier can be obtained from the message receiving callback function of the data-Modules Modules. If the user sets it to 0, this field will not be uploaded.*/
    uint64_t msg_id;

    /**
     * @brief Log content, <b>must be a string ending with the terminator '\0'</b>*/
    char *content;
} aiot_logpost_msg_t;


/**
 * @brief The event type that notifies the user when a status change that deserves the user's attention occurs within the logpost Modules*/
typedef enum {
    /**
     * @brief receives the log configuration data issued by the cloud*/
    AIOT_LOGPOSTEVT_CONFIG_DATA,
} aiot_logpost_event_type_t;

/**
 * @brief logpost Modules notifies the user of the event content when a status change that deserves the user's attention occurs.*/
typedef struct {
    /**
     * @brief The event type corresponding to the event content. For more information, please refer to @ref aiot_logpost_event_type_t*/
    aiot_logpost_event_type_t  type;

    union {
        /**
         * @brief log configuration data structure*/
        struct {
            /**
             * @brief Log switch status, 0: close log upload; 1: open log upload*/
            uint8_t on_off;
        } config_data;
    } data;
} aiot_logpost_event_t;


/**
 * When a status change worthy of the user's attention occurs within the @brief logpost Modules, the event callback function called by the user is notified.
 *
 * @param[in] handle logpost session handle
 * @param[in] The content of the event that occurred in the event logpost Modules
 * @param[in] userdata pointer to user context data, this pointer is set by the user by calling @ref aiot_logpost_setopt to configure the @ref AIOT_LOGPOSTOPT_USERDATA option
 *
 * @return void*/
typedef void (*aiot_logpost_event_handler_t)(void *handle,
        const aiot_logpost_event_t *event, void *userdata);

/**
 * @brief Create a logpost session instance and configure session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL logpost instance
 * @retval NULL initialization failed, usually caused by memory allocation failure*/
void *aiot_logpost_init(void);

/**
 * @brief configure logpost session
 *
 * @param[in] handle logpost session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_logpost_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_logpost_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval STATE_SUCCESS parameter configuration successful
 * @retval STATE_USER_INPUT_NULL_POINTER The input parameter handle or data is NULL
 * @retval STATE_USER_INPUT_OUT_RANGE The enumeration value of the input parameter optioin>=AIOT_LOGPOSTOPT_MAX
 * @retval others refer to @ref aiot_state_api.h*/
int32_t aiot_logpost_setopt(void *handle, aiot_logpost_option_t option, void *data);

/**
 * @brief Send log messages to the server
 *
 * @param[in] handle logpost session handle
 * @param[in] msg message structure, which can specify the log corresponding Modules, log level, etc. For more information, please refer to @ref aiot_logpost_msg_t
 *
 * @return int32_t
 * @retval STATE_SUCCESS request sent successfully
 * @retval STATE_USER_INPUT_NULL_POINTER input parameter <i>handle</i> or <i>msg</i> is NULL
 * @retval STATE_SYS_DEPEND_MALLOC_FAILED Memory allocation failed
 * @retval STATE_LOGPOST_MQTT_HANDLE_IS_NULL The user did not call @ref aiot_logpost_setopt to configure the MQTT handle
 * @retval others refer to the corresponding error code description in @ref aiot_state_api.h or @ref STATE_SHADOW_BASE
 **/
int32_t aiot_logpost_send(void *handle, aiot_logpost_msg_t *msg);

/**
 * @brief End the logpost session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to logpost session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful*/
int32_t aiot_logpost_deinit(void **handle);


#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_LOGPOST_API_H__ */

