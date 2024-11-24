#include "luat_debug.h"
#include "luat_rtos.h"
#include "socket_service.h"

/*Function Modules name: data transmission and simulated network disconnection test;

The main logic of data sending test is as follows:
1. After powering on, start a task, call the socket_service_send_data interface every 3 seconds inside the task, and send data "send data from test task" to the server;
   The sending result is notified to this function Modules through the callback function send_data_from_task_callback;
2. After powering on, start a timer, call the socket_service_send_data interface once after 3 seconds, and send data "send data from test timer" to the server;
   The sending result is notified to this functional Modules through the callback function send_data_from_timer_callback; then a 3-second timer is started to send data, and so on;

The main logic of the simulated network disconnection test is as follows (this function is turned off by default, if you need to turn it on, please refer to the few lines of code comments at the end of this file):
1. After powering on, start a task, enter flight mode within the task for 5 seconds, then exit flight mode, and then enter flight mode again after 30 seconds, and so on;*/

#define SEND_DATA_FROM_TASK "send data from test task"
#define SEND_DATA_FROM_TIMER "send data from test timer"

static luat_rtos_semaphore_t g_s_send_data_from_task_semaphore_handle;
static luat_rtos_timer_t g_s_send_data_timer;


//result: 0 successful, 1 socket not connected; the remaining error values   are the error cause values   returned by the lwip send interface
static void send_data_from_task_callback(int result, uint32_t callback_param)
{
	LUAT_DEBUG_PRINT("async result %d, callback_param %d", result, callback_param);
	luat_rtos_semaphore_release(g_s_send_data_from_task_semaphore_handle);
}

static void test_send_data_task_proc(void *arg)
{
	int result;

	luat_rtos_semaphore_create(&g_s_send_data_from_task_semaphore_handle, 1);

	while (1)
	{
		LUAT_DEBUG_PRINT("send request");

		result = socket_service_send_data(SEND_DATA_FROM_TASK, strlen(SEND_DATA_FROM_TASK), send_data_from_task_callback, 0);

		if (0 == result)
		{
			luat_rtos_semaphore_take(g_s_send_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
		}
		else
		{
			LUAT_DEBUG_PRINT("sync result %d", result);
		}		

		luat_rtos_task_sleep(3000);
	}
	
}

static void send_data_timer_callback(void);

//result: 0 successful, 1 socket not connected; the remaining error values   are the error cause values   returned by the lwip send interface
static void send_data_from_timer_callback(int result, uint32_t callback_param)
{
	LUAT_DEBUG_PRINT("async result %d, callback_param %d", result, callback_param);
	luat_rtos_timer_start(g_s_send_data_timer, 3000, 0, send_data_timer_callback, NULL);
}

static void send_data_timer_callback(void)
{
	LUAT_DEBUG_PRINT("send request");

	int result = socket_service_send_data(SEND_DATA_FROM_TIMER, strlen(SEND_DATA_FROM_TIMER), send_data_from_timer_callback, 0);

	if (0 != result)
	{
		LUAT_DEBUG_PRINT("sync result %d", result);
		luat_rtos_timer_start(g_s_send_data_timer, 3000, 0, send_data_timer_callback, NULL);
	}
}

static void flymode_task_proc(void *arg)
{
	luat_rtos_task_sleep(5000);
	LUAT_DEBUG_PRINT("entry");
	while (1)
	{
		luat_rtos_task_sleep(30000);
		LUAT_DEBUG_PRINT("enter flymode");
		luat_mobile_set_flymode(0,1);
		luat_rtos_task_sleep(5000);
		luat_mobile_set_flymode(0,0);
		LUAT_DEBUG_PRINT("exit flymode");
	}
}

void test_service_init(void)
{
	luat_rtos_task_handle test_send_data_task_handle;
	luat_rtos_task_create(&test_send_data_task_handle, 2048, 30, "test_send_data", test_send_data_task_proc, NULL, 0);

	luat_rtos_timer_create(&g_s_send_data_timer);
	luat_rtos_timer_start(g_s_send_data_timer, 3000, 0, send_data_timer_callback, NULL);

	// This task simulates a disconnection scenario by continuously entering and exiting flight mode.
	// Only used for simulation testing, you can open it yourself if necessary
	// luat_rtos_task_handle flymode_task_handle;
	// luat_rtos_task_create(&flymode_task_handle, 2048, 20, "flymode", flymode_task_proc, NULL, NULL);
}

