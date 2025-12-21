/**
 * @file ntp_private.h
 * @brief The macro definitions and data structure declarations within the ntp Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __NTP_PRIVATE_H__
#define __NTP_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*Include the header files of the standard C library this way*/
#include "core_stdinc.h"

/*TODO: This paragraph lists other Modules header files that need to be included in the SDK, separated from the previous paragraph by a blank line.*/
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_ntp_api.h"      /*Internal header files are a superset of user-visible header files*/

/*TODO: Define the session handle structure inside the ntp Modules. It is not visible to SDK users and can only get pointers of void *handle type.*/
typedef struct {
    aiot_sysdep_portfile_t     *sysdep;             /*Reference pointer to the underlying dependency callback collection*/
    void *mqtt_handle;

    int8_t time_zone;
    uint32_t deinit_timeout_ms;

    aiot_ntp_recv_handler_t    recv_handler;       /*When the component reads content from the protocol stack, it notifies the user of the callback*/
    aiot_ntp_event_handler_t   event_handler;
    void *userdata;                                 /*One of the input parameters when the component calls the above two ntp_handlers*/

    /*---- The above are configurable by users in the API ----*/
    void *data_mutex;

    uint8_t exec_enabled;
    uint32_t exec_count;

} ntp_handle_t;

#define NTP_MODULE_NAME                      "ntp"  /*Module name string used for memory statistics*/

#define NTP_DEFAULT_DEINIT_TIMEOUT_MS        (2 * 1000)
#define NTP_DEFAULT_TIME_ZONE                (0)

#define NTP_REQUEST_TOPIC_FMT                "/ext/ntp/%s/%s/request"
#define NTP_REQUEST_PAYLOAD_FMT              "{\"deviceSendTime\":\"%s\"}"
#define NTP_RESPONSE_TOPIC_FMT               "/ext/ntp/%s/%s/response"

#define NTP_DEINIT_INTERVAL_MS               (100)

#if defined(__cplusplus)
}
#endif
#endif  /* __NTP_PRIVATE_H__ */

