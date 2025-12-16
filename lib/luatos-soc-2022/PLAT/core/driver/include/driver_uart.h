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

#ifndef __CORE_UART_H__
#define __CORE_UART_H__
#include "bsp_common.h"
/**
 * @brief Perform basic initialization work on the serial port
 *
 * @param UartID serial port number 0~MAX, corresponding to USART1~UARTX, 0 is used for log output by default
 * @param BaudRate baud rate
 * @param TxCacheLen Send the initial cache, if it is unilog, you can write 0
 * @param RxCacheLen receives the initial cache
 * @param DataBits data bits 5~8
 * @param Parity parity check, optional:
 * UART_PARITY_NONE,
UART_PARITY_ODD,
UART_PARITY_EVEN,
 * @param StopBits stop bits
 * UART_STOP_BIT1,
UART_STOP_BIT1_5,
UART_STOP_BIT2,
 * @param CB callback function, used to notify whether new data arrives, DMA TX or RX is completed, and whether there are errors*/
void Uart_BaseInitEx(uint8_t UartID, uint32_t BaudRate, uint32_t TxCacheLen, uint32_t RxCacheLen, uint8_t DataBits, uint8_t Parity, uint8_t StopBits, CBFuncEx_t CB);


void Uart_SetCb(uint8_t UartID, CBFuncEx_t CB);

void Uart_DeInit(uint8_t UartID);

uint8_t Uart_IsTSREmpty(uint8_t UartID);

//void Uart_Tx(uint8_t UartID, const uint8_t *Data, uint32_t Len);


int Uart_RxBufferCB(uint8_t UartID, CBFuncEx_t CB);

int Uart_TxTaskSafe(uint8_t UartID, const void *Data, uint32_t Len);

uint32_t Uart_RxTaskSafe(uint8_t UartID, uint8_t *Data, uint32_t Len);

void Uart_TxStop(uint8_t UartID);

int Uart_RxBufferRead(uint8_t UartID, uint8_t *Data, uint32_t Len);
void Uart_RxBufferClear(uint8_t UartID);

void Uart_PrintReg(uint8_t UartID);
void Uart_ChangeBR(uint8_t UartID, uint32_t BaudRate);
uint32_t Uart_GetLastError(uint8_t UartID);
void Uart_SetDebug(uint8_t UartID, uint8_t OnOff);
void Uart_SetErrorDropData(uint8_t UartID, uint8_t IsDrop);
void LPUart_SpecialConfig(int16_t RxDLROffsetTable[5], uint8_t DisableLP);
#endif
