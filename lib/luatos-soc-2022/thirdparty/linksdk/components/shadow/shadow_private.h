/**
 * @file shadow_private.h
 * @brief The macro definitions and data structure declarations within the shadow Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __SHADOW_PRIVATE_H__
#define __SHADOW_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*Include the header files of the standard C library this way*/
#include "core_stdinc.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_shadow_api.h"

/*The session handle structure inside the shadow Modules is not visible to SDK users, and only void *handle type pointers can be obtained.*/
typedef struct {
    aiot_sysdep_portfile_t *sysdep;
    void *mqtt_handle;

    aiot_shadow_recv_handler_t recv_handler;
    void *userdata;
} shadow_handle_t;

#define SHADOW_MODULE_NAME                  "shadow"

#define SHADOW_UPDATE_TOPIC_FMT             "/shadow/update/%s/%s"
#define SHADOW_GET_TOPIC                    "/shadow/get/+/+"

#define SHADOW_PAYLOAD_REQ_FMT              "{\"method\":\"%s\",\"state\":{\"reported\":%s},\"version\":%s}"
#define SHADOW_PAYLOAD_CLEAN_FMT            "{\"method\":\"update\",\"state\":{\"desired\":\"null\"},\"version\":%s}"
#define SHADOW_PAYLOAD_GET                  "{\"method\":\"get\"}"

#define SHADOW_JSON_KEY_METHOD              "method"
#define SHADOW_JSON_KEY_PAYLOAD             "payload"
#define SHADOW_JSON_KEY_STATUS              "status"
#define SHADOW_JSON_KEY_TIMESTAMP           "timestamp"
#define SHADOW_JSON_KEY_STATE               "state"
#define SHADOW_JSON_KEY_VERSION             "version"


#if defined(__cplusplus)
}
#endif
#endif  /* __SHADOW_PRIVATE_H__ */

