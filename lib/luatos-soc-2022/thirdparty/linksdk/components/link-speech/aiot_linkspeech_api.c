#include "aiot_linkspeech_api.h"
#include "aiot_fs_api.h"
#include "aiot_dm_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_state_api.h"
#include <stdio.h>
#include <cJSON.h>
#include <string.h>
#include "core_string.h"
#include "http_download.h"
#include "core_list.h"
#include "speech_trans.h"
#include "core_log.h"

#define TAG "LINKSPEECH"

/*File system abstract interface*/
static aiot_fs_t *fs = NULL;
/*operating system abstract interface*/
static aiot_sysdep_portfile_t *sysdep = NULL;
#define AIOT_LSOPT_SPEECH_BUFFER_SIZE_DEFAULT  10
#define AIOT_SPEECH_DUPLICATE_SIZE_DEFAULT  15

typedef struct {
    char *jobcode;
    char *json_url;
    struct core_list_head linked_node;
} core_download_node_t;

typedef struct {
    uint64_t msg_id;
    char *rrpc_id;
    char *id;
    char *format;
    char *speechs;
    struct core_list_head play_list;
    struct core_list_head linked_node;
} core_speech_task_t;

typedef struct {
    /*object model object*/
    void *dm_handle;
    /*audio playback interface*/
    player_cb_t player;
    /*User settings working directory*/
    char *work_dir;
    int32_t https_enable;
    int32_t speech_buffer_size;

    /*The above content is user configuration, the following is runtime status*/

    /*Download task management*/
    void *download_mutex;
    struct core_list_head download_task_list;
    /*Broadcast task management*/
    void *speech_task_mutex;
    int32_t speech_task_num;
    struct core_list_head speech_task_list;
    /*Used to determine whether it is necessary to exit Qianli Transsion*/
    int8_t running;
    /*Task being reported*/
    core_speech_task_t *playing_task;
    /*Play audio callback parameters*/
    play_param_t play_param;
    /*Prevent repeat playback*/
    char *dup_id[AIOT_SPEECH_DUPLICATE_SIZE_DEFAULT];
    int8_t write_pos;
} linkspeech_handle_t;

typedef struct {
    linkspeech_handle_t *linkspeech;
    char *file_name;
} download_userdata_t;

static int32_t file_write(uint32_t offset, uint8_t *data, uint32_t data_len, void *userdata) {
    char *file_name = (char *)userdata;
    if(userdata == NULL || data == NULL) {
        return -1;
    }

    core_log3(sysdep, 0, "file %s, offset %d, len %d\r\n", file_name, &offset, &data_len);

    return fs->file_write(file_name, offset, data, data_len);
}

/*Download file to local file system*/
static int32_t core_download_file(const char *remote_url, const char *local_url, int32_t tls) {
    http_download_params_t params;
    core_log2(sysdep, 0, "[http] remote %s, local %s\r\n", (char *)remote_url, (char *)local_url);

    if(tls == 0) {
        return core_http_download_request((char *)remote_url, NULL, file_write, (void *)local_url);
    } else {
        memset(&params, 0, sizeof(http_download_params_t));
        params.https_enable = 1;
        return core_http_download_request((char *)remote_url, &params, file_write, (void *)local_url);
    }
}

/*Event reporting function demonstration*/
static int32_t core_send_event_post(void *dm_handle, char *event_id, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_EVENT_POST;
    msg.data.event_post.event_id = event_id;
    msg.data.event_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

static int32_t core_delete_speech_task(core_speech_task_t *speech_task)
{
    play_node_t *node = NULL, *next = NULL;
    if(speech_task == NULL) {
        return -1;
    }

    if(speech_task->rrpc_id != NULL) {
        sysdep->core_sysdep_free(speech_task->rrpc_id);
    }

    if(speech_task->id != NULL) {
        sysdep->core_sysdep_free(speech_task->id);
    }

    if(speech_task->format != NULL) {
        sysdep->core_sysdep_free(speech_task->format);
    }

    if(speech_task->speechs != NULL) {
        sysdep->core_sysdep_free(speech_task->speechs);
    }

    core_list_for_each_entry_safe(node, next, &speech_task->play_list, linked_node, play_node_t) {
        core_list_del(&node->linked_node);
        if(node->filename != NULL) {
            sysdep->core_sysdep_free(node->filename);
        }
        sysdep->core_sysdep_free(node);
    }

    sysdep->core_sysdep_free(speech_task);

    return STATE_SUCCESS;
}

/*Convert corpus information into corpus file path*/
static int32_t core_speech_task_prepare(core_speech_task_t *speech_task)
{
    cJSON *speechs = cJSON_Parse(speech_task->speechs);
    char *value = NULL;
    /*If it is json format data*/
    if (speechs == NULL) {
        return -1;
    }

    /*Add corpus information to playlist*/
    CORE_INIT_LIST_HEAD(&speech_task->play_list);
    for (int i = 0; i < cJSON_GetArraySize(speechs); i++)
    {
        value = cJSON_GetArrayItem(speechs, i)->valuestring;
        if(*value == '{' && *(value + 1)== '$') {
            money_to_speech(value + 2, speech_task->format, &speech_task->play_list);
        } else {
            name_to_speech(value, speech_task->format, &speech_task->play_list);
        }
    }

    cJSON_Delete(speechs);
    return STATE_SUCCESS;
}

/*Check whether it is played repeatedly. If it is not played repeatedly, add it to the replay report list.*/
static int32_t core_speech_duplicate_check(linkspeech_handle_t *linkspeech, core_speech_task_t *speech_task)
{
    int i = 0;

    /*Check for plagiarism*/
    for(i = 0; i < AIOT_SPEECH_DUPLICATE_SIZE_DEFAULT; i++) {
        if(linkspeech->dup_id[i] != NULL && 0 == strcmp(speech_task->id, linkspeech->dup_id[i])) {
            return -1;
        }
    }

    /*Add to*/
    if(linkspeech->dup_id[linkspeech->write_pos] != NULL) {
        sysdep->core_sysdep_free(linkspeech->dup_id[linkspeech->write_pos]);
        linkspeech->dup_id[linkspeech->write_pos] = NULL;
    }

    core_strdup(sysdep, &linkspeech->dup_id[linkspeech->write_pos], speech_task->id, TAG);
    linkspeech->write_pos++;
    if(linkspeech->write_pos >= AIOT_SPEECH_DUPLICATE_SIZE_DEFAULT) {
        linkspeech->write_pos = 0;
    }

    return STATE_SUCCESS;
}

/*Check whether the corpus file exists*/
static int32_t core_speech_task_file_check(linkspeech_handle_t *linkspeech, core_speech_task_t *speech_task)
{
    play_node_t *node = NULL, *next = NULL;
    char *path = NULL;
    char *src[2] = { NULL, NULL};
    int32_t res = STATE_SUCCESS;

    res = core_speech_task_prepare(speech_task);
    if(res != STATE_SUCCESS ) {
        return -1;
    }

    core_list_for_each_entry_safe(node, next, &speech_task->play_list, linked_node, play_node_t) {
        src[0] = linkspeech->work_dir;
        src[1] = node->filename;
        path = NULL;
        core_sprintf(sysdep, &path, "%s/%s", src, sizeof(src)/sizeof(char *), TAG);
        sysdep->core_sysdep_free(node->filename);
        node->filename = path;
        if(fs->file_size(node->filename) <= 0) {
            core_log1(sysdep, 0, "loss file %s\r\n", node->filename);
            res = -1;
        }
    }
    return res;
}

static int32_t send_speech_task_result(linkspeech_handle_t *linkspeech, core_speech_task_t* speech_task, int32_t code)
{
    aiot_dm_msg_t msg;
    int32_t res = STATE_SUCCESS;
    char *data = NULL;
    char *src[] = { NULL, NULL, NULL };

    if(linkspeech == NULL || speech_task == NULL) {
        return -1;
    }

    switch(code){
        case 1:{
            src[0] = "1";
            src[1] = "duplicate speechbroad id";
        }
        break;
        case 2:{
            src[0] = "2";
            src[1] = "speechs file not found";
        }
        break;
        default:{
            src[0] = "0";
            src[1] = "success";
        }
        break;
    }
    src[2] = speech_task->id;
    core_sprintf(sysdep, &data, "{\"result\":%s,\"error_message\":\"%s\",\"task_id\":\"%s\"}", src, sizeof(src) / sizeof(char *), TAG );

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    if(speech_task->rrpc_id != NULL) {
        msg.type = AIOT_DMMSG_SYNC_SERVICE_REPLY;
        msg.data.sync_service_reply.rrpc_id = speech_task->rrpc_id;
        msg.data.sync_service_reply.msg_id = speech_task->msg_id;
        msg.data.sync_service_reply.service_id = "SyncSpeechBroadcast";
        msg.data.sync_service_reply.code = 200;
        msg.data.sync_service_reply.data = data;
    } else {
        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_ASYNC_SERVICE_REPLY;
        msg.data.async_service_reply.msg_id = speech_task->msg_id;
        msg.data.async_service_reply.code = 200;
        msg.data.async_service_reply.service_id = "SpeechBroadcast";
        msg.data.async_service_reply.data = data;
    }

    res = aiot_dm_send(linkspeech->dm_handle, &msg);
    sysdep->core_sysdep_free(data);

    return res;
}

static core_speech_task_t* core_speech_update_task(linkspeech_handle_t *linkspeech)
{
    core_speech_task_t* speech_task = NULL;

    sysdep->core_sysdep_mutex_lock(linkspeech->speech_task_mutex);
    if(core_list_empty(&linkspeech->speech_task_list)) {
        sysdep->core_sysdep_mutex_unlock(linkspeech->speech_task_mutex);
        return NULL;
    }

    speech_task = core_list_first_entry(&linkspeech->speech_task_list, core_speech_task_t, linked_node);
    core_list_del(&speech_task->linked_node);
    sysdep->core_sysdep_mutex_unlock(linkspeech->speech_task_mutex);

    return speech_task;
}

static int32_t core_check_play_task(linkspeech_handle_t *linkspeech)
{
    play_node_t *node = NULL;

    if( linkspeech->playing_task == NULL ) {
        linkspeech->playing_task = core_speech_update_task(linkspeech);
        if(linkspeech->playing_task == NULL || core_list_empty(&linkspeech->playing_task->play_list)) {
            return -1;
        }
    } else {
        /*After playing a single file, delete it*/
        node = core_list_first_entry(&linkspeech->playing_task->play_list, play_node_t, linked_node);
        core_list_del(&node->linked_node);
        sysdep->core_sysdep_free(node->filename);
        sysdep->core_sysdep_free(node);

        /*Query whether the playback task is completed*/
        if(core_list_empty(&linkspeech->playing_task->play_list)) {
            /*Asynchronous reply task playback completed*/
            if(linkspeech->playing_task->rrpc_id == NULL) {
                send_speech_task_result(linkspeech, linkspeech->playing_task, 0);
            }
            /*Recycle task resources*/
            sysdep->core_sysdep_mutex_lock(linkspeech->speech_task_mutex);
            core_list_del(&linkspeech->playing_task->linked_node);
            core_delete_speech_task(linkspeech->playing_task);
            linkspeech->speech_task_num--;
            sysdep->core_sysdep_mutex_unlock(linkspeech->speech_task_mutex);

            /*Update the next broadcast task*/
            linkspeech->playing_task = core_speech_update_task(linkspeech);
            if(linkspeech->playing_task == NULL || core_list_empty(&linkspeech->playing_task->play_list)) {
                return -1;
            }
        }
    }

    if( linkspeech->playing_task != NULL && linkspeech->player != NULL ) {
        node = core_list_first_entry(&linkspeech->playing_task->play_list, play_node_t, linked_node);
        linkspeech->player(node->filename, &linkspeech->play_param);
    }

    return STATE_SUCCESS;
}

static core_speech_task_t *core_speech_broadcast_parse(linkspeech_handle_t *linkspeech, char *data, int datalen)
{
    cJSON *speechs = NULL, *format = NULL, *id = NULL;
    core_speech_task_t *speech_task = NULL;
    cJSON *root = cJSON_Parse(data);

    /*If it is json format data*/
    if (root == NULL) {
        return NULL;
    }

    speechs = cJSON_GetObjectItem(root, "speechs");
    if (speechs == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    format = cJSON_GetObjectItem(root, "format");
    if (format == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    id = cJSON_GetObjectItem(root, "id");
    if (id == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    speech_task = sysdep->core_sysdep_malloc(sizeof(core_speech_task_t), TAG);
    if(speech_task == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    memset(speech_task, 0, sizeof(*speech_task));
    CORE_INIT_LIST_HEAD(&speech_task->play_list);

    core_strdup(sysdep, &speech_task->format, format->valuestring, TAG);
    core_strdup(sysdep, &speech_task->id, id->valuestring, TAG);
    speech_task->speechs = cJSON_Print(speechs);
    cJSON_Delete(root);

    return speech_task;
}

/*Parse the corpus download task and execute it*/
static int32_t core_speech_download_parse(linkspeech_handle_t *linkspeech, char *data, int datalen)
{
    char *jobcode = NULL;
    char *value = NULL;
    uint32_t value_len = 0;
    core_download_node_t *node = NULL;

    /*Parse and download jobcode*/
    int res = core_json_value(data, datalen,
                              "jobcode", strlen("jobcode"), &value, &value_len);
    if (res != STATE_SUCCESS) {
        return -1;
    }

    jobcode = sysdep->core_sysdep_malloc(value_len + 1, TAG);
    memcpy(jobcode, value, value_len);
    jobcode[value_len] = 0;

    res = core_json_value(data, datalen,
                          "url", strlen("url"), &value, &value_len);
    if (res != STATE_SUCCESS) {
        sysdep->core_sysdep_free(jobcode);
        return -1;
    }

    /*Record the downloaded URL and jobcode and insert them into the download task list*/
    node = (core_download_node_t *)sysdep->core_sysdep_malloc(sizeof(core_download_node_t), TAG);
    memset(node, 0, sizeof(*node));
    node->json_url = sysdep->core_sysdep_malloc(value_len + 1, TAG);
    memcpy(node->json_url, value, value_len);
    *(node->json_url + value_len) = 0;
    node->jobcode = jobcode;

    sysdep->core_sysdep_mutex_lock(linkspeech->download_mutex);
    core_list_add_tail(&node->linked_node, &linkspeech->download_task_list);
    sysdep->core_sysdep_mutex_unlock(linkspeech->download_mutex);
    return STATE_SUCCESS;
}

/*Delete corpus in batches*/
static int32_t core_speech_delete_parse(linkspeech_handle_t *linkspeech, char *data, uint32_t datalen)
{
    cJSON *root = NULL, *speechs = NULL, *speech = NULL;
    cJSON *formart, *id;
    char *filename = NULL;
    char *src[3] = {NULL, NULL, NULL};
    int32_t res = STATE_SUCCESS;

    //If it is json format data
    root = cJSON_Parse(data);
    if (root == NULL) {
        return -1;
    }

    speechs = cJSON_GetObjectItem(root, "speechs");
    if (speechs == NULL) {
        cJSON_Delete(root);
        return -1;
    }

    for (int i = 0; i < cJSON_GetArraySize(speechs); i++)
    {
        speech = cJSON_GetArrayItem(speechs, i);
        if(speech != NULL) {
            formart = cJSON_GetObjectItem(speech, "format");
            id = cJSON_GetObjectItem(speech, "id");
            src[0] = linkspeech->work_dir;
            src[1] = id->valuestring;
            src[2] = formart->valuestring;
            core_sprintf(sysdep, &filename, "%s/%s.%s", src, sizeof(src) / sizeof(char *), TAG );
            /*Delete the file if it exists*/
            if(fs->file_size(filename) > 0) {
                fs->file_delete(filename);
            }
            sysdep->core_sysdep_free(filename);
        }
    }
    cJSON_Delete(root);

    return res;
}


/*Object model service call processing*/
static void core_dm_recv_async_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    linkspeech_handle_t *linkspeech = (linkspeech_handle_t *)userdata;
    core_speech_task_t *speech_task = NULL;
    aiot_dm_msg_t msg;
    int32_t res = STATE_SUCCESS;

    if(linkspeech == NULL) {
        return;
    }

    /*message reply*/
    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_ASYNC_SERVICE_REPLY;
    msg.data.async_service_reply.msg_id = recv->data.async_service_invoke.msg_id;
    msg.data.async_service_reply.code = 200;
    msg.data.async_service_reply.service_id = recv->data.async_service_invoke.service_id;
    msg.data.async_service_reply.data = "{}";

    if(0 == strcmp(recv->data.async_service_invoke.service_id, "SpeechPost")) {
        /*Corpus distribution*/
        core_speech_download_parse(linkspeech, recv->data.async_service_invoke.params, recv->data.async_service_invoke.params_len);
        res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            core_log(sysdep, 0, "aiot_dm_send failed\r\n");
        }
    } else if(0 == strcmp(recv->data.async_service_invoke.service_id, "DeleteSpeech")) {
        /*Batch deletion of corpus*/
        core_speech_delete_parse(linkspeech, recv->data.async_service_invoke.params, recv->data.async_service_invoke.params_len);
        res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            core_log(sysdep, 0, "aiot_dm_send failed\r\n");
        }
    }
    else if(0 == strcmp(recv->data.async_service_invoke.service_id, "SpeechBroadcast")) {
        if(linkspeech->speech_task_num > linkspeech->speech_buffer_size) {
            core_log2(sysdep, 0, "SpeechBroadcast task buff full [%d] > [%d]\r\n", &linkspeech->speech_task_num, &linkspeech->speech_buffer_size);
            return;
        }
        /*Corpus combination broadcast*/
        speech_task = core_speech_broadcast_parse(linkspeech, recv->data.async_service_invoke.params, recv->data.async_service_invoke.params_len);
        if(speech_task != NULL) {
            speech_task->rrpc_id = NULL;
            speech_task->msg_id = recv->data.async_service_invoke.msg_id;
            if(STATE_SUCCESS != core_speech_duplicate_check(linkspeech, speech_task)) {
                core_log1(sysdep, 0, "SpeechBroadcast duplicate id [%s]\r\n", speech_task->id);
                send_speech_task_result(linkspeech, speech_task, 1);
                core_delete_speech_task(speech_task);
                return;
            } else if(STATE_SUCCESS != core_speech_task_file_check(linkspeech, speech_task)){
                send_speech_task_result(linkspeech, speech_task, 2);
                core_delete_speech_task(speech_task);
                return;
            } else {
                sysdep->core_sysdep_mutex_lock(linkspeech->speech_task_mutex);
                core_list_add_tail(&speech_task->linked_node, &linkspeech->speech_task_list);
                linkspeech->speech_task_num++;
                sysdep->core_sysdep_mutex_unlock(linkspeech->speech_task_mutex);
                if( linkspeech->playing_task == NULL ) {
                    core_check_play_task(linkspeech);
                }
            } 
        }
    }
}

/*Object model synchronization service call processing*/
static void core_dm_recv_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    linkspeech_handle_t *linkspeech = (linkspeech_handle_t *)userdata;
    core_speech_task_t *speech_task = NULL;

    if(0 == strcmp(recv->data.sync_service_invoke.service_id, "SyncSpeechBroadcast")) {
        if(linkspeech->speech_task_num > linkspeech->speech_buffer_size) {
            core_log2(sysdep, 0, "SpeechBroadcast task buff full [%d] > [%d]\r\n", &linkspeech->speech_task_num, &linkspeech->speech_buffer_size);
            return;
        }
        /*Corpus combination broadcast*/
        speech_task = core_speech_broadcast_parse(linkspeech, recv->data.sync_service_invoke.params, recv->data.sync_service_invoke.params_len);
        if(speech_task != NULL) {
            speech_task->rrpc_id = NULL;
            core_strdup(sysdep, &speech_task->rrpc_id, recv->data.sync_service_invoke.rrpc_id, TAG);
            speech_task->msg_id = recv->data.async_service_invoke.msg_id;
            if(STATE_SUCCESS != core_speech_duplicate_check(linkspeech, speech_task)) {
                core_log1(sysdep, 0, "SpeechBroadcast duplicate id [%s]\r\n", speech_task->id);
                send_speech_task_result(linkspeech, speech_task, 1);
                core_delete_speech_task(speech_task);
                return;
            } else if(STATE_SUCCESS != core_speech_task_file_check(linkspeech, speech_task)){
                send_speech_task_result(linkspeech, speech_task, 2);
                core_delete_speech_task(speech_task);
                return;
            } else {
                sysdep->core_sysdep_mutex_lock(linkspeech->speech_task_mutex);
                core_list_add_tail(&speech_task->linked_node, &linkspeech->speech_task_list);
                linkspeech->speech_task_num++;
                sysdep->core_sysdep_mutex_unlock(linkspeech->speech_task_mutex);
                /*Synchronous calls need to return immediately. Return immediately.*/
                send_speech_task_result(linkspeech, speech_task, 0);
                if( linkspeech->playing_task == NULL ) {
                    core_check_play_task(linkspeech);
                }
            } 
        }
    }
}
/*User data receiving and processing callback function*/
static void core_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    switch (recv->type) {
    /*Asynchronous service call*/
    case AIOT_DMRECV_ASYNC_SERVICE_INVOKE: {
        core_dm_recv_async_service_invoke(dm_handle, recv, userdata);
    }
    break;

    /*Synchronous service calls*/
    case AIOT_DMRECV_SYNC_SERVICE_INVOKE: {
        core_dm_recv_sync_service_invoke(dm_handle, recv, userdata);
    }
    break;
    default:
        break;
    }
}

static void core_play_finished(char *filename, void *userdata)
{
    linkspeech_handle_t *linkspeech = (linkspeech_handle_t *)userdata;
    if(linkspeech == NULL) {
        return;
    }

    core_log1(sysdep, 0, "core_play_finished file %s\r\n", filename);
    core_check_play_task(linkspeech);
}

void *aiot_linkspeech_init()
{
    linkspeech_handle_t *linkspeech = NULL;
    int i = 0;
    sysdep = aiot_sysdep_get_portfile();
    if (NULL == sysdep) {
        return NULL;
    }

    linkspeech = sysdep->core_sysdep_malloc(sizeof(linkspeech_handle_t), TAG);
    if(linkspeech == NULL) {
        return NULL;
    }

    memset(linkspeech, 0, sizeof(linkspeech_handle_t));
    /*Configuration item is set to empty*/
    linkspeech->dm_handle = NULL;
    linkspeech->player = NULL;
    linkspeech->work_dir = NULL;
    linkspeech->speech_buffer_size = AIOT_LSOPT_SPEECH_BUFFER_SIZE_DEFAULT;

    /*Download management initialization*/
    linkspeech->download_mutex = sysdep->core_sysdep_mutex_init();
    CORE_INIT_LIST_HEAD(&linkspeech->download_task_list);

    /*Voice broadcast task initialization*/
    linkspeech->speech_task_mutex = sysdep->core_sysdep_mutex_init();
    linkspeech->speech_task_num = 0;
    CORE_INIT_LIST_HEAD(&linkspeech->speech_task_list);

    linkspeech->play_param.on_finish = core_play_finished;
    linkspeech->play_param.userdata = linkspeech;
    linkspeech->playing_task = NULL;
    linkspeech->https_enable = 0;

    for(i = 0; i < sizeof(linkspeech->dup_id) / sizeof(char *); i++){
        linkspeech->dup_id[i] = NULL;
    }
    linkspeech->write_pos = 0;

    return linkspeech;
}

int32_t aiot_linkspeech_deinit(void **handle)
{
    linkspeech_handle_t *linkspeech = NULL;
    core_download_node_t *down_node = NULL, *down_next = NULL;
    core_speech_task_t *speech_node = NULL, *speech_next = NULL;
    int32_t res = STATE_SUCCESS;
    int i = 0;

    if (NULL == handle || NULL == *handle) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    linkspeech = (linkspeech_handle_t *)*handle;

    sysdep->core_sysdep_mutex_lock(linkspeech->speech_task_mutex);
    /*Delete broadcast task list*/
    core_list_for_each_entry_safe(speech_node, speech_next, &linkspeech->speech_task_list, linked_node, core_speech_task_t) {
        core_list_del(&speech_node->linked_node);
        core_delete_speech_task(speech_node);
        speech_node = NULL;
    }
    linkspeech->speech_task_num = 0;
    /*Delete the task being reported*/
    if(linkspeech->playing_task != NULL) {
        core_delete_speech_task(linkspeech->playing_task);
        linkspeech->playing_task = NULL;
    }
    sysdep->core_sysdep_mutex_unlock(linkspeech->speech_task_mutex);
    sysdep->core_sysdep_mutex_deinit(&linkspeech->speech_task_mutex);

    sysdep->core_sysdep_mutex_lock(linkspeech->download_mutex);
    core_list_for_each_entry_safe(down_node, down_next, &linkspeech->download_task_list, linked_node, core_download_node_t) {
        core_list_del(&down_node->linked_node);
        sysdep->core_sysdep_free(down_node->jobcode);
        sysdep->core_sysdep_free(down_node->json_url);
        sysdep->core_sysdep_free(down_node);
    }
    sysdep->core_sysdep_mutex_unlock(linkspeech->download_mutex);
    sysdep->core_sysdep_mutex_deinit(&linkspeech->download_mutex);

    if(linkspeech->work_dir != NULL) {
        sysdep->core_sysdep_free(linkspeech->work_dir);
    }

    if(linkspeech->dm_handle != NULL) {
        /*Destroy the DATA-MODEL instance, generally it will not run here*/
        res = aiot_dm_deinit(&linkspeech->dm_handle);
        if (res < STATE_SUCCESS) {
            return res;
        }
    }

    for(i = 0; i < sizeof(linkspeech->dup_id) / sizeof(char *); i++){
        if(linkspeech->dup_id[i] != NULL) {
            sysdep->core_sysdep_free(linkspeech->dup_id[i]);
        }
        linkspeech->dup_id[i] = NULL;
    }
    linkspeech->write_pos = 0;

    sysdep->core_sysdep_free(linkspeech);

    return STATE_SUCCESS;
}

int32_t aiot_linkspeech_setopt(void *handle, aiot_linkspeech_option_t option, void *data)
{
    linkspeech_handle_t *linkspeech = NULL;
    uint8_t     post_reply = 1;
    int32_t res = STATE_SUCCESS;

    if (NULL == handle || NULL == data) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (option >= AIOT_LSOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    linkspeech = (linkspeech_handle_t *)handle;

    switch(option) {
    case AIOT_LSOPT_MQTT_HANDLE: {
        if(linkspeech->dm_handle != NULL) {
            return -1;
        }
        /*Create DATA-MODEL instance*/
        linkspeech->dm_handle = aiot_dm_init();
        if (linkspeech->dm_handle == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        /*Configure MQTT instance handle*/
        aiot_dm_setopt(linkspeech->dm_handle, AIOT_DMOPT_MQTT_HANDLE, data );
        /*Configure message reception processing callback function*/
        aiot_dm_setopt(linkspeech->dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)core_dm_recv_handler);
        /*Configure the userdata of the callback function*/
        aiot_dm_setopt(linkspeech->dm_handle, AIOT_DMOPT_USERDATA, (void *)linkspeech);
        /*Configure whether the cloud needs to reply post_reply to the device. If it is 1, it means that the cloud needs to reply, otherwise it means that there is no reply.*/
        aiot_dm_setopt(linkspeech->dm_handle, AIOT_DMOPT_POST_REPLY, (void *)&post_reply);
    }
    break;

    case AIOT_LSOPT_FILE_OPS: {
        fs = (aiot_fs_t *)data;
    }
    break;

    case AIOT_LSOPT_PLAYER_CALLBACK: {
        linkspeech->player = (player_cb_t)data;
    }
    break;

    case AIOT_LSOPT_WORK_DIR: {
        if (linkspeech->work_dir != NULL) {
            sysdep->core_sysdep_free(linkspeech->work_dir);
            linkspeech->work_dir = NULL;
        }

        core_strdup(sysdep, &linkspeech->work_dir, (char *)data, TAG);
    }
    break;

    case AIOT_LSOPT_HTTPS_ENABLE: {
        if(*(int32_t *)data != 0 && *(int32_t *)data != 1) {
            return -1;
        }
        linkspeech->https_enable = *(int32_t *)data;
    }
    break;

    case AIOT_LSOPT_SPEECH_BUFFER_SIZE: {
        if(*(int32_t *)data <= 0 || *(int32_t *)data > 255) {
            return -1;
        }
        linkspeech->speech_buffer_size = *(int32_t *)data;
    }
    break;

    default:
        break;
    }

    return res;
}

/*Corpus download url.json file parses and downloads the corpus*/
static int32_t core_speech_download_file_parse(linkspeech_handle_t *linkspeech, char *data, uint32_t datalen)
{
    cJSON *root = NULL, *audios = NULL, *audio = NULL;
    cJSON *formart, *id, *url;
    char *local_file = NULL;
    char *src[3] = {NULL, NULL, NULL};
    int32_t res = STATE_SUCCESS;

    //If it is json format data
    root = cJSON_Parse(data);
    if (root == NULL) {
        return -1;
    }

    audios = cJSON_GetObjectItem(root, "audios");
    if (audios == NULL) {
        cJSON_Delete(root);
        return -1;
    }

    for (int i = 0; i < cJSON_GetArraySize(audios); i++)
    {
        audio = cJSON_GetArrayItem(audios, i);
        if(audio != NULL) {
            formart = cJSON_GetObjectItem(audio,"format");
            id = cJSON_GetObjectItem(audio,"id");
            url = cJSON_GetObjectItem(audio,"url");
            src[0] = linkspeech->work_dir;
            src[1] = id->valuestring;
            src[2] = formart->valuestring;
            core_sprintf(sysdep, &local_file, "%s/%s.%s", src, sizeof(src) / sizeof(char *), TAG );
            if(fs->file_size(local_file) > 0) {
                fs->file_delete(local_file);
            }
            res = core_download_file(url->valuestring, local_file, linkspeech->https_enable);
            if(res != STATE_SUCCESS) {
                sysdep->core_sysdep_free(local_file);
                break;
            }
            sysdep->core_sysdep_free(local_file);
        }
    }
    cJSON_Delete(root);

    return res;
}

int32_t aiot_linkspeech_start(void *handle)
{
    linkspeech_handle_t *linkspeech = (linkspeech_handle_t *)handle;
    char *data = NULL;
    char *response = NULL;
    char *response_src[2] = {NULL};
    char *json_file = NULL;
    char *json_file_src[2] = { NULL, NULL };
    int32_t res = STATE_SUCCESS;
    char *result_success = "0";
    char *result_error = "255";
    int32_t file_len = 0, read_len = 0;
    core_download_node_t *node = NULL;

    if (NULL == handle || fs == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if(linkspeech->work_dir == NULL || linkspeech->player == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    linkspeech->running = 1;

    while(linkspeech->running) {
        /*Read download tasks*/
        node = NULL;
        sysdep->core_sysdep_mutex_lock(linkspeech->download_mutex);
        node = core_list_first_entry(&linkspeech->download_task_list, core_download_node_t, linked_node);
        if(&node->linked_node != &linkspeech->download_task_list) {
            core_list_del(&node->linked_node);
        } else {
            node = NULL;
        }
        sysdep->core_sysdep_mutex_unlock(linkspeech->download_mutex);

        /*Handle a single download task*/
        if(node != NULL) {
            /*Generate local file name*/
            json_file_src[0] = linkspeech->work_dir;
            json_file_src[1] = node->jobcode;
            core_sprintf(sysdep, &json_file, "%s/%s.json", json_file_src, sizeof(json_file_src) / sizeof(char *), TAG);

            /*Download the json file containing the corpus URL to the local*/
            core_download_file(node->json_url, json_file, linkspeech->https_enable);

            /*Read the file content and copy it to data*/
            file_len = fs->file_size(json_file);
            data = sysdep->core_sysdep_malloc(file_len + 1, TAG);
            read_len = fs->file_read(json_file, 0, (uint8_t *)data, file_len);
            core_log3(sysdep, 0, "[%s] read_len %d, file_len %d \r\n", json_file, &read_len, &file_len);
            /*Delete unused json files recording corpus addresses*/
            fs->file_delete(json_file);
            sysdep->core_sysdep_free(json_file);

            if(read_len == file_len) {
                /*Parse the corpus file content and trigger the download*/
                res = core_speech_download_file_parse(linkspeech, data, read_len);
                if(res == STATE_SUCCESS) {
                    response_src[1] = result_success;
                } else {
                    response_src[1] = result_error;
                }
            } else {
                response_src[1] = result_error;
            }

            /*Delete file related content*/
            sysdep->core_sysdep_free(data);

            /*Reply Cloud download completed*/
            response_src[0] = node->jobcode;
            core_sprintf(sysdep, &response, "{\"jobcode\":\"%s\", \"result\":%s}", response_src, sizeof(response_src) / sizeof(char *), TAG);
            core_send_event_post(linkspeech->dm_handle, "SpeechUpdateResponse", response);
            sysdep->core_sysdep_free(response);

            /*Delete download task node*/
            sysdep->core_sysdep_free(node->json_url);
            sysdep->core_sysdep_free(node->jobcode);
            sysdep->core_sysdep_free(node);
        } else {
            sysdep->core_sysdep_sleep(1000);
        }
    }

    return res;
}

int32_t aiot_linkspeech_stop(void *handle)
{
    linkspeech_handle_t *linkspeech = NULL;
    int32_t res = STATE_SUCCESS;

    if (NULL == handle) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    linkspeech = (linkspeech_handle_t *)handle;
    linkspeech->running = 0;

    return res;
}
