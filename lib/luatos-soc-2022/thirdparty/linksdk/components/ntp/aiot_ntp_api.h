/**
 * @file aiot_ntp_api.h
 * @brief ntp Modules header file, providing the ability to obtain UTC time
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 * The NTP Modules is used to obtain UTC time from the Alibaba Cloud IoT platform. The API usage process is as follows:
 *
 * 1. First refer to the instructions of @ref aiot_mqtt_api.h to ensure that the `MQTT` connection with the IoT platform is successfully established.
 *
 * 2. Call @ref aiot_ntp_init to initialize the ntp session and obtain the session handle
 *
 * 3. Call @ref aiot_ntp_setopt to configure the parameters of the NTP session. For common configuration items, see the description of @ref aiot_ntp_setopt
 *
 * 4. Call @ref aiot_ntp_send_request to send NTP request
 *
 * 5. After the received UTC time is processed by the SDK, the @ref AIOT_NTPOPT_RECV_HANDLER callback function configured by @ref aiot_ntp_setopt will be called to notify the user of the current time.
 **/
#ifndef __AIOT_NTP_API_H__
#define __AIOT_NTP_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x1100~-0x11FF expresses the status code of the SDK in the ntp Modules*/
#define STATE_NTP_BASE                                             (-0x1100)

/**
 * @brief MQTT session handle is not set, please set the MQTT session handle through @ref aiot_ntp_setopt*/
#define STATE_NTP_MISSING_MQTT_HANDLE                              (-0x1101)

/**
 * @brief When the ntp Modules receives a message from the network, it notifies the user of the message type*/
typedef enum {
    AIOT_NTPRECV_LOCAL_TIME
} aiot_ntp_recv_type_t;

/**
 * @brief When the ntp Modules receives a message from the network, it notifies the user of the message content*/
typedef struct {
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_ntp_recv_type_t*/
    aiot_ntp_recv_type_t  type;
    union {
        /**
         * @brief utc event stamp and time zone converted date, subject to the time zone set by @ref AIOT_NTPOPT_TIME_ZONE*/
        struct {
            uint64_t timestamp;
            uint32_t year;
            uint32_t mon;
            uint32_t day;
            uint32_t hour;
            uint32_t min;
            uint32_t sec;
            uint32_t msec;
        } local_time;
    } data;
} aiot_ntp_recv_t;

/**
 * @brief When the ntp Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle ntp session handle
 * @param[in] packet ntp message structure, which stores the content of the received ntp message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_ntp_recv_handler_t)(void *handle,
        const aiot_ntp_recv_t *packet, void *userdata);

/**
 * @brief ntp internal event type*/
typedef enum {
    /**
     * @brief The fields in the received NTP response are illegal*/
    AIOT_NTPEVT_INVALID_RESPONSE,
    /**
     * @brief The time field format in the received NTP response is incorrect*/
    AIOT_NTPEVT_INVALID_TIME_FORMAT,
} aiot_ntp_event_type_t;

/**
 * @brief NTP internal events*/
typedef struct {
    /**
     * @brief NTP internal event type. For more information, please refer to @ref aiot_ntp_event_type_t
     **/
    aiot_ntp_event_type_t type;
} aiot_ntp_event_t;

/**
 * @brief ntp event callback function
 *
 * @details
 *
 * This function is called when the NTP internal event is triggered
 **/
typedef void (*aiot_ntp_event_handler_t)(void *handle, const aiot_ntp_event_t *event, void *userdata);

/**
 * @brief @ref aiot_ntp_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_ntp_setopt
 *
 * 1. When the data type of data is char *, take configuring @ref AIOT_NTPOPT_MQTT_HANDLE as an example:
 *
 * void *mqtt_handle = aiot_mqtt_init();
 * aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_MQTT_HANDLE, mqtt_handle);
 *
 * 2. When the data type of data is other data types, take configuring @ref AIOT_NTPOPT_TIME_ZONE as an example:
 *
 * int8_t time_zone = 8;
 * aiot_mqtt_setopt(ntp_handle, AIOT_NTPOPT_TIME_ZONE, (void *)&time_zone);*/
typedef enum {
    /**
     * @brief ntp session requires an MQTT handle. You need to establish an MQTT connection first, and then set the MQTT handle.
     *
     * @details
     *
     * Data type: (void *)*/
    AIOT_NTPOPT_MQTT_HANDLE,

    /**
     * @brief ntp session After obtaining the utc time, it will be converted into local time according to the time zone value, and then notified through @ref aiot_ntp_recv_handler_t
     *
     * @details
     *
     * Value examples: East 8th district, the value is 8; West 3rd district, the value is -3
     *
     * Data type: (int8_t *)*/
    AIOT_NTPOPT_TIME_ZONE,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message to notify the user
     *
     * @details
     *
     * Data type: ( @ref aiot_ntp_recv_handler_t )*/
    AIOT_NTPOPT_RECV_HANDLER,

    /**
     * @brief Events occurring within ntp will be notified from this callback function
     *
     * @details
     *
     * Data type: (@ref aiot_ntp_event_handler_t)*/
    AIOT_NTPOPT_EVENT_HANDLER,

    /**
     * @brief User needs SDK temporary context
     *
     * @details This context pointer will be passed to the user by the SDK when the callbacks set by AIOT_NTPOPT_RECV_HANDLER and AIOT_NTPOPT_EVENT_HANDLER are called.
     *
     * Data type: (void *)*/
    AIOT_NTPOPT_USERDATA,

    /**
     * @brief When destroying the ntp instance, the time to wait for other APIs to complete execution
     *
     * @details
     *
     * When calling @ref aiot_ntp_deinit to destroy the NTP instance, if you continue to call other aiot_ntp_xxx API, the API will return @ref STATE_USER_INPUT_EXEC_DISABLED error
     *
     * At this point, users should stop calling other aiot_ntp_xxx APIs
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_NTPOPT_DEINIT_TIMEOUT_MS,
    AIOT_NTPOPT_MAX
} aiot_ntp_option_t;

/**
 * @brief Create an ntp session instance and configure session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL ntp instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_ntp_init(void);

/**
 * @brief configure ntp session
 *
 * @details
 *
 * Common configuration items are as follows
 *
 * + `AIOT_NTPOPT_MQTT_HANDLE`: MQTT session handle of the established connection
 *
 * + `AIOT_NTPOPT_TIME_ZONE`: Time zone setting, SDK will convert the received UTC time according to the configured time zone
 *
 * + `AIOT_NTPOPT_RECV_HANDLER`: time data receiving callback function, after the SDK completes UTC time conversion, output through this callback function
 *
 * @param[in] handle ntp session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_ntp_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_ntp_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval >=STATE_SUCCESS Parameter configuration successful
 **/
int32_t aiot_ntp_setopt(void *handle, aiot_ntp_option_t option, void *data);

/**
 * @brief End the ntp session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to ntp session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_ntp_deinit(void **handle);

/**
 * @brief Send ntp message request to ntp server
 *
 * @details
 *
 * Send an NTP request, and then the SDK will call the @ref AIOT_NTPOPT_RECV_HANDLER callback function configured through @ref aiot_ntp_setopt to notify the user of the current time
 *
 * @param handle ntp session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS request failed to send
 * @retval >=STATE_SUCCESS The request was sent successfully*/
int32_t aiot_ntp_send_request(void *handle);

#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_NTP_API_H__ */

