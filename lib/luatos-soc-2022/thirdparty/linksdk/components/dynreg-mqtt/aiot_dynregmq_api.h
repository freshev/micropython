/**
 * @file aiot_dynregmq_api.h
 * @brief dynregmq Modules header file, provides dynamic registration capability of device information based on MQTT
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __AIOT_DYNREGMQ_API_H__
#define __AIOT_DYNREGMQ_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x0500~-0x05FF expresses the status code of the SDK in the dynregmq Modules*/
#define STATE_DYNREGMQ_BASE                                             (-0x0500)

/**
 * @brief When executing @ref aiot_dynregmq_deinit, the waiting time for the execution of other APIs to end exceeds the set timeout, and the DYNREGMQ instance fails to be destroyed.*/
#define STATE_DYNREGMQ_DEINIT_TIMEOUT                                   (-0x0501)

/**
 * @brief needs to execute @ref aiot_dynregmq_send_request first to send a dynamic registration request*/
#define STATE_DYNREGMQ_NEED_SEND_REQUEST                                (-0x0502)

/**
 * @brief Receiving server response timeout*/
#define STATE_DYNREGMQ_AUTH_TIMEOUT                                     (-0x0503)

/**
 * @brief @ref aiot_dynregmq_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_dynregmq_setopt
 *
 * 1. When the data type of data is char *, take configuring @ref AIOT_DYNREGMQOPT_HOST as an example:
 *
 * char *host = "xxx";
 * aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_HOST, host);
 *
 * 2. When the data type of data is other data types, take configuring @ref AIOT_DYNREGMQOPT_PORT as an example:
 *
 * uint16_t port = 443;
 * aiot_mqtt_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PORT, (void *)&port);*/
typedef enum {
    /**
     * @brief mqtt dynamic registration When establishing a connection with the server, the security credentials used by the network. Dynamic registration must use TLS to establish a connection.
     *
     * @details
     *
     * This configuration item is used to configure @ref aiot_sysdep_network_cred_t security credential data for the underlying network
     *
     * The option in @ref aiot_sysdep_network_cred_t should be configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA, and the connection should be established in tls mode
     *
     * Data type: (aiot_sysdep_network_cred_t *)*/
    AIOT_DYNREGMQOPT_NETWORK_CRED,

    /**
     * @brief mqtt dynamic registration server domain name address or IP address
     *
     * @details
     *
     * Alibaba Cloud IoT platform domain name address list (you must replace ${pk} with your own product key):
     *
     * Use tls certificate to establish connection:
     *
     * | Domain name address | Region | Port number
     * |------------------------------------------------ -|---------|---------
     * | ${pk}.iot-as-mqtt.cn-shanghai.aliyuncs.com | Shanghai | 443
     * | ${pk}.iot-as-mqtt.ap-southeast-1.aliyuncs.com | Singapore | 443
     * | ${pk}.iot-as-mqtt.ap-northeast-1.aliyuncs.com | Japan | 443
     * | ${pk}.iot-as-mqtt.us-west-1.aliyuncs.com | US West | 443
     * | ${pk}.iot-as-mqtt.eu-central-1.aliyuncs.com | Germany | 443
     *
     * Data type: (char *)*/
    AIOT_DYNREGMQOPT_HOST,

    /**
     * @brief mqtt dynamic registration server port number
     *
     * @details
     *
     * When connecting to the Alibaba Cloud IoT platform mqtt dynamic registration server:
     *
     * Must use tls method to establish the connection, the port number is set to 443
     *
     * Data type: (uint16_t *)*/
    AIOT_DYNREGMQOPT_PORT,

    /**
     * @brief The productKey of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_DYNREGMQOPT_PRODUCT_KEY,

    /**
     * @brief The productSecret of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_DYNREGMQOPT_PRODUCT_SECRET,

    /**
     * @brief The deviceName of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_DYNREGMQOPT_DEVICE_NAME,

    /**
     * @brief The maximum time interval that the dynregmq session can consume when sending messages
     *
     * @details
     *
     * Data type: (uint32_t) Default value: (5 * 1000) ms*/
    AIOT_DYNREGMQOPT_SEND_TIMEOUT_MS,

    /**
     * @brief The maximum time interval that the dynregmq session can consume when receiving messages
     *
     * @details
     *
     * Data type: (uint32_t) Default value: (5 * 1000) ms*/
    AIOT_DYNREGMQOPT_RECV_TIMEOUT_MS,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message to notify the user
     *
     * @details
     *
     * Data type: (aiot_dynregmq_http_recv_handler_t)*/
    AIOT_DYNREGMQOPT_RECV_HANDLER,

    /**
     * @brief User needs SDK temporary context
     *
     * @details
     *
     * This context pointer will be passed to the user by the SDK when the callbacks set by AIOT_DYNREGMQOPT_RECV_HANDLER and AIOT_DYNREGMQOPT_EVENT_HANDLER are called.
     *
     * Data type: (void *)*/
    AIOT_DYNREGMQOPT_USERDATA,

    /**
     * @brief dynregmq Modules timeout for receiving messages
     *
     * @details
     *
     * Data type: (uint32_t) Default value: (5 * 1000) ms*/
    AIOT_DYNREGMQOPT_TIMEOUT_MS,

    /**
     * @brief When destroying a dynregmq instance, wait for the completion of other API executions
     *
     * @details
     *
     * When calling @ref aiot_dynregmq_deinit to destroy the MQTT instance, if you continue to call other aiot_dynregmq_xxx API, the API will return @ref STATE_USER_INPUT_EXEC_DISABLED error
     *
     * At this time, users should stop calling other aiot_dynregmq_xxx APIs
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_DYNREGMQOPT_DEINIT_TIMEOUT_MS,

    /**
     * @brief Whether to use the whitelist-free function
     *
     * @details
     *
     * 1. If configured to 0, it is whitelist mode. To use this mode, users must enter the deviceName in the console in advance. After the dynamic registration is completed, the service will return deviceSecret. Users can pass
     * AIOT_DYNREGMQRECV_DEVICEINFO_WL type data callback obtains deviceSecret.
     * 2. If configured to 1, it is a whitelist-free mode. In this mode, users do not need to enter the deviceName in the console in advance. After the dynamic registration is completed, the service will return MQTT connection establishment information. Users can
     * AIOT_DYNREGMQRECV_DEVICEINFO_NWL type data callback obtains clientid, username, password. The user needs to pass these three parameters
     * The aiot_mqtt_setopt interface uses AIOT_MQTTOPT_CLIENTID, AIOT_MQTTOPT_USERNAME, AIOT_MQTTOPT_PASSWORD configuration options
     * Configured into the MQTT handle.
     *
     * Data type: (uint8_t *) Default value: (0)
     **/
    AIOT_DYNREGMQOPT_NO_WHITELIST,

    /**
     * @brief The IoT platform instance ID purchased by the user. When the user uses a self-purchased instance and uses the whitelist-free method, the instance ID must be set
     *
     * @details
     *
     * Data type: (char *)
     **/
    AIOT_DYNREGMQOPT_INSTANCE_ID,

    AIOT_DYNREGMQOPT_MAX
} aiot_dynregmq_option_t;


/**
 * @brief dynregmq Modules notifies the user of the message type when receiving a message from the network*/
typedef enum {
    /**
     * @brief Device information returned by the server in whitelist mode*/
    AIOT_DYNREGMQRECV_DEVICEINFO_WL,

    /**
     * @brief Device information returned by the server in whitelist-free mode*/
    AIOT_DYNREGMQRECV_DEVICEINFO_NWL,
} aiot_dynregmq_recv_type_t;

/**
 * @brief dynregmq Modules notifies the user of the message content when it receives a message from the network*/
typedef struct {
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_dynregmq_recv_type_t*/
    aiot_dynregmq_recv_type_t  type;
    union {
        /**
         * @brief Device information returned by the server in whitelist mode*/
        struct {
            char *device_secret;
        } deviceinfo_wl;

        /**
         * @brief Device information returned by the server in whitelist-free mode*/
        struct {
            char *clientid;
            char *username;
            char *password;
        } deviceinfo_nwl;
    } data;
} aiot_dynregmq_recv_t;

/**
 * @brief When the dynregmq Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle dynregmq session handle
 * @param[in] packet dynregmq message structure, which stores the content of the received dynregmq message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_dynregmq_recv_handler_t)(void *handle,
        const aiot_dynregmq_recv_t *packet, void *userdata);


/**
 * @brief Create a dynregmq session instance and configure session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL dynregmq instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_dynregmq_init(void);

/**
 * @brief configure dynregmq session
 *
 * @param[in] handle dynregmq session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_dynregmq_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_dynregmq_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval >=STATE_SUCCESS Parameter configuration successful
 **/
int32_t aiot_dynregmq_setopt(void *handle, aiot_dynregmq_option_t option, void *data);

/**
 * @brief End the dynregmq session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to dynregmq session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_dynregmq_deinit(void **handle);

/**
 * @brief Send dynregmq message request to dynregmq server
 *
 * @param handle dynregmq session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS request failed to send
 * @retval >=STATE_SUCCESS The request was sent successfully*/
int32_t aiot_dynregmq_send_request(void *handle);

/**
 * @brief Receive dynregmq messages from the network
 *
 * @param handle dynregmq session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS data reception failed
 * @retval >=STATE_SUCCESS data received successfully*/
int32_t aiot_dynregmq_recv(void *handle);

#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_DYNREGMQMQ_API_H__ */

