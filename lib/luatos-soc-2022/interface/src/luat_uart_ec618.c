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

#include "luat_base.h"
#include "luat_uart.h"
#include "luat_rtos.h"

#ifdef __LUATOS__
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_hmeta.h"
#endif

#include <stdio.h>
#include "cms_def.h"
#include "bsp_custom.h"
#include "bsp_common.h"
#include "driver_gpio.h"
#include "driver_uart.h"
#define MAX_DEVICE_COUNT (UART_MAX+1)
static luat_uart_ctrl_param_t uart_cb[MAX_DEVICE_COUNT]={0};
static Buffer_Struct g_s_vuart_rx_buffer;
static uint32_t g_s_vuart_rx_base_len;
static uint8_t g_s_vuart_tx_lock;
typedef struct
{
    timer_t *rs485_timer;
    union
    {
        uint32_t rs485_param;
        struct
        {
            uint32_t wait_time:30;
            uint32_t rx_level:1;
            uint32_t is_485used:1;
        }rs485_param_bit;
    };
    uint32_t rx_buf_size;
    uint8_t alt_type;
    uint8_t rs485_pin;
    struct 
    {
        uint8_t is_enable:1;
        uint8_t is_start:1;
        uint8_t unuse:6;
    }rx_info;
    uint8_t luatos_rx_msg_cnt;
}serials_info;

static serials_info g_s_serials[MAX_DEVICE_COUNT - 1] ={0};

#ifdef __LUATOS__

static int32_t luat_uart_recv_run(lua_State *L, void* ptr)
{
    int uart_id = (int)ptr;
    if (g_s_serials[uart_id].luatos_rx_msg_cnt)
    {
        g_s_serials[uart_id].luatos_rx_msg_cnt--;
    }
    return l_uart_handler(L, ptr);
}

void luat_uart_recv_cb(int uart_id, uint32_t data_len){
    if (g_s_serials[uart_id].luatos_rx_msg_cnt > 5)
    {
        return;
    }
        rtos_msg_t msg;
        msg.handler = luat_uart_recv_run;
        msg.ptr = uart_id;
        msg.arg1 = uart_id;
        msg.arg2 = data_len;
        g_s_serials[uart_id].luatos_rx_msg_cnt++;
        int re = luat_msgbus_put(&msg, 0);
}

void luat_uart_sent_cb(int uart_id, void *param){
        rtos_msg_t msg;
        msg.handler = l_uart_handler;
        msg.ptr = NULL;
        msg.arg1 = uart_id;
        msg.arg2 = 0;
        int re = luat_msgbus_put(&msg, 0);
}
#endif

static LUAT_RT_RET_TYPE luat_uart_wait_timer_cb(LUAT_RT_CB_PARAM)
{
    uint32_t uartid = (uint32_t)param;
    if (g_s_serials[uartid].rs485_param_bit.is_485used) {
        GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
    }
    uart_cb[uartid].sent_callback_fun(uartid, NULL);
}

int luat_uart_pre_setup(int uart_id, uint8_t use_alt_type)
{
    if (uart_id >= MAX_DEVICE_COUNT){
        return 0;
    }
    g_s_serials[uart_id].alt_type = use_alt_type;
}


void luat_uart_sent_dummy_cb(int uart_id, void *param) {;}
void luat_uart_recv_dummy_cb(int uart_id, void *param) {;}

static int32_t luat_uart_cb(void *pData, void *pParam){
    uint32_t uartid = (uint32_t)pData;
    uint32_t State = (uint32_t)pParam;
    uint32_t len;
//    DBG("luat_uart_cb pData:%d pParam:%d ",uartid,State);
    switch (State){
        case UART_CB_TX_BUFFER_DONE:
            if (g_s_serials[uartid].rs485_param_bit.is_485used)
            {
                if (g_s_serials[uartid].rs485_param_bit.wait_time)
                {
                    luat_start_rtos_timer(g_s_serials[uartid].rs485_timer, g_s_serials[uartid].rs485_param_bit.wait_time, 0);
                }
                else
                {
                    GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
                    uart_cb[uartid].sent_callback_fun(uartid, NULL);
                }
            }
            else
            {
                uart_cb[uartid].sent_callback_fun(uartid, NULL);
            }
            break;
        case UART_CB_TX_ALL_DONE:
            if (g_s_serials[uartid].rs485_param_bit.is_485used) {
                GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
            }
            uart_cb[uartid].sent_callback_fun(uartid, NULL);
            break;
        case UART_CB_RX_BUFFER_FULL:
            //Only UART1 can wake up
#ifdef __LUATOS__
            if (UART_ID1 == uartid)
            {
                uart_cb[uartid].recv_callback_fun(uartid, 0xffffffff);
            }
#else
            if (UART_ID1 == uartid)
            {
                uart_cb[uartid].recv_callback_fun(uartid, 0);
            }
#endif
            break;
        case UART_CB_RX_TIMEOUT:
            if (g_s_serials[uartid].rx_info.is_enable)
            {
                g_s_serials[uartid].rx_info.is_start = 0;
            }
            len = Uart_RxBufferRead(uartid, NULL, 0);
            uart_cb[uartid].recv_callback_fun(uartid, len);
            break;
        case UART_CB_RX_NEW:
            len = Uart_RxBufferRead(uartid, NULL, 0);
            if(g_s_serials[uartid].rx_info.is_enable)
            {
                if (!g_s_serials[uartid].rx_info.is_start)
                {
                    g_s_serials[uartid].rx_info.is_start = 1;
                    uart_cb[uartid].recv_callback_fun(uartid, 0xffffffff);
                }
            }
            if (len > g_s_serials[uartid].rx_buf_size)
            {
                uart_cb[uartid].recv_callback_fun(uartid, len);
            }
            break;
        case UART_CB_ERROR:
            break;
    }
}


extern uint8_t luat_gpio_alt_check(uint8_t pin);

int luat_uart_setup(luat_uart_t* uart) {
    if (!luat_uart_exist(uart->id)) {
        DBG("uart.setup FAIL!!!");
        return -1;
    }
    size_t buffsize = uart->bufsz;
    if (buffsize < 2048)
        buffsize = 2048;
    else if (buffsize > 8*1024)
        buffsize = 8*1024;
    if (uart->id >= MAX_DEVICE_COUNT){
        OS_ReInitBuffer(&g_s_vuart_rx_buffer, buffsize);
        g_s_vuart_rx_base_len = g_s_vuart_rx_buffer.MaxLen;
        return 0;
    }
    char model[40] = {0};
    Uart_SetDebug(uart->id, (uart->debug_enable == LUAT_UART_DEBUG_ENABLE)?1:0);
    Uart_SetErrorDropData(uart->id, (uart->error_drop == LUAT_UART_RX_ERROR_DROP_DATA)?1:0);
    switch (uart->id)
    {
    case UART_ID0:
        if (g_s_serials[UART_ID0].alt_type) {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_16, 0), 3, 0, 0);
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_17, 0), 3, 0, 0);
            GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_16, 0), 1, 1);
        }
        else {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_14, 0), 3, 0, 0);
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_15, 0), 3, 0, 0);
            GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_14, 0), 1, 1);
        }
        break;
    case UART_ID1:
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_18, 0), 1, 0, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_19, 0), 1, 0, 0);
        GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_18, 0), 1, 1);
        break;
    case UART_ID2:
#ifdef __LUATOS__
        luat_hmeta_model_name(model);
        if ((1 == g_s_serials[UART_ID2].alt_type) || !strcmp("Air780EG", model) || !strcmp("Air780EG-BD", model) || !strcmp("Air780EUG", model))
#else
        if ((1 == g_s_serials[UART_ID2].alt_type))
#endif
        {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_12, 0), 5, 0, 0);
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_13, 0), 5, 0, 0);
            GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_12, 0), 1, 1);
        }
        else if (2 == g_s_serials[UART_ID2].alt_type)
        {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_6, 0), 3, 0, 0);
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_7, 0), 3, 0, 0);
            GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_6, 0), 1, 1);
        }
        else
        {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_10, 0), 3, 0, 0);
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_11, 0), 3, 0, 0);
            GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_10, 0), 1, 1);
        }
        break;
    default:
        break;
    }
    int parity = 0;
     if (uart->parity == 1)parity = UART_PARITY_ODD;
     else if (uart->parity == 2)parity = UART_PARITY_EVEN;
     int stop_bits = (uart->stop_bits)==1?UART_STOP_BIT1:UART_STOP_BIT2;
     {
         if (!uart_cb[uart->id].recv_callback_fun)
             uart_cb[uart->id].recv_callback_fun = luat_uart_recv_dummy_cb;
         if (!uart_cb[uart->id].sent_callback_fun)
             uart_cb[uart->id].sent_callback_fun = luat_uart_sent_dummy_cb;
         g_s_serials[uart->id].rx_buf_size = buffsize;
         Uart_BaseInitEx(uart->id, uart->baud_rate, 1024, buffsize, (uart->data_bits), parity, stop_bits, luat_uart_cb);
#ifdef __LUATOS__
         g_s_serials[uart->id].rs485_param_bit.is_485used = (uart->pin485 < HAL_GPIO_NONE)?1:0;
#else
         g_s_serials[uart->id].rs485_param_bit.is_485used = (uart->delay > 0)?1:0;
#endif
         g_s_serials[uart->id].rs485_pin = uart->pin485;
         g_s_serials[uart->id].rs485_param_bit.rx_level = uart->rx_level;

         g_s_serials[uart->id].rs485_param_bit.wait_time = uart->delay/1000;
         if (!g_s_serials[uart->id].rs485_param_bit.wait_time)
         {
             g_s_serials[uart->id].rs485_param_bit.wait_time = 1;
         }
         if (g_s_serials[uart->id].rs485_param_bit.is_485used)
         {
            if (!g_s_serials[uart->id].rs485_timer) {
                g_s_serials[uart->id].rs485_timer = luat_create_rtos_timer(luat_uart_wait_timer_cb, (uint32_t*)uart->id, NULL);
            }
            uint8_t alt = luat_gpio_alt_check(g_s_serials[uart->id].rs485_pin);
            GPIO_IomuxEC618(GPIO_ToPadEC618(g_s_serials[uart->id].rs485_pin, alt), alt, 0, 0);
            //GPIO_IomuxEC618(GPIO_ToPadEC618(g_s_serials[uart->id].rs485_pin, 0), 0, 0, 0);
            GPIO_Config(g_s_serials[uart->id].rs485_pin, 0, g_s_serials[uart->id].rs485_param_bit.rx_level);
         }
    }
    return 0;
}

int luat_uart_write(int uartid, void* data, size_t length) {
    if (luat_uart_exist(uartid)) {
        if (uartid >= MAX_DEVICE_COUNT){
            g_s_vuart_tx_lock = 1;
            unsigned i = 0;
            while(i < length)
            {
                if ((length - i) <= 512)
                {
                    g_s_vuart_tx_lock = 0;
                    usb_serial_output(CMS_CHAN_4, data + i, length - i);
                    i = length;
                }
                else
                {
                    usb_serial_output(CMS_CHAN_4, data + i, 512);
                    i += 512;
                }
            }

            return length;
        }else{
            if (g_s_serials[uartid].rs485_param_bit.is_485used) GPIO_Output(g_s_serials[uartid].rs485_pin, !g_s_serials[uartid].rs485_param_bit.rx_level);
            int ret = Uart_TxTaskSafe(uartid, data, length);
            if (ret == 0)
                return length;
            return 0;
        }
    }
    else {
        return -1;
    }
    return 0;
}

void luat_uart_clear_rx_cache(int uartid)
{
    if (luat_uart_exist(uartid)) {
        if (uartid >= MAX_DEVICE_COUNT){
            g_s_vuart_rx_buffer.Pos = 0;
        }
        else
        {
            Uart_RxBufferClear(uartid);
        }
    }
}

int luat_uart_read(int uartid, void* buffer, size_t len) {
    int rcount = 0;
    if (luat_uart_exist(uartid)) {

        if (uartid >= MAX_DEVICE_COUNT){
            if (!buffer)
            {
                return g_s_vuart_rx_buffer.Pos;
            }
            rcount = (g_s_vuart_rx_buffer.Pos > len)?len:g_s_vuart_rx_buffer.Pos;
            memcpy(buffer, g_s_vuart_rx_buffer.Data, rcount);
            OS_BufferRemove(&g_s_vuart_rx_buffer, rcount);
            if (!g_s_vuart_rx_buffer.Pos && g_s_vuart_rx_buffer.MaxLen > g_s_vuart_rx_base_len)
            {
                OS_ReInitBuffer(&g_s_vuart_rx_buffer, g_s_vuart_rx_base_len);
            }
        }
        else
        {
            rcount = Uart_RxBufferRead(uartid, (uint8_t *)buffer, len);
            if (rcount < len)
            {
                g_s_serials[uartid].luatos_rx_msg_cnt = 0;
            }
        }
    }
    return rcount;
}
int luat_uart_close(int uartid) {
    if (luat_uart_exist(uartid)) {
        if (uartid >= MAX_DEVICE_COUNT){
            OS_DeInitBuffer(&g_s_vuart_rx_buffer);
            return 0;
        }
        Uart_DeInit(uartid);
        return 0;
    }
    return -1;
}

int luat_uart_exist(int uartid) {
    // EC618 only supports uart0,uart1,uart2
    // uart0 is the output of ulog
    // uart1 supports downloading and is also available to users
    // uart2 is available to the user
    // if (uartid == 0 || uartid == 1 || uartid == 2) {
    // Currently only supports UART1 and UART2
    if (uartid >= LUAT_VUART_ID_0) uartid = MAX_DEVICE_COUNT - 1;
    return (uartid >= MAX_DEVICE_COUNT)?0:1;
}

#ifdef __LUATOS__
void soc_usb_serial_output_done(uint8_t channel)
{
    if (!g_s_vuart_tx_lock)
    {
        luat_uart_sent_cb(LUAT_VUART_ID_0, NULL);
    }

}
#endif

static void luat_usb_recv_cb(uint8_t channel, uint8_t *input, uint32_t len){
    if (input) {
        if (!g_s_vuart_rx_buffer.MaxLen)
        {
            DBG("usb serial not init,%d,%x,%d!", channel, input, len);
            return;
        }
        OS_BufferWrite(&g_s_vuart_rx_buffer, input, len);
#ifdef __LUATOS__
        rtos_msg_t msg;
        msg.handler = l_uart_handler;
        msg.ptr = NULL;
        msg.arg1 = LUAT_VUART_ID_0;
        msg.arg2 = len;
        int re = luat_msgbus_put(&msg, 0);
#endif
        if (uart_cb[UART_MAX].recv_callback_fun){
            uart_cb[UART_MAX].recv_callback_fun(LUAT_VUART_ID_0,len);
        }
    }else{
        switch(len){
            case 0:
                DBG("usb serial connected");
                break;
            default:
                DBG("usb serial disconnected");
                break;
        }
    }
}

int luat_setup_cb(int uartid, int received, int sent) {
    if (luat_uart_exist(uartid)) {
#ifdef __LUATOS__
        if (uartid >= UART_MAX){
            set_usb_serial_input_callback(luat_usb_recv_cb);
        }else{
            if (received){
                uart_cb[uartid].recv_callback_fun = luat_uart_recv_cb;
            }

            if (sent){
                uart_cb[uartid].sent_callback_fun = luat_uart_sent_cb;
            }
        }
#endif
    }
    return 0;
}

int luat_uart_ctrl(int uart_id, LUAT_UART_CTRL_CMD_E cmd, void* param){
    if (luat_uart_exist(uart_id)) {
        if (uart_id >= MAX_DEVICE_COUNT){
            uart_id = UART_MAX;
            set_usb_serial_input_callback(luat_usb_recv_cb);
        }
        if (cmd == LUAT_UART_SET_RECV_CALLBACK){
            uart_cb[uart_id].recv_callback_fun = param;
        }else if(cmd == LUAT_UART_SET_SENT_CALLBACK){
            uart_cb[uart_id].sent_callback_fun = param;
        }
        return 0;
    }
    return -1;
}

int luat_uart_wait_485_tx_done(int uartid)
{
    int cnt = 0;
    if (luat_uart_exist(uartid)){
        if (g_s_serials[uartid].rs485_param_bit.is_485used) {
            luat_stop_rtos_timer(g_s_serials[uartid].rs485_timer);
            while(!Uart_IsTSREmpty(uartid)) {cnt++;}
            GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
        }
    }
    return cnt;
}

int luat_uart_rx_start_notify_enable(int uart_id, uint8_t is_enable)
{
    if (luat_uart_exist(uart_id)){
        g_s_serials[uart_id].rx_info.is_enable = is_enable;
        return 0;
    }
    return -1;
}

#ifdef __LUATOS__
#include "pssys.h"
#include "ec618.h"
#include "timer.h"
#include "slpman.h"
#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif
#define EIGEN_TIMER(n)             ((TIMER_TypeDef *) (AP_TIMER0_BASE_ADDR + 0x1000*n))
static CommonFun_t irq_cb[4];
static __CORE_FUNC_IN_RAM__ void luat_uart_soft_hwtimer0_callback(void)
{
    __IO uint32_t SR = EIGEN_TIMER(0)->TSR;
    EIGEN_TIMER(0)->TSR = SR;
    irq_cb[0]();
}

static __CORE_FUNC_IN_RAM__ void luat_uart_soft_hwtimer1_callback(void)
{
    __IO uint32_t SR = EIGEN_TIMER(1)->TSR;
    EIGEN_TIMER(1)->TSR = SR;
    irq_cb[1]();
}

static __CORE_FUNC_IN_RAM__ void luat_uart_soft_hwtimer2_callback(void)
{
    __IO uint32_t SR = EIGEN_TIMER(2)->TSR;
    EIGEN_TIMER(2)->TSR = SR;
    irq_cb[2]();
}

static __CORE_FUNC_IN_RAM__ void luat_uart_soft_hwtimer4_callback(void)
{
    __IO uint32_t SR = EIGEN_TIMER(4)->TSR;
    EIGEN_TIMER(4)->TSR = SR;
    irq_cb[3]();
}

int luat_uart_soft_setup_hwtimer_callback(int hwtimer_id, CommonFun_t callback)
{
    TimerConfig_t timerConfig;
    if (!callback)
    {
        TIMER_deInit(hwtimer_id);
        return 0;
    }
    switch(hwtimer_id)
    {
    case 0:
        if (callback)
        {
            CLOCK_setClockSrc(FCLK_TIMER0, FCLK_TIMER0_SEL_26M);
            CLOCK_setClockDiv(FCLK_TIMER0, 1);
            XIC_SetVector(PXIC0_TIMER0_IRQn, luat_uart_soft_hwtimer0_callback);
            XIC_EnableIRQ(PXIC0_TIMER0_IRQn);
        }
        irq_cb[0] = callback;
        break;
    case 1:
        if (callback)
        {
            CLOCK_setClockSrc(FCLK_TIMER1, FCLK_TIMER1_SEL_26M);
            CLOCK_setClockDiv(FCLK_TIMER1, 1);
            XIC_SetVector(PXIC0_TIMER1_IRQn, luat_uart_soft_hwtimer1_callback);
            XIC_EnableIRQ(PXIC0_TIMER1_IRQn);
        }
        irq_cb[1] = callback;
        break;
    case 2:
        if (callback)
        {
            CLOCK_setClockSrc(FCLK_TIMER2, FCLK_TIMER2_SEL_26M);
            CLOCK_setClockDiv(FCLK_TIMER2, 1);
            XIC_SetVector(PXIC0_TIMER2_IRQn, luat_uart_soft_hwtimer2_callback);
            XIC_EnableIRQ(PXIC0_TIMER2_IRQn);
        }
        irq_cb[2] = callback;
        break;
    case 4:
        if (callback)
        {
            CLOCK_setClockSrc(FCLK_TIMER4, FCLK_TIMER4_SEL_26M);
            CLOCK_setClockDiv(FCLK_TIMER4, 1);
            XIC_SetVector(PXIC0_TIMER4_IRQn, luat_uart_soft_hwtimer4_callback);
            XIC_EnableIRQ(PXIC0_TIMER4_IRQn);
        }
        irq_cb[3] = callback;
        break;
    default:
        return -1;
    }

    TIMER_getDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_RELOAD_ON_MATCH0;
    TIMER_init(hwtimer_id, &timerConfig);
    TIMER_interruptConfig(hwtimer_id, TIMER_MATCH0_SELECT, TIMER_INTERRUPT_LEVEL);
    TIMER_interruptConfig(hwtimer_id, TIMER_MATCH1_SELECT, TIMER_INTERRUPT_DISABLED);
    TIMER_interruptConfig(hwtimer_id, TIMER_MATCH2_SELECT, TIMER_INTERRUPT_DISABLED);
    return 0;
}
void __CORE_FUNC_IN_RAM__ luat_uart_soft_gpio_fast_output(int pin, uint8_t value)
{
    GPIO_FastOutput(pin, value);
}
uint8_t __CORE_FUNC_IN_RAM__ luat_uart_soft_gpio_fast_input(int pin)
{
    return GPIO_Input(pin);
}
void __CORE_FUNC_IN_RAM__ luat_uart_soft_gpio_fast_irq_set(int pin, uint8_t on_off)
{
    if (on_off)
    {
        GPIO_ExtiConfig(pin, 0, 0, 1);
    }
    else
    {
        GPIO_ExtiConfig(pin, 0, 0, 0);
    }
}
uint32_t luat_uart_soft_cal_baudrate(uint32_t baudrate)
{
    return (26000000/baudrate);
}
void __CORE_FUNC_IN_RAM__ luat_uart_soft_hwtimer_onoff(int hwtimer_id, uint32_t period)
{
    EIGEN_TIMER(hwtimer_id)->TCCR = 0;

    // Enable TIMER IRQ
    if (period)
    {
        EIGEN_TIMER(hwtimer_id)->TMR[0] = period - 1;
        EIGEN_TIMER(hwtimer_id)->TCCR = 1;
    }
}

void __CORE_FUNC_IN_RAM__ luat_uart_soft_sleep_enable(uint8_t is_enable)
{
    slpManDrvVoteSleep(SLP_VOTE_LPUSART, is_enable?SLP_HIB_STATE:SLP_ACTIVE_STATE);
}
#endif
