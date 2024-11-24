/**
 * @file aiot_state_api.h
 * @brief SDK Core status dock file, all api return values   in Core are listed here
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 **/

#ifndef _AIOT_STATE_API_H_
#define _AIOT_STATE_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief SDK’s log information output callback function prototype*/
typedef int32_t (* aiot_state_logcb_t)(int32_t code, char *message);

/**
 * @brief Set the callback function used for SDK log information output
 *
 * @param handler log callback function
 *
 * @return int32_t reserved*/
int32_t aiot_state_set_logcb(aiot_state_logcb_t handler);

/**
 * @brief API execution successful
 **/
#define STATE_SUCCESS                                               (0x0000)

/**
 * @brief -0x0100~-0x01FF expresses the status code fed back by the SDK when checking user input parameters
 **/
#define STATE_USER_INPUT_BASE                                       (-0x0100)

/**
 * @brief The user input parameter contains an illegal null pointer
 **/
#define STATE_USER_INPUT_NULL_POINTER                               (-0x0101)

/**
 * @brief User input parameters contain out-of-bounds values
 **/
#define STATE_USER_INPUT_OUT_RANGE                                  (-0x0102)

/**
 * @brief The configuration items entered by the user cannot be understood by the SDK Modules
 **/
#define STATE_USER_INPUT_UNKNOWN_OPTION                             (-0x0103)

/**
 * @brief productKey is missing from user input parameters
 **/
#define STATE_USER_INPUT_MISSING_PRODUCT_KEY                        (-0x0104)

/**
 * @brief deviceName is missing in user input parameters
 **/
#define STATE_USER_INPUT_MISSING_DEVICE_NAME                        (-0x0105)

/**
 * @brief deviceSecret is missing in user input parameters
 **/
#define STATE_USER_INPUT_MISSING_DEVICE_SECRET                      (-0x0106)

/**
 * @brief productSecret is missing in user input parameters
 **/
#define STATE_USER_INPUT_MISSING_PRODUCT_SECRET                     (-0x0107)

/**
 * @brief The domain name address or IP address is missing in the user input parameters
 **/
#define STATE_USER_INPUT_MISSING_HOST                               (-0x0108)

/**
 * @brief The user has called the destruction function to destroy the instance (such as @ref aiot_mqtt_deinit), and other APIs that operate on the instance should not be called again
 **/
#define STATE_USER_INPUT_EXEC_DISABLED                              (-0x0109)

/**
 * @brief The JSON string input by the user failed to parse
 **/
#define STATE_USER_INPUT_JSON_PARSE_FAILED                          (-0x010A)

/**
 * @brief -0x0200~-0x02FF expresses the status code fed back when the SDK calls system dependent functions
 **/
#define STATE_SYS_DEPEND_BASE                                       (-0x0200)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_malloc failed to apply for memory
 **/
#define STATE_SYS_DEPEND_MALLOC_FAILED                              (-0x0201)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_setopt's input parameter other than cred is illegal
 **/
#define STATE_SYS_DEPEND_NWK_INVALID_OPTION                         (-0x0202)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_establish failed to establish network
 **/
#define STATE_SYS_DEPEND_NWK_EST_FAILED                             (-0x0203)

/**
 * @brief SDK detects network disconnection
 **/
#define STATE_SYS_DEPEND_NWK_CLOSED                                 (-0x0204)

/**
 * @brief SDK actually reads less data from the network than expected
 **/
#define STATE_SYS_DEPEND_NWK_READ_LESSDATA                          (-0x0205)

/**
 * @brief The actual data written by the SDK to the network is less than expected
 **/
#define STATE_SYS_DEPEND_NWK_WRITE_LESSDATA                         (-0x0206)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_recv timeout return
 **/
#define STATE_SYS_DEPEND_NWK_READ_OVERTIME                          (-0x0207)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_setopt's cred input parameter is illegal
 **/
#define STATE_SYS_DEPEND_NWK_INVALID_CRED                           (-0x0208)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_send encountered exception when sending data
 **/
#define STATE_SYS_DEPEND_NWK_SEND_ERR                               (-0x0209)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_recv encountered exception when receiving data
 **/
#define STATE_SYS_DEPEND_NWK_RECV_ERR                               (-0x020A)

/**
 * @brief -0x0300~-0x03FF expresses the status code of the SDK in the MQTT Modules
 **/
#define STATE_MQTT_BASE                                             (-0x0300)

/**
 * @brief When MQTT tries to establish a connection, the format of the CONNACK message returned from the server is incorrect.
 **/
#define STATE_MQTT_CONNACK_FMT_ERROR                                (-0x0301)

/**
 * @brief When establishing an MQTT connection, the server reports an error that is incompatible with the MQTT protocol version currently used by the client.
 **/
#define STATE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION      (-0x0302)

/**
 * @brief When MQTT establishes a connection, the server reports an error that the service is unavailable. It may be that the clientId value is incorrect or the heartbeat interval is unreasonable.
 **/
#define STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE                 (-0x0303)

/**
 * @brief When establishing a MQTT connection, the username and password returned by the server are illegal.
 **/
#define STATE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD              (-0x0304)

/**
 * @brief When MQTT establishes a connection, the server returns authentication failure and the username or password is incorrect. This is usually caused by a triplet error.
 **/
#define STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED                     (-0x0305)

/**
 * @brief When MQTT establishes a connection, the server returns an unknown CONNACK message
 **/
#define STATE_MQTT_CONNACK_RCODE_UNKNOWN                            (-0x0306)

/**
 * @brief When MQTT caches QoS1 messages, it is detected that the packetId is wrapped
 **/
#define STATE_MQTT_PUBLIST_PACKET_ID_ROLL                           (-0x0307)

/**
 * @brief When MQTT publishes or subscribes, it is detected that the Topic format does not comply with the protocol specifications.
 **/
#define STATE_MQTT_TOPIC_INVALID                                    (-0x0308)

/**
 * @brief When MQTT publishes or subscribes or unsubscribes, the SDK feedbacks the status code of the Topic content.
 **/
#define STATE_MQTT_LOG_TOPIC                                        (-0x0309)

/**
 * @brief When MQTT sends and receives messages, the SDK feeds back the status code of the MQTT message content.
 **/
#define STATE_MQTT_LOG_HEXDUMP                                      (-0x030A)

/**
 * @brief MQTT connection established successfully
 **/
#define STATE_MQTT_CONNECT_SUCCESS                                  (-0x030B)

/**
 * The MQTT message read by @brief SDK contains a message length field that does not comply with the protocol specification.
 **/
#define STATE_MQTT_MALFORMED_REMAINING_LEN                          (-0x030C)

/**
 * @brief The MQTT message read by the SDK does not meet the number of bytes parsed according to the protocol specification.
 **/
#define STATE_MQTT_MALFORMED_REMAINING_BYTES                        (-0x030D)

/**
 * @brief SDK reads an MQTT message type that is not yet supported
 **/
#define STATE_MQTT_PACKET_TYPE_UNKNOWN                              (-0x030E)

/**
 * @brief When MQTT subscribes or unsubscribes, get an operation failure response from the server
 **/
#define STATE_MQTT_SUBACK_RCODE_FAILURE                             (-0x030F)

/**
 * @brief When MQTT subscribes or unsubscribes, an unparsable response is obtained from the server.
 **/
#define STATE_MQTT_SUBACK_RCODE_UNKNOWN                             (-0x0310)

/**
 * @brief When MQTT receives a message, the Topic in the message fails to match the understandable Topic list.
 **/
#define STATE_MQTT_TOPIC_COMPARE_FAILED                             (-0x0311)

/**
 * @brief When executing @ref aiot_mqtt_deinit, in order to wait for the completion of other API executions and exceeding the set timeout, the MQTT instance fails to be destroyed
 **/
#define STATE_MQTT_DEINIT_TIMEOUT                                   (-0x0312)

/**
 * @brief MQTT active connection server related log status code
 **/
#define STATE_MQTT_LOG_CONNECT                                      (-0x0313)

/**
 * @brief After MQTT is disconnected, log status codes related to automatic reconnection to the server
 **/
#define STATE_MQTT_LOG_RECONNECTING                                 (-0x0314)

/**
 * @brief MQTT connection server timeout related log status code
 **/
#define STATE_MQTT_LOG_CONNECT_TIMEOUT                              (-0x0315)

/**
 * @brief Log status codes related to MQTT actively disconnecting from the server
 **/
#define STATE_MQTT_LOG_DISCONNECT                                   (-0x0316)

/**
 * @brief When MQTT connects to the server, the log status code related to the user name used
 **/
#define STATE_MQTT_LOG_USERNAME                                     (-0x0317)

/**
 * @brief Log status code related to the password used when MQTT connects to the server
 **/
#define STATE_MQTT_LOG_PASSWORD                                     (-0x0318)

/**
 * @brief When MQTT connects to the server, the log status code related to the clientId used
 **/
#define STATE_MQTT_LOG_CLIENTID                                     (-0x0319)

/**
 * @brief When MQTT connects to the server, the log status code related to the PSK-TLS key used
 **/
#define STATE_MQTT_LOG_TLS_PSK                                      (-0x031A)

/**
 * @brief When MQTT publishes or subscribes or unsubscribes, the length of the Topic exceeds the limit of the IoT platform and execution is suspended.
 **/
#define STATE_MQTT_TOPIC_TOO_LONG                                   (-0x031B)

/**
 * @brief When MQTT publishes a message, the length of the Payload exceeds the limit of the IoT platform and the execution is aborted.
 **/
#define STATE_MQTT_PUB_PAYLOAD_TOO_LONG                             (-0x031C)

/**
 * @brief The backup IP address used when MQTT connects to the server
 **/
#define STATE_MQTT_LOG_BACKUP_IP                                    (-0x031D)

/**
 * @brief Received illegal MQTT PINRESP message
 **/
#define STATE_MQTT_RECV_INVALID_PINRESP_PACKET                      (-0x031E)

/**
 * @brief Received illegal MQTT PUBLISH message
 **/
#define STATE_MQTT_RECV_INVALID_PUBLISH_PACKET                      (-0x031F)

/**
 * @brief Received illegal MQTT PUBACK message
 **/
#define STATE_MQTT_RECV_INVALID_PUBACK_PACKET                       (-0x0320)

/**
 * @brief When MQTT connects to the server, the log status code of the host used
 **/
#define STATE_MQTT_LOG_HOST                                         (-0x032A)

/**
 * @brief -0x0400~-0x04FF expresses the status code of the SDK in the HTTP Modules
 **/
#define STATE_HTTP_BASE                                             (-0x0400)

/**
 * @brief When parsing the received HTTP message, the valid status line cannot be obtained and the HTTP StatusCode cannot be obtained.
 **/
#define STATE_HTTP_STATUS_LINE_INVALID                              (-0x0401)

/**
 * @brief When parsing the received HTTP message, the body part of the message has been received and there is no more data.
 **/
#define STATE_HTTP_READ_BODY_FINISHED                               (-0x0402)

/**
 * @brief When executing @ref aiot_http_deinit, waiting for the completion of other API executions exceeds the set timeout period, and the HTTP instance fails to be destroyed.
 **/
#define STATE_HTTP_DEINIT_TIMEOUT                                   (-0x0403)

/**
 * @brief Because the StatusCode of the HTTP authentication response is not 200, the authentication failed.
 **/
#define STATE_HTTP_AUTH_CODE_FAILED                                 (-0x0404)

/**
 * @brief Authentication failed because the HTTP authentication response reception was not completed.
 **/
#define STATE_HTTP_AUTH_NOT_FINISHED                                (-0x0405)

/**
 * @brief Because the Token cannot be parsed in the HTTP authentication response, the authentication failed.
 **/
#define STATE_HTTP_AUTH_TOKEN_FAILED                                (-0x0406)

/**
 * @brief The device has not been authenticated yet, you need to call @ref aiot_http_auth for device authentication first.
 **/
#define STATE_HTTP_NEED_AUTH                                        (-0x0407)

/**
 * @brief HTTP response data reception is not completed, need to confirm whether the network is abnormal
 **/
#define STATE_HTTP_RECV_NOT_FINISHED                                (-0x0408)

/**
 * @brief The internal buffer length is too short, you need to call @ref aiot_http_setopt to configure the @ref AIOT_HTTPOPT_HEADER_BUFFER_LEN option to increase the buffer length
 **/
#define STATE_HTTP_HEADER_BUFFER_TOO_SHORT                          (-0x0409)

/**
 * @brief HTTP header reception exception
 **/
#define STATE_HTTP_HEADER_INVALID                                   (-0x040A)

/**
 * @brief The status code used by the HTTP header to send related logs
 **/
#define STATE_HTTP_LOG_SEND_HEADER                                  (-0x040B)

/**
 * @brief Status code used in logs related to HTTP content sending
 **/
#define STATE_HTTP_LOG_SEND_CONTENT                                 (-0x040C)

/**
 * @brief HTTP header receives the status code used by related logs
 **/
#define STATE_HTTP_LOG_RECV_HEADER                                  (-0x040D)

/**
 * @brief Status code used for HTTP content reception related logs
 **/
#define STATE_HTTP_LOG_RECV_CONTENT                                 (-0x040E)

/**
 * @brief Status code used in logs related to HTTP connection disconnection
 **/
#define STATE_HTTP_LOG_DISCONNECT                                   (-0x040F)

/**
 * @brief Status code used in HTTP authentication interaction related logs
 **/
#define STATE_HTTP_LOG_AUTH                                         (-0x0410)

/**
 * @brief Because the content of the HTTP response message does not meet expectations, the authentication failed.
 **/
#define STATE_HTTP_AUTH_NOT_EXPECTED                                (-0x0411)

/**
 * @brief The payload part of the HTTP message is empty, and the reception has been completed
 **/
#define STATE_HTTP_READ_BODY_EMPTY                                  (-0x0412)

/**
 * @brief -0x0F00~-0x0FFF expresses the status code of the SDK in the system's underlying dependent Modules
 **/
#define STATE_PORT_BASE                                             (-0x0F00)

/**
 * @brief The underlying dependent function encountered an illegal null pointer parameter and failed to execute.
 **/
#define STATE_PORT_INPUT_NULL_POINTER                               (-0x0F01)

/**
 * @brief The underlying dependency function encounters an input parameter that exceeds the reasonable value range, and execution fails.
 **/
#define STATE_PORT_INPUT_OUT_RANGE                                  (-0x0F02)

/**
 * @brief The underlying dependent function encountered an application memory error and failed to execute.
 **/
#define STATE_PORT_MALLOC_FAILED                                    (-0x0F03)

/**
 * @brief The underlying dependent function encounters a missing domain name address or IP address and fails to execute.
 **/
#define STATE_PORT_MISSING_HOST                                     (-0x0F04)

/**
 * @brief The underlying dependency function encounters a TCP client establishment process that has not yet been implemented, and execution fails.
 **/
#define STATE_PORT_TCP_CLIENT_NOT_IMPLEMENT                         (-0x0F05)

/**
 * @brief The underlying dependency function encounters a TCP server establishment process that has not yet been implemented, and execution fails.
 **/
#define STATE_PORT_TCP_SERVER_NOT_IMPLEMENT                         (-0x0F06)

/**
 * @brief The underlying dependency function encounters a UDP client establishment process that has not yet been implemented, and execution fails.
 **/
#define STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT                         (-0x0F07)

/**
 * @brief The underlying dependency function encounters a UDP server establishment process that has not yet been implemented, and the execution fails.
 **/
#define STATE_PORT_UDP_SERVER_NOT_IMPLEMENT                         (-0x0F08)

/**
 * @brief The underlying dependency function encountered an incomprehensible network layer setting option and failed to execute.
 **/
#define STATE_PORT_NETWORK_UNKNOWN_OPTION                           (-0x0F09)

/**
 * @brief The underlying dependent function encountered an incomprehensible socket type and failed to execute.
 **/
#define STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE                      (-0x0F0A)

/**
 * @brief The underlying dependency function encountered a domain name DNS resolution error and failed to execute.
 **/
#define STATE_PORT_NETWORK_DNS_FAILED                               (-0x0F0B)

/**
 * @brief The underlying dependency function encountered a socket creation error when establishing a network connection, and the execution failed.
 **/
#define STATE_PORT_NETWORK_SOCKET_CREATE_FAILED                     (-0x0F0C)

/**
 * @brief The underlying dependent function encountered a socket configuration error when establishing a network connection, and the execution failed.
 **/
#define STATE_PORT_NETWORK_SOCKET_CONFIG_FAILED                     (-0x0F0D)

/**
 * @brief The underlying dependency function encountered a bind error when establishing a network connection and failed to execute.
 **/
#define STATE_PORT_NETWORK_SOCKET_BIND_FAILED                       (-0x0F0E)

/**
 * @brief The underlying dependent function encountered a TCP connection timeout and failed to execute.
 **/
#define STATE_PORT_NETWORK_CONNECT_TIMEOUT                          (-0x0F0F)

/**
 * @brief The underlying dependent function encountered a TCP connection establishment error and failed to execute.
 **/
#define STATE_PORT_NETWORK_CONNECT_FAILED                           (-0x0F10)

/**
 * @brief The underlying dependent function encountered a socket select error and failed to execute.
 **/
#define STATE_PORT_NETWORK_SELECT_FAILED                            (-0x0F11)

/**
 * @brief The underlying dependency function encountered an error in sending data from the network layer and failed to execute.
 **/
#define STATE_PORT_NETWORK_SEND_FAILED                              (-0x0F12)

/**
 * @brief The underlying dependency function encountered an error in receiving data from the network layer and failed to execute.
 **/
#define STATE_PORT_NETWORK_RECV_FAILED                              (-0x0F13)

/**
 * @brief When the underlying dependent function sent data, it encountered that the connection was closed and the execution failed.
 **/
#define STATE_PORT_NETWORK_SEND_CONNECTION_CLOSED                   (-0x0F14)

/**
 * @brief When the underlying dependent function received data, it encountered that the connection was closed and the execution failed.
 **/
#define STATE_PORT_NETWORK_RECV_CONNECTION_CLOSED                   (-0x0F15)

/**
 * @brief The underlying dependency function encountered an incomprehensible security credential option and failed to execute.
 **/
#define STATE_PORT_TLS_INVALID_CRED_OPTION                          (-0x0F16)

/**
 * @brief The underlying dependency function encountered an illegal maximum TLS fragment length configuration and failed to execute.
 **/
#define STATE_PORT_TLS_INVALID_MAX_FRAGMENT                         (-0x0F17)

/**
 * @brief The underlying dependency function encountered an illegal server certificate and failed to execute.
 **/
#define STATE_PORT_TLS_INVALID_SERVER_CERT                          (-0x0F18)

/**
 * @brief The underlying dependency function encountered an illegal client certificate and failed to execute.
 **/
#define STATE_PORT_TLS_INVALID_CLIENT_CERT                          (-0x0F19)

/**
 * @brief The underlying dependency function encountered an illegal client key and failed to execute.
 **/
#define STATE_PORT_TLS_INVALID_CLIENT_KEY                           (-0x0F1A)

/**
 * @brief The underlying dependent function encountered a socket creation error when establishing a TLS connection, and the execution failed.
 **/
#define STATE_PORT_TLS_SOCKET_CREATE_FAILED                         (-0x0F1B)

/**
 * @brief The underlying dependent function encountered a socket connection establishment error when establishing a TLS connection, and the execution failed.
 **/
#define STATE_PORT_TLS_SOCKET_CONNECT_FAILED                        (-0x0F1C)

/**
 * @brief The underlying dependency function encountered a handshake failure when establishing a TLS connection and failed to execute.
 **/
#define STATE_PORT_TLS_INVALID_RECORD                               (-0x0F1D)

/**
 * @brief The underlying dependent function encountered a data reception error on the TLS connection and failed to execute.
 **/
#define STATE_PORT_TLS_RECV_FAILED                                  (-0x0F1E)

/**
 * @brief The underlying dependent function encountered a data sending error on the TLS connection and failed to execute.
 **/
#define STATE_PORT_TLS_SEND_FAILED                                  (-0x0F1F)

/**
 * @brief The underlying dependent function is on the TLS connection. When receiving data, the connection is closed and the execution fails.
 **/
#define STATE_PORT_TLS_RECV_CONNECTION_CLOSED                       (-0x0F20)

/**
 * @brief The underlying dependent function is on the TLS connection. When sending data, the connection is closed and the execution fails.
 **/
#define STATE_PORT_TLS_SEND_CONNECTION_CLOSED                       (-0x0F21)

/**
 * @brief The underlying dependent function encountered a PSK configuration error when establishing a TLS connection and failed to execute.
 **/
#define STATE_PORT_TLS_CONFIG_PSK_FAILED                            (-0x0F22)

/**
 * @brief The underlying dependency function encountered a handshake error other than an illegal record when establishing a TLS connection, and the execution failed.
 **/
#define STATE_PORT_TLS_INVALID_HANDSHAKE                            (-0x0F23)

/**
 * @brief PSK configuration error during DTLS handshake, execution failed
 **/
#define STATE_PORT_DTLS_CONFIG_PSK_FAILED                           (-0x0F24)

/**
 * @brief DTLS handshake failed, execution failed
 **/
#define STATE_PORT_DTLS_HANDSHAKE_FAILED                            (-0x0F25)

/**
 * @brief DTLS failed to establish connection and execution failed
 **/
#define STATE_PORT_NETWORK_DTLS_CONNECT_FAILED                      (-0x0F26)

/**
 * @brief The previous DTLS handshake is still in progress
 **/
#define STATE_PORT_DTLS_HANDSHAKE_IN_PROGRESS                       (-0x0F27)

/**
 * @brief The number of cached qos 1 messages exceeds expectations
 **/
#define STATE_QOS_CACHE_EXCEEDS_LIMIT                               (-0x0F28)

/**
 * @brief core_adapter adaptation Modules
 **/
#define STATE_ADAPTER_COMMON                                        (-0x1000)

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _AIOT_STATE_API_H_ */

