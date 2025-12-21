/**
 * @file aiot_mqtt_api.h
 * @brief MQTT Modules header file, providing the ability to connect to the Alibaba Cloud IoT platform using the MQTT protocol
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 * @details
 *
 * The MQTT Modules is used to establish a connection with the Alibaba Cloud IoT platform. The API usage process is as follows:
 *
 * 1. Call @ref aiot_mqtt_init to initialize the MQTT session and obtain the session handle
 *
 * 2. Call @ref aiot_mqtt_setopt to configure the parameters of the MQTT session. For common configuration items, see the description of @ref aiot_mqtt_setopt
 *
 * 3. Call @ref aiot_mqtt_connect to establish a connection with the Alibaba Cloud IoT platform
 *
 * 4. Start a thread, and call @ref aiot_mqtt_process intermittently in the thread to process heartbeat and QoS1 messages
 *
 * 5. Start a thread, and continuously call @ref aiot_mqtt_recv in the thread to receive MQTT messages on the network
 *
 * + When receiving a message, check the parameters of the current MQTT session in the following order. When a certain description is met, it will be notified through the corresponding callback function and stop checking.
 *
 * + Check whether the received message topic has passed the @ref AIOT_MQTTOPT_APPEND_TOPIC_MAP parameter configuration callback function of @ref aiot_mqtt_setopt
 *
 * + Check whether the received message topic has been configured with a callback function through @ref aiot_mqtt_sub API
 *
 * + Check whether the default callback function is configured via the @ref AIOT_MQTTOPT_RECV_HANDLER parameter of @ref aiot_mqtt_setopt
 *
 * 6. After the above steps, the MQTT connection has been established and can maintain the connection with the IoT platform. Next, use APIs such as @ref aiot_mqtt_sub and @ref aiot_mqtt_pub to implement business logic according to your own scenario.
 **/

#ifndef _AIOT_MQTT_API_H_
#define _AIOT_MQTT_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief MQTT message type
 *
 * @details
 *
 * The MQTT message type passed in @ref aiot_mqtt_recv_handler_t*/
typedef enum {
    /**
     * @brief MQTT PUBLISH message*/
    AIOT_MQTTRECV_PUB,

    /**
     * @brief MQTT PINGRESP message*/
    AIOT_MQTTRECV_HEARTBEAT_RESPONSE,

    /**
     * @brief MQTT SUBACK message*/
    AIOT_MQTTRECV_SUB_ACK,

    /**
     * @brief MQTT UNSUB message*/
    AIOT_MQTTRECV_UNSUB_ACK,

    /**
     * @brief MQTT PUBACK message*/
    AIOT_MQTTRECV_PUB_ACK,

} aiot_mqtt_recv_type_t;

typedef struct {
    /**
     * @brief MQTT message type, for more information, please refer to @ref aiot_mqtt_recv_type_t*/
    aiot_mqtt_recv_type_t type;
    /**
     * @brief MQTT message union, content is selected according to type*/
    union {
        /**
         * @brief MQTT PUBLISH message*/
        struct {
            uint8_t qos;
            char *topic;
            uint16_t topic_len;
            uint8_t *payload;
            uint32_t payload_len;
        } pub;
        /**
         * @brief AIOT_MQTTRECV_SUB_ACK
         */
        struct {
            int32_t res;
            uint8_t max_qos;
            uint16_t packet_id;
        } sub_ack;
        /**
         * @brief AIOT_MQTTRECV_UNSUB_ACK
         */
        struct {
            uint16_t packet_id;
        } unsub_ack;
        /**
         * @brief AIOT_MQTTRECV_PUB_ACK
         */
        struct {
            uint16_t packet_id;
        } pub_ack;
    } data;
} aiot_mqtt_recv_t;

/**
 * @brief MQTT message receiving callback function prototype
 *
 * @param[in] handle MQTT instance handle
 * @param[in] packet MQTT message structure, which stores the received MQTT message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (*aiot_mqtt_recv_handler_t)(void *handle, const aiot_mqtt_recv_t *packet, void *userdata);

/**
 * @brief MQTT internal event type*/
typedef enum {
    /**
     * @brief This event is triggered when the MQTT instance successfully connects to the network for the first time.*/
    AIOT_MQTTEVT_CONNECT,
    /**
     * @brief This event is triggered when the MQTT instance disconnects from the network and then reconnects successfully.*/
    AIOT_MQTTEVT_RECONNECT,
    /**
     * @brief This event is triggered when the MQTT instance disconnects from the network*/
    AIOT_MQTTEVT_DISCONNECT
} aiot_mqtt_event_type_t;

typedef enum {
    /**
     * @brief MQTT instance network connection was disconnected due to network failure*/
    AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT,
    /**
     * @brief The MQTT instance network connection was disconnected due to heartbeat loss exceeding the specified number of times (@ref AIOT_MQTTOPT_HEARTBEAT_MAX_LOST)*/
    AIOT_MQTTDISCONNEVT_HEARTBEAT_DISCONNECT
} aiot_mqtt_disconnect_event_type_t;

/**
 * @brief MQTT internal events*/
typedef struct {
    /**
     * @brief MQTT internal event type. For more information, please refer to @ref aiot_mqtt_event_type_t
     **/
    aiot_mqtt_event_type_t type;
    /**
     * @brief MQTT event data union*/
    union {
        /**
         * @brief When the MQTT connection is disconnected, the specific reason for the disconnection*/
        aiot_mqtt_disconnect_event_type_t disconnect;
    } data;
} aiot_mqtt_event_t;

/**
 * @brief MQTT event callback function
 *
 * @details
 *
 * This function is called when MQTT internal events are triggered. Such as successful connection/disconnection/reconnection success
 **/
typedef void (*aiot_mqtt_event_handler_t)(void *handle, const aiot_mqtt_event_t *event, void *userdata);

/**
 * @brief Data when using @ref aiot_mqtt_setopt to configure @ref AIOT_MQTTOPT_APPEND_TOPIC_MAP
 *
 * @details
 *
 * Used to configure topic and corresponding callback function before establishing MQTT connection
 **/
typedef struct {
    char *topic;
    aiot_mqtt_recv_handler_t handler;
    void *userdata;
} aiot_mqtt_topic_map_t;

/**
 * @brief @ref aiot_mqtt_setopt function option parameter. For the data type in each option below, it refers to the data type of the data parameter in @ref aiot_mqtt_setopt
 *
 * @details
 *
 * 1. When the data type of data is char *, take configuring @ref AIOT_MQTTOPT_HOST as an example:
 *
 * char *host = "xxx";
 *
 * aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, host);
 *
 * 2. When the data type of data is other data types, take configuring @ref AIOT_MQTTOPT_PORT as an example:
 *
 * uint16_t port = 443;
 *
 * aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);*/
typedef enum {
    /**
     * @brief MQTT server domain name address or IP address
     *
     * @details
     *
     * Alibaba Cloud IoT platform domain name address list (you must replace ${pk} with your own product key):
     *
     * Use tcp or tls certificate to establish connection:
     *
     * | Domain name address | Region | Port number
     * |------------------------------------------------ -|---------|---------
     * | ${pk}.iot-as-mqtt.cn-shanghai.aliyuncs.com | Shanghai | 443
     * | ${pk}.iot-as-mqtt.ap-southeast-1.aliyuncs.com | Singapore | 443
     * | ${pk}.iot-as-mqtt.ap-northeast-1.aliyuncs.com | Japan | 443
     * | ${pk}.iot-as-mqtt.us-west-1.aliyuncs.com | US West | 443
     * | ${pk}.iot-as-mqtt.eu-central-1.aliyuncs.com | Germany | 443
     *
     * Use tls psk method to establish connection:
     *
     * | Domain name address | Region | Port number
     * |-----------------------------------------|------ ---|---------
     * | ${pk}.itls.cn-shanghai.aliyuncs.com | Shanghai | 1883
     *
     * Use tls x509 client certificate to establish connection:
     *
     * | Domain name address | Region | Port number
     * |-------------------------------------|---------| ----------
     * | x509.itls.cn-shanghai.aliyuncs.com | Shanghai | 1883
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_HOST,

    /**
     * @brief MQTT server port number
     *
     * @details
     *
     * When connecting to Alibaba Cloud IoT platform:
     *
     * 1. If you are using tcp or tls certificate method, the port number is set to 443
     *
     * 2. If you are using tls psk and tls x509 client certificate methods, the port number is set to 1883
     *
     * Data type: (uint16_t *)*/
    AIOT_MQTTOPT_PORT,

    /**
     * @brief The product key of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_PRODUCT_KEY,

    /**
     * @brief The device name of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_DEVICE_NAME,

    /**
     * @brief The device secret of the device can be obtained from <a href="http://iot.console.aliyun.com/">Alibaba Cloud IoT Platform Console</a>
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_DEVICE_SECRET,

    /**
     * @brief Extended clientid when the device connects to the Alibaba Cloud IoT platform
     *
     * @details
     *
     * If you need to report the Modules vendor ID, Modules ID and os information, fill in the following format:
     *
     * "mid=<Modules ID>,pid=<Modules vendor ID>,os=<operating system>"
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_EXTEND_CLIENTID,

    /**
     * @brief The security mode when the device connects to the Alibaba Cloud IoT platform. No configuration is required when using standard tcp or tls.
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_SECURITY_MODE,

    /**
     * @brief When using custom connection credentials to connect to the mqtt server, the username of the credentials
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_USERNAME,

    /**
     * @brief When using custom connection credentials to connect to the mqtt server, the password of the credentials
     *
     * @brief
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_PASSWORD,

    /**
     * @brief When using custom connection credentials to connect to the mqtt server, the clientid of the credentials
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_MQTTOPT_CLIENTID,

    /**
     * @brief When MQTT establishes a connection, the heartbeat interval parameter in the CONNECT message
     *
     * @details
     *
     * Limited by Alibaba Cloud IoT platform, the value range is 30 ~ 1200s
     *
     * 1. If the set value is less than 30, mqtt connection establishment will be rejected by the cloud, and the @ref aiot_mqtt_connect function will return @ref STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE error
     *
     * 2. If the set value is greater than 1200, the mqtt connection can still be established, but this parameter will be overwritten by the server to 1200
     *
     * Data type: (uint16_t *) Value range: 30 ~ 1200s Default value: 1200s*/
    AIOT_MQTTOPT_KEEPALIVE_SEC,

    /**
     * @brief When MQTT establishes a connection, the clean session parameter in the CONNECT message
     *
     * @details
     *
     * 1. If the clean session is 0 when the device goes online, the QoS1 message pushed by the server before going online will be pushed to the device at this time
     *
     * 2. If the clean session is 1 when the device goes online, the QoS1 messages pushed by the server before going online will be discarded.
     *
     * Data type: (uint8_t *) Value range: 0, 1 Default value: 1*/
    AIOT_MQTTOPT_CLEAN_SESSION,

    /**
     * @brief The security credentials used by the network when establishing MQTT connection
     *
     * @details
     *
     * This configuration item is used to configure @ref aiot_sysdep_network_cred_t security credential data for the underlying network
     *
     * 1. If this option is not configured, MQTT will establish the connection directly through tcp.
     *
     * 2. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_NONE, MQTT will establish the connection directly in tcp mode
     *
     * 3. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA, MQTT will establish the connection in tls mode
     *
     * 4. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK, MQTT will establish the connection in tls psk mode
     *
     * Data type: (aiot_sysdep_network_cred_t *)*/
    AIOT_MQTTOPT_NETWORK_CRED,

    /**
     * @brief When MQTT establishes a connection, the timeout period for establishing a network connection
     *
     * @details
     *
     * refers to the timeout period for establishing a socket connection
     *
     * Data type: (uint32_t *) Default value: (5 *1000) ms
     **/
    AIOT_MQTTOPT_CONNECT_TIMEOUT_MS,

    /**
     * @brief Configure the MQTT PINGREQ message sending interval. (Heartbeat sending interval)
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (25 * 1000) ms*/
    AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS,

    /**
     * @brief Configure the maximum number of consecutive losses allowed for MQTT PINGRESP messages. When this number is exceeded, the reconnection mechanism is triggered.
     *
     * @details
     *
     * Data type: (uint8_t *) Default value: (2)*/
    AIOT_MQTTOPT_HEARTBEAT_MAX_LOST,

    /**
     * @brief Open/close MQTT reconnection mechanism
     *
     * @details
     *
     * Data type: (uint8_t *) Value range: 0, 1 Default value: 1*/
    AIOT_MQTTOPT_RECONN_ENABLED,

    /**
     * @brief When the reconnection mechanism is triggered due to heartbeat loss or network disconnection, the time interval for reconnection attempts
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_MQTTOPT_RECONN_INTERVAL_MS,

    /**
     * @brief When MQTT sends data, the longest time it takes in the protocol stack
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (5 * 1000) ms*/
    AIOT_MQTTOPT_SEND_TIMEOUT_MS,

    /**
     * @brief When MQTT receives data, the longest time it takes in the protocol stack
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (5 * 1000) ms*/
    AIOT_MQTTOPT_RECV_TIMEOUT_MS,

    /**
     * @brief QoS1 message retransmission interval
     *
     * @details
     *
     * After sending the qos1 MQTT PUBLISH message, if the mqtt PUBACK message is not received within @ref AIOT_MQTTOPT_REPUB_TIMEOUT_MS time,
     * @ref aiot_mqtt_process will resend this qo1 MQTT PUBLISH message until the PUBACK message is received
     *
     * Data type: (uint32_t *) Default value: (3 * 1000) ms*/
    AIOT_MQTTOPT_REPUB_TIMEOUT_MS,

    /**
     * @brief When destroying the MQTT instance, wait for other APIs to complete execution
     *
     * @details
     *
     * When calling @ref aiot_mqtt_deinit to destroy the MQTT instance, if you continue to call other aiot_mqtt_xxx API, the API will return @ref STATE_USER_INPUT_EXEC_DISABLED error
     *
     * At this time, users should stop calling other aiot_mqtt_xxx APIs
     *
     * Data type: (uint32_t *) Default value: (2 * 1000) ms*/
    AIOT_MQTTOPT_DEINIT_TIMEOUT_MS,

    /**
     * @brief The data received from the MQTT server is notified from this default callback function
     *
     * @details
     *
     * 1. If this callback function is not configured, when a message arrives but the corresponding registered topic cannot be found, the message will be discarded.
     *
     * 2. If this callback function has been configured, when a message arrives but the corresponding registered topic cannot be found, the message will be notified through the default callback function.
     *
     * Data type: (@ref aiot_mqtt_recv_handler_t)*/
    AIOT_MQTTOPT_RECV_HANDLER,

    /**
     * @brief Events that occur within the MQTT client will be notified from this callback function, such as online/disconnected/re-online
     *
     * @details
     *
     * Data type: (@ref aiot_mqtt_event_handler_t)*/
    AIOT_MQTTOPT_EVENT_HANDLER,

    /**
     * @brief You can configure the MQTT topic and its corresponding callback function before MQTT establishes a connection.
     *
     * @details
     *
     * Data type: (@ref aiot_mqtt_topic_map_t)*/
    AIOT_MQTTOPT_APPEND_TOPIC_MAP,

    /**
     * @brief Cancel the corresponding relationship between the previously established MQTT topic and its callback function
     *
     * @details
     *
     * Data type: (@ref aiot_mqtt_topic_map_t)*/
    AIOT_MQTTOPT_REMOVE_TOPIC_MAP,

    /**
     * @brief appends the request ID string to the topic of the publish message for full-link log tracking
     *
     * @details
     *
     * Data type: (uint8_t *) Default value: 0
     *
     * If configured to 0, the request ID string will not be appended. If configured to 1, the request ID string will be appended.*/
    AIOT_MQTTOPT_APPEND_REQUESTID,

    /**
     * @brief User needs SDK temporary context
     *
     * @details
     *
     * 1. The context will be passed back to the user in @ref AIOT_MQTTOPT_RECV_HANDLER and @ref AIOT_MQTTOPT_EVENT_HANDLER
     *
     * 2. When using @ref AIOT_MQTTOPT_APPEND_TOPIC_MAP or @ref aiot_mqtt_sub without specifying userdata, the context will also be passed to these callback functions.
     *
     * Data type: (void *)*/
    AIOT_MQTTOPT_USERDATA,

    /**
    * @brief The maximum number of QOS 1 messages cached by the SDK
    *
    * @details
    *
    * After sending a qos1 MQTT PUBLISH message, the message will be added to a retransmission list. After receiving the pub ack message, the message will be deleted from the list.
    * This list needs to set a maximum value, otherwise the list may continue to expand under weak network conditions until the memory overhead is exhausted.
    *
    * Data type: (uint16_t *) Default value: 20*/
    AIOT_MQTTOPT_MAX_REPUB_NUM,

    AIOT_MQTTOPT_MAX
} aiot_mqtt_option_t;

/**
 * @brief Initialize mqtt instance and set default parameters
 *
 * @return void*
 * @retval non-NULL MQTT instance handle
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void   *aiot_mqtt_init(void);

/**
 * @brief Set mqtt parameters
 *
 * @details
 *
 * Commonly used configuration options are listed below. At least the following options need to be configured to use the basic functions of MQTT.
 *
 * The remaining configuration options have default values   and can be adjusted according to business needs.
 *
 * + `AIOT_MQTTOPT_HOST`: Configure the connected Alibaba Cloud MQTT site address
 *
 * + `AIOT_MQTTOPT_PORT`: Configure the connected Alibaba Cloud MQTT site port number
 *
 * + `AIOT_MQTTOPT_PRODUCT_KEY`: configure the productKey of the device
 *
 * + `AIOT_MQTTOPT_DEVICE_NAME`: deviceName of the configured device
 *
 * + `AIOT_MQTTOPT_DEVICE_SECRET`: configure the deviceSecret of the device
 *
 * + `AIOT_MQTTOPT_NETWORK_CRED`: Configure security credentials when establishing MQTT connections
 *
 * + `AIOT_MQTTOPT_RECV_HANDLER`: configure the default data receiving callback function
 *
 * + `AIOT_MQTTOPT_EVENT_HANDLER`: configure MQTT event notification callback function
 *
 * @param[in] handle mqtt handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_mqtt_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_mqtt_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter setting failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS Parameter setting successful
 **/
int32_t aiot_mqtt_setopt(void *handle, aiot_mqtt_option_t option, void *data);

/**
 * @brief Release the resources of the mqtt instance handle
 *
 * @param[in] handle pointer to mqtt instance handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_mqtt_deinit(void **handle);

/**
 * @brief Establish a connection with the MQTT server
 *
 * @details
 *
 * Use the mqtt connection parameters configured by @ref aiot_mqtt_setopt to connect to the mqtt server. The connection establishment parameters used are selected in the following order
 *
 * 1. If the following options are configured, directly use the configured connection parameters to connect to any MQTT server specified by the @ref AIOT_MQTTOPT_HOST option
 *
 * + @ref AIOT_MQTTOPT_USERNAME
 * + @ref AIOT_MQTTOPT_PASSWORD
 * + @ref AIOT_MQTTOPT_CLIENTID
 *
 * 2. If the following options are configured, it is forced to use the signature algorithm of the Alibaba Cloud platform to calculate the connection parameters as the username/password of MQTT to connect to the Alibaba Cloud platform.
 *
 * + @ref AIOT_MQTTOPT_PRODUCT_KEY
 * + @ref AIOT_MQTTOPT_DEVICE_NAME
 * + @ref AIOT_MQTTOPT_DEVICE_SECRET
 *
 * @param[in] handle MQTT instance handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful
 *
 * @note
 *
 * When configuring @ref AIOT_MQTTOPT_USERNAME, @ref AIOT_MQTTOPT_PASSWORD and @ref AIOT_MQTTOPT_CLIENTID to configure custom connection credentials,
 *
 * This function ignores @ref AIOT_MQTTOPT_PRODUCT_KEY, @ref AIOT_MQTTOPT_DEVICE_NAME and @ref AIOT_MQTTOPT_DEVICE_SECRET,
 *
 * Directly use custom credentials to connect to the specified MQTT server*/
int32_t aiot_mqtt_connect(void *handle);

/**
 * @brief Disconnect from MQTT server
 *
 * @details
 *
 * Send an MQTT DISCONNECT message to the MQTT server, and then disconnect from the network
 *
 * If you need to establish a connection with the MQTT server again, just call @ref aiot_mqtt_connect
 *
 * @param[in] handle MQTT instance handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful*/
int32_t aiot_mqtt_disconnect(void *handle);

/**
 * @brief Send MQTT PINGREQ message to maintain heartbeat
 *
 * @details
 *
 * @ref aiot_mqtt_process includes a mechanism for regularly sending heartbeat messages. If there are special needs, you can use this function to directly send heartbeat messages.
 *
 * @param[in] handle MQTT instance handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful*/
int32_t aiot_mqtt_heartbeat(void *handle);

/**
 * @brief This function is used to handle scheduled heartbeat sending and retransmission logic of qos1 messages
 *
 * @details
 *
 * 1. Send heartbeat to mqtt broker to maintain mqtt connection. The heartbeat sending interval is controlled by @ref AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS configuration item
 *
 * 2. If a mqtt PUBLISH message of qos1 does not receive the mqtt PUBACK response message within @ref AIOT_MQTTOPT_REPUB_TIMEOUT_MS time, this function will resend the message until it succeeds
 *
 * @param[in] handle MQTT instance handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful
 *
 * @note
 *
 * This function is non-blocking and needs to be called intermittently. The calling interval should be less than the minimum value of @ref AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS and @ref AIOT_MQTTOPT_REPUB_TIMEOUT_MS when no mqtt is received within the time.
 *
 * To ensure that the heartbeat sending and QoS1 message retransmission logic work properly*/
int32_t aiot_mqtt_process(void *handle);

/**
 * @brief Send a PUBLISH message to the MQTT server with QoS 0, used to publish the specified message
 *
 * @param[in] handle MQTT instance handle
 * @param[in] topic specifies the topic of the MQTT PUBLISH message
 * @param[in] payload specifies the payload of the MQTT PUBLISH message
 * @param[in] payload_len specifies the payload_len of the MQTT PUBLISH message
 * @param[in] qos specifies the qos value of mqtt, only qos0 and qos1 are supported
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful*/
int32_t aiot_mqtt_pub(void *handle, char *topic, uint8_t *payload, uint32_t payload_len, uint8_t qos);

/**
 * @brief Send an mqtt SUBSCRIBE message to the MQTT server for subscribing to the specified topic
 *
 * @param[in] handle MQTT instance handle
 * @param[in] topic specifies the topic of the MQTT SUBSCRIBE message
 * @param[in] handler The MQTT PUBLISH message callback function corresponding to the topic. When a message is published to the topic, the callback function is called
                    If handler is passed in as NULL, the SDK calls the callback function configured by @ref AIOT_MQTTOPT_RECV_HANDLER
                    If aiot_mqtt_sub() is called multiple times and different handlers are specified for the same topic, different handlers will be called when a message arrives.
 * @param[in] qos specifies the maximum qos value that the topic expects the mqtt server to support. Only qos0 and qos1 are supported.
 * @param[in] userdata allows the SDK to save the user context. When the callback function is called, this context will be passed back to the user through the handler.
 * If the context is not specified, the context configured through @ref AIOT_MQTTOPT_USERDATA will be returned to the user through the handler
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful*/
int32_t aiot_mqtt_sub(void *handle, char *topic, aiot_mqtt_recv_handler_t handler, uint8_t qos, void *userdata);

/**
 * @brief Send an mqtt UNSUBSCRIBE message to the MQTT server to unsubscribe from the specified topic
 *
 * @param[in] handle MQTT instance handle
 * @param[in] topic specifies the topic of the MQTT UNSUBSCRIBE message
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed, for more information, please refer to @ref aiot_state_api.h
 * @retval >=STATE_SUCCESS execution successful*/
int32_t aiot_mqtt_unsub(void *handle, char *topic);

/**
 * @brief Try to receive MQTT messages from the network
 *
 * @details
 *
 * In addition to receiving MQTT messages from the network, this function also includes a reconnection mechanism
 *
 * 1. When MQTT heartbeat loss exceeds the number of times configured by @ref AIOT_MQTTOPT_HEARTBEAT_MAX_LOST, the reconnection mechanism is triggered
 *
 * + The reconnection interval is specified by @ref AIOT_MQTTOPT_RECONN_INTERVAL_MS
 *
 * 2. When the SDK detects that the network is disconnected, the reconnection mechanism is triggered.
 *
 * + The reconnection interval is specified by @ref AIOT_MQTTOPT_RECONN_INTERVAL_MS
 *
 * @param[in] handle
 *
 * @retval STATE_SYS_DEPEND_NWK_READ_LESSDATA was executed successfully. At this time, there are no MQTT messages that can be received on the network.
 * @retval >=STATE_SUCCESS execution successful
 * @retval Other return values   execution failed, please refer to @ref aiot_state_api.h for more information
 *
 * @note
 *
 * When the network connection is normal and @ref aiot_mqtt_deinit is not called, this function is blocking and needs to be called continuously.
 *
 * 1. When the network connection is disconnected, this function will return immediately, and the return value is @ref STATE_SYS_DEPEND_NWK_CLOSED
 *
 * 2. When @ref aiot_mqtt_deinit is called, the function will return immediately, and the return value is @ref STATE_USER_INPUT_EXEC_DISABLED*/
int32_t aiot_mqtt_recv(void *handle);

#if defined(__cplusplus)
}
#endif

#endif

