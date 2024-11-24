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

#ifndef LUAT_DEBUG_H
#define LUAT_DEBUG_H
#include "luat_base.h"
/**
 * @defgroup luatos_debug debugging interface
 * @{*/

/**
 * @brief System processing after exception occurs
 **/
typedef enum LUAT_DEBUG_FAULT_MODE
{
	LUAT_DEBUG_FAULT_RESET, /**< Restart after an exception occurs. It is strongly recommended for batch products.*/
	LUAT_DEBUG_FAULT_HANG, /**< crashes after an exception occurs. It is strongly recommended to use it during the testing phase.*/
	LUAT_DEBUG_FAULT_HANG_RESET, /**< After an exception occurs, try to upload the crash information to the PC tool. Restart after the upload is successful or times out. If there is no requirement for the crash reset time, you can choose to use this mode in the initial batch.*/
	LUAT_DEBUG_FAULT_SAVE_RESET /**< After an exception occurs, save the information to flash and then restart.*/
}LUAT_DEBUG_FAULT_MODE_E;

/**
 * @brief format print and output to LOG port
 * Prioritize trying to print soc log (USB port output log luatools view)
 * Secondly epat log (USB port output log EPAT view)
 * Finally uart0 log (UART0 port output log EPAT view)
 * When viewing the log with luatools: up to 64 bytes of data can be printed in the interrupt, and up to 1k bytes of data can be printed in the task; when called in the task, a certain amount of stack space is required
 *
 * @param fmt format
 * @param ... subsequent variables*/
void luat_debug_print(const char *fmt, ...);
/**
 * @brief luat_debug_print macro is defined as LUAT_DEBUG_PRINT
 * @param fmt format
 * @param ... subsequent variables*/
#define LUAT_DEBUG_PRINT(fmt, argv...) luat_debug_print("%s %d:"fmt, __FUNCTION__,__LINE__, ##argv)

/**
 * @brief Assertion processing and format printing output to the LOG port
 *
 * @param fun_name asserted function
 * @param line_no line number
 * @param fmt format
 * @param ... subsequent variables*/
void luat_debug_assert(const char *fun_name, unsigned int line_no, const char *fmt, ...);


#define LUAT_DEBUG_ASSERT(condition, fmt, argv...)  do {  \
														{ \
															if((condition) == 0) \
															{ \
																luat_debug_assert(__FUNCTION__, __LINE__, fmt, ##argv); \
															}\
														} \
													} while(0) ///< The luat_debug_assert macro is defined as LUAT_DEBUG_ASSERT

/**
 * @brief Set the system processing mode after an exception occurs
 *
 * @param mode processing mode LUAT_DEBUG_FAULT_RESET restart mode
                        LUAT_DEBUG_FAULT_HANG crash mode*/
void luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_MODE_E mode);

/**
 * @brief Whether to enable/stop csdk log
 *
 * @param onoff switch 0 off 1 on, the default is on when the machine is turned on*/
void luat_debug_print_onoff(unsigned char onoff);

void luat_debug_dump(uint8_t *data, uint32_t len);
/** @}*/

#endif
