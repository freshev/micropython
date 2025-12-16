#include "common_api.h"
#include <stdio.h>
#include <string.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <sys/stat.h>
// #include <dirent.h>
// #include <stdlib.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_linkspeech_api.h"
#include "aiot_fs_api.h"

#include "luat_mobile.h"
#include "bsp_custom.h"
#include "luat_rtos.h"
#include "luat_audio_play_ec618.h"
#include "luat_i2s_ec618.h"
#include "luat_gpio.h"
#include "luat_debug.h"

static luat_rtos_semaphore_t audio_semaphore_handle;
/*TODO: Replace with the triplet of your own device*/
char *product_key       = "${YourProductKey}";
char *device_name       = "${YourDeviceName}";
char *device_secret     = "${YourDeviceSecret}";

char  *host = "${YourProductKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com";
uint16_t port = 443;

/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    luat_rtos_task_handle linkspeech_init_task_handle;
/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;

static luat_rtos_task_handle g_mqtt_process_thread;
static luat_rtos_task_handle g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;
static luat_rtos_task_handle g_linkspeech_thread;

typedef struct play_info {
    char filename[128];
    play_param_t *ext_params;
} play_info_t;
/*Audio playback information is assigned by the Qianli Transmission Modules in the callback.*/
play_info_t *g_play_info = NULL;

luat_rtos_mutex_t player_mutex;

int32_t file_size(char *path) {
    // struct stat st;
    // return 0 == stat(path, &st) ? st.st_size : -1;
    size_t filesize = luat_fs_fsize(path);
    return filesize >= 0 ? filesize : -1;
}

int32_t file_delete(char *path) {
    return luat_fs_remove(path);
}

uint8_t g_s_is_link_up = 0;


int32_t file_write(char *path, uint32_t offset, uint8_t *data, uint32_t len)
{
    int32_t res = -1;
    /*Opens a binary file for read/write, allowing reading or appending data to the end of the file.*/
    FILE* f = luat_fs_fopen(path, "ab+");
    if (f == NULL) {
        return -1;
    }

    // if(0 != luat_fs_fseek(f, offset, SEEK_SET)) {
    //     luat_fs_fclose(f);
    //     return -1;
    // }

    res = luat_fs_fwrite(data, len, 1, f);
    luat_fs_fclose(f);
    return res;
}

int32_t file_read(char *path, uint32_t offset, uint8_t *data, uint32_t len)
{
    int32_t res = -1;
    FILE* f = luat_fs_fopen(path, "rb");
    if (f == NULL) {
        return -1;
    }

    if(0 != luat_fs_fseek(f, offset, SEEK_SET)) {
        luat_fs_fclose(f);
        return -1;
    }

    res = luat_fs_fread(data, 1, len, f);
    luat_fs_fclose(f);
    return res;
}

aiot_fs_t posix_fs = {
    .file_size = file_size,
    .file_delete = file_delete,
    .file_write = file_write,
    .file_read = file_read,
};

void player(const char *filename, play_param_t *params)
{
    LUAT_DEBUG_PRINT("this is filename %s %d %d", filename, luat_fs_fexist(filename), luat_fs_fsize(filename));
    luat_rtos_mutex_lock(player_mutex, LUAT_WAIT_FOREVER);
    if(g_play_info != NULL) {
        luat_rtos_mutex_unlock(player_mutex);
        return;
    }

    g_play_info = malloc(sizeof(play_info_t));
    memset(g_play_info, 0, sizeof(play_info_t));
    if(strlen(filename) > sizeof(g_play_info->filename)) {
        luat_rtos_mutex_unlock(player_mutex);
        return;
    }
    memcpy(g_play_info->filename, filename, strlen(filename));
    g_play_info->ext_params = params;
    luat_rtos_mutex_unlock(player_mutex);
}

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * The code of the above log is 0317 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
int32_t demo_state_logcb(int32_t code, char *message)
{
    LUAT_DEBUG_PRINT("%s", message);
    return 0;
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
    /*SDK has successfully established a connection with the mqtt server because the user called the aiot_mqtt_connect() interface.*/
    case AIOT_MQTTEVT_CONNECT: {
        LUAT_DEBUG_PRINT("AIOT_MQTTEVT_CONNECT\n");
    }
    break;

    /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
    case AIOT_MQTTEVT_RECONNECT: {
        LUAT_DEBUG_PRINT("AIOT_MQTTEVT_RECONNECT\n");
    }
    break;

    /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
    case AIOT_MQTTEVT_DISCONNECT: {
        char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                      ("heartbeat disconnect");
        LUAT_DEBUG_PRINT("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
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
        luat_rtos_task_sleep(1);
    }
    luat_rtos_task_delete(g_mqtt_process_thread);
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
            luat_rtos_task_sleep(1);
        }
    }
    luat_rtos_task_delete(g_mqtt_recv_thread);
    return NULL;
}

void *mqtt_service_init() {
    int32_t     res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/
    void       *mqtt_handle;

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
        LUAT_DEBUG_PRINT("aiot_mqtt_init failed\n");
        return NULL;
    }

    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }

    /*Configure MQTT server address*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
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
    /*Configure MQTT event callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /*Establish an MQTT connection with the server*/
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        aiot_mqtt_deinit(&mqtt_handle);
        LUAT_DEBUG_PRINT("aiot_mqtt_connect failed: -0x%04X\n", -res);
        return NULL;
    }

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    g_mqtt_process_thread_running = 1;
    luat_rtos_task_create(&g_mqtt_process_thread, 20 * 1024, 20, "process_task", demo_mqtt_process_thread, mqtt_handle, 0);
    if (g_mqtt_process_thread == NULL) {
        LUAT_DEBUG_PRINT("luat_rtos_task_create demo_mqtt_process_thread failed: %d\n", res);
        aiot_mqtt_disconnect(mqtt_handle);
        aiot_mqtt_deinit(&mqtt_handle);
        return NULL;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
    luat_rtos_task_create(&g_mqtt_recv_thread, 20 * 1024, 20, "recv_task", demo_mqtt_recv_thread, mqtt_handle, 0);
    if (g_mqtt_recv_thread == NULL) {
        LUAT_DEBUG_PRINT("luat_rtos_task_create demo_mqtt_recv_thread failed: %d\n", res);
        aiot_mqtt_disconnect(mqtt_handle);
        aiot_mqtt_deinit(&mqtt_handle);
        return NULL;
    }

    return mqtt_handle;
}

// /* The thread that executes the Qianli Transmission service */
void *demo_linkspeech_thread(void *args)
{
    aiot_linkspeech_start(args);
    luat_rtos_task_delete(g_linkspeech_thread);
    return NULL;
}

int linkspeech_init(void *args)
{
    while(!g_s_is_link_up){
		luat_rtos_task_sleep(1000);
	}
    int32_t res = STATE_SUCCESS;
    void *linkspeech_handle = NULL;
    void *mqtt_handle = NULL;
    char *work_dir = ""; /*The folder where the corpus is saved*/
    int32_t https_enable = 0;

    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);
    /*Create mqtt example and initialize mqtt connection service*/
    mqtt_handle = mqtt_service_init();
    if(mqtt_handle == NULL) {
        luat_rtos_task_delete(linkspeech_init_task_handle);
        return -1;
    }

    /*Create a Qianli Transsion instance*/
    linkspeech_handle = aiot_linkspeech_init();
    if (linkspeech_handle == NULL) {
        LUAT_DEBUG_PRINT("aiot_dm_init failed");
        luat_rtos_task_delete(linkspeech_init_task_handle);
        return -1;
    }

    /*Set the folder to save the corpus*/
    // memset(work_dir, 0, sizeof(work_dir));
    // char *s = getcwd(work_dir, sizeof(work_dir));
    // if(s == NULL) {
    //     return -1;
    // }
    // strncat(work_dir, "/speech_src", strlen("/speech_src"));
    // DIR *dir = NULL;
    // if(NULL == (dir = opendir(work_dir))) {
    //     if(0 != mkdir(work_dir, 0775)) {
    //         return -1;
    //     }
    // } else {
    //     closedir(dir);
    // }
    aiot_linkspeech_setopt(linkspeech_handle, AIOT_LSOPT_WORK_DIR, (void *)work_dir);

    /*Related mqtt*/
    aiot_linkspeech_setopt(linkspeech_handle, AIOT_LSOPT_MQTT_HANDLE, mqtt_handle);
    /*Set up file system operations*/
    aiot_linkspeech_setopt(linkspeech_handle, AIOT_LSOPT_FILE_OPS, (void *)&posix_fs);
    /*Set audio playback interface*/
    aiot_linkspeech_setopt(linkspeech_handle, AIOT_LSOPT_PLAYER_CALLBACK, (void *)player);
    /*Set file download protocol, 0:http 1:https*/
    aiot_linkspeech_setopt(linkspeech_handle, AIOT_LSOPT_HTTPS_ENABLE, (void *)&https_enable);
    /*Create a thread to process the logic of Qianli Transmission. This thread is mainly used for corpus downloading.*/
    luat_rtos_task_create(&g_linkspeech_thread, 20 * 1024, 20, "link_task", demo_linkspeech_thread, linkspeech_handle, 0);
    if (res < 0) {
        LUAT_DEBUG_PRINT("luat_rtos_task_create demo_mqtt_recv_thread failed: %d\n", res);
        aiot_linkspeech_deinit(&linkspeech_handle);
        aiot_mqtt_disconnect(mqtt_handle);
        aiot_mqtt_deinit(&mqtt_handle);
        luat_rtos_task_delete(linkspeech_init_task_handle);
        return -1;
    }

    /*The main loop processes the playback task and creates a lock to protect the audio playback object from being accessed concurrently and causing exceptions.*/
    // luat_rtos_mutex_create(&player_mutex);
    while (1) {
        play_info_t info;
        luat_rtos_mutex_lock(player_mutex, LUAT_WAIT_FOREVER);
        if(g_play_info != NULL) {
            memcpy(&info, g_play_info, sizeof(play_info_t));
            free(g_play_info);
            g_play_info = NULL;
            luat_rtos_mutex_unlock(player_mutex);
            // char cmd[256];
            // memset(cmd, 0, sizeof(cmd));
            // snprintf(cmd, sizeof(cmd), "play %s", info.filename);
            /*blocking play*/
            // res = system(cmd);
            luat_audio_play_info_t tainfo[1];
            tainfo[0].path = info.filename;
            luat_audio_play_multi_files(0, tainfo, 1);
            luat_rtos_semaphore_take(audio_semaphore_handle, LUAT_WAIT_FOREVER);
            // if(res < 0) {
            //     LUAT_DEBUG_PRINT("AAAAAAAAAAAAA  555555555555");
            //     luat_rtos_task_delete(linkspeech_init_task_handle);
            //     return -1;
            // }
            /*This action will trigger the next playback*/
            info.ext_params->on_finish((char *)info.filename, info.ext_params->userdata);
            continue;
        } else {
            luat_rtos_mutex_unlock(&player_mutex);
        }

        luat_rtos_task_sleep(10000);
    }
    luat_rtos_mutex_delete(player_mutex);

    /*Stop sending and receiving actions*/
    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;

    /*Stop Qianli Transmission service*/
    aiot_linkspeech_stop(linkspeech_handle);

    /*Disconnect the MQTT connection, generally it will not run here*/
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_linkspeech_deinit(&linkspeech_handle);
        aiot_mqtt_deinit(&mqtt_handle);
        LUAT_DEBUG_PRINT("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        luat_rtos_task_delete(linkspeech_init_task_handle);
        return -1;
    }

    /*Destroy the Qianli Transmission instance, generally it will not run to this point.*/
    res = aiot_linkspeech_deinit(&linkspeech_handle);
    if (res < STATE_SUCCESS) {
        LUAT_DEBUG_PRINT("aiot_dm_deinit failed: -0x%04X\n", -res);
        luat_rtos_task_delete(linkspeech_init_task_handle);
        return -1;
    }

    /*Destroy the MQTT instance, generally it will not run here*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        LUAT_DEBUG_PRINT("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        luat_rtos_task_delete(linkspeech_init_task_handle);
        return -1;
    }

    return 0;
}

static void linkspeech_task_init(void)
{
    luat_rtos_task_create(&linkspeech_init_task_handle, 20 * 1024, 20, "init_task", linkspeech_init, NULL, 0);
}
static void mobile_event_cb(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	switch(event)
	{
	case LUAT_MOBILE_EVENT_CFUN:
		LUAT_DEBUG_PRINT("CFUN message, status %d", status);
		break;
	case LUAT_MOBILE_EVENT_SIM:
		LUAT_DEBUG_PRINT("SIM card message");
		switch(status)
		{
		case LUAT_MOBILE_SIM_READY:
			LUAT_DEBUG_PRINT("SIM card works normally");
			break;
		case LUAT_MOBILE_NO_SIM:
			LUAT_DEBUG_PRINT("SIM card does not exist");
			break;
		case LUAT_MOBILE_SIM_NEED_PIN:
			LUAT_DEBUG_PRINT("SIM card requires PIN code");
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_REGISTER_STATUS:
		LUAT_DEBUG_PRINT("Mobile network service status changes, currently %d", status);
		break;
	case LUAT_MOBILE_EVENT_CELL_INFO:
		switch(status)
		{
		case LUAT_MOBILE_CELL_INFO_UPDATE:
			break;
		case LUAT_MOBILE_SIGNAL_UPDATE:
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_PDP:
		LUAT_DEBUG_PRINT("CID %d PDP activation status changed to %d", index, status);
		break;
	case LUAT_MOBILE_EVENT_NETIF:
		LUAT_DEBUG_PRINT("The internet working status changes to %d", status);
		switch (status)
		{
		case LUAT_MOBILE_NETIF_LINK_ON:
			LUAT_DEBUG_PRINT("Can access the Internet");
            g_s_is_link_up = 1;
			break;
		default:
            g_s_is_link_up = 0;
			LUAT_DEBUG_PRINT("Can't access the Internet");
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_TIME_SYNC:
		LUAT_DEBUG_PRINT("UTC time synchronized over mobile network");
		break;
	case LUAT_MOBILE_EVENT_CSCON:
		LUAT_DEBUG_PRINT("RRC status %d", status);
		break;
	default:
		break;
	}
}

static void task_init(void){
    luat_mobile_event_register_handler(mobile_event_cb);
}



// AIR780E+TM8211 development board configuration
#define CODEC_PWR_PIN HAL_GPIO_12
#define CODEC_PWR_PIN_ALT_FUN 4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN 0
#define CHARGE_EN_PIN HAL_GPIO_2
#define CHARGE_EN_PIN_ALT_FUN 0


static luat_rtos_task_handle audio_task_handle;
static uint8_t audio_sleep_handler = 0xff;
static HANDLE g_s_delay_timer;

void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
    HAL_I2sSrcAdjustVolumn(data, len, 10);
    LUAT_DEBUG_PRINT("%x,%d,%d,%d,%d", data, len, bits, channels);
}
void app_pa_on(uint32_t arg)
{
    luat_gpio_set(PA_PWR_PIN, 1); // If it is a 780E+ audio expansion board, you can comment out this line of code because the PA is always on
}
void audio_event_cb(uint32_t event, void *param)
{
    LUAT_DEBUG_PRINT("event_cb %d", event);
    switch (event)
    {
    case MULTIMEDIA_CB_AUDIO_DECODE_START:
        luat_gpio_set(CODEC_PWR_PIN, 1);
        luat_audio_play_write_blank_raw(0, 6, 1);
        break;
    case MULTIMEDIA_CB_AUDIO_OUTPUT_START:
        luat_rtos_timer_start(g_s_delay_timer, 200, 0, app_pa_on, NULL); // If it is a 780E+ audio expansion board, you can comment out this line of code because the PA is always on
        break;
    case MULTIMEDIA_CB_TTS_INIT:
        break;
    case LUAT_MULTIMEDIA_CB_TTS_DONE:
        if (!luat_audio_play_get_last_error(0))
        {
            luat_audio_play_write_blank_raw(0, 1, 0);
        }
        break;
    case MULTIMEDIA_CB_AUDIO_DONE:
        luat_rtos_timer_stop(g_s_delay_timer);
        LUAT_DEBUG_PRINT("audio play done, result=%d!", luat_audio_play_get_last_error(0));
        luat_gpio_set(PA_PWR_PIN, 0); // If it is a 780E+ audio expansion board, you can comment out this line of code because the PA is always on
        luat_gpio_set(CODEC_PWR_PIN, 0);
        luat_rtos_semaphore_release(audio_semaphore_handle);
        break;
    }
}

void audio_task_init(void)
{
    luat_rtos_timer_create(&g_s_delay_timer);

    luat_gpio_cfg_t gpio_cfg;
    luat_gpio_set_default_cfg(&gpio_cfg);

    gpio_cfg.pull = LUAT_GPIO_DEFAULT;

    // If it is a 780E+ audio expansion board, you can comment out the following two lines of code because the PA is long on
    gpio_cfg.pin = PA_PWR_PIN;
    luat_gpio_open(&gpio_cfg);

    gpio_cfg.pin = CODEC_PWR_PIN;
    luat_gpio_open(&gpio_cfg);
    gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
    luat_gpio_open(&gpio_cfg);

    luat_audio_play_global_init(audio_event_cb, audio_data_cb, luat_audio_play_file_default_fun, NULL, NULL);
    // luat_i2s_base_setup(0, I2S_MODE_I2S, I2S_FRAME_SIZE_16_16); //If it is a 780E+ audio expansion board, open this line of comment code. This configuration corresponds to ES7148/ES7149
    luat_i2s_base_setup(0, I2S_MODE_MSB, I2S_FRAME_SIZE_16_16); // The configuration here corresponds to TM8211
    luat_rtos_semaphore_create(&audio_semaphore_handle, 1);
}


INIT_HW_EXPORT(task_init, "0");
INIT_TASK_EXPORT(linkspeech_task_init, "1");
INIT_TASK_EXPORT(audio_task_init, "1");
