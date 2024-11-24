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
#include "FreeRTOS.h"
#include "task.h"
#include "slpman.h"
#include "pad.h"
#include "gpio.h"
#include "example_main.h"


//1: In IDLE mode, GPIO output, input, and interrupt testing
//2: In automatic SLEEP1 mode, take AGPIOWU0 (GPIO20) as an example to demonstrate how to control the flashing of the indicator light with the lowest power consumption in SLEEP1 and wake-up states.
static uint8_t sTestMode = 1;

static bool sLedGpioHigh = false;

static GpioPinMap_t allGpioMap[] =
{
	{0, 15, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{1, 16, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{2, 17, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{3, 18, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{4, 19, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{5, 20, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{6, 21, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{7, 22, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{8, 23, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{9, 24, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{10, 25, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{11, 26, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{12, 11, PAD_MUX_ALT4, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{13, 12, PAD_MUX_ALT4, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	// {14, 13, PAD_MUX_ALT4, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, //Always high level 3.3V, this pin is occupied by uart0 tx output log, because a uart must be reserved in the secondary development sleep state to output the log. So there is no need to debug the GPIO function here.
	// {15, 14, PAD_MUX_ALT4, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, //Always high level 3.3V, this pin is occupied by uart0 rx output log, because a uart must be reserved in the secondary development sleep state to output the log. So there is no need to debug the GPIO function here.
	{16, 31, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{17, 32, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{18, 33, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{19, 34, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{20, 40, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, //External indicator light, the abnormality is only 2.0V when the output is high, and the normal value is 0V when the output is low; disconnect the indicator light and resistor, 3.3V is normal when the output is high, and 0V is normal when the output is low;
	{21, 41, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{22, 42, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, //External button, normal 3.3V when the output is high, abnormal 2.0V when the output is low; disconnect the button and resistor, 3.3V is normal when the output is high, 0V is normal when the output is low;
	// {23, 43, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, //Air600E has no lead
	{24, 44, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{25, 45, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, 
	{26, 46, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, 
	{27, 47, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, 
	{28, 48, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE}, 
	{29, 35, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{30, 36, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
	{31, 37, PAD_MUX_ALT0, PAD_PULL_UP_DISABLE, PAD_PULL_DOWN_DISABLE},
};

void before_sleep(void *pdata, slpManLpState state)
{
	DBG("entry");
	slpManAONIOLatchEn(true);
}


void after_sleep(void *pdata, slpManLpState state)
{
	DBG("entry");	

	PadConfig_t padConfig;
    PAD_getDefaultConfig(&padConfig);

    padConfig.mux = PAD_MUX_ALT0;
    PAD_setPinConfig(LED_GPIO_ADDR, &padConfig); 

    // LED pin config
    GpioPinConfig_t config;
    config.pinDirection = GPIO_DIRECTION_OUTPUT;
    config.misc.initOutput = sLedGpioHigh ? 1 : 0;

    GPIO_pinConfig(LED_GPIO_PORT, LED_GPIO_PIN, &config); 
}


static void task_gpio_test(void *param)
{
	uint8_t i = 0;

	DBG("enter");	

    while(1)
    {
		//In IDLE mode, GPIO output, input, and interrupt testing
		if (1 == sTestMode)		
		{			
			slpManSetPmuSleepMode(true, SLP_IDLE_STATE, false);			

			while(1)
			{
				
				for (i=0; i<sizeof(allGpioMap)/sizeof(allGpioMap[0]); i++)
				{
					GPIO_pinWrite(GET_GPIO_PORT(allGpioMap[i].gpioId), 1 << GET_GPIO_PIN(allGpioMap[i].gpioId), 1 << GET_GPIO_PIN(allGpioMap[i].gpioId));
				}
				DBG("high");
				vTaskDelay(1000);

				for (i=0; i<sizeof(allGpioMap)/sizeof(allGpioMap[0]); i++)
				{
					GPIO_pinWrite(GET_GPIO_PORT(allGpioMap[i].gpioId), 1 << GET_GPIO_PIN(allGpioMap[i].gpioId), 0);
				}
				DBG("low");
				vTaskDelay(1000);
			}
		}
		else if (2 == sTestMode)		
		{		
			uint8_t slpmanLedHandler = 0xff;	
			slpManRet_t ret;
			GpioPinConfig_t gpio_config;

			slpManRegisterUsrdefinedBackupCb(before_sleep, NULL);
			slpManRegisterUsrdefinedRestoreCb(after_sleep, NULL);

			slpManSetPmuSleepMode(true, SLP_SLP1_STATE, false);	
			slpManAONIOLatchEn(true);

			slpManApplyPlatVoteHandle("ledSLP1Vote",&slpmanLedHandler); 
			
			uint32_t startTick;
			//AGPIOWU0 (GPIO20) controls the indicator light, on for 1 second and off for 1 second
			while(1)
			{
				ret = slpManPlatVoteDisableSleep(slpmanLedHandler, SLP_SLP1_STATE);
            	EC_ASSERT(ret == RET_TRUE, ret, 0, 0);
				
				gpio_config.pinDirection = GPIO_DIRECTION_OUTPUT;
				gpio_config.misc.initOutput = 1;
				GPIO_pinConfig(LED_GPIO_PORT, LED_GPIO_PIN, &gpio_config);
				sLedGpioHigh = true;

				ret = slpManPlatVoteEnableSleep(slpmanLedHandler, SLP_SLP1_STATE);
                EC_ASSERT(ret == RET_TRUE, ret, 0, 0);

				startTick = xTaskGetTickCount();
				vTaskDelay(1000);
				DBG("retain high tick %d",xTaskGetTickCount()-startTick);



				ret = slpManPlatVoteDisableSleep(slpmanLedHandler, SLP_SLP1_STATE);
            	EC_ASSERT(ret == RET_TRUE, ret, 0, 0);

				gpio_config.pinDirection = GPIO_DIRECTION_OUTPUT;
				gpio_config.misc.initOutput = 0;
				GPIO_pinConfig(LED_GPIO_PORT, LED_GPIO_PIN, &gpio_config);
				sLedGpioHigh = false;

				ret = slpManPlatVoteEnableSleep(slpmanLedHandler, SLP_SLP1_STATE);
                EC_ASSERT(ret == RET_TRUE, ret, 0, 0);

				startTick = xTaskGetTickCount();
				vTaskDelay(1000);
				DBG("retain low tick %d",xTaskGetTickCount()-startTick);
			}
		}
    }
}


static void task_gpio_test_init(void)
{
	xTaskCreate(task_gpio_test, "task_gpio_test", 256, NULL, 20, NULL);
}

static void all_gpio_init_output(bool wakeup)
{
	if (!wakeup)
	{
		*(uint32_t *)0x4D020170 = 0x1;
		slpManAONIOPowerOn();    
		slpManAONIOVoltSet(IOVOLT_3_30V);
		slpManNormalIOVoltSet(IOVOLT_3_30V);
	}	

	PadConfig_t pad_config;
	GpioPinConfig_t gpio_config;
	PAD_getDefaultConfig(&pad_config);

	uint8_t i = 0;
	for (i=0; i<sizeof(allGpioMap)/sizeof(allGpioMap[0]); i++)
	{
		pad_config.mux = allGpioMap[i].mux;
		pad_config.pullUpEnable = allGpioMap[i].pullUpEnable;
		pad_config.pullDownEnable = allGpioMap[i].pullDownEnable;
		PAD_setPinConfig(allGpioMap[i].pinAddr, &pad_config);
		gpio_config.pinDirection = GPIO_DIRECTION_OUTPUT;
		gpio_config.misc.initOutput = 0;
		GPIO_pinConfig(GET_GPIO_PORT(allGpioMap[i].gpioId), GET_GPIO_PIN(allGpioMap[i].gpioId), &gpio_config);
	}
}


void wakeup_pad_init(void)
{
    APmuWakeupPadSettings_t wakeupPadSetting;
    wakeupPadSetting.negEdgeEn = false;
    wakeupPadSetting.posEdgeEn = false;
    wakeupPadSetting.pullDownEn = false;
    wakeupPadSetting.pullUpEn = false;

    apmuSetWakeupPadCfg(WAKEUP_PAD_3, false, &wakeupPadSetting);
    apmuSetWakeupPadCfg(WAKEUP_PAD_4, false, &wakeupPadSetting);
    apmuSetWakeupPadCfg(WAKEUP_PAD_5, false, &wakeupPadSetting);

	NVIC_DisableIRQ(PadWakeup3_IRQn);
	NVIC_DisableIRQ(PadWakeup4_IRQn);
	NVIC_DisableIRQ(PadWakeup5_IRQn);
}

static void gpio_init(void)
{
	wakeup_pad_init();
	all_gpio_init_output(false);
}

//Start gpio_init, start the position hardware initial level 1
INIT_HW_EXPORT(gpio_init, "1");
//Start task_gpio_test, start location task level 2
INIT_TASK_EXPORT(task_gpio_test_init, "2");

