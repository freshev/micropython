/** This routine is suitable for POSIX devices such as `Linux` that support pthread. It demonstrates using the SDK to configure MQTT parameters and establish a connection, and then create 2 threads
 *
 * + One thread for keep-alive connections
 * + A thread is used to receive messages, and enters the default data callback when a message arrives, and enters the event callback when the connection status changes
 *
 * Then send a TASK query request on the MQTT connection. If the response message from the cloud platform arrives, the callback function of TASK message processing will be called from the receiving thread and the local time after time adjustment will be printed out.
 *
 * The parts that require user attention or modification have been marked in comments with TODO
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_task_api.h"


/*TODO: Replace with the triplet of your own device*/
char *product_key       = "${YourProductKey}";
char *device_name       = "${YourDeviceName}";
char *device_secret     = "${YourDeviceSecret}";

/*TODO: Replace with the access point of your own instance

    For enterprise instances, or public instances under the Internet of Things platform service opened after July 30, 2021 (including that day)
    The format of mqtt_host is "${YourInstanceId}.mqtt.iothub.aliyuncs.com"
    Where ${YourInstanceId}: Please replace it with the Id of your enterprise/public instance

    For public instances under the Internet of Things platform service opened before July 30, 2021 (not including the same day)
    You need to modify mqtt_host to: mqtt_host = "${YourProductKey}.iot-as-mqtt.${YourRegionId}.aliyuncs.com"
    Among them, ${YourProductKey}: Please replace it with the ProductKey of the product to which the device belongs. You can log in to the IoT platform console and obtain it from the device details page of the corresponding instance.
    ${YourRegionId}: Please replace it with the region code where your IoT platform device is located, such as cn-shanghai, etc.
    Complete mqtt_host example in this case: a1TTmBPIChA.iot-as-mqtt.cn-shanghai.aliyuncs.com

    For details, please see: https://help.aliyun.com/document_detail/147356.html*/
char  *mqtt_host = "${YourInstanceId}.mqtt.iothub.aliyuncs.com";


/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;

static task_desc_t *g_local_task_desc = NULL;
static pthread_t g_mqtt_process_thread;
static pthread_t g_mqtt_recv_thread;
static pthread_t g_task_thread;     /*The thread used to perform the task*/
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

void demo_free_local_task(task_desc_t **task);

void *demo_task_thread(void *data)
{
#if 0
    while (1) {
        /*TODO: Execute task*/
    }
#endif
    sleep(5);
    return NULL;
}

/*Log callback function, SDK logs will be output from here*/
static int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /*SDK has successfully established a connection with the mqtt server because the user called the aiot_mqtt_connect() interface.*/
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\n");
        }
        break;

        /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\n");
        }
        break;

        /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
        }
        break;

        default: {

        }
    }
}

/*MQTT default message processing callback, which is called when the SDK receives an MQTT message from the server and there is no corresponding user callback processing*/
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            printf("heartbeat response\n");
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            printf("pub, payload: %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
        }
        break;

        default: {

        }
    }
}

/*Thread that executes aiot_mqtt_process, including heartbeat sending and QoS1 message resending*/
void *demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        sleep(1);
    }
    return NULL;
}

/*The thread that executes aiot_mqtt_recv, including automatic network reconnection and receiving MQTT messages from the server*/
void *demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            sleep(1);
        }
    }
    return NULL;
}

int32_t demo_mqtt_start(void **handle)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    uint16_t    port = 1883;      /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/


    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);

    /*Create security credentials for the SDK, used to establish TLS connections*/
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /*Use RSA certificate to verify MQTT server*/
    cred.max_tls_fragment = 16384; /*The maximum fragment length is 16K, other optional values     are 4K, 2K, 1K, 0.5K*/
    cred.sni_enabled = 1;                               /*When establishing a TLS connection, support Server Name Indicator*/
    cred.x509_server_cert = ali_ca_cert;                 /*RSA root certificate used to verify the MQTT server*/
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /*RSA root certificate length used to verify the MQTT server*/

    /*Create 1 MQTT client instance and initialize default parameters internally*/
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        printf("aiot_mqtt_init failed\n");
        return -1;
    }

    /*TODO: If the following code is not commented, the routine will use TCP instead of TLS to connect to the cloud platform*/
    /*
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
    */

    /*Configure MQTT server address*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)mqtt_host);
    /*Configure MQTT server port*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /*Configure device productKey*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /*Configure device deviceName*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /*Configure device deviceSecret*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /*Configure the security credentials for the network connection, which have been created above*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /*Configure MQTT default message receiving callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /*Configure MQTT event callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /*Establish an MQTT connection with the server*/
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        return -1;
    }

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    g_mqtt_process_thread_running = 1;
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %d\n", res);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\n", res);
        g_mqtt_process_thread_running = 0;
        pthread_join(g_mqtt_process_thread, NULL);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    *handle = mqtt_handle;

    return 0;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;

    mqtt_handle = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    pthread_join(g_mqtt_process_thread, NULL);
    pthread_join(g_mqtt_recv_thread, NULL);

    /*Disconnect MQTT connection*/
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        return -1;
    }

    /*Destroy the MQTT instance*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return 0;
}

static void _demo_deep_free_base(char *in)
{
    if (NULL == in) {
        return;
    }
    free(in);
}

static void *_demo_deep_copy_base(char *in)
{
    uint32_t len = 0;
    void *tmp = NULL;
    if (NULL == in) {
        return NULL;
    }
    len = strlen(in) + 1;
    tmp = (char *)malloc(len);
    if (NULL == tmp) {
        return NULL;
    }
    memset(tmp, 0, len);
    memcpy(tmp, in, strlen(in));
    return tmp;
}

void demo_copy_task_to_local_task(task_desc_t **dst_desc, const task_desc_t *in_desc)
{
    if (NULL == *dst_desc) {
        *dst_desc = (task_desc_t *)malloc(sizeof(task_desc_t));
    }

    (*dst_desc)->status = in_desc->status;
    (*dst_desc)->progress = in_desc->progress;
    (*dst_desc)->handle = in_desc->handle;
    (*dst_desc)->task_id = _demo_deep_copy_base(in_desc->task_id);
    (*dst_desc)->job_document = _demo_deep_copy_base(in_desc->job_document);
    (*dst_desc)->sign_method = _demo_deep_copy_base(in_desc->sign_method);
    (*dst_desc)->sign = _demo_deep_copy_base(in_desc->sign);
    (*dst_desc)->document_file_url = _demo_deep_copy_base(in_desc->document_file_url);
    (*dst_desc)->status_details = _demo_deep_copy_base(in_desc->status_details);
}

void demo_free_local_task(task_desc_t **task)
{
    if (NULL != *task) {
        _demo_deep_free_base((*task)->task_id);
        _demo_deep_free_base((*task)->job_document);
        _demo_deep_free_base((*task)->sign_method);
        _demo_deep_free_base((*task)->sign);
        _demo_deep_free_base((*task)->document_file_url);
        _demo_deep_free_base((*task)->status_details);
        free(*task);
    }

    *task = NULL;
}

void demo_task_recv_handler(void *handle, const aiot_task_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_TASKRECV_NOTIFY: {
            const task_desc_t *in_desc = &(packet->data.notify);

            printf("revice task notify, task_id:[%s],status:[%d],job_document[%s],document_file_url:[%s],\
            sign_method:[%s],sign[%s]\r\n",
                   in_desc->task_id, in_desc->status, in_desc->job_document,
                   in_desc->document_file_url, in_desc->sign_method, in_desc->sign);

            /*1. When there is no task record in the handle, save the cloud downlink task to the default_task_desc field in the handle.*/
            if (NULL == g_local_task_desc) {
                demo_copy_task_to_local_task(&g_local_task_desc, in_desc);
                /*Start the task. This is only a reference implementation, and users can adapt it according to the actual situation.*/
                int res = pthread_create(&g_task_thread, NULL, demo_task_thread, g_local_task_desc);
                if (res != 0) {
                    printf("pthread_create demo_task_thread failed: %d\r\n", res);
                } else {
                    /*The download thread is set to detach type and can exit independently after the firmware content is obtained.*/
                    pthread_detach(g_task_thread);
                }
                /*Change task status.TODO: This is only a reference implementation. When the task is completed, the status should be set to AIOT_TASK_STATUS_SUCCEEDED*/
                g_local_task_desc->status = AIOT_TASK_STATUS_IN_PROGRESS;
                aiot_task_update(handle, g_local_task_desc);
                demo_free_local_task(&g_local_task_desc);
                break;
            }

            /*2. If the status is set to the final state by the cloud, clean up the local tasks here*/
            if (in_desc->status == AIOT_TASK_STATUS_CANCELLED || in_desc->status == AIOT_TASK_STATUS_REMOVED
                || in_desc->status == AIOT_TASK_STATUS_TIMED_OUT) {
                /*TODO: Clean up local tasks and stop threads*/
                /*If the task is a default task recorded in the handle, its memory must be cleared; if it is recorded outside the handle, the user needs to maintain the memory himself.*/
                if (NULL != g_local_task_desc && 0 == strcmp(in_desc->task_id, g_local_task_desc->task_id)) {
                    /*Release local task memory*/
                    demo_free_local_task(&g_local_task_desc);
                }
                break;
            }

            /*3. If there is a task record locally, the cloud may have updated the description of the current task. You need to check the updated content.*/
            if (in_desc->status == AIOT_TASK_STATUS_IN_PROGRESS) {
                if (NULL != g_local_task_desc && 0 == strcmp(in_desc->task_id, g_local_task_desc->task_id)) {
                    /*TODO: Update the local task description. The user may have to pause the current task before updating, depending on the user*/
                    break;
                }
            }

            /*4. If the above situation is not the case, it means that a new task has come. At this time, the user already has a task being executed, and another task has come. The user can create a list in main.*/
            /*Pass this list in as userdata, record the tasks in this list, and maintain it yourself*/
            break;
        }
        case AIOT_TASKRECV_GET_DETAIL_REPLY: {
            const task_get_detail_reply_t *in_reply = &(packet->data.get_detail_reply);
            printf("revice task get detail, code:[%d]\r\n", in_reply->code);
            if (200 == in_reply->code) {
                printf("revice task get detail reply, task_id:[%s],status:[%d]\r\n",
                       in_reply->task.task_id, in_reply->task.status);
                if (in_reply->task.status != AIOT_TASK_STATUS_NOT_FOUND) {
                    printf("job_document[%s],document_file_url:[%s], sign_method:[%s],sign[%s]\r\n",
                           in_reply->task.job_document, in_reply->task.document_file_url,
                           in_reply->task.sign_method, in_reply->task.sign);
                    task_desc_t task;
                    memset(&task, 0, sizeof(task));
                    memcpy(&task, &(in_reply->task), sizeof(task));
                    /*TODO: Execute the task by starting a thread*/

                    /*Change the task status. TODO: This is only a reference implementation, users can adapt according to the actual situation. When the task is completed, the status should be set to AIOT_TASK_STATUS_SUCCEEDED*/
                    task.status = AIOT_TASK_STATUS_IN_PROGRESS;
                    task.progress = 88;
                    aiot_task_update(handle, &task);
                }
            }
            break;
        }
        case AIOT_TASKRECV_GET_LIST_REPLY: {
            const task_get_list_reply_t *in_reply = &(packet->data.get_list_reply);
            printf("revice task get list reply, number:[%d]\r\n", in_reply->number);
            for (int i = 0; i < in_reply->number; i++) {
                printf("task list[%d]:task_id[%s],status[%d]\r\n", i, in_reply->tasks[i].task_id,
                       in_reply->tasks[i].status);
                aiot_task_get_task_detail(handle, in_reply->tasks[i].task_id);
            }
            break;
        }
        case AIOT_TASKRECV_UPDATE_REPLY: {
            const task_update_reply_t *update_reply = &(packet->data.update_reply);
            printf("revice task update reply, code:[%d]\r\n", update_reply->code);

            if (200 == update_reply->code) {
                printf("revice task update reply, task_id:[%s]\r\n", update_reply->task_id);
            }

            if (71012 == update_reply->code) {
                printf("aiot_task_update task's status_details value must be json format\r\n");
            }
            /* TODO */
            break;
        }
        default:
            /* TODO */
            break;
    }
}

int main(int argc, char *argv[])
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL, *task_handle = NULL;

    /*Establish an MQTT connection and start the keepalive thread and receiving thread*/
    res = demo_mqtt_start(&mqtt_handle);
    if (res < 0) {
        printf("demo_mqtt_start failed\n");
        return -1;
    }

    /*Create a task client instance and initialize default parameters internally*/
    task_handle = aiot_task_init();
    if (task_handle == NULL) {
        demo_mqtt_stop(&mqtt_handle);
        printf("aiot_task_init failed\n");
        return -1;
    }

    res = aiot_task_setopt(task_handle, AIOT_TASKOPT_MQTT_HANDLE, mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_task_setopt AIOT_TASKOPT_MQTT_HANDLE failed, res: -0x%04X\n", -res);
        aiot_task_deinit(&task_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    res = aiot_task_setopt(task_handle, AIOT_TASKOPT_RECV_HANDLER, demo_task_recv_handler);
    if (res < STATE_SUCCESS) {
        printf("aiot_task_setopt AIOT_TASKOPT_RECV_HANDLER failed, res: -0x%04X\n", -res);
        aiot_task_deinit(&task_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    /*Send a task request to obtain the next executable task to the cloud platform*/
    res = aiot_task_get_task_detail(task_handle, NULL);
    if (res < STATE_SUCCESS) {
        aiot_task_deinit(&task_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    /*The main thread goes to sleep, and when the task response from the cloud platform arrives, the receiving thread will call demo_task_recv_handler()*/
    while (1) {
        sleep(1);
    }

    /*Destroy the task instance, generally it will not run here*/
    res = aiot_task_deinit(&task_handle);
    if (res < STATE_SUCCESS) {
        demo_mqtt_stop(&mqtt_handle);
        printf("aiot_task_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    /*Destroy the MQTT instance and exit the thread. Generally, it will not run here.*/
    res = demo_mqtt_stop(&mqtt_handle);
    if (res < 0) {
        printf("demo_start_stop failed\n");
        return -1;
    }

    return 0;
}
