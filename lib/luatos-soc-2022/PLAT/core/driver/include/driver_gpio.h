/**
 * @file driver_gpio.h
 * @brief When using the API of this Modules, you cannot use the original GPIO API and wakeupPAD related API, but you can use the PAD ordinary API. All APIs are task unsafe and interrupt unsafe! ! !
 * @version 0.1
 * @date 2022-10-25
 *
 * @copyright
 **/

#ifndef __CORE_GPIO_H__
#define __CORE_GPIO_H__
#include "bsp_common.h"
/**
 * @brief GPIO global initialization
 *
 * @param Fun interrupt callback function, during callback, PIN number is pData, void *->uint32_t. Here are unified settings for all GPIOs. You can leave them blank and then configure each GPIO individually.*/
void GPIO_GlobalInit(CBFuncEx_t Fun);

/**
 * @brief GPIO initialization
 *
 * @param Pin Pin serial number
 * @param IsInput Whether it is an input function, 1 yes, 0 no
 * @param InitValue initial level, 1 high, 0 low, only valid for output*/
void GPIO_Config(uint32_t Pin, uint8_t IsInput, uint8_t InitValue);

/**
 * @brief GPIO initialization and synchronous control of pull-up and pull-down resistors to enhance driving force
 *
 * @param Pin Pin serial number
 * @param IsInput Whether it is an input function, 1 yes, 0 no
 * @param InitValue initial level, 1 is high and pulls up, 0 is low and pulls down
 * @param AltFun multiplexing function, most Pads and GPIOs are in one-to-one correspondence, if there are multiple Pads corresponding to 1 GPIO, if AltFun cannot match, the PAD of Alt0 will be used by default*/
void GPIO_ConfigWithPullEC618(uint32_t Pin, uint8_t IsInput, uint8_t InitValue, uint8_t AltFun);

/**
 * @brief GPIO pull-up and pull-down control
 *
 * @param Pad Pad serial number, please note that this is not the Pin serial number
 * @param IsPull Whether pull-down is required
 * @param IsUp whether to pull up*/
void GPIO_PullConfig(uint32_t Pad, uint8_t IsPull, uint8_t IsUp);

/**
 * @brief GPIO external interrupt initialization, GPIO20~22 is only configured as a dual-edge interrupt because the wakeuppad function is turned on, so there is a weak pull-up!
 *
 * @param Pin Pin serial number
 * @param IsLevel Whether it is a level interrupt, 0 edge type, 1 level type
 * @param IsRiseHigh Rising edge or high level. Most GPIOs of EC618 can only be interrupted on a single edge, so the rising edge and falling edge are configured at the same time and set to the rising edge. GPIO20~22 can achieve dual edge triggering because it overlaps with wakeuppad.
 * @param IsFallLow falling edge or low level*/
void GPIO_ExtiConfig(uint32_t Pin, uint8_t IsLevel, uint8_t IsRiseHigh, uint8_t IsFallLow);

/**
 * @brief GPIO multiplexing function
 *
 * @param Pad Pad serial number, please note that this is not the Pin serial number
 * @param Function Multiplexing function, this needs to be determined according to the actual situation of the chip, currently it is 0~7
 * @param AutoPull The default configuration of whether to enable peripheral functions by pulling up and down, 1 to enable, 0 to disable. When GPIO_PullConfig is called, it will be automatically closed. If configured as a peripheral function, it is recommended to enable it.
 * @param IsInputBuffer Whether the input buffering function is enabled, 1 is on, 0 is off. If it is an interruption, it is recommended to enable it.
 **/
void GPIO_IomuxEC618(uint32_t Pad, uint32_t Function, uint8_t AltFunctionPull, uint8_t IsInputBuffer);
/**
 * @brief GPIO output level
 *
 * @param Pin Pin serial number
 * @param Level 1 high level, 0 low level*/
void GPIO_Output(uint32_t Pin, uint8_t Level);
void GPIO_FastOutput(uint32_t Pin, uint8_t Level);
/**
 * @brief GPIO output level, and synchronously controls the pull-up and pull-down resistors to enhance the driving force. At the same time, we hope to solve the problem of not being able to maintain the level in sleep mode.
 *
 * @param Pin Pin serial number
 * @param Level 1 high level, 0 low level
 * @param AltFun multiplexing function, most Pads and GPIOs are in one-to-one correspondence, if there are multiple Pads corresponding to 1 GPIO, if AltFun cannot match, the PAD of Alt0 will be used by default*/
void GPIO_OutputWithPullEC618(uint32_t Pin, uint8_t Level, uint8_t AltFun);

/**
 * @brief Read GPIO input level
 *
 * @param Pin Pin serial number
 * @return 1 high level, 0 low level, others are invalid*/
uint8_t GPIO_Input(uint32_t Pin);

/**
 * @brief flip GPIO output level
 *
 * @param Pin Pin serial number
 * @return none*/
void GPIO_Toggle(uint32_t Pin);
/**
 * @brief GPIO multiple pin output levels on the same port, effective for distribution similar to STM32GPIO
 *
 * @param Port port number 0 or 1
 * @param Pins Pin serial number combination
 * @param Level 1 high level, 0 low level*/
void GPIO_OutputMulti(uint32_t Port, uint32_t Pins, uint32_t Level);

/**
 * @brief Read the input levels of multiple pins on the same GPIO port. If it is port 1, the bits corresponding to GPIO20~GPIO22 may be abnormal, so you need to use GPIO_Input
 *
 * @param Port port number 0 or 1
 * @return 0x0000~0xffff, each bit represents the level of a pin*/
uint32_t GPIO_InputMulti(uint32_t Port);

/**
 * @brief Flip the output levels of multiple pins on the same GPIO port
 * @param Port port number
 * @param Pins Pin serial number combination
 * @return none*/
void GPIO_ToggleMulti(uint32_t Port, uint32_t Pins);

/**
 * @brief GPIO analog single line output mode
 * @param Pin Pin serial number
 * @param Data output level sequence
 * @param BitLen How many bits are there in the output level sequence?
 * @param Delay The delay between each bit
 * @return none*/
void GPIO_OutPulse(uint32_t Pin, uint8_t *Data, uint16_t BitLen, uint16_t Delay);

/**
 * @brief Set external interrupt. Note that ordinary GPIO cannot be used in low power mode and can only be woken up with wakepad.
 * @param Pin Pin serial number, WAKEUPPAD3~5 has been reused with GPIO, additionally HAL_WAKEUP_0~HAL_WAKEUP_2 can be used
 * @param CB interrupt callback function
 * @param Pins param when interrupting callback
 * @return none*/
void GPIO_ExtiSetCB(uint32_t Pin, CBFuncEx_t CB, void *pParam);

/**
 * @brief GPIO is initialized to OD gate output
 * @param Pin Pin serial number
 * @param InitValue initial level
 * @return none*/
void GPIO_ODConfig(uint32_t Pin, uint8_t InitValue);

/**
 * @brief Convert the Pad serial number from the GPIO serial number. If there are multiple Pad serial numbers, AltFun will be used to determine which one to use.
 *
 * @param Pin Pin serial number
 * @param AltFun multiplexing function, most Pads and GPIOs are in one-to-one correspondence, if there are multiple Pads corresponding to 1 GPIO, if AltFun cannot match, the PAD of Alt0 will be used by default
 * @return Pad serial number*/
uint32_t GPIO_ToPadEC618(uint32_t Pin, uint8_t AltFun);

/**
 * @brief Set WAKEUPPAD0,1,2
 *
 * @param Pin Pin serial number, HAL_WAKEUP_0~HAL_WAKEUP_2
 * @param IsRiseHigh rising edge trigger
 * @param IsFallLow falling edge trigger
 * @param Pullup pull up
 * @param Pulldown drop-down*/
void GPIO_WakeupPadConfig(uint32_t Pin, uint8_t IsRiseHigh, uint8_t IsFallLow, uint8_t Pullup, uint8_t Pulldown);

void GPIO_DriverWS2812B(uint32_t Pin, uint8_t *Data, uint32_t Len, uint8_t Bit0H, uint8_t Bit0L, uint8_t Bit1H, uint8_t Bit1L);
#endif
