#include "net_led_Task.h"
#include "audio_Task.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"

extern QueueHandle_t audioQueueHandle;
uint8_t link_UP = 0;
/*-----------------------------------------------------------mobile event----------------------------------------------------------*/
static void mobile_event_callback_t(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
    switch (event)
    {
    case LUAT_MOBILE_EVENT_NETIF:
        switch (status)
        {
        case LUAT_MOBILE_NETIF_LINK_ON:
            LUAT_DEBUG_PRINT("ÍøÂç×¢²e³É¹¦");
            ip_addr_t dns_ip[2];
            uint8_t type, dns_num;
            dns_num = 2;
            soc_mobile_get_default_pdp_part_info(&type, NULL, NULL, &dns_num, dns_ip);
            if (type & 0x80)
            {
                if (index != 4)
                {
                    return;
                }
                else
                {
                    NmAtiNetifInfo *pNetifInfo = malloc(sizeof(NmAtiNetifInfo));
                    NetMgrGetNetInfo(0xff, pNetifInfo);
                    if (pNetifInfo->ipv6Cid != 0xff)
                    {
                        net_lwip_set_local_ip6(&pNetifInfo->ipv6Info.ipv6Addr);
                    }
                    free(pNetifInfo);
                }
            }
            if (dns_num > 0)
            {
                network_set_dns_server(NW_ADAPTER_INDEX_LWIP_GPRS, 2, &dns_ip[0]);
                if (dns_num > 1)
                {
                    network_set_dns_server(NW_ADAPTER_INDEX_LWIP_GPRS, 3, &dns_ip[1]);
                }
            }
            net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_GPRS, 1);
            link_UP = 1;
            audioQueueData net_link = {0};
            net_link.playType = TTS_PLAY;
            net_link.priority = MONEY_PLAY;
            char str[] = "ÍøÂç×¢²e³É¹¦";
            net_link.message.tts.data = malloc(sizeof(str));
            memcpy(net_link.message.tts.data, str, sizeof(str));
            net_link.message.tts.len = sizeof(str);
            if (pdTRUE != xQueueSend(audioQueueHandle, &net_link, 0))
            {
                LUAT_DEBUG_PRINT("start send audio fail");
            }
            break;
        default:
            link_UP = 0;
            LUAT_DEBUG_PRINT("ÍøÂç×¢²eÊ§°Ü");
            break;
        }
    case LUAT_MOBILE_EVENT_SIM:
        switch (status)
        {
        case LUAT_MOBILE_SIM_READY:
            break;
        case LUAT_MOBILE_NO_SIM:
            LUAT_DEBUG_PRINT("What do you think?");
            audioQueueData sim_state = {0};
            sim_state.playType = TTS_PLAY;
            sim_state.priority = MONEY_PLAY;
            char str[] = "What?";
            sim_state.message.tts.data = malloc(sizeof(str));
            memcpy(sim_state.message.tts.data, str, sizeof(str));
            sim_state.message.tts.len = sizeof(str);
            if (pdTRUE != xQueueSend(audioQueueHandle, &sim_state, 0))
            {
                LUAT_DEBUG_PRINT("start send audio fail");
            }
        default:
            break;
        }
    default:
        break;
    }
}
void Task_netinfo_call(void)
{
    luat_mobile_event_register_handler(mobile_event_callback_t);
    net_lwip_init();
    net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
    network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
}
/*-----------------------------------------------------------mobile event-------------------------------------------------------*/

/*-----------------------------------------------------------NET_LED begin-------------------------------------------------------*/
static void NET_LED_FUN(void *param)
{
    luat_gpio_cfg_t net_led_cfg;
    luat_gpio_set_default_cfg(&net_led_cfg);

    net_led_cfg.pin = NET_LED_PIN;
    luat_gpio_open(&net_led_cfg);
    while (1)
    {
        if (link_UP)
        {
            luat_gpio_set(NET_LED_PIN, 1);
            luat_rtos_task_sleep(500);
            luat_gpio_set(NET_LED_PIN, 0);
            luat_rtos_task_sleep(500);
        }
        else
        {
            luat_gpio_set(NET_LED_PIN, 1);
            luat_rtos_task_sleep(5000);
            luat_gpio_set(NET_LED_PIN, 0);
            luat_rtos_task_sleep(1000);
        }
    }
}

void NET_LED_Task(void)
{
    luat_rtos_task_handle NET_LED_Task_HANDLE;
    luat_rtos_task_create(&NET_LED_Task_HANDLE, 1 * 1024, 20, "NET_LED_TASK", NET_LED_FUN, NULL, NULL);
}
