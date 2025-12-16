/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_pm.h"
#include "luat_network_adapter.h"
#include "common_api.h"
#include "luat_gpio.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "networkmgr.h"
#define PM_TEST_DEEP_SLEEP_TIMER_ID	3
#define PM_TEST_SERVICE_IP	"47.99.172.211" //Switch to your own server. In order to facilitate the demonstration of PSM mode, the demo uses UDP.
#define PM_TEST_SERVICE_PORT	45433 //Change to your own server
#define PM_TEST_PERIOD	1800 //The test time for a single project is 30 minutes, which can be modified if necessary.
luat_rtos_task_handle pm_task_handle;
static uint8_t pm_wakeup_by_timer_flag;
luat_rtos_task_handle g_s_task_handle;
static int32_t luat_test_socket_callback(void *pdata, void *param)
{
	OS_EVENT *event = (OS_EVENT *)pdata;
	LUAT_DEBUG_PRINT("%x", event->ID);
	return 0;
}

static void pm_deep_sleep_timer_callback(uint8_t id)
{
	if (PM_TEST_DEEP_SLEEP_TIMER_ID == id)
	{
		LUAT_DEBUG_PRINT("deep sleep timer %d", id);
		pm_wakeup_by_timer_flag = 1;
	}
}

static int pm_deep_sleep_gpio_wakeup(void *pdata, void *param)
{

}

static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	LUAT_DEBUG_PRINT("%d,%d,%d", event, index, status);
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
			char imsi[20];
			char iccid[24] = {0};
			luat_mobile_get_iccid(0, iccid, sizeof(iccid));
			LUAT_DEBUG_PRINT("ICCID %s", iccid);
			luat_mobile_get_imsi(0, imsi, sizeof(imsi));
			LUAT_DEBUG_PRINT("IMSI %s", imsi);
			char imei[20] = {0};
			luat_mobile_get_imei(0, imei, sizeof(imei));
			LUAT_DEBUG_PRINT("IMEI %s", imei);

			luat_socket_check_ready(index, NULL);
		}
	}
	else if (LUAT_MOBILE_EVENT_CSCON == event)
	{

	}
}



static void socket_task(void *param)
{
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
	const char remote_ip[] = PM_TEST_SERVICE_IP;
	static network_ctrl_t *g_s_network_ctrl;
	const char hello[] = "hello, luatos!";
	int result;
	uint8_t *rx_data = malloc(1024);
	uint32_t tx_len, rx_len;
	uint8_t is_break,is_timeout,retry;
	g_s_network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_init_ctrl(g_s_network_ctrl, g_s_task_handle, luat_test_socket_callback, NULL);
	network_set_base_mode(g_s_network_ctrl, 0, 15000, 0, 0, 0, 0);	//UDP

	if (luat_pm_get_wakeup_reason() > LUAT_PM_WAKEUP_FROM_POR)
	{
		retry = 0;
		luat_gpio_close(HAL_GPIO_20);
		luat_pm_deep_sleep_mode_timer_stop(PM_TEST_DEEP_SLEEP_TIMER_ID);
		LUAT_DEBUG_PRINT("wakeup from deep sleep");
		//First make sure that the network status is correct before hibernation, otherwise you need to restore the network
		result = network_wait_link_up(g_s_network_ctrl, 100);
		if (result < 0)
		{
			LUAT_DEBUG_PRINT("lte stack maybe error, %d", result);
			luat_mobile_reset_stack();
		}
RETRY:
		result = network_connect(g_s_network_ctrl, remote_ip, sizeof(remote_ip) - 1, NULL, PM_TEST_SERVICE_PORT, 30000);
		if (!result)
		{
			result = network_tx(g_s_network_ctrl, hello, sizeof(hello) - 1, 0, NULL, 0, &tx_len, 15000);
			result = network_wait_rx(g_s_network_ctrl, 5000, &is_break, &is_timeout);

			if (!is_timeout && !is_break)
			{
				do
				{
					result = network_rx(g_s_network_ctrl, rx_data, 1024, 0, NULL, NULL, &rx_len);
					if (rx_len > 0)
					{
						LUAT_DEBUG_PRINT("rx %d", rx_len);
					}
				}while(!result && rx_len > 0);
			}
			else
			{
				//There is no data after timeout, maybe the protocol stack is in an error state
				network_close(g_s_network_ctrl, 5000);
				if (!retry)
				{
					retry = 1;
					LUAT_DEBUG_PRINT("lte stack maybe error");
					luat_mobile_reset_stack();
					goto RETRY;
				}
				else
				{
					LUAT_DEBUG_PRINT("lte stack still error");
				}
			}
			LUAT_DEBUG_PRINT("now sleep again");
			luat_pm_deep_sleep_mode_register_timer_cb(PM_TEST_DEEP_SLEEP_TIMER_ID, pm_deep_sleep_timer_callback);
			luat_pm_deep_sleep_mode_timer_start(PM_TEST_DEEP_SLEEP_TIMER_ID, PM_TEST_PERIOD * 1000);
			luat_pm_set_power_mode(LUAT_PM_POWER_MODE_POWER_SAVER, 0);
			//The lowest power consumption is to turn off the GPIO23 output
			luat_gpio_cfg_t gpio_cfg = {0};
			gpio_cfg.pin = HAL_GPIO_23;
			gpio_cfg.mode = LUAT_GPIO_INPUT;
			luat_gpio_open(&gpio_cfg);

			//Use GPIO20 to wake up
			gpio_cfg.pin = HAL_GPIO_20;
			gpio_cfg.mode = LUAT_GPIO_IRQ;
			gpio_cfg.irq_cb = pm_deep_sleep_gpio_wakeup;
			gpio_cfg.pull = LUAT_GPIO_PULLUP;
			gpio_cfg.irq_type = LUAT_GPIO_FALLING_IRQ;
			luat_gpio_open(&gpio_cfg);

			//If the powerkey is grounded, the pull-up of the powerkey must also be turned off.
			pwrKeyHwDeinit(0);
			while(1)
			{
				luat_rtos_task_sleep(15000);
				LUAT_DEBUG_PRINT("error no sleep");
				luat_pm_print_state();
			}
		}
	}

	while(1)
	{
		result = network_connect(g_s_network_ctrl, remote_ip, sizeof(remote_ip) - 1, NULL, PM_TEST_SERVICE_PORT, 30000);
		if (!result)
		{
			result = network_tx(g_s_network_ctrl, hello, sizeof(hello) - 1, 0, NULL, 0, &tx_len, 15000);
			while (!result)
			{
				result = network_wait_rx(g_s_network_ctrl, 300000, &is_break, &is_timeout);
				if (!result)
				{
					if (!is_timeout && !is_break)
					{
						do
						{
							result = network_rx(g_s_network_ctrl, rx_data, 1024, 0, NULL, NULL, &rx_len);
							if (rx_len > 0)
							{
								LUAT_DEBUG_PRINT("rx %d", rx_len);
							}
						}while(!result && rx_len > 0);
					}
					else if (is_timeout)
					{
						result = network_tx(g_s_network_ctrl, hello, sizeof(hello) - 1, 0, NULL, 0, &tx_len, 15000);
					}
				}
			}
		}
		LUAT_DEBUG_PRINT("The network is disconnected, try again immediately");
		network_close(g_s_network_ctrl, 5000);
//		luat_rtos_task_sleep(15000);
	}


}

static void pm_task(void *param)
{

	LUAT_DEBUG_PRINT("Start demonstrating networked low-power features");
	LUAT_DEBUG_PRINT("Response priority mode test %d minutes", PM_TEST_PERIOD/60);
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_HIGH_PERFORMANCE, 0);
	luat_rtos_task_sleep(PM_TEST_PERIOD * 1000);
	LUAT_DEBUG_PRINT("Balanced mode test %d minutes", PM_TEST_PERIOD/60);
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_BALANCED, 0);
	luat_rtos_task_sleep(PM_TEST_PERIOD * 1000);
	LUAT_DEBUG_PRINT("No low power mode testing for %d minutes", PM_TEST_PERIOD/60);
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_NORMAL, 0);
	luat_rtos_task_sleep(PM_TEST_PERIOD * 1000);
	LUAT_DEBUG_PRINT("PSM+ mode, wake up every %d minutes to send data", PM_TEST_PERIOD/60);
	luat_pm_deep_sleep_mode_register_timer_cb(PM_TEST_DEEP_SLEEP_TIMER_ID, pm_deep_sleep_timer_callback);
	luat_pm_deep_sleep_mode_timer_start(PM_TEST_DEEP_SLEEP_TIMER_ID, PM_TEST_PERIOD * 1000);
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_POWER_SAVER, 0);
	//The lowest power consumption is to turn off the GPIO23 output
	luat_gpio_cfg_t gpio_cfg = {0};
	gpio_cfg.pin = HAL_GPIO_23;
	gpio_cfg.mode = LUAT_GPIO_INPUT;
	luat_gpio_open(&gpio_cfg);

	//Use GPIO20 to wake up
	gpio_cfg.pin = HAL_GPIO_20;
	gpio_cfg.mode = LUAT_GPIO_IRQ;
	gpio_cfg.irq_cb = pm_deep_sleep_gpio_wakeup;
	gpio_cfg.pull = LUAT_GPIO_PULLUP;
	gpio_cfg.irq_type = LUAT_GPIO_FALLING_IRQ;
	luat_gpio_open(&gpio_cfg);

	//If the powerkey is grounded, the pull-up of the powerkey must also be turned off.
	pwrKeyHwDeinit(0);
	luat_rtos_task_sleep(3000);
	LUAT_DEBUG_PRINT("It has not entered the extreme power consumption mode. There may be a problem with the protocol stack. Restart the protocol stack to restore it.");
	luat_pm_deep_sleep_mode_timer_stop(PM_TEST_DEEP_SLEEP_TIMER_ID);
	luat_pm_deep_sleep_mode_timer_start(PM_TEST_DEEP_SLEEP_TIMER_ID, PM_TEST_PERIOD * 1000);
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_NORMAL, 0);
	luat_rtos_task_sleep(1000);
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_POWER_SAVER, 0);
	while(1)
	{
		luat_rtos_task_sleep(15000);
		LUAT_DEBUG_PRINT("error no sleep");
	}
}

/** Almost all logs in this demo are output from uart0, and the baud rate must be increased.*/
void soc_get_unilog_br(uint32_t *baudrate)
{
	*baudrate = 6000000; //UART0 uses the log port to output a 6M baud rate, which must be converted from high-performance USB to TTL, such as CH343.
}


static void pm_pre_init(void)
{
	luat_pm_deep_sleep_mode_register_timer_cb(PM_TEST_DEEP_SLEEP_TIMER_ID, pm_deep_sleep_timer_callback);
}

static void pm(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	luat_rtos_task_create(&g_s_task_handle, 4 * 1024, 50, "socket", socket_task, NULL, 16);
	if (LUAT_PM_WAKEUP_FROM_POR == luat_pm_get_wakeup_reason())
	{
		luat_rtos_task_create(&pm_task_handle, 2 * 1024, 50, "pm", pm_task, NULL, 16);
	}



}
INIT_HW_EXPORT(pm_pre_init, "1");
INIT_TASK_EXPORT(pm, "1");
