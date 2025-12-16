/**
 * @file dynreg_private.h
 * @brief dynreg The macro definitions and data structure declarations within the Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __DYNREG_PRIVATE_H__
#define __DYNREG_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*Include the header files of the standard C library this way*/
#include "core_stdinc.h"

/*TODO: This paragraph lists other Modules header files that need to be included in the SDK, separated from the previous paragraph by a blank line.*/
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "core_http.h"
#include "aiot_dynreg_api.h"      /*Internal header files are a superset of user-visible header files*/

/*TODO: Define the session handle structure inside the dynreg Modules. It is not visible to SDK users and can only get pointers of void *handle type.*/
typedef struct {

    aiot_sysdep_portfile_t     *sysdep;             /*Reference pointer to the underlying dependency callback collection*/
    aiot_sysdep_network_cred_t *cred;               /*Points to the security credentials used for the current connection*/

    char       *host;                               /*Session target server domain name*/
    uint16_t    port;                               /*Session target server port*/
    char       *product_key;
    char       *product_secret;
    char       *device_name;

    aiot_dynreg_recv_handler_t    recv_handler;       /*When the component reads content from the protocol stack, it notifies the user of the callback*/
    void *userdata;                                /*One of the input parameters when the component calls the above two dynreg_handlers*/

    uint32_t    recv_timeout_ms;                       /*The maximum waiting time when receiving packets from the protocol stack*/
    uint32_t    send_timeout_ms;                       /*The longest time it takes to write to the protocol stack*/
    uint32_t    timeout_ms;
    uint32_t    deinit_timeout_ms;

    /*---- The above are configurable by users in the API ----*/

    /*---- The following are used internally by DYNREG and are not visible to users ----*/
    void *http_handle;
    core_http_response_t response;
    uint32_t    response_body_len;
    uint8_t     exec_enabled;
    uint32_t    exec_count;

    void       *data_mutex;     /*Protect local data structures*/

} dynreg_handle_t;

#define DYNREG_MODULE_NAME                      "dynreg"  /*Module name string used for memory statistics*/

#define DYNREG_DEFAULT_TIMEOUT_MS               (5 * 1000)
#define DYNREG_DEFAULT_DEINIT_TIMEOUT_MS        (2 * 1000)
#define DYNREG_DEFAULT_RECV_TIMEOUT             (5 * 1000)
#define DYNREG_DEFAULT_SEND_TIMEOUT             (5 * 1000)

#define DYNREG_PATH                             "/auth/register/device"

#define DYNREG_DEINIT_INTERVAL_MS               (100)
#define DYNREG_RESPONSE_BODY_LEN                (192)

#if defined(__cplusplus)
}
#endif
#endif  /* __DYNREG_PRIVATE_H__ */

