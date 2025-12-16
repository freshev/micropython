/**
 * @file diag_private.h
 * @brief diag The macro definitions and data structure declarations within the Modules are not oriented to other Moduless, let alone users.
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 **/
#ifndef __DIAG_PRIVATE_H__
#define __DIAG_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"
#include "core_list.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_diag_api.h"

typedef struct {
    uint64_t last_check_time;
} diag_mqtt_process_t;

typedef struct {
    uint64_t timestamp;
    int32_t code;
    uint8_t *data;
    uint32_t data_len;
} diag_raw_data_t;

typedef struct {
    uint64_t timestamp;
    uint16_t code;
    char *module_name;
    char *level;
    char *desc;
    uint8_t qos;
    struct core_list_head linked_node;
} diag_desc_node_t;

typedef struct {
    uint8_t is_diag;                                /*Whether the diagnostic item is in diagnostic status*/
    char *level;                                    /*Alarm level*/
    uint64_t start_time;                            /*Statistics start time*/
    uint64_t stop_time;                             /*Statistics end time*/
    void *extra_data;                               /*Statistics node additional data*/
    struct core_list_head linked_node;
} diag_running_state_node_t;

typedef struct {
    uint32_t code;                                 /*Diagnostic item code*/
    char *name;                                     /*Diagnostic item name*/
    uint8_t is_reported;                            /*Whether it has been reported in the current statistical period*/
    uint64_t report_start_time;                     /*Current statistical period start time*/
    uint32_t alert_counts;                          /*The cumulative number of alarms generated in the current statistical period*/
    uint32_t stat_number;                           /*Current number of nodes*/
    uint32_t max_stat_number;                       /*Maximum number of statistical nodes*/
    uint8_t qos;                                    /*QoS value used when sending alarm messages to the cloud using MQTT*/
    void *mutex;                                    /*data protection lock*/
    struct core_list_head linked_list;
} diag_running_state_t;

typedef void (*diag_running_state_node_extra_clean_t)(void *handle, void *extra_data);
typedef int32_t (*diag_running_state_node_extra_stop_t)(void *handle, diag_running_state_node_t *node, uint32_t stat_idx, uint32_t stat_number, void *extra_data);
typedef int32_t (*diag_report_desc_append_t)(void *handle, diag_running_state_t *running_state, diag_running_state_node_t *node, char **desc);

typedef struct {
    diag_running_state_node_extra_clean_t extra_clean_cb;
    diag_running_state_node_extra_stop_t extra_stop_cb;
    diag_report_desc_append_t desc_append_cb;
} diag_stat_callback_t;

typedef struct {
    aiot_diag_config_t config;
    diag_running_state_t running_state;
    diag_stat_callback_t stat_cb;
} diag_stat_t;

typedef struct {
    uint32_t msgid;
} diag_alink_uplink_extra_data_t;

#define DIAG_STAT_ITEM_NUMBER                              (3)

typedef struct {
    aiot_sysdep_portfile_t     *sysdep;             /*Reference pointer to the underlying dependency callback collection*/
    void *mqtt_handle;

    uint8_t local_report_enabled;
    uint8_t cloud_report_enabled;

    diag_stat_t diag_stat[DIAG_STAT_ITEM_NUMBER];

    uint32_t deinit_timeout_ms;

    aiot_diag_event_handler_t   event_handler;      /*When the internal running status of the component changes, the callback is notified to the user*/
    aiot_diag_recv_handler_t    recv_handler;       /*When the component reads content from the protocol stack, it notifies the user of the callback*/
    void *userdata;                                 /*One of the input parameters when the component calls the above two diag_handlers*/

    /*---- The above are configurable by users in the API ----*/

    /*---- The following are used internally by DIAG and are not visible to users ----*/

    void       *data_mutex;     /*Protect local data structures*/

    uint8_t diag_status;       /*Local diagnostic Modules status, 0: stop, 1: start*/
    uint8_t cloud_switch;

    diag_mqtt_process_t mqtt_process;

    uint8_t exec_enabled;
    uint32_t exec_count;

} diag_handle_t;

typedef struct {
    uint32_t stat_idx;
    char *name;
    uint32_t code;
    aiot_diag_config_t def_config;
    uint32_t def_max_stat_number;
    diag_stat_callback_t def_stat_cb;
    uint8_t qos;
} diag_config_t;

#define DIAG_MODULE_NAME                                   "diag"  /*Module name string used for memory statistics*/

#define DIAG_DAFAULT_LOCAL_REPORT_ENABLED                  (1)
#define DIAG_DAFAULT_CLOUD_REPORT_ENABLED                  (1)
#define DIAG_DEFAULT_DEINIT_TIMEOUT_MS                     (2 * 1000)

/* MQTT connection diag default configuration */
#define DIAG_DEFAULT_MQTT_CONN_ENABLED                     (1)
#define DIAG_DEFAULT_MQTT_CONN_INTERVAL_MS                 (30 * 1000)
#define DIAG_DEFAULT_MQTT_CONN_WARNING_THRESHOLD           (200)
#define DIAG_DEFAULT_MQTT_CONN_FATAL_THRESHOLD             (500)
#define DIAG_DEFAULT_MQTT_CONN_MAX_STAT_NUMBER             (20)

/* MQTT heartbeag diag default configuration */
#define DIAG_DEFAULT_MQTT_HB_ENABLED                       (1)
#define DIAG_DEFAULT_MQTT_HB_INTERVAL_MS                   (30 * 1000)
#define DIAG_DEFAULT_MQTT_HB_WARNING_THRESHOLD             (800)
#define DIAG_DEFAULT_MQTT_HB_FATAL_THRESHOLD               (1500)
#define DIAG_DEFAULT_MQTT_HB_MAX_STAT_NUMBER               (20)

/* MQTT alink uplink default configuration */
#define DIAG_DEFAULT_ALINK_UPLINK_ENABLED                  (1)
#define DIAG_DEFAULT_ALINK_UPLINK_INTERVAL_MS              (30 * 1000)
#define DIAG_DEFAULT_ALINK_UPLINK_WARNING_THRESHOLD        (600)
#define DIAG_DEFAULT_ALINK_UPLINK_FATAL_THRESHOLD          (1000)
#define DIAG_DEFAULT_ALINK_UPLINK_MAX_STAT_NUMBER          (20)

#define DIAG_REPORT_TOPIC_FMT                              "/sys/%s/%s/thing/log/post"
#define DIAG_REPORT_PAYLOAD_FMT                            "{\"id\":\"%s\",\"version\":\"1.0\",\"params\":[{\"utcTime\":\"%s\"," \
                                                           "\"logLevel\":\"%s\",\"module\":\"%s\",\"code\":\"%s\",\"traceContext\":\"%s\",\"logContent\":\"%s\"}]}"

#define DIAG_DEINIT_INTERVAL_MS                            (100)
#define DIAG_MQTT_PROCESS_CHECK_INTERVAL_MS                (2000)
#define DIAG_REPORT_LEVEL_WARNING_STR                      "WARN"
#define DIAG_REPORT_LEVEL_FATAL_STR                        "FATAL"

#define DIAG_STATE_MQTT_BASE                               (STATE_MQTT_BASE)
#define DIAG_STATE_DM_BASE                                 (-0x0A00)

/* MQTT connection diag constant */
#define DIAG_MQTT_CONNECTION_STAT_INDEX                    (0)
#define DIAG_MQTT_CONNECTION_NAME_STR                      "DiagMqttConnection"
#define DIAG_TLV_MQTT_CONNECTION                           (0x0010)

/* MQTT heartbeat diag constant */
#define DIAG_MQTT_HEARTBEAT_STAT_INDEX                     (1)
#define DIAG_MQTT_HEARTBEAT_NAME_STR                       "DiagMqttHeartbeat"
#define DIAG_TLV_MQTT_HEARTBEAT                            (0x0020)

/* MQTT alink uplink diag constant */
#define DIAG_ALINK_UPLINK_STAT_INDEX                       (2)
#define DIAG_ALINK_UPLINK_NAME_STR                         "DiagAlinkUplink"
#define DIAG_TLV_ALINK_UPLINK                              (0x0030)
#define DIAG_TLV_ALINK_MSGID                               (0x0031)


/* internal state code */
#define STATE_DIAG_STOP_NODE_NOT_MATCH                     (-0x14FF)

#if defined(__cplusplus)
}
#endif
#endif  /* __DIAG_PRIVATE_H__ */

