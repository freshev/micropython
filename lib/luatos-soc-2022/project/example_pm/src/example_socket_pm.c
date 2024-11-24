#include "common_api.h"
#include "sockets.h"
#include "dns.h"
#include "lwip/ip4_addr.h"
#include "netdb.h"
#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "string.h"
#include "luat_pm.h"
#include "luat_gpio.h"
#define DEMO_SERVER_TCP_IP "112.125.89.8"
#define DEMO_SERVER_TCP_PORT 36514
#define TEST_TAG "test_tag"

/*This demo test Modules enters the power consumption of HIBERNATE mode

Test hardware: except for single Modules, sim card holder and USB socket, no peripheral devices

After this demo enters HIBERNATE sleep mode, the Modules sleep current is nearly 4uA.*/

static uint8_t link_UP; //Network status indication
static luat_rtos_task_handle tcp_task_handle;
static void mobile_event_callback_t(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
    switch (event)
    {
    case LUAT_MOBILE_EVENT_NETIF:
        switch (status)
        {
        case LUAT_MOBILE_NETIF_LINK_ON:
            link_UP = 1;
            LUAT_DEBUG_PRINT("Online registration successful");
            break;
        default:
            LUAT_DEBUG_PRINT("Network failed to register successfully");
            link_UP = 0;
            break;
        }
    case LUAT_MOBILE_EVENT_SIM:
        switch (status)
        {
        case LUAT_MOBILE_SIM_READY:
            LUAT_DEBUG_PRINT("SIM card inserted");
            break;
        case LUAT_MOBILE_NO_SIM:
            LUAT_DEBUG_PRINT("SIM card not inserted");
        default:
            break;
        }
    default:
        break;
    }
}
static void register_moblie_callback(void)
{
    luat_mobile_event_register_handler(mobile_event_callback_t);
}

static luat_pm_deep_sleep_mode_timer_callback_t timer2_cb(LUAT_PM_DEEPSLEEP_TIMERID_E id)
{
    LUAT_DEBUG_PRINT("test timeout timer id %d", id);
}

static void init_deepsleep_timer_cb(void)
{
    luat_pm_deep_sleep_mode_register_timer_cb(LUAT_PM_DEEPSLEEP_TIMER_ID2, timer2_cb);
}

static void demo_socket_pm_task(void *arg)
{
    int retry_times = 0;
    char helloworld[] = "helloworld";
    char txbuf[128] = {0};

    ip_addr_t remote_ip={0};
    struct sockaddr_in name;
    socklen_t sockaddr_t_size = sizeof(name);
    int ret, h_errnop;
    struct timeval to;
    int socket_id = -1;
    struct hostent dns_result;
    struct hostent *p_result;

    luat_gpio_cfg_t gpio = {0};
    luat_gpio_set_default_cfg(&gpio);
    gpio.pin = HAL_GPIO_23;
    gpio.mode = LUAT_GPIO_INPUT;
    luat_gpio_open(&gpio);                //Configure AGPIO3 as input. The default is output high. Pull-up VDD_EXT. After configuring it as input, the current can be reduced by 12uA.
                                          //If you need VDD_EXT to maintain level output after the Modules sleeps, do not configure AGPIO3, otherwise VDDEXT will power down and power on as the Modules wakes up from sleep.
    luat_pm_wakeup_pad_cfg_t cfg = {0};
    cfg.neg_edge_enable = 1;
    cfg.pos_edge_enable = 0;
    cfg.pull_up_enable = 1;
    cfg.pull_down_enable = 0;

    uint32_t expire_time = luat_pm_get_deep_sleep_mode_timer_remain_time(LUAT_PM_DEEPSLEEP_TIMER_ID2);
    if(0xffffffff == expire_time)
        LUAT_DEBUG_PRINT("timer id is invalid or timer is stop");
    else
        LUAT_DEBUG_PRINT("timer id expire times %d", expire_time);
    luat_pm_wakeup_pad_set(true, LUAT_PM_WAKEUP_PAD_0, &cfg); // Configure wakeup interrupt, deep sleep can also be awakened through wakeup
    luat_pm_wakeup_pad_set(true, LUAT_PM_WAKEUP_PAD_3, &cfg);
    luat_pm_wakeup_pad_set(true, LUAT_PM_WAKEUP_PAD_4, &cfg);
    cfg.pull_up_enable = 0;
    luat_pm_wakeup_pad_set(true, LUAT_PM_WAKEUP_PAD_1, &cfg); // Disable wakeup1 pull-up, which can reduce the current by 8uA
    luat_rtos_task_sleep(2000);
    int wakeup_reason = luat_pm_get_wakeup_reason();
    LUAT_DEBUG_PRINT("wakeup reason %d", wakeup_reason);
    if (wakeup_reason != LUAT_PM_WAKEUP_FROM_POR)
    {
        if (LUAT_PM_WAKEUP_FROM_PAD == wakeup_reason)
        {
            int timer2_status = luat_pm_deep_sleep_mode_timer_is_running(LUAT_PM_DEEPSLEEP_TIMER_ID2);
            LUAT_DEBUG_PRINT("this is timer running status %d", timer2_status);
            if (1 == timer2_status)
            {
                luat_pm_deep_sleep_mode_timer_stop(LUAT_PM_DEEPSLEEP_TIMER_ID2);
            }
            timer2_status = luat_pm_deep_sleep_mode_timer_is_running(LUAT_PM_DEEPSLEEP_TIMER_ID2);
            LUAT_DEBUG_PRINT("this is timer running status %d", timer2_status);
        }
        luat_mobile_set_flymode(0, 0); // The control enters flight mode before entering sleep mode. You need to exit flight mode after waking up.
    }

    while (!link_UP)
    {
        luat_rtos_task_sleep(1000);
        LUAT_DEBUG_PRINT("Waiting for network registration");
    }

    ret = lwip_gethostbyname_r(DEMO_SERVER_TCP_IP, &dns_result, txbuf, 128, &p_result, &h_errnop);
    if (!ret)
    {
        remote_ip = *((ip_addr_t *)dns_result.h_addr_list[0]);
    }
    else
    {
        luat_rtos_task_sleep(1000);
        LUAT_DEBUG_PRINT("dns fail");
    }

    while (retry_times < 3)
    {
        socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        memset(&name, 0, sizeof(name));
       
        name.sin_family = AF_INET;
        name.sin_addr.s_addr = remote_ip.u_addr.ip4.addr;
        name.sin_port = htons(DEMO_SERVER_TCP_PORT);
        ret = connect(socket_id, (const struct sockaddr *)&name, sockaddr_t_size);
        if (0 == ret)
        {
            LUAT_DEBUG_PRINT("tcp demo connect success %d", ret);
            ret = send(socket_id, helloworld, strlen(helloworld), 0);
            if (ret == strlen(helloworld)) //Enter deep sleep mode test after successfully sending data
            {
                close(socket_id);
                socket_id = -1;
                LUAT_DEBUG_PRINT("tcp demo send data success");
                goto exit;
            }
            else
            {
                LUAT_DEBUG_PRINT("tcp demo send data fail");
            }
        }
        else
        {
            LUAT_DEBUG_PRINT("tcp demo connect fail %d", ret);
        }
        close(socket_id);
        socket_id = -1;
        retry_times++;
        luat_rtos_task_sleep(5000);
    }
exit:
    LUAT_DEBUG_PRINT("socket quit");
    luat_rtos_task_sleep(3000);
    luat_mobile_set_flymode(0, 1); //Enter airplane mode
    luat_rtos_task_sleep(2000);
    luat_pm_set_usb_power(0); // Turn off USB power
    luat_rtos_task_sleep(2000);
    luat_pm_deep_sleep_mode_timer_start(LUAT_PM_DEEPSLEEP_TIMER_ID2, 86400000); // Enable deep sleep timer, 24 hours
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_STANDBY, TEST_TAG);               //Enter deep sleep mode
    luat_rtos_task_sleep(5000);
    LUAT_DEBUG_PRINT("entry deepsleep mode fail");
    luat_rtos_task_delete(tcp_task_handle);
}

void demo_socket_pm_init(void)
{
    luat_rtos_task_create(&tcp_task_handle, 4 * 2048, 80, "socket_pm", demo_socket_pm_task, NULL, NULL);
}

INIT_HW_EXPORT(register_moblie_callback, "1");
INIT_HW_EXPORT(init_deepsleep_timer_cb, "1");
INIT_TASK_EXPORT(demo_socket_pm_init, "1");
