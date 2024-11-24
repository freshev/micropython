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
#include "luat_gpio.h"
#include "platform_define.h"

/*The following precautions need to be luat when using GPIO on the Yixin 618 platform:

The system has several working modes: ACTIVE, IDLE, SLEEP1, SLEEP2, HIBERNATE. For the differences between the several modes, please refer to the description of example_pm;

1. IO is divided into two types: normal IO and retention/AON IO.
   normal IO: normal IO
              In ACTIVE and IDLE working modes, it can be used under normal control;
In SLEEP1, SLEEP2, and HIBERNATE operating modes, the IO power supply is forcibly turned off and cannot be used;
   retention/AON IO: retention IO
              In ACTIVE and IDLE working modes, it can be used under normal control;
In the SLEEP1, SLEEP2, and HIBERNATE operating modes, it can only be used as an output, and can only maintain one output level state and cannot flip the control;

2. When the pin with AON function is used for retention/AON IO function or other peripheral functions, the software configuration pull-up is invalid, and only the default pull-down can be used;

3. After the system enters SLEEP1, SLEEP2, and HIBERNATE sleep modes, in addition to the software running code calling the wake-up interface to actively wake up, the external events that can wake up include the following:
   (1). Configure the wakeup pad pin for the wakeup software function;
   (2). Low power uart, if the baud rate is 9600, the data will not be lost when waking up, other baud rates will lose part of the previous data;
   (3). low power usb
   (4).power key;
   (5). charge pad;
   (6).rtc;
   For details, refer to the description of example_pm.

4. All IO supports interrupts:
   The three IOs of GPIO20, GPIO21 and GPIO22 can be configured as dual edge interrupts or high and low level interrupts at the same time;

   The remaining GPIOs only support single-edge and single-level interrupts; if these GPIOs are configured with double-edge or double-level interrupts, they will be automatically configured by the system as rising edge or high-level interrupts;
   Therefore, in order to prevent the system from automatically adjusting the configuration, the remaining GPIOs are only configured as single-edge or single-level interrupts when using interrupts.*/

//This example is based on EVB_Air780E_V1.5 hardware

#define NET_LED_PIN HAL_GPIO_27
#define LCD_RST_PIN HAL_GPIO_1

#define LCD_RS_PIN HAL_GPIO_10
#define LCD_CS_PIN HAL_GPIO_8

#define DTR_PIN HAL_GPIO_22
#define LCD_DATA_PIN HAL_GPIO_9

#define DEMO_IRQ_PIN HAL_GPIO_20

//Control the NET indicator light to flash
static void task_gpio_output_run(void *param)
{	
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);

	gpio_cfg.pin = NET_LED_PIN;
	luat_gpio_open(&gpio_cfg);
	while(1)
	{
		luat_gpio_set(NET_LED_PIN, 1);
		LUAT_DEBUG_PRINT("net led on");
		luat_rtos_task_sleep(1000);
		luat_gpio_set(NET_LED_PIN, 0);
		LUAT_DEBUG_PRINT("net led off");
		luat_rtos_task_sleep(1000);
	}	
}

void task_gpio_output_init(void)
{
	luat_rtos_task_handle task_gpio_output_handle;
	luat_rtos_task_create(&task_gpio_output_handle, 4 * 1204, 50, "gpio_output_test", task_gpio_output_run, NULL, 32);
}


//LCD_RST pin and NET pin are short-circuited
//NET pin: flip the output high and low levels every one second through the task_gpio_output_run function
//LCD_RST pin: read the input level every second
void task_gpio_input_run(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);

	gpio_cfg.pin = LCD_RST_PIN;
	gpio_cfg.mode = LUAT_GPIO_INPUT;
	luat_gpio_open(&gpio_cfg);

	int level = 0;

	while(1)
	{
		level = luat_gpio_get(LCD_RST_PIN);
		LUAT_DEBUG_PRINT("get lcd rst pin %d",level);
		luat_rtos_task_sleep(1000);
		level = luat_gpio_get(LCD_RST_PIN);
		LUAT_DEBUG_PRINT("get lcd rst pin %d",level);
		luat_rtos_task_sleep(1000);
	}
}

void task_gpio_input_init(void)
{
	luat_rtos_task_handle task_gpio_input_handle;
	luat_rtos_task_create(&task_gpio_input_handle, 4 * 1204, 50, "gpio_input_test", task_gpio_input_run, NULL, 32);
}

int single_interrupt_cnt = 0;
int both_interrupt_cnt = 0;
int gpio_irq(int pin, void* args)
{
	if (pin == LCD_CS_PIN)
	{
		single_interrupt_cnt++;
	}
	else if (pin == DTR_PIN)
	{
		both_interrupt_cnt++;
	}	
	
	//Note: A line of LUAT_DEBUG_PRINT log in the interrupt service program, if viewed through Luatools, only supports up to 64 bytes
	LUAT_DEBUG_PRINT("pin:%d, level:%d,", pin, luat_gpio_get(pin));
}

//GPIO single edge interrupt test
void task_gpio_single_interrupt_run(void)
{
	luat_gpio_cfg_t gpio_cfg;

	//Configure LCD_CS_PIN as interrupt pin
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = LCD_CS_PIN;
	gpio_cfg.mode = LUAT_GPIO_IRQ;

	//LCD_CS_PIN pin only supports single-edge or single-level interrupts;
	//Only single edge interrupts are demonstrated here, configured as LUAT_GPIO_RISING_IRQ, LUAT_GPIO_FALLING_IRQ
	//Do not configure it as LUAT_GPIO_BOTH_IRQ, because configuring it as LUAT_GPIO_BOTH_IRQ will be automatically modified by the system to LUAT_GPIO_RISING_IRQ;
	gpio_cfg.irq_type = LUAT_GPIO_RISING_IRQ; 

	gpio_cfg.pull = LUAT_GPIO_PULLUP;
	gpio_cfg.irq_cb = gpio_irq;	
	luat_gpio_open(&gpio_cfg);

	//Configure LCD_RS_PIN as the output pin
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = LCD_RS_PIN;
	luat_gpio_open(&gpio_cfg);

	while(1)
	{
		luat_gpio_set(LCD_RS_PIN, 1);
		LUAT_DEBUG_PRINT("LCD_RS output %d, LCD_CS input %d",luat_gpio_get(LCD_RS_PIN), luat_gpio_get(LCD_CS_PIN));
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("after high input, number of single interrupts %d", single_interrupt_cnt);

		luat_gpio_set(LCD_RS_PIN, 0);
		LUAT_DEBUG_PRINT("LCD_RS output %d, LCD_CS input %d",luat_gpio_get(LCD_RS_PIN), luat_gpio_get(LCD_CS_PIN));
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("after low input, number of single interrupts %d", single_interrupt_cnt);		
	}	

}

void task_gpio_single_interrupt_init(void)
{
	luat_rtos_task_handle task_gpio_single_interrupt_handle;
	luat_rtos_task_create(&task_gpio_single_interrupt_handle, 4 * 1204, 50, "gpio_single_interrupt_test", task_gpio_single_interrupt_run, NULL, 32);
}


//GPIO double edge interrupt test
void task_gpio_both_interrupt_run(void)
{
	luat_gpio_cfg_t gpio_cfg;

	//Configure DTR as an interrupt pin
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = DTR_PIN;
	gpio_cfg.mode = LUAT_GPIO_IRQ;

	//The DTR_PIN pin supports dual-edge or high-low level interrupts;
	//Only double edge interrupts are demonstrated here, which can be configured as LUAT_GPIO_BOTH_IRQ, LUAT_GPIO_RISING_IRQ, LUAT_GPIO_FALLING_IRQ
	gpio_cfg.irq_type = LUAT_GPIO_BOTH_IRQ; 

	gpio_cfg.pull = LUAT_GPIO_PULLUP;
	gpio_cfg.irq_cb = gpio_irq;	
	luat_gpio_open(&gpio_cfg);

	//Configure LCD_DATA_PIN as the output pin
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = LCD_DATA_PIN;
	luat_gpio_open(&gpio_cfg);

	while(1)
	{
		luat_gpio_set(LCD_DATA_PIN, 1);
		LUAT_DEBUG_PRINT("LCD_DATA output %d, DTR input %d",luat_gpio_get(LCD_DATA_PIN), luat_gpio_get(DTR_PIN));
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("after high input, number of both interrupts %d", both_interrupt_cnt);

		luat_gpio_set(LCD_DATA_PIN, 0);
		LUAT_DEBUG_PRINT("LCD_DATA output %d, DTR input %d",luat_gpio_get(LCD_DATA_PIN), luat_gpio_get(DTR_PIN));
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("after low input, number of both interrupts %d", both_interrupt_cnt);		
	}	

}

void task_gpio_both_interrupt_init(void)
{
	luat_rtos_task_handle task_gpio_both_interrupt_handle;
	luat_rtos_task_create(&task_gpio_both_interrupt_handle, 4 * 1204, 50, "gpio_both_interrupt_test", task_gpio_both_interrupt_run, NULL, 32);
}
int gpio_level_irq(void *data, void* args)
{
	int pin = (int)data;
	LUAT_DEBUG_PRINT("pin:%d, level:%d,", pin, luat_gpio_get(pin));
	luat_gpio_ctrl(DEMO_IRQ_PIN, LUAT_GPIO_CMD_SET_IRQ_MODE, LUAT_GPIO_NO_IRQ);
}
//GPIO level interrupt test
void task_gpio_level_interrupt_run(void)
{
	luat_gpio_cfg_t gpio_cfg;

	//Configure HAL_GPIO_20 as an interrupt pin
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = DEMO_IRQ_PIN;
	gpio_cfg.mode = LUAT_GPIO_IRQ;

	//The DTR_PIN pin supports dual-edge or high-low level interrupts;
	//So here it can be configured as LUAT_GPIO_BOTH_IRQ, LUAT_GPIO_RISING_IRQ, LUAT_GPIO_FALLING_IRQ, LUAT_GPIO_HIGH_IRQ, LUAT_GPIO_LOW_IRQ,
	gpio_cfg.irq_type = LUAT_GPIO_HIGH_IRQ;

	gpio_cfg.pull = LUAT_GPIO_PULLUP;
	gpio_cfg.irq_cb = gpio_level_irq;
	luat_gpio_open(&gpio_cfg);

	while(1)
	{
		if (!luat_gpio_get(DEMO_IRQ_PIN))
		{
			LUAT_DEBUG_PRINT("IO is already low, you can turn on the high-level interrupt again");
			luat_gpio_ctrl(DEMO_IRQ_PIN, LUAT_GPIO_CMD_SET_IRQ_MODE, LUAT_GPIO_HIGH_IRQ);
		}
		luat_rtos_task_sleep(1000);

	}

}

void task_gpio_level_interrupt_init(void)
{
	luat_rtos_task_handle task_gpio_level_interrupt_handle;
	luat_rtos_task_create(&task_gpio_level_interrupt_handle, 1024, 50, "gpio_level_interrupt_test", task_gpio_level_interrupt_run, NULL, 0);
}



INIT_TASK_EXPORT(task_gpio_output_init, "0");
INIT_TASK_EXPORT(task_gpio_input_init, "1");
INIT_TASK_EXPORT(task_gpio_single_interrupt_init, "2");
INIT_TASK_EXPORT(task_gpio_both_interrupt_init, "3");
INIT_TASK_EXPORT(task_gpio_level_interrupt_init, "4");
