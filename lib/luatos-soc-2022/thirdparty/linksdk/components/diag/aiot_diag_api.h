/**
 * @file aiot_diag_api.h
 * @brief diag Modules header file, providing the ability to diagnose the SDK
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __AIOT_DIAG_API_H__
#define __AIOT_DIAG_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x0.00~-0x0.FF expresses the status code of the SDK in the diag Modules*/
#define STATE_DIAG_BASE                                             (-0x1400)

/**
 * Description of @brief STATE_DIAG_CODE1*/
#define STATE_DIAG_LOG_UNKNOWN_STATE_CODE_BASE                      (-0x1401)

/**
 * Description of @brief STATE_DIAG_CODE2*/
#define STATE_DIAG_CODE2                                            (-0x1402)

/**
 * Description of @brief STATE_DIAG_PUB_FAILED*/
#define STATE_DIAG_PUB_FAILED                                       (-0x1403)

/**
 * When the @brief diag Modules receives a message from the network, it notifies the user of the message type*/
typedef enum {
    AIOT_DIAGRECV_DIAG_CONTROL
} aiot_diag_recv_type_t;

/**
 * When the @brief diag Modules receives a message from the network, it notifies the user of the message content*/
typedef struct {
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_diag_recv_type_t*/
    aiot_diag_recv_type_t  type;
    union {
        /**
         * Cloud control instructions received by @brief*/
        struct {
            /**
             * @brief 0: turn off the diagnostic function, 1: turn on the diagnostic function*/
            uint32_t data;
        } diag_control;
    } data;
} aiot_diag_recv_t;

/**
 * When the @brief diag Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle diag session handle
 * @param[in] packet diag message structure, which stores the content of the received diag message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_diag_recv_handler_t)(void *handle, const aiot_diag_recv_t *packet, void *userdata);

/**
 * @brief diag The event type that notifies the user when a status change worthy of the user's attention occurs within the diag Modules*/
typedef enum {
    /**
     * @brief Alarm information generated by the diagnostic Modules*/
    AIOT_DIAGEVT_ALERT
} aiot_diag_event_type_t;

/**
 * @brief diag Modules notifies the user of the event content when a status change worthy of the user's attention occurs within the Modules*/
typedef struct {
    /**
     * @brief The event type corresponding to the event content. For more information, please refer to @ref aiot_diag_event_type_t*/
    aiot_diag_event_type_t  type;
    union {
        struct {
            /**
             * @brief alarm Modules name*/
            char *module_name;
            /**
             * @brief Alarm level*/
            char *level;
            /**
             * @brief Alarm information description string*/
            char *desc;
        } alert;
    } data;
} aiot_diag_event_t;

/**
 * When a status change that deserves the user's attention occurs within the @brief diag Modules, the event callback function called by the user is notified.
 *
 * @param[in] handle, diag session handle
 * @param[in] event, the content of the event that occurred in the diag Modules
 * @param[in] userdata, user context
 *
 * @return void*/
typedef void (*aiot_diag_event_handler_t)(void *handle, const aiot_diag_event_t *event, void *userdata);

/**
 * @brief configuration parameters of diagnostic items*/
typedef struct {
    /**
     * @brief Switch whether to diagnose the current diagnostic item
     *
     * @details
     *
     * 0: Turn off the diagnosis of the current diagnosis item, 1: Turn on the diagnosis of the current diagnosis item*/
    uint8_t enabled;
    /**
     * @brief The minimum time interval between two consecutive alarms for the current diagnostic item*/
    uint32_t interval_ms;
    /**
     * @brief warning level alarm threshold*/
    int64_t warning_threashold;
    /**
     * @brief fatal level alarm threshold*/
    int64_t fatal_threshold;
} aiot_diag_config_t;

/**
 * @brief @ref aiot_diag_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_diag_setopt
 *
 * 1. When the data type of data is void *, take the configuration of @ref AIOT_DIAGOPT_MQTT_HANDLE as an example:
 *
 * void *mqtt_handle = aiot_mqtt_init();
 * aiot_diag_setopt(diag_handle, AIOT_DIAGOPT_MQTT_HANDLE, mqtt_handle);
 *
 * 2. When the data type of data is other data types, take configuring @ref AIOT_DIAGOPT_LOCAL_REPORT_ENABLED as an example:
 *
 * uint8_t local_report_enabled = 1;
 * aiot_mqtt_setopt(diag_handle, AIOT_DIAGOPT_LOCAL_REPORT_ENABLED, (void *)&local_report_enabled);*/
typedef enum {
    /**
     * @brief diag session requires an MQTT handle. You need to establish an MQTT connection first, and then set the MQTT handle.
     *
     * @details
     *
     * Data type: (void *)*/
    AIOT_DIAGOPT_MQTT_HANDLE,

    /**
     * @brief Whether it is necessary to output alarm information from the event callback function
     *
     * @details
     *
     * 0: Do not output alarm information from the event callback function, 1: Output alarm information from the event callback function
     *
     * Data type: (uint8_t *)*/
    AIOT_DIAGOPT_LOCAL_REPORT_ENABLED,

    /**
     * @brief Whether it is necessary to report alarm information to the cloud
     *
     * @details
     *
     * 0: Do not report alarm information to the cloud, 1: Report alarm information to the cloud
     *
     * Data type: (uint8_t *)*/
    AIOT_DIAGOPT_CLOUD_REPORT_ENABLED,

    /**
     * @brief MQTT connection duration alarm configuration
     *
     * @details
     *
     * Data type: ( @ref aiot_diag_config_t )*/
    AIOT_DIAGOPT_MQTT_CONNECTION,

    /**
     * @brief MQTT heartbeat loss alarm configuration
     *
     * @details
     *
     * Data type: ( @ref aiot_diag_config_t )*/
    AIOT_DIAGOPT_MQTT_HEARTBEAT,

    /**
     * @brief Alink protocol uplink packet reply speed alarm configuration
     *
     * @details
     *
     * Data type: ( @ref aiot_diag_config_t )*/
    AIOT_DIAGOPT_ALINK_UPLINK,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message to notify the user
     *
     * @details
     *
     * Data type: ( @ref aiot_diag_recv_handler_t )*/
    AIOT_DIAGOPT_RECV_HANDLER,

    /**
     * @brief diag Internal events will be notified from this callback function
     *
     * @details
     *
     * Data type: (@ref aiot_diag_event_handler_t)*/
    AIOT_DIAGOPT_EVENT_HANDLER,

    /**
     * @brief User needs SDK temporary context
     *
     * @details This context pointer will be passed to the user by the SDK when the callbacks set by AIOT_DIAGOPT_RECV_HANDLER and AIOT_DIAGOPT_EVENT_HANDLER are called.
     *
     * Data type: (void *)*/
    AIOT_DIAGOPT_USERDATA,
    AIOT_DIAGOPT_MAX
} aiot_diag_option_t;

/**
 * @brief Create a diag session instance and configure session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL diag instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_diag_init(void);

/**
 * @brief configure diag session
 *
 * @param[in] handle diag session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_diag_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_diag_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval >=STATE_SUCCESS Parameter configuration successful
 **/
int32_t aiot_diag_setopt(void *handle, aiot_diag_option_t option, void *data);

/**
 * @brief End the diag session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to diag session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_diag_deinit(void **handle);

/**
 * @brief Start diagnosing SDK internal information
 *
 * @param handle diag session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS request failed to send
 * @retval >=STATE_SUCCESS The request was sent successfully*/
int32_t aiot_diag_start(void *handle);

/**
 * @brief Stop diagnosing SDK internal information
 *
 * @param handle diag session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS data reception failed
 * @retval >=STATE_SUCCESS data received successfully*/
int32_t aiot_diag_stop(void *handle);

#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_DIAG_API_H__ */
