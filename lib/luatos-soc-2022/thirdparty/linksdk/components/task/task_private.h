
/**
 * @file task_private.h
 * @brief The macro definitions and data structure declarations within the task Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __TASK_PRIVATE_H__
#define __TASK_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*Include the header files of the standard C library this way*/
#include "core_stdinc.h"

/*TODO: This paragraph lists other Modules header files that need to be included in the SDK, separated from the previous paragraph by a blank line.*/
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_task_api.h"

#define TASK_MODULE_NAME                        "TASK"

#define TASK_NOTIFY_TOPIC                       "/sys/+/+/thing/job/notify_reply"
#define TASK_QUERY_RESPONSE_TOPIC               "/sys/+/+/thing/job/get_reply"

#define TASK_QUERY_TASK_TOPIC_FMT               "/sys/%s/%s/thing/job/get"
#define TASK_UPDATE_TASK_TOPIC_FMT              "/sys/%s/%s/thing/job/update"
#define TASK_NOTIFY_REPLY_TOPIC_FMT             "/sys/%s/%s/thing/job/notify_reply"

/*JSON format of ALINK request*/
#define TASK_REQUEST_QUERY_TASK_FMT             "{\"id\":\"%s\",\"version\":\"1.0.0\",\"params\":{\"taskId\":\"%s\"}}"
#define TASK_REQUEST_UPDATE_TASK_FMT            "{\"id\":\"%s\",\"version\":\"1.0.0\",\"params\":{\"taskId\":\"%s\",\"status\":\"%s\",\"progress\":%s,\"statusDetails\":%s}}"
#define TASK_REQUEST_UPDATE_TASK_NO_DETAIL_FMT  "{\"id\":\"%s\",\"version\":\"1.0.0\",\"params\":{\"taskId\":\"%s\",\"status\":\"%s\",\"progress\":%s}}"

#define TASK_NOTIFY_REPLY_FMT                   "{\"id\":\"%s\",\"code\":%s,\"data\":{}}"

#define TASK_GET_LIST_REPLY_ARRAY_MAX           10

#define TASK_GET_LIST_REPLY_TASK_ID             "$list"

typedef struct {
    aiot_sysdep_portfile_t      *sysdep;
    aiot_task_recv_handler_t    recv_handler;
    void *mqtt_handle;
    void                        *userdata;
    task_desc_t
    *default_task_desc;   /*The data structure used to store task descriptions in handle. Only one task is stored in handle. If the user wants to store multiple tasks, he or she can define a list externally.*/
    void                        *data_mutex;
} task_handle_t;

/*Structure definition containing downstream topics and corresponding processing functions*/
typedef struct {
    char *topic;
    aiot_mqtt_recv_handler_t func;
} task_recv_topic_map_t;

typedef struct {
    char *str;
    aiot_task_status_t status;
} task_status_map_t;

typedef struct {
    char *pos;
    int len;
} task_list_json;

#if defined(__cplusplus)
}
#endif
#endif  /* __TASK_PRIVATE_H__ */
