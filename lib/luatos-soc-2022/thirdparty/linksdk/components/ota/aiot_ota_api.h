/**
 * @file aiot_ota_api.h
 * @brief OTA Modules header file, providing the device with the ability to obtain firmware upgrades and remote configuration
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 * @details
 *
 * The OTA Modules can be used to cooperate with the firmware upgrade service of the Alibaba Cloud platform. The OTA control operation process is Introductiond on the [Push firmware to device] (https://help.aliyun.com/document_detail/58328.html) page
 *
 * + Refer to the [Device Side OTA Upgrade](https://help.aliyun.com/document_detail/58328.html) page to understand the network interaction process when the device side cooperates with OTA upgrade
 *
 * + If the device wants to use the OTA service, it must first report the current firmware version number to the cloud with @ref aiot_ota_report_version, otherwise it will not be able to receive firmware information push
 * + When the device downloads firmware, the device uses @ref aiot_download_send_request to obtain it from the cloud. The SDK will not automatically download the firmware.
 * + When the device is downloading or after the download is completed, the device needs to write the firmware content in the memory buffer to Flash by itself. The SDK does not contain the logic for firmware burning.
 * + After the device upgrade is completed, the new upgraded firmware version number needs to be reported to the cloud through the @ref aiot_ota_report_version interface, otherwise the cloud will not consider the upgrade completed.
 * + When the device receives the upgrade task from @ref aiot_ota_recv_handler_t, if the task is ignored, it can receive the task again by calling @ref aiot_ota_query_firmware
 *
 * + When the user pushes the firmware download address and other information to the device on the console, it does so through the MQTT channel, so the prerequisite for device OTA upgrade is to successfully establish and keep the MQTT long connection channel online.
 **/

#ifndef __AIOT_OTA_API_H__
#define __AIOT_OTA_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief The types of OTA messages downlinked from the cloud are divided into two types: firmware upgrade and remote configuration.
 *
 * @details
 * The MQTT message type passed in @ref aiot_ota_recv_handler_t
 **/
typedef enum {

    /**
     * @brief The OTA message received is a firmware upgrade message
     **/
    AIOT_OTARECV_FOTA,

    /**
     * @brief The OTA message received is a remote configuration message
     **/
    AIOT_OTARECV_COTA
} aiot_ota_recv_type_t;

/**
 * @brief The type of digest method used in the OTA process, divided into two types: MD5 and SHA256
 **/

typedef enum {

    /**
     * @brief The digest method of the OTA firmware received is MD5
     **/
    AIOT_OTA_DIGEST_MD5,

    /**
     * @brief The digest method of the OTA firmware received is SHA256
     **/
    AIOT_OTA_DIGEST_SHA256,
    AIOT_OTA_DIGEST_MAX
} aiot_ota_digest_type_t;

/**
 * @brief OTA method of downloading upgrade files
 **/

typedef enum {

    /**
     * @brief The way to download OTA files is HTTPS
     **/
    AIOT_OTA_PROTOCOL_HTTPS,

    /**
     * @brief The way to download OTA files is MQTT
     **/
    AIOT_OTA_PROTOCOL_MQTT,
    AIOT_OTA_PROTOCOL_MAX
} aiot_ota_protocol_type_t;

/**
 * @brief Description of the firmware upgrade task pushed down from the cloud, including URL, size, signature, etc.
 **/
typedef struct {

    /**
     * @brief product_key of the device to be upgraded
     **/
    char       *product_key;

    /**
     * @brief device_name of the device to be upgraded
     **/
    char       *device_name;

    /** @brief Link required to download firmware
     **/
    char       *url;
    /** @brief mqtt file download, identifying the streamid of the file stream
     **/
    uint32_t   stream_id;
    /** @brief mqtt file download, fileid identifying the file
     **/
    uint32_t   stream_file_id;
    /** @brief The size of the firmware, in Byte
     **/
    uint32_t    size_total;

    /** @brief How to digitally sign firmware in the cloud, see @ref aiot_ota_digest_type_t for details
     **/
    uint8_t     digest_method;

    /** @brief The result of digital signature calculation on firmware in the cloud
     **/
    char       *expect_digest;

    /**
     * @brief Firmware version information. If it is firmware information, the version field is the version number of the firmware. If it is a remote configuration message, it is the configured configId
     **/
    char       *version;

    /**
     * @brief Modules corresponding to the current firmware
     **/
    char       *module;

    /**
     * @brief *The mqtt handle needed to report messages to the cloud during the firmware upgrade process*/
    void       *mqtt_handle;

    /**
     * @brief The extended content in the current download information
     **/
    char       *extra_data;

    /**
      *
      */
    char  *file_name;

    /**
     * @brief The total number of urls in the ota task*/
    uint32_t file_num;

    /**
     * @brief The serial number of the current download task*/
    uint32_t file_id;
    /**
     * @brief protocol for downloading ota files*/
    aiot_ota_protocol_type_t protocol_type;

} aiot_download_task_desc_t;

/**
* @brief OTA message downlinked from the cloud, including its message type (firmware upgrade/remote configuration) and specific description of the upgrade task
**/
typedef struct {

    /**
     * @brief Cloud downlink OTA message type, for more information, please refer to @ref aiot_ota_recv_type_t*/
    aiot_ota_recv_type_t        type;

    /**
     * @brief Description of the firmware upgrade task pushed down by the cloud, including url, size, signature, etc. For more information, please refer to @ref aiot_download_task_desc_t*/
    aiot_download_task_desc_t   *task_desc;
} aiot_ota_recv_t;

/**
 * @brief The reception callback function when the device receives the OTA mqtt downlink message. In this callback function, the user can see the version number of the firmware to be upgraded and decide the upgrade strategy (whether to upgrade, when to upgrade, etc.)

 * @param[in] handle OTA instance handle
 * @param[in] msg Cloud downlink OTA message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_ota_recv_handler_t)(void *handle, const aiot_ota_recv_t *const msg, void *userdata);

/**
* @brief Type of fragmented message received during firmware downloading process
**/
typedef enum {

    /**
    * @brief Firmware fragmentation message based on HTTP transmission
    **/
    AIOT_DLRECV_HTTPBODY
} aiot_download_recv_type_t;

/**
* @brief Description of fragmented messages received during firmware downloading, including type, stored buffer address, buffer length, and current download progress
**/
typedef struct {

    /**
    * @brief The type of fragmented packets received during the downloading of firmware. For details, see @ref aiot_download_recv_type_t
    **/
    aiot_download_recv_type_t type;

    struct {

        /**
        * @brief During the process of downloading the firmware, the buffer address allocated by the SDK to store the firmware content downloaded from the cloud. The SDK will actively release it after the callback function ends. The user needs to copy and save the message by himself.
        **/
        uint8_t *buffer;

        /**
        * @brief During the process of downloading the firmware, the size of the buffer allocated by the SDK to store the firmware content downloaded from the cloud can be adjusted by the user via @ref AIOT_DLOPT_BODY_BUFFER_MAX_LEN
        **/
        uint32_t len;

        /**
        * @brief The percentage of the current download progress
        **/
        int32_t  percent;
    } data;
} aiot_download_recv_t;


/**
 * @brief After the upgrade starts, the packet receiving callback function is used when the device receives the firmware content divided into segments. The current default is to push down the segmented firmware content through https packets.

 * @param[in] handle download instance handle
 * @param[in] packet The fragmented firmware packet downstream from the cloud
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_download_recv_handler_t)(void *handle, const aiot_download_recv_t *packet,
        void *userdata);

/**
 * @brief The error code agreed with the cloud during the OTA process, so that the cloud knows where the error occurred during the upgrade process.
 **/

typedef enum {

    /**
     * @brief Error description of device upgrade agreed with the cloud*/
    AIOT_OTAERR_UPGRADE_FAILED = -1,

    /**
     * @brief Error code description for device download errors agreed with the cloud*/
    AIOT_OTAERR_FETCH_FAILED = -2,

    /**
     * @brief Error code description for errors when verifying digital signatures with firmware agreed with the cloud*/
    AIOT_OTAERR_CHECKSUM_MISMATCH = -3,

    /**
     * @brief The error code description for firmware burning errors agreed with the cloud*/
    AIOT_OTAERR_BURN_FAILED = -4
} aiot_ota_protocol_errcode_t;

/**
 * @brief When calling the @ref aiot_ota_setopt interface, the available values   of the option parameter
 **/

typedef enum {

    /**
     * @brief Set the user callback function for processing OTA messages
     *
     * @details
     *
     * In this callback, the user may receive two kinds of messages: firmware upgrade message or remote configuration message.
     *
     * No matter what kind of message, it contains url, version, digest method, sign, etc.
     *
     * The user needs to decide the upgrade strategy in this callback, including whether to upgrade and when to upgrade. If an upgrade is required, aiot_download_init needs to be called to initialize a download instance handle.
     * For details, see the use cases of fota_xxx_xxx.c in the demos directory.
     *
     * Data type: (void *)*/
    AIOT_OTAOPT_RECV_HANDLER,

    /**
     * @brief Set MQTT handle
     *
     * @details
     *
     * The channel capability of MQTT is used in the OTA process to report the version number, progress, and error code to the cloud.
     *
     * Data type: (void *)*/
    AIOT_OTAOPT_MQTT_HANDLE,

    /**
      * @brief User needs SDK temporary context
      *
      * @details
      *
      * This context will be passed back to the user in @ref AIOT_OTAOPT_RECV_HANDLER
      *
      * Data type: (void *)*/
    AIOT_OTAOPT_USERDATA,

    /**
      * @brief If the current ota is for an external Modules (mcu, etc.), you need to set the Modules name through this field
      *
      * @details
      *
      * OTA may be conducted for an external Modules. When reporting the version number, you need to know the name of the Modules.
      * The name of the Modules is set through this field.
      *
      * Data type: (void *)*/
    AIOT_OTAOPT_MODULE,
    AIOT_OTAOPT_MAX
} aiot_ota_option_t;

/**
 * @brief When calling the @ref aiot_download_setopt interface, the available values   of the option parameter
 **/

typedef enum {

    /**
     * @brief The security credentials used by the network when the device establishes a connection with the firmware server through HTTP
     *
     * @details
     *
     * This configuration item is used to configure @ref aiot_sysdep_network_cred_t security credential data for the underlying network
     *
     * 1. If this option is not configured, HTTP will establish the connection directly in tcp mode.
     *
     * 2. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_NONE, HTTP will establish the connection directly in tcp mode
     *
     * 3. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA, HTTP will establish the connection in tls mode
     *
     * 4. If the option in @ref aiot_sysdep_network_cred_t is configured as @ref AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK, HTTP will establish the connection in tls psk mode
     *
     * Data type: (aiot_sysdep_network_cred_t *)*/
    AIOT_DLOPT_NETWORK_CRED,

    /**
     * @brief The port number for the device to access the firmware download server through HTTP
     *
     * @details
     *
     * If you are using tcp or tls certificate method, the port number is set to 443
     *
     * Data type: (uint16_t *)*/
    AIOT_DLOPT_NETWORK_PORT,

    /**
     * @brief The maximum time spent in the protocol stack when receiving firmware content via HTTP
     *
     * @details
     *
     * Data type: (uint32_t *) Default value: (5 * 1000) ms*/
    AIOT_DLOPT_RECV_TIMEOUT_MS,

    /**
     * @brief HTTP data receiving callback function
     *
     * @details
     *
     * Data type: (aiot_download_recv_handler_t)*/
    AIOT_DLOPT_RECV_HANDLER,

    /**
     * @brief User needs SDK temporary context
     *
     * @details
     *
     * When HTTP data is received, the context will be given from the userdata parameter of @ref aiot_download_recv_handler_t
     *
     * Data type: (void *)*/
    AIOT_DLOPT_USERDATA,

    /**
     * @brief Set the specific content of the download task contained in the download instance handle
     *
     * @details
     *
     * After receiving the mqtt message from OTA, if the user decides to upgrade, he needs to use this option to allocate memory in the download instance handle.
     * Copy the url, version, digest method, sign and other information carried in the OTA message. Only with this information can the download task be started.
     *
     * Data type: (aiot_download_task_desc_t *)
     *
     **/
    AIOT_DLOPT_TASK_DESC,

    /**
     * @brief Set the starting address for downloading according to the range
     *
     * @details
     *
     * In the HTTP range request (range requests) feature, it means that downloading starts from the byte.
     * If you specify to start downloading from the beginning, the value of start is 0
     *
     * Data type: (uint32_t *)
     *
     **/
    AIOT_DLOPT_RANGE_START,

    /**
     * @brief Set the end address of downloading according to range
     *
     * @details
     * In the HTTP range request (range requests) feature, it means that the download will end after reaching this byte.
     * If you specify to download from the beginning to end after 10 bytes,
     * You need to specify start = 0, end = 9, so a total of 10 bytes
     *
     * Data type: (uint32_t *)
     *
     **/
    AIOT_DLOPT_RANGE_END,

    /**
    * @brief When the device receives the http message returned from the firmware download server, the maximum length of the body given in the @ref aiot_download_recv_handler_t callback function each time
    *
    * @details
    * If the received data does not reach this length, the length given by the aiot_download_recv_handler_t callback function is the length actually received by the device
    *
    * Data type: (uint32_t *) Default value: (2 *1024) Bytes*/
    AIOT_DLOPT_BODY_BUFFER_MAX_LEN,
    AIOT_DLOPT_MAX
} aiot_download_option_t;

/**
 * @brief The device actively queries the cloud for upgrade tasks
 *
 * @details
 * After the device is online, if an OTA task is deployed on the cloud, the SDK will disclose the task to the user through @ref aiot_ota_recv_handler_t.
 * Due to current busy business and other reasons, this OTA task may be temporarily ignored. Then the user can call this API to let the cloud push this OTA task down again when the business is idle.
 * Similarly, the SDK will disclose the OTA task to the user from @ref aiot_ota_recv_handler_t
 *
 * @return int32_t
 * @retval STATE_SUCCESS The request was sent successfully
 * @retval STATE_OTA_QUERY_FIRMWARE_HANDLE_IS_NULL The handle handle used as an input parameter has not been initialized and needs to be initialized by calling @ref aiot_ota_init
 **/
int32_t aiot_ota_query_firmware(void *handle);

/**
 * @brief Create an OTA instance
 *
 * @return void*
 * @retval non-NULL ota instance handle
 * @retval NULL initialization failed, either because portfile was not set, or memory allocation failed.
 **/
void   *aiot_ota_init();

/**
 * @brief Destroy ota instance handle
 *
 * @param[in] handle pointer to ota instance handle
 *
 * @return int32_t
 * @retval STATE_OTA_DEINIT_HANDLE_IS_NULL handle or the address pointed to by handle is empty
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_ota_deinit(void **handle);

/**
 * @brief reports the version number of ordinary devices (non-gateway sub-devices)
 *
 * @details
 *
 * If the cloud does not know the current firmware version number of a device, it will not provide OTA services for it, and if it does not know the new version number of the device, it will not consider it to have been successfully upgraded.
 *
 * Therefore, for OTA to work normally, the device is generally required to call this interface every time it is powered on and report the currently running firmware version number string to the cloud.
 *
 * The handle of the parameter is obtained through @ref aiot_ota_init. For example, the code to report the current version number is "1.0.0" is written
 *
 * ```c
 * handle = aiot_ota_init();
 * ...
 * aiot_ota_report_version(handle, "1.0.0");
 * ```
 *
 * @param[in] handle pointer to ota instance handle
 * @param[in] version The version number to be reported
 *
 * @return int32_t
 * @retval STATE_OTA_REPORT_HANDLE_IS_NULL ota handle is empty
 * @retval STATE_OTA_REPORT_VERSION_IS_NULL The version number entered by the user is empty
 * @retval STATE_OTA_REPORT_MQTT_HANDLE_IS_NULL The mqtt handle in the ota_handle handle is empty
 * @retval STATE_OTA_REPORT_FAILED abort execution report
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_ota_report_version(void *handle, char *version);

/**
 * @brief is used by sub-devices in the gateway to report version numbers
 *
 * @param[in] handle ota instance handle
 * @param[in] product_key product_key of the device
 * @param[in] device_name The name of the device
 * @param[in] version version number
 *
 * @return int32_t
 * @retval STATE_SUCCESS reported successfully
 * @retval STATE_OTA_REPORT_EXT_HANELD_IS_NULL ota handle is empty
 * @retval STATE_OTA_REPORT_EXT_VERSION_NULL The version number entered by the user is empty
 * @retval STATE_OTA_REPORT_EXT_PRODUCT_KEY_IS_NULL The product_key input of the sub-device is empty
 * @retval STATE_OTA_REPORT_EXT_DEVICE_NAME_IS_NULL The device_name input of the sub-device is empty
 * @retval STATE_OTA_REPORT_EXT_MQTT_HANDLE_IS_NULL mqtt_handle in ota handle is empty
 * @retval STATE_OTA_REPORT_FAILED abort execution report
 **/
int32_t aiot_ota_report_version_ext(void *handle, char *product_key, char *device_name, char *version);

/**
 * @brief Set the parameters of the ota handle
 *
 * @details
 *
 * Configure OTA sessions, common configuration options include
 *
 * + `AIOT_OTAOPT_MQTT_HANDLE`: associate the MQTT session handle returned by @ref aiot_mqtt_init with the OTA session
 * + `AIOT_OTAOPT_RECV_HANDLER`: Set the data processing callback for OTA messages. This user callback will be called by @ref aiot_mqtt_recv when there is an OTA message.
 *
 * @param[in] handle ota handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_ota_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_ota_option_t
 *
 * @return int32_t
 * @retval STATE_OTA_SETOPT_HANDLE_IS_NULL ota handle is empty
 * @retval STATE_OTA_SETOPT_DATA_IS_NULL parameter data field is empty
 * @retval STATE_USER_INPUT_UNKNOWN_OPTION option is not supported
 * @retval STATE_SUCCESS parameter setting is successful
 **/
int32_t aiot_ota_setopt(void *handle, aiot_ota_option_t option, void *data);

/**
 * @brief Initialize the download instance and set default parameters
 *
 * @return void*
 * @retval non-NULL download instance handle
 * @retval NULL initialization failed, or portfile is not set, or there is insufficient memory to allocate download or http instances.
 **/
void   *aiot_download_init();

/**
 * @brief releases the resources of the download instance handle
 *
 * @param[in] handle pointer to download instance handle
 *
 * @return int32_t
 * @retval STATE_DOWNLOAD_DEINIT_HANDLE_IS_NULL handle or the content pointed to by handle is empty
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_download_deinit(void **handle);

/**
 * @brief Download a buffer through the download instance handle
 *
 * @details
 *
 * After the user parses the OTA message and knows the download address of the firmware, he can use this interface to download the firmware content via HTTP.
 *
 * The downloaded content will be passed to the user through the callback function. The user calls @ref aiot_download_setopt to set his own data processing callback function to the SDK
 *
 * @param[in] handle pointer to download instance handle
 *
 * @return int32_t
 * @retval >STATE_SUCCESS indicates the number of bytes downloaded
 * @retval STATE_DOWNLOAD_HTTPRSP_CODE_ERROR The url link used to download is inaccessible, and the code returned is not 200 or 206
 * @retval STATE_DOWNLOAD_FINISHED The download of the entire firmware package is completed
 * @retval STATE_DOWNLOAD_RANGE_FINISHED When downloading in segments, a single segment download is completed
 * @retval STATE_DOWNLOAD_HTTPRSP_HEADER_ERROR There is no Content-Length field in the http reply message after accessing the download link
 * @retval STATE_DOWNLOAD_RECV_HANDLE_IS_NULL The download handle is empty
 * @retval STATE_DOWNLOAD_RENEWAL_REQUEST_SENT performs breakpoint resumption and resends the download request to the firmware server.
 * @retval For other error information, please refer to @ref aiot_state_api.h
 **/
int32_t aiot_download_recv(void *handle); /*Return conditions: Network error | Verification error | EOF read | buf filled*/

/**
 * @brief Set download handle parameters
 *
 * @details
 *
 * Configure firmware download session options. Common options that need to be set include
 *
 * + `AIOT_DLOPT_RECV_HANDLER`: The user tells the SDK which user function to call to transfer the firmware content buffer when the SDK receives the firmware content.
 * + `AIOT_DLOPT_NETWORK_CRED`: You can configure whether to use HTTP or HTTPS to download firmware content
 * + `AIOT_DLOPT_BODY_BUFFER_MAX_LEN`: This is the length of the buffer. During SDK downloading, a user callback is called every time this length is filled. Therefore, the larger the setting here, the faster the download and the greater the memory overhead.
 *
 * @param[in] handle download handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_download_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_download_option_t
 *
 * @return int32_t
 * @retval STATE_SUCCESS parameter setting is successful
 * @retval STATE_DOWNLOAD_SETOPT_HANDLE_IS_NULL The download handle is empty
 * @retval STATE_DOWNLOAD_SETOPT_DATA_IS_NULL The data field is empty
 * @retval STATE_DOWNLOAD_SETOPT_COPIED_DATA_IS_NULL copy task_desc failed
 * @retval STATE_DOWNLOAD_SETOPT_MALLOC_SHA256_CTX_FAILED Failed to allocate memory for the context of shs256 algorithm
 * @retval STATE_DOWNLOAD_SETOPT_MALLOC_MD5_CTX_FAILED failed to allocate memory for the context of the MD5 algorithm.
 * @retval For other error information, please refer to @ref aiot_state_api.h
 **/
int32_t aiot_download_setopt(void *handle, aiot_download_option_t option, void *data);

/**
 * @brief Report the download completion percentage or error code
 *
 * @details
 *
 * After the device starts the process of downloading firmware, you can use this interface to report the progress to the cloud, including download progress or error information.
 *
 * + If the download is normal, it can be reported in the form of an integer. The currently downloaded content accounts for the percentage of the overall size of the firmware. The percent parameter will be automatically calculated by the SDK and passed to the user in the callback.
 * + If the download is abnormal or the firmware burning after download is abnormal, you can also use this interface to report the exception to the cloud. For the protocol error code convention between the device and the cloud, see @ref aiot_ota_protocol_errcode_t
 * + The content reported through @ref aiot_download_report_progress will affect the display of the console, such as displaying OTA upgrade progress, displaying OTA upgrade failure, etc.
 *
 * @param[in] handle download handle
 * @param[in] percent Percentage or error code of the completion of the currently downloaded content
 *
 * @return int32_t
 * @retval STATE_SUCCESS parameter setting is successful
 * @retval STATE_DOWNLOAD_REPORT_HANDLE_IS_NULL The handle is empty when reporting
 * @retval STATE_DOWNLOAD_REPORT_TASK_DESC_IS_NULL When reporting, task_desc is empty and the corresponding product_key and device_name cannot be found.
 * @retval For other error information, please refer to @ref aiot_state_api.h
 **/
int32_t aiot_download_report_progress(void *handle, int32_t percent);

/**
 * @brief Send GET firmware message request to the cloud
 *
 * @details
 *
 * After the device knows the firmware download address through the OTA message callback function, it can call this interface to download a piece of firmware.
 *
 * + This length can be set by the `AIOT_DLOPT_RANGE_START` and `AIOT_DLOPT_RANGE_END` options
 * + If no setting is made, the default SDK will request the entire firmware content, but the user will be notified once every time the user buffer is filled. The buffer length is configured with the `AIOT_DLOPT_BODY_BUFFER_MAX_LEN` option.
 *
 * @param[in] handle download handle, including the firmware URL and other information
 *
 * @return int32_t
 * @retval STATE_SUCCESS request sent successfully
 * @retval STATE_DOWNLOAD_REQUEST_HANDLE_IS_NULL handle is empty when sending GET request
 * @retval STATE_DOWNLOAD_REQUEST_URL_IS_NULL When sending a GET request, task_desc is not empty, but the url is empty.
 * @retval STATE_DOWNLOAD_SEND_REQUEST_FAILED When sending a GET request, the http underlying packet sending logic reports an error
 * @retval STATE_DOWNLOAD_REQUEST_TASK_DESC_IS_NULL The task_desc field is empty when sending a GET request
 * @retval For other error information, please refer to @ref aiot_state_api.h*/
int32_t aiot_download_send_request(void *handle);

/**
 * @brief -0x0900~-0x09FF expresses the status code of the SDK in the OTA Modules, and also includes the `STATE_DOWNLOAD_XXX` used when downloading
 **/
#define STATE_OTA_BASE                                              (-0x0900)

/**
 * @brief OTA firmware download completed, checksum verification successful
 **/
#define STATE_OTA_DIGEST_MATCH                                      (-0x0901)

/**
 * @brief Failed when reporting OTA download progress message or firmware version number message to the server
 **/
#define STATE_OTA_REPORT_FAILED                                     (-0x0902)

/**
 * @brief An error occurred when the OTA Modules received firmware content data
 **/
#define STATE_DOWNLOAD_RECV_ERROR                                   (-0x0903)

/**
 * @brief OTA Modules checksum signature verification error occurs when downloading firmware
 *
 * @details
 *
 * Error caused by the mismatch between the md5 or sha256 calculation result of the firmware and the expected value of the cloud notification
 **/
#define STATE_OTA_DIGEST_MISMATCH                                   (-0x0904)

/**
 * @brief OTA Modules made an error when parsing the MQTT downlink JSON message pushed by the server
 *
 * @details
 *
 * In the JSON message downloaded from the cloud, the target key cannot be found, and therefore the corresponding value cannot be found.
 **/
#define STATE_OTA_PARSE_JSON_ERROR                                  (-0x0905)

/**
 * @brief OTA Modules sends HTTP message and fails when requesting to download firmware
 *
 * @details
 *
 * The OTA Modules failed to send a GET request to the server where the firmware is stored.
 **/
#define STATE_DOWNLOAD_SEND_REQUEST_FAILED                          (-0x0906)

/**
 * @brief The downloaded firmware content of the OTA Modules has reached the end of the previously set range, and the download will not continue.
 *
 * @details
 *
 * When downloading according to the range, the location specified by the range_end field has been downloaded. If the user continues to try to download at this time, the SDK will return an error code to prompt the user.
 **/
#define STATE_DOWNLOAD_RANGE_FINISHED                               (-0x0907)

/**
 * @brief When the OTA Modules applied for memory to parse the JSON message, it did not obtain the required memory and the parsing failed.
 **/
#define STATE_OTA_PARSE_JSON_MALLOC_FAILED                          (-0x0908)

/**
 * @brief When destroying the OTA session instance, it is found that the session handle is empty and the destruction action is aborted.
 **/
#define STATE_OTA_DEINIT_HANDLE_IS_NULL                             (-0x0909)

/**
 * @brief When configuring the OTA session instance, it was found that the session handle was empty and the configuration action was aborted.
 **/
#define STATE_OTA_SETOPT_HANDLE_IS_NULL                             (-0x090A)

/**
 * @brief When configuring the OTA session instance, it was found that the configuration data was empty and the configuration action was aborted.
 **/
#define STATE_OTA_SETOPT_DATA_IS_NULL                               (-0x090B)

/**
 * @brief When destroying the download session instance, it is found that the session handle is empty and the destruction action is aborted.
 **/
#define STATE_DOWNLOAD_DEINIT_HANDLE_IS_NULL                        (-0x090C)

/**
 * @brief When configuring the download session instance, it was found that the session handle was empty and the configuration action was aborted.
 **/
#define STATE_DOWNLOAD_SETOPT_HANDLE_IS_NULL                        (-0x090D)

/**
 * @brief When configuring the download session instance, it was found that the configuration data was empty and the configuration action was aborted.
 **/
#define STATE_DOWNLOAD_SETOPT_DATA_IS_NULL                          (-0x090E)

/**
 * @brief When configuring the download session instance, an internal error occurred while synchronizing the configuration from the OTA session, and the configuration action was aborted.
 **/
#define STATE_DOWNLOAD_SETOPT_COPIED_DATA_IS_NULL                   (-0x090F)

/**
 * @brief When the directly connected device reports the version number, the OTA handle is empty and the report is aborted.
 **/
#define STATE_OTA_REPORT_HANDLE_IS_NULL                             (-0x0910)

/**
 * @brief When the directly connected device reports the version number, the version number string is empty, and the report is aborted.
 **/
#define STATE_OTA_REPORT_VERSION_IS_NULL                            (-0x0911)

/**
 * @brief When the directly connected device reports the version number, the MQTT handle is empty and the reporting is aborted.
 **/
#define STATE_OTA_REPORT_MQTT_HANDLE_IS_NULL                        (-0x0912)

/**
 * @brief When the gateway reports the version number for the sub-device, the OTA handle is empty and the report is aborted.
 **/
#define STATE_OTA_REPORT_EXT_HANELD_IS_NULL                         (-0x0913)

/**
 * @brief When the gateway reports the version number for the sub-device, the version number string is empty and the report is aborted.
 **/
#define STATE_OTA_REPORT_EXT_VERSION_NULL                           (-0x0914)

/**
 * @brief When the gateway reports the version number for the sub-device, the productKey of the sub-device is empty and the report is aborted.
 **/
#define STATE_OTA_REPORT_EXT_PRODUCT_KEY_IS_NULL                    (-0x0915)

/**
 * @brief When the gateway reports the version number for the sub-device, the sub-device deviceName is empty and the report is aborted.
 **/
#define STATE_OTA_REPORT_EXT_DEVICE_NAME_IS_NULL                    (-0x0916)

/**
 * @brief When the gateway reports the version number for the sub-device, the MQTT session handle is empty and the reporting is aborted.
 **/
#define STATE_OTA_REPORT_EXT_MQTT_HANDLE_IS_NULL                    (-0x0917)

/**
 * @brief When reporting download progress or OTA error code, the download session handle is empty and the execution is aborted.
 **/
#define STATE_DOWNLOAD_REPORT_HANDLE_IS_NULL                        (-0x0918)

/**
 * @brief When reporting download progress or OTA error code, the task description data structure is empty and the execution report is aborted.
 **/
#define STATE_DOWNLOAD_REPORT_TASK_DESC_IS_NULL                     (-0x0919)

/**
 * @brief When calling aiot_download_recv to receive firmware content, the user callback for processing the received data is null and execution is aborted.
 **/
#define STATE_DOWNLOAD_RECV_HANDLE_IS_NULL                          (-0x091A)

/**
 * @brief When calling aiot_download_send_request to send a firmware download request, the handle of the download session is empty and execution is aborted.
 **/
#define STATE_DOWNLOAD_REQUEST_HANDLE_IS_NULL                       (-0x091B)

/**
 * @brief When calling aiot_download_send_request to send a firmware download request, the task description is empty and execution is aborted.
 **/
#define STATE_DOWNLOAD_REQUEST_TASK_DESC_IS_NULL                    (-0x091C)

/**
 * @brief When calling aiot_download_send_request to send a firmware download request, the firmware URL in the task description is empty and execution is aborted.
 **/
#define STATE_DOWNLOAD_REQUEST_URL_IS_NULL                          (-0x091D)

/**
 * @brief When parsing the MQTT downlink message notifying OTA, the digest method is not md5 or sha256, and the SDK does not support it.
 **/
#define STATE_OTA_UNKNOWN_DIGEST_METHOD                             (-0x091E)

/**
 * @brief The collection of the entire firmware (rather than a single downloaded fragment) has been completed, and the cumulative number of bytes collected is consistent with the expected number of bytes of the firmware
 **/
#define STATE_DOWNLOAD_FINISHED                                     (-0x091F)

/**
 * @brief When the device sends a GET request to the firmware server, the Status Code in the HTTP message returned by the server is wrong, neither 200 nor 206.
 **/
#define STATE_DOWNLOAD_HTTPRSP_CODE_ERROR                           (-0x0920)

/**
 * @brief When the device sends a GET request to the firmware server, the HTTP message header returned by the server does not indicate Content-Length.
 **/
#define STATE_DOWNLOAD_HTTPRSP_HEADER_ERROR                         (-0x0921)

/**
 * @brief After the OTA firmware download failed, the breakpoint resume was in progress, and the SDK reinitiated the download request to the server.
 **/
#define STATE_DOWNLOAD_RENEWAL_REQUEST_SENT                         (-0x0922)

/**
 * @brief The OTA Modules encountered a failure when applying for memory to calculate the SHA256 checksum of the firmware.
 **/
#define STATE_DOWNLOAD_SETOPT_MALLOC_SHA256_CTX_FAILED              (-0x0923)

/**
 * @brief The OTA Modules encountered a failure when calculating the MD5 checksum memory of the firmware.
 **/
#define STATE_DOWNLOAD_SETOPT_MALLOC_MD5_CTX_FAILED                 (-0x0924)

/**
 * @brief When the OTA Modules parses the firmware download URL from the task description data structure, it encounters that the HOST field is empty and the parsing fails.
 **/
#define STATE_OTA_PARSE_URL_HOST_IS_NULL                            (-0x0925)

/**
 * @brief When the OTA Modules parses the firmware download URL from the task description data structure, it encounters that the PATH field is empty and the parsing fails.
 **/
#define STATE_OTA_PARSE_URL_PATH_IS_NULL                            (-0x0926)

/**
 * @brief When the OTA Modules downloads firmware in multiple segments, the cumulative total size of the multiple segments exceeds the expected value of the firmware.
 *
 * @details One possible reason is that the user divided the firmware into multiple ranges for downloading, but due to overlap between different ranges, etc., the final total download volume exceeded the total size of the firmware.
 **/
#define STATE_DOWNLOAD_FETCH_TOO_MANY                               (-0x0927)

/**
 * @brief When querying the cloud for OTA upgrade tasks, the OTA handle is empty.
 *
 * @details You need to call the aiot_ota_init function to initialize an OTA handle first, and then pass the handle to aiot_ota_query_firmware. If the handle has not been initialized, or
 * If the initialization is unsuccessful, this error will be reported.
 **/
#define STATE_OTA_QUERY_FIRMWARE_HANDLE_IS_NULL                     (-0x0928)

/**
 * @brief The host field in the url field in the OTA downlink message exceeds the length limit
 *
 * @details In the OTA downlink message, the URL for storing the firmware is included. The URL contains the host field. If the host field exceeds the limit (currently 1024 bytes), this error will be reported.
 **/
#define STATE_OTA_HOST_STRING_OVERFLOW                              (-0x0929)

/**
 * @brief The path field in the url field in the OTA downlink message exceeds the length limit
 *
 * @details The downlink message of OTA contains the URL where the firmware is stored. The URL contains the path field. If the path field exceeds the limit (currently 1024 bytes), this error will be reported.
 **/
#define STATE_OTA_PATH_STRING_OVERFLOW                              (-0x092A)

#if defined(__cplusplus)
}
#endif

#endif  /* #ifndef __AIOT_OTA_API_H__ */

