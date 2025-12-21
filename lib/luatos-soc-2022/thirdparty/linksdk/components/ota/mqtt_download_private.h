/**
 * @file mqtt_download_private.h
 * @brief The macro definitions and data structure declarations within the mqtt_download Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __MQTT_DOWNLOAD_PRIVATE_H__
#define __MQTT_DOWNLOAD_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_ota_api.h"
#include "aiot_mqtt_download_api.h"      /*Internal header files are a superset of user-visible header files*/

/*Define the session handle structure inside the mqtt_download Modules. It is not visible to SDK users and can only get pointers of void *handle type.*/
typedef struct {
    aiot_sysdep_portfile_t               *sysdep;       /*Reference pointer to the underlying dependency callback collection*/
    aiot_mqtt_download_recv_handler_t    recv_handler;  /*When the component reads content from the protocol stack, it notifies the user of the callback*/
    void                                 *userdata;     /*One of the input parameters when the component calls the above two mqtt_download_handlers*/
    aiot_download_task_desc_t            *task_desc;    /*Target description information of a certain download activity, such as URL, etc.*/
    uint32_t                             range_start;
    uint32_t                             range_end;
    uint32_t                             request_size;  /*The size of each request*/
    /*---- The above are configurable by users in the API ----*/
    uint32_t        msg_id;
    uint32_t        size_fetched;
    int32_t         percent;
    uint64_t        last_request_time;
    uint64_t        failed_counter;
    int32_t         status;
    int32_t         last_percent;
    uint32_t        range_size;
    int8_t          md5_enabled;

    void            *digest_ctx;
    void            *data_mutex;
    void            *recv_mutex;
} mqtt_download_handle_t;

/*Module name string used for memory statistics*/
#define MQTT_DOWNLOAD_MODULE_NAME                    "mqtt_download"

/*After sending a request, set the timeout for waiting for a reply*/
#define MQTT_DOWNLOAD_DEFAULT_RECV_TIMEOUT           (10 * 1000)
/*Default single request length*/
#define MQTT_DOWNLOAD_DEFAULT_REQUEST_SIZE           (5 * 1024)

/*Topic definitions for requests and responses*/
#define MQTT_DOWNLOAD_REQUEST_TOPIC                  "/sys/%s/%s/thing/file/download"
#define MQTT_DOWNLOAD_RESPONSE_TOPIC                 "/sys/%s/%s/thing/file/download_reply"

/*The maximum length of TOKEN*/
#define MQTT_DOWNLOAD_TOKEN_MAXLEN                   (128)
/*The interval for reporting progress, unit: %*/
#define MQTT_DOWNLOAD_REPORT_INTERNEL                (5)


#if defined(__cplusplus)
}
#endif
#endif  /* __MQTT_DOWNLOAD_PRIVATE_H__ */

