/**
 * @file devinfo_private.h
 * @brief The macro definitions and data structure declarations within the devinfo Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __DEVINFO_PRIVATE_H__
#define __DEVINFO_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"

/*TODO: This paragraph lists other Modules header files that need to be included in the SDK, separated from the previous paragraph by a blank line.*/
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_devinfo_api.h"      /*Internal header files are a superset of user-visible header files*/

typedef struct {
    aiot_sysdep_portfile_t     *sysdep;             /*Reference pointer to the underlying dependency callback collection*/
    void *mqtt_handle;

    aiot_devinfo_event_handler_t   event_handler;      /*When the internal running status of the component changes, the callback is notified to the user*/
    aiot_devinfo_recv_handler_t    recv_handler;       /*When the component reads content from the protocol stack, it notifies the user of the callback*/
    void *userdata;                                 /*One of the input parameters when the component calls the above two devinfo_handlers*/

    uint32_t    deinit_timeout_ms;

    /*---- The above are configurable by users in the API ----*/

    /*---- The following are used internally by DEVINFO and are not visible to users ----*/

    void       *data_mutex;     /*Protect local data structures*/
    uint8_t     exec_enabled;
    uint32_t    exec_count;
} devinfo_handle_t;

#define DEVINFO_MODULE_NAME                    "devinfo"  /*Module name string used for memory statistics*/

#define DEVINFO_DEFAULT_DEINIT_TIMEOUT_MS      (2 * 1000)

#define DEVINFO_UPDATE_TOPIC_FMT               "/sys/%s/%s/thing/deviceinfo/update"
#define DEVINFO_UPDATE_REPLY_TOPIC             "/sys/+/+/thing/deviceinfo/update_reply"
#define DEVINFO_DELETE_TOPIC_FMT               "/sys/%s/%s/thing/deviceinfo/delete"
#define DEVINFO_DELETE_REPLY_TOPIC             "/sys/+/+/thing/deviceinfo/delete_reply"

#define DEVINFO_DEINIT_INTERVAL_MS               (100)

#if defined(__cplusplus)
}
#endif
#endif  /* __DEVINFO_PRIVATE_H__ */

