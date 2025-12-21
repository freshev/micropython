/**
 * @file aiot_dynreg_api.h
 * @brief dynreg Modules header file, providing the ability to obtain device information
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __AIOT_DYNREG_API_H__
#define __AIOT_DYNREG_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x0600~-0x06FF expresses the status code of the SDK in the dynreg Modules*/
#define STATE_DYNREG_BASE                                             (-0x0600)

/**
 * @brief When executing @ref aiot_dynreg_deinit, the waiting time for other API execution to end exceeds the set timeout, and the MQTT instance fails to be destroyed.*/
#define STATE_DYNREG_DEINIT_TIMEOUT                                   (-0x0601)

/**
 * @brief needs to execute @ref aiot_dynreg_send_request first to send a dynreg request*/
#define STATE_DYNREG_NEED_SEND_REQUEST                                (-0x0602)

/**
 * @brief dynreg Modules returned wrong http status code*/
#define STATE_DYNREG_INVALID_STATUS_CODE                              (-0x0603)

/**
 * @brief received an illegal device secret*/
#define STATE_DYNREG_INVALID_DEVICE_SECRET                            (-0x0604)

/**
 * @brief dynreg Modules notifies the user of the message type when receiving a message from the network*/
typedef enum {
    /**
     * http status code returned by @brief dynreg Modules*/
    AIOT_DYNREGRECV_STATUS_CODE,
    /**
     * Device information returned by @brief dynreg Modules*/
    AIOT_DYNREGRECV_DEVICE_INFO,
} aiot_dynreg_recv_type_t;

/**
 * When the @brief dynreg Modules receives a message from the network, it notifies the user of the message content*/
typedef struct {
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_dynreg_recv_type_t*/
    aiot_dynreg_recv_type_t  type;
    union {
        /**
         * http status code returned by @brief dynreg Modules*/
        struct {
            uint32_t code;
        } status_code;
        /**
         * Device information returned by @brief dynreg Modules*/
        struct {
            char *device_secret;
        } device_info;
    } data;
} aiot_dynreg_recv_t;

/**
 * When the @brief dynreg Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle dynreg session handle
 * @param[in] packet dynreg message structure, which stores the content of the received dynreg message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_dynreg_recv_handler_t)(void *handle,
        const aiot_dynreg_recv_t *packet, void *userdata);

/**
 * @brief @ref aiot_dynreg_setopt The optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_dynreg_setopt
 *
 * 1. When the data type of data is char *, take configuring @ref AIOT_DYNREGOPT_HOST as an example:
 *
 * char *host = "xxx";
 * aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_HOST, host);
 *
 * 2. When the data type of data is other data types, take configuring @ref AIOT_DYNREGOPT_PORT as an example:
 *
 * uint16_t port = 443;
 * aiot_mqtt_setopt(dynreg_handle, AIOT_DYNREGOPT_PORT, (void *)&port);*/
typedef enum {
    /**
     * @brief http dynamic registration The security credentials used by the network when establishing a connection with the server
     *
     * @details
     *
     * This configuration item is used to configure @ref aiot_sysdep_network_cred_t security credential data for the underlying network
     *
     * The option in @ref aiot_sysdep_network_cred_t should be configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA, and the connection should be established in tls mode
     *
     * Data type: (aiot_sysdep_network_cred_t *)*/
    AIOT_DYNREGOPT_NETWORK_CRED,

    /**
     * @brief http dynamic registration server domain name address or IP address
     *
     * @details
     *
     * Alibaba Cloud Internet of Things Platform http dynamic registration server domain name address list:
     *
     * | Domain name address | Region | Port number
     * |------------------------------------------------ -|---------|---------
     * | iot-auth.cn-shanghai.aliyuncs.com | Domestic | 443
     * | iot-auth.ap-southeast-1.aliyuncs.com | Overseas | 443
     *
     * Data type: (char *)*/
    AIOT_DYNREGOPT_HOST,

    /**
     * @brief http dynamic registration server port number
     *
     * @details
     *
     * When connecting to the Alibaba Cloud IoT platform http dynamic registration server:
     *
     * Must use tls method to establish the connection, the port number is set to 443
     *
     * Data type: (uint16_t *)*/
    AIOT_DYNREGOPT_PORT,

    /**
     * @brief The productKey of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_DYNREGOPT_PRODUCT_KEY,

    /**
     * @brief The productSecret of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_DYNREGOPT_PRODUCT_SECRET,

    /**
     * @brief The deviceName of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_DYNREGOPT_DEVICE_NAME,

    /**
     * @brief dynreg The maximum time interval that a session can consume when sending messages
     *
     * @details
     *
     * Data type: (uint32_t) Default value: (5 * 1000) ms*/
    AIOT_DYNREGOPT_SEND_TIMEOUT_MS,

    /**
     * @brief dynreg The maximum time interval that the session can consume when receiving messages
     *
     * @details
     *
     * Data type: (uint32_t) Default value: (5 * 1000) ms*/
    AIOT_DYNREGOPT_RECV_TIMEOUT_MS,

    /**
     * @brief sets the callback, which is called when the SDK receives a network message to notify the user
     *
     * @details
     *
     * Data type: (aiot_dynreg_http_recv_handler_t)*/
    AIOT_DYNREGOPT_RECV_HANDLER,

    /**
     * @brief User needs SDK temporary context
     *
     * @details
     *
     * This context pointer will be passed to the user by the SDK when the callbacks set by AIOT_DYNREGOPT_RECV_HANDLER and AIOT_DYNREGOPT_EVENT_HANDLER are called.
     *
     * Data type: (void *)*/
    AIOT_DYNREGOPT_USERDATA,

    /**
     * @brief dynreg Modules timeout for receiving messages
     *
     * @details
     *
     * Data type: (uint32_t) Default value: (5 * 1000) ms*/
    AIOT_DYNREGOPT_TIMEOUT_MS,

    /**
     * @brief When destroying a dynreg instance, the time to wait for other APIs to complete execution
     *
     * @details
     *
     * When calling @ref aiot_dynreg_deinit to destroy the MQTT instance, if you continue to call other aiot_dynreg_xxx API, the API will return @ref STATE_USER_INPUT_EXEC_DISABLED error
     *
     * At this point, users should stop calling other aiot_dynreg_xxx APIs
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_DYNREGOPT_DEINIT_TIMEOUT_MS,
    AIOT_DYNREGOPT_MAX
} aiot_dynreg_option_t;

/**
 * @brief Create a dynreg session instance and configure session parameters with default values
 *
 * @return void *
 * @retval handle to non-NULL dynreg instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_dynreg_init(void);

/**
 * @brief configure dynreg session
 *
 * @param[in] handle dynreg session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_dynreg_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_dynreg_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval >=STATE_SUCCESS Parameter configuration successful
 **/
int32_t aiot_dynreg_setopt(void *handle, aiot_dynreg_option_t option, void *data);

/**
 * @brief End the dynreg session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to dynreg session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_dynreg_deinit(void **handle);

/**
 * @brief Send dynreg message request to dynreg server
 *
 * @param handle dynreg session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS request failed to send
 * @retval >=STATE_SUCCESS The request was sent successfully*/
int32_t aiot_dynreg_send_request(void *handle);

/**
 * @brief Receive dynreg messages from the network
 *
 * @param handle dynreg session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS data reception failed
 * @retval >=STATE_SUCCESS data received successfully*/
int32_t aiot_dynreg_recv(void *handle);

#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_DYNREG_API_H__ */

