
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "common_api.h"
#include "luat_debug.h"

#define LED_PIN_ALT_FUN	0

#define LED1_PIN	HAL_GPIO_24
#define LED2_PIN	HAL_GPIO_28
#define LED3_PIN	HAL_GPIO_27

static luat_rtos_task_handle led_task_handle;

static void led_task(void *param)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	luat_rtos_task_handle task_handle;

	gpio_cfg.pull = LUAT_GPIO_DEFAULT;

	//If it is a 780E+ audio expansion board, you need to comment out the following four lines of code, because there is only one controllable LED on this board
	gpio_cfg.pin = LED1_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = LED2_PIN;
	luat_gpio_open(&gpio_cfg);


	gpio_cfg.pin = LED3_PIN;
	luat_gpio_open(&gpio_cfg);

	while (1)
	{
		luat_rtos_task_sleep(200);
		luat_gpio_set(LED1_PIN, 1);				//If it is a 780E+ audio expansion board, you need to comment out this line of code
		luat_gpio_set(LED2_PIN, 1);				//If it is a 780E+ audio expansion board, you need to comment out this line of code
		luat_gpio_set(LED3_PIN, 1);
		luat_rtos_task_sleep(200);
		luat_gpio_set(LED1_PIN, 0);				//If it is a 780E+ audio expansion board, you need to comment out this line of code
		luat_gpio_set(LED2_PIN, 0);				//If it is a 780E+ audio expansion board, you need to comment out this line of code
		luat_gpio_set(LED3_PIN, 0);
	}
	luat_rtos_task_delete(led_task_handle);
}

void led_task_init(void)
{
	int result = luat_rtos_task_create(&led_task_handle, 2048, 20, "led task", led_task, NULL, NULL);
    LUAT_DEBUG_PRINT("cloud_speaker_led_task create task result %d", result);
}
