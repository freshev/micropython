/**
 * @file aiot_mqtt_download_api.h
 * @brief mqtt_download Modules header file, providing the ability to transfer mqtt files
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __AIOT_MQTT_DOWNLOAD_API_H__
#define __AIOT_MQTT_DOWNLOAD_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x1D00~-0x1DFF expresses the status code of the SDK in the mqtt_download Modules*/
#define STATE_MQTT_DOWNLOAD_BASE                                             (-0x1D00)

/**
 * @brief mqtt_download_handle is empty*/
#define STATE_MQTT_DOWNLOAD_MQTT_HANDLE_NULL                                 (-0x1D01)

/**
 * @brief The task is not initialized*/
#define STATE_MQTT_DOWNLOAD_TASK_DEINIT                                      (-0x1D02)

/**
 * @brief After initialization, execute the request for the first time*/
#define STATE_MQTT_DOWNLOAD_INIT                                             (-0x1D03)

/**
 * @brief Downloading file*/
#define STATE_MQTT_DOWNLOAD_ING                                              (-0x1D04)

/**
 * @brief The file has been downloaded but has not been verified*/
#define STATE_MQTT_DOWNLOAD_FINISHED                                         (-0x1D05)

/**
 * @brief The file download is completed and the verification is passed*/
#define STATE_MQTT_DOWNLOAD_SUCCESS                                          (-0x1D06)

/**
 * @brief File download and reception failed. It still failed after several retries, and it was considered a timeout.*/
#define STATE_MQTT_DOWNLOAD_FAILED_TIMEOUT                                   (-0x1D07)

/**
 * @brief When downloading the complete file, MD5 verification failed.*/
#define STATE_MQTT_DOWNLOAD_FAILED_MISMATCH                                  (-0x1D08)

/**
 * @brief Receive length verification error*/
#define STATE_MQTT_DOWNLOAD_FAILED_RECVERROR                                 (-0x1D09)

/**
 * @brief The user actively cancels the download task*/
#define STATE_MQTT_DOWNLOAD_ABORT                                            (-0x1D0A)

/**
 * @brief The download length set is empty*/
#define STATE_MQTT_DOWNLOAD_FILESIZE_ERROR                                   (-0x1D0B)



/**
 * @brief mqtt_download Modules notifies the user of the message type when receiving a message from the network*/
typedef enum {
    /**
     * @brief Return message of file fragment download request*/
    AIOT_MDRECV_DATA_RESP,
} aiot_mqtt_download_recv_type_t;

/**
 * When the @brief mqtt_download Modules receives a message from the network, it notifies the user of the message content*/
typedef struct {
    /**
     * @brief The message type corresponding to the message content. For more information, please refer to @ref aiot_mqtt_download_recv_type_t*/
    aiot_mqtt_download_recv_type_t  type;
    union {
        /**
         * @brief data_resp_type description*/
        struct {
            /**
             * @brief User standard file name, usually the set file name*/
            char *filename;
            /**
             * @brief file offset*/
            uint32_t offset;
            /**
             * @brief received data length*/
            uint32_t data_size;
            /**
             * @brief received data*/
            char *data;
            /**
             * @brief received data*/
            int32_t percent;
            /**
             * @brief The total size of the file*/
            int32_t file_lenth;
        } data_resp;
    } data;
} aiot_mqtt_download_recv_t;

/**
 * When the @brief mqtt_download Modules receives a message from the network, it notifies the user of the data callback function called
 *
 * @param[in] handle mqtt_download session handle
 * @param[in] packet mqtt_download message structure, which stores the content of the received mqtt_download message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_mqtt_download_recv_handler_t)(void *handle,
        const aiot_mqtt_download_recv_t *packet, void *userdata);


/**
 * @brief @ref aiot_mqtt_download_setopt optional value of the option parameter of the interface.
 *
 * @details The data type in each option below refers to the data type of the data parameter in @ref aiot_mqtt_download_setopt*/
typedef enum {
    /**
     * @brief Set the user callback function for processing OTA messages
     *
     * @details
     *
     * Data type: (aiot_mqtt_download_recv_handler_t)*/
    AIOT_MDOPT_RECV_HANDLE,

    /**
     * @brief Set MQTT handle
     *
     * @details
     *
     * The channel capability of MQTT is used during the file download process to request and receive data.
     *
     * Data type: (void *)*/
    AIOT_MDOPT_MQTT_HANDLE,

    /**
      * @brief User needs SDK temporary context
      *
      * @details
      *
      * This context will be passed back to the user in @ref AIOT_OTAOPT_RECV_HANDLER
      *
      * Data type: (void *)*/
    AIOT_MDOPT_USERDATA,
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
    AIOT_MDOPT_TASK_DESC,

    /**
     * @brief Set the starting address for downloading according to the range
     *
     * @details
     *
     * In the MQTT range request feature, it means downloading starts from the byte
     * If you specify to start downloading from the beginning, the value of start is 0
     *
     * Data type: (uint32_t *)
     *
     **/
    AIOT_MDOPT_RANGE_START,
    /**
    * @brief Set the end address of downloading according to range
    *
    * @details
    * In the MQTT range request feature, it means that downloading ends after reaching this byte.
    * If you specify to download from the beginning to end after 10 bytes,
    * You need to specify start = 0, end = 9, so a total of 10 bytes
    *
    * Data type: (uint32_t *)
    *
    **/
    AIOT_MDOPT_RANGE_END,

    /**
    * @brief The maximum length of data in a single request
    *
    * @details
    *
    * Data type: (uint32_t *) Default value: (2 *1024) Bytes*/
    AIOT_MDOPT_DATA_REQUEST_SIZE,

    AIOT_MDOPT_MAX,
} aiot_mqtt_download_option_t;

/**
 * @brief Create mqtt_download session instance and configure session parameters with default values
 *
 * @return void *
 * @retval The handle of the non-NULL mqtt_download instance
 * @retval NULL initialization failed, usually caused by memory allocation failure
 **/
void *aiot_mqtt_download_init(void);

/**
 * @brief configure mqtt_download session
 *
 * @param[in] handle mqtt_download session handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_mqtt_download_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_mqtt_download_option_t
 *
 * @return int32_t
 * @retval <STATE_SUCCESS parameter configuration failed
 * @retval >=STATE_SUCCESS Parameter configuration successful
 **/
int32_t aiot_mqtt_download_setopt(void *handle, aiot_mqtt_download_option_t option, void *data);

/**
 * @brief End the mqtt_download session, destroy the instance and recycle resources
 *
 * @param[in] handle pointer to the mqtt_download session handle
 *
 * @return int32_t
 * @retval <STATE_SUCCESS execution failed
 * @retval >=STATE_SUCCESS execution successful
 **/
int32_t aiot_mqtt_download_deinit(void **handle);

/**
 * @brief handles download logic
 *
 * @param handle mqtt_download session handle
 *
 * @return int32_t
 * @retval STATE_MQTT_DOWNLOAD_INIT initialization completed
 * @retval STATE_MQTT_DOWNLOAD_ING is downloading
 * @retval STATE_MQTT_DOWNLOAD_SUCCESS Download completed
 * @retval STATE_MQTT_DOWNLOAD_FAILED_RECVERROR data reception error
 * @retval STATE_MQTT_DOWNLOAD_FAILED_TIMEOUT receive timeout
 * @retval STATE_MQTT_DOWNLOAD_FAILED_MISMATCH verification error
 * @retval STATE_MQTT_DOWNLOAD_MQTT_HANDLE_NULL handle is not initialized
 * @retval STATE_MQTT_DOWNLOAD_TASK_DEINIT no task is set*/
int32_t aiot_mqtt_download_process(void *handle);



#if defined(__cplusplus)
}
#endif

#endif  /* __AIOT_MQTT_DOWNLOAD_API_H__ */

