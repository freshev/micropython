/**
 * @file dynregmq_private.h
 * @brief The macro definitions and data structure declarations within the dynregmq Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __DYNREGMQ_PRIVATE_H__
#define __DYNREGMQ_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*Include the header files of the standard C library this way*/
#include "core_stdinc.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_dynregmq_api.h"      /*Internal header files are a superset of user-visible header files*/

typedef struct {
    uint32_t code;
    uint8_t *content;
    uint32_t content_len;
    uint32_t content_total_len;
} core_mqtt_response_t;

/*Define the session handle structure inside the dynregmq Modules. It is not visible to SDK users and can only get pointers of void *handle type.*/
typedef struct {

    aiot_sysdep_portfile_t     *sysdep;             /*Reference pointer to the underlying dependency callback collection*/
    aiot_sysdep_network_cred_t *cred;               /*Points to the security credentials used for the current connection*/

    char       *host;                               /*Session target server domain name*/
    uint16_t    port;                               /*Session target server port*/
    char       *product_key;
    char       *product_secret;
    char       *device_name;
    uint8_t     flag_nowhitelist;                   /*Whether to use the whitelist-free function*/
    char
    *instance_id;                        /*Instance ID. When the user uses a self-purchased instance and uses the whitelist-free method, the instance ID needs to be set.*/

    aiot_dynregmq_recv_handler_t    recv_handler;   /*When the component reads content from the protocol stack, it notifies the user of the callback*/
    void       *userdata;                           /*One of the input parameters when the component calls the above two dynregmq_handlers*/

    uint32_t    recv_timeout_ms;                    /*The maximum waiting time when receiving packets from the protocol stack*/
    uint32_t    send_timeout_ms;                    /*The longest time it takes to write to the protocol stack*/
    uint32_t    timeout_ms;
    uint32_t    deinit_timeout_ms;

    /*---- The above are configurable by users in the API ----*/

    /*---- The following are all used internally by DYNREGMQ and are not visible to users ----*/
    void        *mqtt_handle;
    uint8_t     flag_completed;

    uint8_t     exec_enabled;
    uint32_t    exec_count;
    void       *data_mutex;     /*Protect local data structures*/

} dynregmq_handle_t;

#define DYNREGMQ_MODULE_NAME                      "dynregmq"  /*Module name string used for memory statistics*/

#define DYNREGMQ_DEFAULT_TIMEOUT_MS               (5 * 1000)
#define DYNREGMQ_DEFAULT_DEINIT_TIMEOUT_MS        (2 * 1000)
#define DYNREGMQ_DEFAULT_RECV_TIMEOUT             (5 * 1000)
#define DYNREGMQ_DEFAULT_SEND_TIMEOUT             (5 * 1000)

#define DYNREGMQ_DEINIT_INTERVAL_MS               (100)

#if defined(__cplusplus)
}
#endif
#endif  /* __DYNREGMQ_PRIVATE_H__ */

