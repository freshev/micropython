/**
 * @file aiot_task_api.h
 * @brief task Modules header file, providing task management capabilities
 * @date 2020-11-25
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 **/

#ifndef __AIOT_TASK_API_H__
#define __AIOT_TASK_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief -0x0B00~-0x0BFF expresses the status code of the SDK in the task Modules
 **/
#define STATE_TASK_BASE                                             (-0x0B00)

/**
 * @brief When destroying the task session instance, it is found that the session handle is empty and the destruction action is aborted.
 **/
#define STATE_TASK_DEINIT_HANDLE_IS_NULL                            (-0x0B01)

/**
 * @brief When configuring a task session instance, it was found that the session handle was empty and the configuration action was aborted.
 **/
#define STATE_TASK_SETOPT_HANDLE_IS_NULL                            (-0x0B02)

/**
 * @brief Log status code when receiving server notify downlink message*/
#define STATE_TASK_RECV_NOTIFY                                      (-0x0B03)

/**
 * @brief An error occurred when parsing the MQTT downstream JSON message pushed by the server*/
#define STATE_TASK_PARSE_NOTIFY_FAILED                              (-0x0B04)

/**
 * @brief When applying for memory to parse the JSON message, the required memory was not obtained and the parsing failed.*/
#define STATE_TASK_PARSE_JSON_MALLOC_FAILED                         (-0x0B05)

/**
 * @brief Log status code when receiving server notify downlink message*/
#define STATE_TASK_PARSE_JSON_ERROR                                 (-0x0B06)

/**
 * @brief received query task id is empty*/
#define STATE_TASK_QUERY_TASK_ID_IS_NULL                            (-0x0B07)

/**
 * @brief The log status code when receiving the server get list reply downlink message*/
#define STATE_TASK_RECV_GET_LIST_REPLY                              (-0x0B08)

/**
 * @brief When configuring a task session instance, it was found that the session handle was empty and the configuration action was aborted.
 **/
#define STATE_TASK_SETOPT_DATA_IS_NULL                              (-0x0B09)

/**
 * @brief The status setting is incorrect when configuring the task description.
 **/
#define STATE_TASK_UPDATE_STATUS_INVALID                            (-0x0B0A)

/**
 * @brief aiot_task_setopt The optional value of the option parameter of the interface.*/

/**
 * @brief update task task status_details can only be NULL or json string object
 **/
#define STATE_TASK_UPDATE_STATUS_DETAILS_INVALID                     (-0x0B0B)

typedef enum {
    /**
     * @brief Set MQTT handle
     *
     * @details
     *
     * The channel capability of MQTT is used in the OTA process to report the version number, progress, and error code to the cloud.
     *
     * Data type: (void *)*/
    AIOT_TASKOPT_MQTT_HANDLE,

    /**
     * @brief Set the user callback function for processing task messages
     *
     * @details
     *
     * Processing functions for data sent or returned from the cloud
     *
     * Data type: (void *)*/
    AIOT_TASKOPT_RECV_HANDLER,

    /**
     * @brief User needs SDK temporary context
     *
     * @details
     *
     * This context pointer will be passed to the user by the SDK when the callback set by AIOT_TASKOPT_RECV_HANDLER is called.
     *
     * Data type: (void *)*/
    AIOT_TASKOPT_USERDATA,
    AIOT_TASKOPT_MAX
} aiot_task_option_t;

/**
 * @brief The status of the task.*/
typedef enum {
    AIOT_TASK_STATUS_QUEUED,             /*Status set by the server: The task is in the queue and has not been pushed yet*/
    AIOT_TASK_STATUS_SENT,               /*Status set by the server: Task has been pushed*/
    AIOT_TASK_STATUS_IN_PROGRESS,        /*The status of the device side setting: task in progress. After the device starts executing a task, it will*/
    AIOT_TASK_STATUS_SUCCEEDED,          /*Status of device settings: Task completed*/
    AIOT_TASK_STATUS_FAILED,             /*Status of device settings: Task execution failed*/
    AIOT_TASK_STATUS_REJECTED,           /*The status of the device-side setting: The device refuses to execute the task*/
    AIOT_TASK_STATUS_CANCELLED,          /*Status set by the server: The task was canceled by the server*/
    AIOT_TASK_STATUS_REMOVED,            /*Status of server settings: Task deleted from server*/
    AIOT_TASK_STATUS_TIMED_OUT,          /*Status set by the server: Task execution timeout*/
    AIOT_TASK_STATUS_NOT_FOUND           /*Server setting status: No information related to this task was found*/
} aiot_task_status_t;

/**
 * @brief Downlink related data structure*/
typedef enum {
    AIOT_TASKRECV_NOTIFY,               /*Corresponding to the downstream topic /sys/{productKey}/{deviceName}/thing/job/notify, the cloud actively pushes it down with task details.*/
    AIOT_TASKRECV_GET_DETAIL_REPLY,     /*Corresponding to the downstream topic /sys/{productKey}/{deviceName}/thing/job/get_reply, it can be the details of a single task or a simple description of the task list*/
    AIOT_TASKRECV_GET_LIST_REPLY,       /*Corresponding to the downstream topic /sys/{productKey}/{deviceName}/thing/job/get_reply, it can be the details of a single task or a simple description of the task list*/
    AIOT_TASKRECV_UPDATE_REPLY          /*Corresponds to the downstream topic /sys/{productKey}/{deviceName}/thing/job/update_reply, which contains the result of the update of a certain task, that is, whether it was successful or not.*/
} aiot_task_recv_type_t;

/**
 * @brief Data structure of task description*/
typedef struct {
    char *task_id;                      /*Task ID*/
    aiot_task_status_t status;          /*Task status*/
    char *job_document;                 /*Task execution rules*/
    char *sign_method;                  /*How to sign files*/
    char *sign;                         /*signature of document*/
    char *document_file_url;            /*URL for downloading the task file*/
    char *status_details;               /*Customer-defined status is transparently transmitted to the cloud. Note that the format is a json object, such as "{\"key\": \"value\"", strlen("\"key\": \"value\"}"*/
    uint8_t progress;                   /*The progress of task processing, number from 0-100*/
    void *handle;                       /*Task processing handle*/
} task_desc_t;

/**
 * @brief A brief description of each task when pulling the list from the cloud*/
typedef struct {
    char *task_id;                      /*Task ID*/
    aiot_task_status_t status;          /*Task status*/
} task_summary_t;

/**
 * @brief Pull the data returned from the list from the cloud*/
typedef struct {
    uint32_t number;                    /*The size of the task list pulled from the cloud*/
    task_summary_t *tasks;              /*Pulled task array pointer*/
} task_get_list_reply_t;

/**
 * @brief Data returned when pulling task details from the cloud*/
typedef struct {
    uint32_t code;                      /*Code returned by the cloud*/
    task_desc_t task;                   /*Task description details*/
} task_get_detail_reply_t;

/**
 * @brief After updating the task status to the cloud, the data returned by the cloud*/
typedef struct {
    uint32_t code;                      /*Code returned by the cloud*/
    char *task_id;                      /*The task id returned after updating the task*/
    aiot_task_status_t status;          /*The status returned after updating the task*/
} task_update_reply_t;

/**
 * @brief The cloud actively delivers or updates the data returned by the task cloud*/
typedef struct {
    aiot_task_recv_type_t type;                     /*Returned data type*/
    union {
        task_desc_t notify;                         /*The cloud actively pushes task data*/
        task_get_list_reply_t get_list_reply;       /*Request the data returned by the task list*/
        task_get_detail_reply_t get_detail_reply;   /*Data returned by requesting task detailed status*/
        task_update_reply_t update_reply;           /*Update data returned by task status*/
    } data;
} aiot_task_recv_t;

/**
 * @brief The receiving callback function when the device receives the mqtt downlink message of the task
 *
 * @param[in] handle task instance handle
 * @param[in] recv Cloud downlink message
 * @param[in] userdata user context
 *
 * @return void*/
typedef void (* aiot_task_recv_handler_t)(void *handle, const aiot_task_recv_t *recv, void *userdata);

/**
 * @brief Create a task instance
 *
 * @return void*
 * @retval non-NULL task instance handle
 * @retval NULL initialization failed, either because portfile was not set, or memory allocation failed.
 **/
void   *aiot_task_init(void);

/**
 * @brief Destroy task instance handle
 *
 * @param[in] handle pointer to task instance handle
 *
 * @return int32_t
 * @retval STATE_USER_INPUT_NULL_POINTER handle or the address pointed to by handle is empty
 * @retval STATE_SUCCESS execution successful
 **/
int32_t aiot_task_deinit(void **handle);

/**
 * @brief Set the parameters of the task handle
 *
 * @details
 *
 * Configure task sessions. Common configuration options include
 *
 * @param[in] handle task handle
 * @param[in] option configuration option, for more information, please refer to @ref aiot_task_option_t
 * @param[in] data configuration option data, for more information, please refer to @ref aiot_task_option_t
 *
 * @return int32_t
 * @retval STATE_TASK_SETOPT_HANDLE_IS_NULL task handle is empty
 * @retval STATE_TASK_SETOPT_DATA_IS_NULL parameter data field is empty
 * @retval STATE_USER_INPUT_UNKNOWN_OPTION option is not supported
 * @retval STATE_SUCCESS parameter setting is successful
 **/
int32_t aiot_task_setopt(void *handle, aiot_task_option_t option, void *data);

/**
 * @brief Get the task list from the cloud
 *
 * @details
 *
 * Get the task list from the cloud
 *
 * @param[in] handle task handle
 *
 * @return int32_t
 * @retval STATE_TASK_SETOPT_DATA_IS_NULL The handle field of the parameter is empty
 * @retval STATE_SUCCESS sent successfully*/
int32_t aiot_task_get_task_list(void *handle);

/*Send the message to /sys/{productKey}/{deviceName}/thing/job/get. If the function input parameter user_task_id is not empty, the payload of the upstream message is "taskId": user_task_id, and returns the details of the task;*/
/*If user_task_id is empty, the payload of the uplink message is "taskId": "$next", and the cloud returns the task with the highest time in the task queue that is not in the final state. The task status is QUEUED, SENT, or IN_PROGRESS. one*/

/**
 * @brief Get task details from the cloud
 *
 * @details
 *
 * Get task details from the cloud
 *
 * @param[in] handle task handle
 * @param[in] user_task_id task id or $next
 *
 * @return int32_t
 * @retval STATE_TASK_SETOPT_DATA_IS_NULL or the handle field of the user_task_id parameter is empty
 * @retval STATE_SUCCESS sent successfully
 **/
int32_t aiot_task_get_task_detail(void *handle, char *user_task_id);

/**
 * @brief Update task status to the cloud
 *
 * @details
 *
 * Update task status to the cloud
 *
 * @param[in] handle task handle
 * @param[in] task task information
 *
 * @return int32_t
 * @retval STATE_TASK_SETOPT_DATA_IS_NULL or the handle field of the task parameter is empty
 * @retval STATE_SUCCESS Update successful
 **/
int32_t aiot_task_update(void *handle, task_desc_t *task);
#if defined(__cplusplus)
}
#endif

#endif  /* #ifndef __AIOT_TASK_API_H__ */


