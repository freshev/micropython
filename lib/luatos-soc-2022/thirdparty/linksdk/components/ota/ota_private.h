#ifndef __OTA_PRIVATE_H__
#define __OTA_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"

#include "core_http.h"
#include "aiot_ota_api.h"

#define OTA_VERSION_TOPIC_PREFIX             "/ota/device/inform"
#define OTA_PROGRESS_TOPIC_PREFIX            "/ota/device/progress"

#define OTA_MODULE_NAME                      "OTA"
#define DOWNLOAD_MODULE_NAME                 "DOWNLOAD"

#define OTA_DEFAULT_DOWNLOAD_BUFLEN          (2 * 1024)
#define OTA_DEFAULT_DOWNLOAD_TIMEOUT_MS      (5 * 1000)

#define OTA_FOTA_TOPIC                       "/ota/device/upgrade/+/+"
#define OTA_FOTA_TOPIC_PREFIX                "/ota/device/upgrade"
#define OTA_COTA_PUSH_TOPIC                  "/sys/+/+/thing/config/push"
#define OTA_COTA_PUSH_TOPIC_POSTFIX          "/thing/config/push"
#define OTA_COTA_GET_REPLY_TOPIC             "/sys/+/+/thing/config/get_reply"
#define OTA_COTA_GET_REPLY_TOPIC_POSTFIX     "/thing/config/get_reply"
#define OTA_COTA_TOPIC_PREFIX                "/sys/"
#define OTA_GET_TOPIC_PREFIX                 "/sys"
#define OTA_GET_TOPIC_SUFFIX                 "thing/ota/firmware/get"
#define OTA_GET_REPLY_TOPIC_SUFFIX           "thing/ota/firmware/get_reply"
#define OTA_OTA_GET_REPLY_TOPIC              "/sys/+/+/thing/ota/firmware/get_reply"

#define OTA_HTTPCLIENT_MAX_URL_LEN           (256)
#define OTA_MAX_DIGIT_NUM_OF_UINT32          (20)
#define OTA_RESPONSE_PARTIAL                 (206)
#define OTA_RESPONSE_OK                      (200)
#define OTA_TOPIC_NUM                        (4)
#define OTA_MD5_LEN                          (32)
#define OTA_SHA256_LEN                       (64)

typedef enum {
    DOWNLOAD_STATUS_START,
    DOWNLOAD_STATUS_FETCH,
    DOWNLOAD_STATUS_RENEWAL,
} download_status_t;

typedef enum {
    OTA_TYPE_FOTA,
    OTA_TYPE_CONFIG_PUSH,
    OTA_TYPE_CONFIG_GET,
} ota_type_t;

/**
 * @brief The handle for processing mqtt messages during the OTA process. This handle is mainly used to receive firmware upgrade messages from the cloud through the mqtt protocol, including the URL of the firmware, etc.
 **/
typedef struct {
    void                        *userdata;            /*The component calls one of the input parameters of recv_handler and passes in user data.*/
    aiot_ota_recv_handler_t     recv_handler;         /*When the OTA mqtt message reaches the device, the user is notified of the callback*/
    aiot_sysdep_portfile_t      *sysdep;

    /*---- The above are configurable by users in the API ----*/
    /*----The following are all used internally by OTA and are not perceived by users ----*/

    void            *mqtt_handle;
    void            *module;
    void            *data_mutex;
} ota_handle_t;

/**
 * @brief handle to handle download tasks, this handle is mainly used to download firmware from the specified url through the http protocol
 **/
typedef struct {
    void
    *userdata;       /*One of the input parameters when the component calls recv_handler, passing in user data*/
    aiot_download_recv_handler_t       recv_handler;    /*Callback function when the device receives a segmented firmware message*/
    aiot_download_task_desc_t          *task_desc;      /*Target description information of a certain download activity, such as URL, etc.*/
    aiot_sysdep_portfile_t             *sysdep;
    uint32_t                           range_start;
    uint32_t                           range_end;

    /*---- The above are configurable by users in the API ----*/
    /*---- The following are all used internally by downloader and are not visible to users ----*/

    uint8_t         download_status;
    void            *http_handle;
    uint32_t        size_fetched;
    uint32_t        range_size_fetched;
    uint32_t        content_len;
    int32_t         percent;
    int32_t         http_rsp_status_code;
    void            *digest_ctx;
    void            *data_mutex;
    void            *recv_mutex;
} download_handle_t;

typedef struct {
    char *pos;
    int len;
} ota_list_json;

#define OTA_ARRAY_MAX (20)

void *_download_deep_copy_task_desc(aiot_sysdep_portfile_t *sysdep, void *data);
int32_t _download_deep_free_task_desc(aiot_sysdep_portfile_t *sysdep, void *data);
int32_t _ota_publish_base(void *handle, char *topic_prefix, char *product_key, char *device_name, char *suffix,
                          char *params);

#if defined(__cplusplus)
}
#endif

#endif  /* __OTA_PRIVATE_H__ */

