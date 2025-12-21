/**
 * @file aiot_linkspeech_api.h
 * @brief LinkSpeech Modules header file, providing the device with the ability to transmit sounds thousands of miles away
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 * @details
 **/

#ifndef __AIOT_LINKSPEECH_API_H__
#define __AIOT_LINKSPEECH_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    /*This callback is executed after the audio broadcast ends, and userdata is required.*/
    void (*on_finish)(char *filename, void *userdata);
    void *userdata;
} play_param_t;

/*Set the audio playback callback. This interface cannot be blocked for a long time.*/
typedef int32_t (*player_cb_t)(char *filename, play_param_t *ext_params);

typedef enum {
    /**
     * @brief The MQTT handle that the Modules depends on
     *
     * @details
     *
     * The Qianli Transmission Modules relies on the underlying MQTT Modules. Users must configure the correct MQTT handle, otherwise it will not work properly.
     *
     * Data type: (void *)*/
    AIOT_LSOPT_MQTT_HANDLE,
    /**
     * @brief File system operations that the Modules depends on
     *
     * @details
     *
     * Qianli Transsion Modules relies on the file system interface and needs to be implemented and set by the user.
     *
     * Data type: (aiot_fs_t *)*/
    AIOT_LSOPT_FILE_OPS,
    /**
     * @brief interface settings for voice playback that the Modules depends on
     *
     * @details
     *
     * Qianli Transmission Modules relies on audio playback interface
     *
     * Data type: (player_cb_t *)*/
    AIOT_LSOPT_PLAYER_CALLBACK,
    /**
     * @brief Set the folder path where Qianli Transsion files are saved
     *
     * @details
     *
     * Data type: (char *)*/
    AIOT_LSOPT_WORK_DIR,
    /**
     * @brief Whether to enable https download, the default is http download
     *
     * @details
     *
     * Data type: (int32_t *) 0: http download 1: https download*/
    AIOT_LSOPT_HTTPS_ENABLE,
    /**
     * @brief The maximum cache number of combined broadcast tasks
     *
     * @details
     * When the combined broadcast speed is lower than the delivery speed, the SDK will cache the broadcast tasks. This parameter is used to set the number of cached broadcast tasks.
     *
     * Data type: (int32_t *) 1～255, default is 10*/
    AIOT_LSOPT_SPEECH_BUFFER_SIZE,

    AIOT_LSOPT_MAX,
} aiot_linkspeech_option_t;

/**
 * @brief Create a LinkSpeech instance
 *
 * @return void*
 * @retval non-NULL linkspeech instance handle
 * @retval NULL initialization failed, either because portfile was not set, or memory allocation failed.
 **/
void   *aiot_linkspeech_init();

/**
 * @brief Destroy LinkSpeech instance handle
 *
 * @param[in] handle pointer to linkspeech instance handle
 *
 * @return int32_t
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_linkspeech_deinit(void **handle);

/**
 * @brief Set the parameters of the LinkSpeech handle
 *
 * @details
 *
 * Configure LinkSpeech sessions. Common configuration options include
 *
 * + `AIOT_LSOPT_MQTT_HANDLE`: associate the MQTT session handle returned by @ref aiot_mqtt_init with the OTA session
 * + `AIOT_LSOPT_RECV_HANDLER`: Set the data processing callback for OTA messages. This user callback will be called by @ref aiot_mqtt_recv when there is an OTA message.
 *
 * @param[in] handle linkspeech handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_linkspeech_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_linkspeech_option_t
 *
 * @return int32_t
 * @retval STATE_USER_INPUT_UNKNOWN_OPTION option is not supported
 * @retval STATE_SUCCESS parameter setting is successful
 **/
int32_t aiot_linkspeech_setopt(void *handle, aiot_linkspeech_option_t option, void *data);


/**
 * @brief If you start the linkspeech service, it will always be blocked.
 *
 * @param[in] handle pointer to linkspeech instance handle
 *
 * @return int32_t
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_linkspeech_start(void *handle);

/**
 * @brief Close the linkspeech (Qianli Transmission) service, aiot_linkspeech_start will exit after execution
 *
 * @param[in] handle pointer to linkspeech instance handle
 *
 * @return int32_t
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_linkspeech_stop(void *handle);


#if defined(__cplusplus)
}
#endif

#endif  /* #ifndef __AIOT_LINKSPEECH_API_H__ */

