/*@Modules uart
@summary Serial port operation library
@version 1.0
@date 2020.03.30
@demo uart
@video https://www.bilibili.com/video/BV1er4y1p75y
@tag LUAT_USE_UART*/
#include "luat_base.h"
#include "luat_uart.h"
#include "luat_mem.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "string.h"
#include "luat_zbuff.h"
#include "luat_gpio.h"
#define LUAT_LOG_TAG "uart"
#include "luat_log.h"
#ifndef LUAT_UART_RX_MAX_LEN
#define LUAT_UART_RX_MAX_LEN 0x10000
#endif
#define MAX_DEVICE_COUNT 9
#define MAX_USB_DEVICE_COUNT 1
typedef struct luat_uart_cb {
    int received;//callback function
    int sent;//callback function
} luat_uart_cb_t;
static luat_uart_cb_t uart_cbs[MAX_DEVICE_COUNT + MAX_USB_DEVICE_COUNT];
static luat_uart_recv_callback_t uart_app_recvs[MAX_DEVICE_COUNT + MAX_USB_DEVICE_COUNT];
#ifdef LUAT_USE_SOFT_UART
#ifndef __LUAT_C_CODE_IN_RAM__
#define __LUAT_C_CODE_IN_RAM__
#endif
#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif
#define LUAT_UART_SOFT_FIFO_CNT (128)
typedef struct
{
	llist_head tx_node;
	uint8_t *data;
	uint32_t len;
}luat_uart_soft_tx_node_t;

typedef struct
{
	uint32_t tx_period;
	uint32_t rx_period;
	uint32_t stop_period;
	int tx_adjust_period;
	int rx_adjust_period;
	Buffer_Struct rx_buffer;
	Buffer_Struct tx_buffer;
	llist_head tx_queue_head;
	uint16_t rx_bit;
	uint16_t rx_buffer_size;
	uint16_t tx_bit;
	uint8_t rx_fifo[LUAT_UART_SOFT_FIFO_CNT];
	uint8_t rx_fifo_cnt;
	uint8_t tx_shift_bits;
	uint8_t rx_shift_bits;
	uint8_t data_bits;
	uint8_t total_bits;
	uint8_t rx_parity_bit;
	uint8_t parity;             /**< parity bit*/
	uint8_t parity_odd;             /**< parity bit*/
	uint8_t tx_pin;
	uint8_t rx_pin;
	uint8_t tx_hwtimer_id;
	uint8_t rx_hwtimer_id;
	uint8_t pin485;
    uint8_t rs485_rx_level;           /**<Level in receiving direction*/
    uint8_t uart_id;
    uint8_t is_inited;
    uint8_t is_tx_busy;
    uint8_t is_rx_busy;
}luat_uart_soft_t;
static luat_uart_soft_t *prv_uart_soft;

static int32_t luat_uart_soft_del_tx_queue(void *pdata, void *param)
{
	luat_uart_soft_tx_node_t *node = (luat_uart_soft_tx_node_t *)pdata;
	luat_heap_alloc(NULL, node->data, 0, 0);
	return LIST_DEL;
}

static uint16_t __LUAT_C_CODE_IN_RAM__ luat_uart_soft_check_party(uint8_t data, uint8_t data_bits, uint8_t is_odd)
{
	uint16_t data_bits2 = data_bits;
	uint8_t party_bits = is_odd;
	while(party_bits)
	{
		party_bits += (data & 0x01);
		data >>= 1;
		party_bits--;
	};
	if (party_bits & 0x01)
	{
		return 1 << data_bits2;
	}
	return 0;
}

static int __LUAT_C_CODE_IN_RAM__ luat_uart_soft_recv_start_irq(int pin, void *param)
{
	luat_uart_soft_hwtimer_onoff(prv_uart_soft->rx_hwtimer_id, prv_uart_soft->rx_period + (prv_uart_soft->rx_period >> 4));
	prv_uart_soft->rx_shift_bits = 0;
	prv_uart_soft->rx_parity_bit = prv_uart_soft->parity_odd;
	luat_uart_soft_gpio_fast_irq_set(pin, 0);
	luat_uart_soft_sleep_enable(0);
	return 0;
}

static int luat_uart_soft_setup(luat_uart_t *uart)
{
	prv_uart_soft->rx_buffer_size = uart->bufsz * 2;
	prv_uart_soft->rx_buffer.MaxLen = uart->bufsz * 2;
	luat_heap_alloc(NULL, prv_uart_soft->rx_buffer.Data, 0, 0);
	prv_uart_soft->rx_buffer.Data = luat_heap_alloc(NULL, NULL, 0, prv_uart_soft->rx_buffer_size);
	if (!prv_uart_soft->rx_buffer.Data)
	{
		LLOGE("soft uart no mem!");
		prv_uart_soft->is_inited = 0;
		return -1;
	}
	prv_uart_soft->is_inited = 1;
	prv_uart_soft->data_bits = uart->data_bits;
	switch(uart->parity)
	{
	case LUAT_PARITY_NONE:
		prv_uart_soft->parity = 0;
		break;
	case LUAT_PARITY_ODD:
		prv_uart_soft->parity = 1;
		prv_uart_soft->parity_odd = 1;
		break;
	case LUAT_PARITY_EVEN:
		prv_uart_soft->parity = 1;
		prv_uart_soft->parity_odd = 0;
		break;
	}
	prv_uart_soft->parity = uart->parity;
	if (prv_uart_soft->tx_adjust_period < 0)
	{
		prv_uart_soft->tx_period = luat_uart_soft_cal_baudrate(uart->baud_rate) - (-prv_uart_soft->tx_adjust_period);
	}
	else
	{
		prv_uart_soft->tx_period = luat_uart_soft_cal_baudrate(uart->baud_rate) + prv_uart_soft->tx_adjust_period;
	}
	if (prv_uart_soft->rx_adjust_period < 0)
	{
		prv_uart_soft->rx_period = luat_uart_soft_cal_baudrate(uart->baud_rate) - (-prv_uart_soft->rx_adjust_period);
	}
	else
	{
		prv_uart_soft->rx_period = luat_uart_soft_cal_baudrate(uart->baud_rate) + prv_uart_soft->rx_adjust_period;
	}

//	LLOGD("soft uart period %u,%u!", prv_uart_soft->tx_period, prv_uart_soft->rx_period);
	switch(uart->stop_bits)
	{
	case 2:
		prv_uart_soft->total_bits = prv_uart_soft->data_bits + prv_uart_soft->parity + 4;
		prv_uart_soft->stop_period = luat_uart_soft_cal_baudrate(uart->baud_rate) * 3;
		break;
	default:
		prv_uart_soft->total_bits = prv_uart_soft->data_bits + prv_uart_soft->parity + 2;
		prv_uart_soft->stop_period = luat_uart_soft_cal_baudrate(uart->baud_rate) * 2;
		break;
	}
	if (uart->pin485 != 0xffffffff)
	{
		prv_uart_soft->pin485 = uart->pin485;
		prv_uart_soft->rs485_rx_level = uart->rx_level;
	}
	else
	{
		prv_uart_soft->pin485 = 0xff;
	}

	luat_gpio_t conf = {0};
	conf.pin = prv_uart_soft->rx_pin;
	conf.mode = Luat_GPIO_IRQ;
	conf.irq_cb = luat_uart_soft_recv_start_irq;
	conf.pull = LUAT_GPIO_PULLUP;
	conf.irq = LUAT_GPIO_FALLING_IRQ;
	conf.alt_func = -1;
	luat_gpio_setup(&conf);
	conf.pin = prv_uart_soft->tx_pin;
	conf.mode = Luat_GPIO_OUTPUT;
	luat_gpio_setup(&conf);
	luat_uart_soft_gpio_fast_output(prv_uart_soft->tx_pin, 1);
	if (prv_uart_soft->pin485 != 0xff)
	{
		conf.pin = prv_uart_soft->pin485;
		conf.mode = Luat_GPIO_OUTPUT;
		luat_gpio_set(prv_uart_soft->pin485, prv_uart_soft->rs485_rx_level);
		luat_gpio_setup(&conf);
	}
	prv_uart_soft->rx_shift_bits = 0;
	prv_uart_soft->tx_shift_bits = 0;
	prv_uart_soft->rx_fifo_cnt = 0;
	prv_uart_soft->is_tx_busy = 0;
	prv_uart_soft->is_rx_busy= 0;
	luat_uart_soft_sleep_enable(1);
	return 0;
}

static void luat_uart_soft_close(void)
{
	luat_uart_soft_hwtimer_onoff(prv_uart_soft->tx_hwtimer_id, 0);
	luat_uart_soft_hwtimer_onoff(prv_uart_soft->rx_hwtimer_id, 0);
	luat_uart_soft_setup_hwtimer_callback(prv_uart_soft->tx_hwtimer_id, NULL);
	luat_uart_soft_setup_hwtimer_callback(prv_uart_soft->rx_hwtimer_id, NULL);
	if (prv_uart_soft->is_inited)
	{
		luat_uart_soft_tx_node_t *node;
		while (!llist_empty(&prv_uart_soft->tx_queue_head))
		{
			node = (luat_uart_soft_tx_node_t *)(prv_uart_soft->tx_queue_head.next);
			llist_del(&node->tx_node);
			luat_heap_alloc(NULL, node->data, 0, 0);
			luat_heap_alloc(NULL, node, 0, 0);
		}
	}
	prv_uart_soft->is_inited = 0;
	luat_gpio_close(prv_uart_soft->rx_pin);
	luat_gpio_close(prv_uart_soft->tx_pin);
	if (prv_uart_soft->pin485 != 0xff)
	{
		luat_gpio_close(prv_uart_soft->pin485);
	}
	luat_heap_alloc(NULL, prv_uart_soft->rx_buffer.Data, 0, 0);
	memset(&prv_uart_soft->rx_buffer, 0, sizeof(Buffer_Struct));
	memset(&prv_uart_soft->tx_buffer, 0, sizeof(Buffer_Struct));
	prv_uart_soft->is_tx_busy = 0;
	prv_uart_soft->is_rx_busy= 0;
	luat_uart_soft_sleep_enable(1);
}

static uint32_t luat_uart_soft_read(uint8_t *data, uint32_t len)
{
//	if (!data) return prv_uart_soft->rx_buffer.Pos;
	uint32_t read_len = (len > prv_uart_soft->rx_buffer.Pos)?prv_uart_soft->rx_buffer.Pos:len;
	memcpy(data, prv_uart_soft->rx_buffer.Data, read_len);
	if (read_len >= prv_uart_soft->rx_buffer.Pos)
	{
		prv_uart_soft->rx_buffer.Pos = 0;
//		if (prv_uart_soft->rx_buffer.MaxLen > prv_uart_soft->rx_buffer_size)
//		{
//			luat_heap_alloc(NULL, prv_uart_soft->rx_buffer.Data, 0, 0);
//			prv_uart_soft->rx_buffer.Data = luat_heap_alloc(NULL, NULL, 0, prv_uart_soft->rx_buffer_size);
//		}
	}
	else
	{
		uint32_t rest = prv_uart_soft->rx_buffer.Pos - read_len;
		memmove(prv_uart_soft->rx_buffer.Data, prv_uart_soft->rx_buffer.Data + read_len, rest);
		prv_uart_soft->rx_buffer.Pos = rest;
	}
	return read_len;
}

static int luat_uart_soft_write(const uint8_t *data, uint32_t len)
{
	luat_uart_soft_tx_node_t *node = luat_heap_alloc(NULL, NULL, 0, sizeof(luat_uart_soft_tx_node_t));
	if (!node)
	{
		return -1;
	}
	node->data = luat_heap_alloc(NULL, NULL, 0, len);
	if (!data)
	{
		luat_heap_alloc(NULL, node, 0, 0);
		return -1;
	}
	memcpy(node->data, data, len);
	node->len = len;
	llist_add_tail(&node->tx_node, &prv_uart_soft->tx_queue_head);
	if (!prv_uart_soft->tx_buffer.Data)
	{
		if (prv_uart_soft->pin485 != 0xff)
		{
			luat_gpio_set(prv_uart_soft->pin485, !prv_uart_soft->rs485_rx_level);
		}
		node = (luat_uart_soft_tx_node_t *)prv_uart_soft->tx_queue_head.next;
		Buffer_StaticInit(&prv_uart_soft->tx_buffer, node->data, node->len);
		prv_uart_soft->tx_shift_bits = 0;
		luat_uart_soft_hwtimer_onoff(prv_uart_soft->tx_hwtimer_id, prv_uart_soft->tx_period);
		luat_uart_soft_gpio_fast_output(prv_uart_soft->tx_pin, 0);
		if (prv_uart_soft->parity)
		{
			prv_uart_soft->tx_bit = (0xffff << prv_uart_soft->data_bits) | luat_uart_soft_check_party(prv_uart_soft->tx_buffer.Data[0], prv_uart_soft->data_bits, prv_uart_soft->parity_odd) | prv_uart_soft->tx_buffer.Data[0];
		}
		else
		{
			prv_uart_soft->tx_bit = (0xffff << prv_uart_soft->data_bits) | prv_uart_soft->tx_buffer.Data[0];
		}
	}
	prv_uart_soft->is_tx_busy = 1;
	luat_uart_soft_sleep_enable(0);
	return 0;
}
#endif
void luat_uart_set_app_recv(int id, luat_uart_recv_callback_t cb) {
    if (luat_uart_exist(id)) {
        uart_app_recvs[id] = cb;
        luat_setup_cb(id, 1, 0); // temporarily overwrite
    }
}

int l_uart_handler(lua_State *L, void* ptr) {
    (void)ptr;
    //LLOGD("l_uart_handler");
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_pop(L, 1);
    int uart_id = msg->arg1;
    if (!luat_uart_exist(uart_id)) {
        //LLOGW("not exist uart id=%ld but event fired?!", uart_id);
        return 0;
    }
    int org_uart_id = uart_id;
	#if !defined(LUAT_USE_WINDOWS) && !defined(LUAT_USE_LINUX) && !defined(LUAT_USE_MACOS)
    if (uart_id >= LUAT_VUART_ID_0)
    {
    	uart_id = MAX_DEVICE_COUNT + uart_id - LUAT_VUART_ID_0;
    }
	#endif
    // sent event
    if (msg->arg2 == 0) {
        //LLOGD("uart%ld sent callback", uart_id);
        if (uart_cbs[uart_id].sent) {
            lua_geti(L, LUA_REGISTRYINDEX, uart_cbs[uart_id].sent);
            if (lua_isfunction(L, -1)) {
                lua_pushinteger(L, org_uart_id);
                lua_call(L, 1, 0);
            }
        }
    }
    else {
        if (uart_app_recvs[uart_id]) {
            uart_app_recvs[uart_id](uart_id, msg->arg2);
        }
        if (uart_cbs[uart_id].received) {
            lua_geti(L, LUA_REGISTRYINDEX, uart_cbs[uart_id].received);
            if (lua_isfunction(L, -1)) {
                lua_pushinteger(L, org_uart_id);
                lua_pushinteger(L, msg->arg2);
                lua_call(L, 2, 0);
            }
            else {
                //LLOGD("uart%ld received callback not function", uart_id);
            }
        }
        else {
            //LLOGD("uart%ld no received callback", uart_id);
        }
    }

    // Return empty data to rtos.recv method
    lua_pushinteger(L, 0);
    return 1;
}

/*Configure serial port parameters
@api uart.setup(id, baud_rate, data_bits, stop_bits, partiy, bit_order, buff_size, rs485_gpio, rs485_level, rs485_delay, debug_enable, error_drop)
@int serial port id, write 0 for uart0, write 1 for uart1, and so on, the maximum value depends on the device
@int baud rate, default 115200, optional baud rate table: {2000000,921600,460800,230400,115200,57600,38400,19200,9600,4800,2400}
@int data bits, default is 8, optional 7/8
@int Stop bit, the default is 1, depending on the actual situation, it can be 0.5/1/1.5/2, etc.
@int check digit, optional uart.None/uart.Even/uart.Odd
@int Big and small endian, default little endian uart.LSB, optional uart.MSB
@int buffer size, default value 1024
@int Conversion GPIO in 485 mode, default value 0xffffffff
@int 485 mode rx direction GPIO level, default value 0
@int 485 mode tx to rx conversion delay time, the default value is 12bit time, unit us, 9600 baud rate fill in 20000
@int Turn on debugging function, enabled by default, fill in uart.DEBUG or non-numeric enable, other values     are turned off, currently only supported by Yixin platform
@int Whether to give up cached data when encountering a receiving error. It is enabled by default. Fill in uart.ERROR_DROP or non-numeric enable. Other values     are closed. Currently only the core platform supports it.
@return int Returns 0 on success, other values   on failure
@usage
--The most commonly used 115200 8N1
uart.setup(1, 115200, 8, 1, uart.NONE)
-- Can be abbreviated as uart.setup(1)

-- 485 automatic switching, select GPIO10 as the transceiver conversion pin
uart.setup(1, 115200, 8, 1, uart.NONE, uart.LSB, 1024, 10, 0, 2000)
-- Do not discard cached data when encountering receiving errors
uart.setup(1, 115200, 8, 1, uart.NONE, nil, 1024, nil, nil, nil, nil, 0)*/
static int l_uart_setup(lua_State *L)
{
    lua_Number stop_bits = luaL_optnumber(L, 4, 1);
    luat_uart_t uart_config = {
        .id = luaL_checkinteger(L, 1),
        .baud_rate = luaL_optinteger(L, 2, 115200),
        .data_bits = luaL_optinteger(L, 3, 8),
        .parity = luaL_optinteger(L, 5, LUAT_PARITY_NONE),
        .bit_order = luaL_optinteger(L, 6, LUAT_BIT_ORDER_LSB),
        .bufsz = luaL_optinteger(L, 7, 1024),
        .pin485 = luaL_optinteger(L, 8, 0xffffffff),
        .rx_level = luaL_optinteger(L, 9, 0),
    };
    if(stop_bits == 0.5)
        uart_config.stop_bits = LUAT_0_5_STOP_BITS;
    else if(stop_bits == 1.5)
        uart_config.stop_bits = LUAT_1_5_STOP_BITS;
    else
        uart_config.stop_bits = (uint8_t)stop_bits;

    uart_config.delay = luaL_optinteger(L, 10, 12000000/uart_config.baud_rate);
    uart_config.debug_enable = luaL_optinteger(L, 11, LUAT_UART_DEBUG_ENABLE);
    uart_config.error_drop = luaL_optinteger(L, 12, LUAT_UART_RX_ERROR_DROP_DATA);
#ifdef LUAT_USE_SOFT_UART
    int result;
    if (prv_uart_soft && (prv_uart_soft->uart_id == uart_config.id))
    {
    	result = luat_uart_soft_setup(&uart_config);
    }
    else
    {
    	result = luat_uart_setup(&uart_config);
    }
    lua_pushinteger(L, result);
#else
    int result = luat_uart_setup(&uart_config);
    lua_pushinteger(L, result);
#endif
    return 1;
}

/*Write serial port
@api uart.write(id, data)
@int serial port id, write 0 for uart0, write 1 for uart1
@string/zbuff The data to be written. If it is zbuff, it will be read from the starting position of the pointer.
@int Optional, the length of data to be sent, the default is to send all
@return int successful data length
@usage
-- Write visible string
uart.write(1, "rdy\r\n")
--Write hexadecimal data string
uart.write(1, string.char(0x55,0xAA,0x4B,0x03,0x86))*/
static int l_uart_write(lua_State *L)
{
    size_t len;
    const char *buf;
    uint8_t id = luaL_checkinteger(L, 1);
    if(lua_isuserdata(L, 2))
    {
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        len = buff->len - buff->cursor;
        buf = (const char *)(buff->addr + buff->cursor);
    }
    else
    {
        buf = lua_tolstring(L, 2, &len);//Get string data
    }
    if(lua_isinteger(L, 3))
    {
        size_t l = luaL_checkinteger(L, 3);
        if(len > l)
            len = l;
    }
#ifdef LUAT_USE_SOFT_UART
    int result;
    if (prv_uart_soft && (prv_uart_soft->uart_id == id))
    {
    	result = luat_uart_soft_write((const uint8_t*)buf, len);
    }
    else
    {
    	result = luat_uart_write(id, (char*)buf, len);
    }
    lua_pushinteger(L, result);
#else
    int result = luat_uart_write(id, (char*)buf, len);
    lua_pushinteger(L, result);
#endif
    return 1;
}

/*Read serial port
@api uart.read(id, len)
@int serial port id, write 0 for uart0, write 1 for uart1
@int read length
@file/zbuff optional: file handle or zbuff object
@return string The read data / When passing in zbuff, return the read length and move the zbuff pointer backward
@usage
uart.read(1, 16)*/
static int l_uart_read(lua_State *L)
{
    uint8_t id = luaL_checkinteger(L, 1);
    uint32_t length = luaL_optinteger(L, 2, 1024);
    if(lua_isuserdata(L, 3)){//Special handling of zbuff objects
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE));
        uint8_t* recv = buff->addr+buff->cursor;
        if(length > buff->len - buff->cursor)
            length = buff->len - buff->cursor;
#ifdef LUAT_USE_SOFT_UART
		int result;
		if (prv_uart_soft && (prv_uart_soft->uart_id == id))
		{
			result = luat_uart_soft_read(recv, length);
		}
		else
		{
			result = luat_uart_read(id, recv, length);
		}
#else
        int result = luat_uart_read(id, recv, length);
#endif
        if(result < 0)
            result = 0;
        buff->cursor += result;
        lua_pushinteger(L, result);
        return 1;
    }
	// No longer limit the minimum number of reads
    // if (length < 512)
    //     length = 512;
	//If 0 bytes are read, an empty string is returned directly
	if (length < 1) {
		lua_pushliteral(L, "");
		return 1;
	}
	// Limit the maximum reading amount, otherwise it will easily crash
	if (length > LUAT_UART_RX_MAX_LEN) {
		LLOGE("uart read length too much %u, change to %u", length, LUAT_UART_RX_MAX_LEN);
		length = LUAT_UART_RX_MAX_LEN;
	}
	uint8_t tmpbuff[128];
	uint8_t* recv = tmpbuff;
	uint8_t* rr = NULL;
	if (length > 128) { // If the amount of reading is relatively large, malloc
		rr = luat_heap_malloc(length);
		recv = rr;
	}
    if (recv == NULL) {
        LLOGE("system is out of memory!!!");
        lua_pushstring(L, "");
        return 1;
    }

    uint32_t read_length = 0;
    while(read_length < length)//Loop after reading
    {
#ifdef LUAT_USE_SOFT_UART
		int result;
		if (prv_uart_soft && (prv_uart_soft->uart_id == id))
		{
			result = luat_uart_soft_read((void*)(recv + read_length), length - read_length);
		}
		else
		{
			result = luat_uart_read(id, (void*)(recv + read_length), length - read_length);
		}
#else
        int result = luat_uart_read(id, (void*)(recv + read_length), length - read_length);
#endif
        if (result > 0) {
            read_length += result;
        }
        else
        {
            break;
        }
    }
    if(read_length > 0)
    {
        if (lua_isinteger(L, 3)) {
            uint32_t fd = luaL_checkinteger(L, 3);
            luat_fs_fwrite(recv, 1, read_length, (FILE*)fd);
        }
        else {
            lua_pushlstring(L, (const char*)recv, read_length);
        }
    }
    else
    {
        lua_pushstring(L, "");
    }
	if (rr != NULL)
    	luat_heap_free(rr);
    return 1;
}

/*Close serial port
@api uart.close(id)
@int serial port id, write 0 for uart0, write 1 for uart1
@return nil no return value
@usage
uart.close(1)*/
static int l_uart_close(lua_State *L)
{
#ifdef LUAT_USE_SOFT_UART
	uint8_t id = luaL_checkinteger(L,1);
	if (prv_uart_soft && (prv_uart_soft->uart_id == id))
	{
		luat_uart_soft_close();
	}
	else
	{
		luat_uart_close(id);
	}
	return 0;
#else
//    uint8_t result = luat_uart_close(luaL_checkinteger(L, 1));
//    lua_pushinteger(L, result);
	luat_uart_close(luaL_checkinteger(L, 1));
    return 0;
#endif
}

/*Register serial port event callback
@api uart.on(id, event, func)
@int serial port id, write 0 for uart0, write 1 for uart1
@string event name
@function callback method
@return nil no return value
@usage
uart.on(1, "receive", function(id, len)
    local data = uart.read(id, len)
    log.info("uart", id, len, data)
end)*/
static int l_uart_on(lua_State *L) {
    int uart_id = luaL_checkinteger(L, 1);
    int org_uart_id = uart_id;
#ifdef LUAT_USE_SOFT_UART
	if (prv_uart_soft && (prv_uart_soft->uart_id == (uint8_t)uart_id))
	{
		;
	}
	else
	{
		if (!luat_uart_exist(uart_id)) {
			lua_pushliteral(L, "no such uart id");
			return 1;
		}
	}
#else
    if (!luat_uart_exist(uart_id)) {
        lua_pushliteral(L, "no such uart id");
        return 1;
    }
#endif
    if (uart_id >= LUAT_VUART_ID_0)
    {
    	uart_id = MAX_DEVICE_COUNT + uart_id - LUAT_VUART_ID_0;
    }

    const char* event = luaL_checkstring(L, 2);
    if (!strcmp("receive", event) || !strcmp("recv", event)) {
        if (uart_cbs[uart_id].received != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, uart_cbs[uart_id].received);
            uart_cbs[uart_id].received = 0;
        }
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            uart_cbs[uart_id].received = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    else if (!strcmp("sent", event)) {
        if (uart_cbs[uart_id].sent != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, uart_cbs[uart_id].sent);
            uart_cbs[uart_id].sent = 0;
        }
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            uart_cbs[uart_id].sent = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    luat_setup_cb(org_uart_id, uart_cbs[uart_id].received, uart_cbs[uart_id].sent);
    return 0;
}


/*Wait for TX to complete in 485 mode. MCU does not support serial port sending shift register empty or similar interrupt. It is used after the send event callback.
@api uart.wait485(id)
@int serial port id, write 0 for uart0, write 1 for uart1
@return int How many times the loop has been waited until tx is completed. It is used to roughly observe whether the delay time is enough. If the return is not 0, it means that the delay needs to be enlarged.*/
static int l_uart_wait485_tx_done(lua_State *L) {
    int uart_id = luaL_checkinteger(L, 1);
    if (!luat_uart_exist(uart_id)) {
    	lua_pushinteger(L, 0);
        return 1;
    }
#ifdef LUAT__UART_TX_NEED_WAIT_DONE
    lua_pushinteger(L, luat_uart_wait_485_tx_done(uart_id));
#else
    lua_pushinteger(L, 0);
#endif
    return 1;
}

/*Check if the serial port number exists
@api uart.exist(id)
@int serial port id, write 0 for uart0, write 1 for uart1, and so on.
@return bool exists and returns true*/
static int l_uart_exist(lua_State *L)
{
#ifdef LUAT_USE_SOFT_UART
	uint8_t id = luaL_checkinteger(L,1);
	if (prv_uart_soft && (prv_uart_soft->uart_id == id))
	{
		lua_pushboolean(L, 1);
	}
	else
	{
		lua_pushboolean(L, luat_uart_exist(id));
	}
	return 1;
#else
    lua_pushboolean(L, luat_uart_exist(luaL_checkinteger(L,1)));
    return 1;
#endif
}


/*Read the serial port in buff mode, read all the data at once and store it in the buff. If the buff space is not enough, it will automatically expand. Currently, air105 and air780e support this operation.
@api uart.rx(id, buff)
@int serial port id, write 0 for uart0, write 1 for uart1
@zbuff zbuff object
@return int Returns the read length and moves the zbuff pointer back
@usage
uart.rx(1, buff)*/
static int l_uart_rx(lua_State *L)
{
    uint8_t id = luaL_checkinteger(L, 1);

    if(lua_isuserdata(L, 2)){//Special handling of zbuff objects
    	luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
#ifdef LUAT_USE_SOFT_UART
		int result;
		if (prv_uart_soft && (prv_uart_soft->uart_id == id))
		{
			result = prv_uart_soft->rx_buffer.Pos;
		}
		else
		{
			result = luat_uart_read(id, NULL, 0);
		}
#else
    	int result = luat_uart_read(id, NULL, 0);
#endif
        if (result > (buff->len - buff->used))
        {
        	__zbuff_resize(buff, buff->len + result);
        }
#ifdef LUAT_USE_SOFT_UART
		if (prv_uart_soft && (prv_uart_soft->uart_id == id))
		{
			luat_uart_soft_read(buff->addr + buff->used, result);
		}
		else
		{
			luat_uart_read(id, buff->addr + buff->used, result);
		}
#else
        luat_uart_read(id, buff->addr + buff->used, result);
#endif
        lua_pushinteger(L, result);
        buff->used += result;
        return 1;
    }
    else
    {
        lua_pushinteger(L, 0);
        return 1;
    }
    return 1;
}

/*Read the remaining data amount in the serial port Rx cache. Currently air105 and air780e support this operation.
@api uart.rxSize(id)
@int serial port id, write 0 for uart0, write 1 for uart1
@return int Returns the read length
@usage
local size = uart.rxSize(1)*/
static int l_uart_rx_size(lua_State *L)
{
    uint8_t id = luaL_checkinteger(L, 1);
#ifdef LUAT_USE_SOFT_UART
	int result;
	if (prv_uart_soft && (prv_uart_soft->uart_id == id))
	{
		result = prv_uart_soft->rx_buffer.Pos;
	}
	else
	{
		result = luat_uart_read(id, NULL, 0);
	}
	lua_pushinteger(L, result);
#else
    lua_pushinteger(L, luat_uart_read(id, NULL, 0));
#endif
    return 1;
}

LUAT_WEAK void luat_uart_clear_rx_cache(int uart_id)
{

}
/*Clear the remaining data in the serial port Rx cache. Currently air105 and air780e support this operation.
@api uart.rxClear(id)
@int serial port id, write 0 for uart0, write 1 for uart1
@usage
uart.rxClear(1)*/
static int l_uart_rx_clear(lua_State *L)
{
    uint8_t id = luaL_checkinteger(L, 1);
#ifdef LUAT_USE_SOFT_UART

	if (prv_uart_soft && (prv_uart_soft->uart_id == id))
	{
		prv_uart_soft->rx_buffer.Pos = 0;
	}
	else
	{
		luat_uart_clear_rx_cache(id);
	}

#else
	luat_uart_clear_rx_cache(id);
#endif
    return 0;
}

/*Write the serial port in buff form, which is equivalent to c language uart_tx(uart_id, &buff[start], len);
@api uart.tx(id, buff, start, len)
@int serial port id, write 0 for uart0, write 1 for uart1
@zbuff The data to be written. If it is zbuff, it will be read from the starting position of the pointer.
@int Optional, the starting position of the data to be sent, the default is 0
@int Optional, the length of data to be sent, the default is valid data in zbuff, the maximum value does not exceed the maximum space of zbuff
@return int successful data length
@usage
uart.tx(1, buf)*/
static int l_uart_tx(lua_State *L)
{
    size_t start, len;
    // const char *buf;
    luat_zbuff_t *buff;
    uint8_t id = luaL_checkinteger(L, 1);
    if(lua_isuserdata(L, 2))
    {
        buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
    }
    else
    {
    	lua_pushinteger(L, 0);
    	return 1;
    }
    start = luaL_optinteger(L, 3, 0);
    len = luaL_optinteger(L, 4, buff->used);
    if (start >= buff->len)
    {
    	lua_pushinteger(L, 0);
    	return 1;
    }
    if ((start + len)>= buff->len)
    {
    	len = buff->len - start;
    }
#ifdef LUAT_USE_SOFT_UART
    int result;
    if (prv_uart_soft && (prv_uart_soft->uart_id == id))
    {
    	result = luat_uart_soft_write((const uint8_t*)(buff->addr + start), len);
    }
    else
    {
    	result = luat_uart_write(id, buff->addr + start, len);
    }
    lua_pushinteger(L, result);
#else
    int result = luat_uart_write(id, buff->addr + start, len);
    lua_pushinteger(L, result);
#endif
    return 1;
}


#ifdef LUAT_USE_SOFT_UART
static int l_uart_soft_handler_tx_done(lua_State *L, void* ptr)
{
	rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
	lua_pop(L, 1);
	if (prv_uart_soft->is_inited)
	{
		luat_uart_soft_tx_node_t *node = (luat_uart_soft_tx_node_t *)(prv_uart_soft->tx_queue_head.next);
		llist_del(&node->tx_node);
		luat_heap_alloc(NULL, node->data, 0, 0);
		luat_heap_alloc(NULL, node, 0, 0);
		if (llist_empty(&prv_uart_soft->tx_queue_head))
		{
			Buffer_StaticInit(&prv_uart_soft->tx_buffer, NULL, 0);
			prv_uart_soft->is_tx_busy = 0;
			if (!prv_uart_soft->is_rx_busy)
			{
				luat_uart_soft_sleep_enable(1);
			}
			if (prv_uart_soft->pin485 != 0xff)
			{
				luat_gpio_set(prv_uart_soft->pin485, prv_uart_soft->rs485_rx_level);
			}
	        if (uart_cbs[prv_uart_soft->uart_id].sent) {
	            lua_geti(L, LUA_REGISTRYINDEX, uart_cbs[prv_uart_soft->uart_id].sent);
	            if (lua_isfunction(L, -1)) {
	                lua_pushinteger(L, prv_uart_soft->uart_id);
	                lua_call(L, 1, 0);
	            }
	        }
		}
		else
		{
			node = (luat_uart_soft_tx_node_t *)prv_uart_soft->tx_queue_head.next;
			Buffer_StaticInit(&prv_uart_soft->tx_buffer, node->data, node->len);
			prv_uart_soft->tx_shift_bits = 0;
			luat_uart_soft_gpio_fast_output(prv_uart_soft->tx_pin, 0);
			luat_uart_soft_hwtimer_onoff(prv_uart_soft->tx_hwtimer_id, prv_uart_soft->tx_period);
			if (prv_uart_soft->parity)
			{
				prv_uart_soft->tx_bit = (0xffff << prv_uart_soft->data_bits) | luat_uart_soft_check_party(prv_uart_soft->tx_buffer.Data[0], prv_uart_soft->data_bits, prv_uart_soft->parity_odd) | prv_uart_soft->tx_buffer.Data[0];
			}
			else
			{
				prv_uart_soft->tx_bit = (0xffff << prv_uart_soft->data_bits) | prv_uart_soft->tx_buffer.Data[0];
			}
		}
	}

    lua_pushinteger(L, 0);
    return 1;
}

static int l_uart_soft_handler_rx_done(lua_State *L, void* ptr)
{
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_pop(L, 1);
	if (prv_uart_soft->is_inited)
	{
//		if (msg->ptr && msg->arg1)
//		{
//			if (((uint32_t)msg->arg1 + prv_uart_soft->rx_buffer.Pos) > prv_uart_soft->rx_buffer.MaxLen)
//			{
//				uint8_t *new = luat_heap_alloc(NULL, NULL, 0, (prv_uart_soft->rx_buffer.MaxLen + (uint32_t)msg->arg1) * 2);
//				if (new)
//				{
//					prv_uart_soft->rx_buffer.MaxLen = (prv_uart_soft->rx_buffer.MaxLen + (uint32_t)msg->arg1) * 2;
//					memcpy(new, prv_uart_soft->rx_buffer.Data, prv_uart_soft->rx_buffer.Pos);
//					luat_heap_alloc(NULL, prv_uart_soft->rx_buffer.Data, 0, 0);
//					prv_uart_soft->rx_buffer.Data = new;
//					memcpy(prv_uart_soft->rx_buffer.Data + prv_uart_soft->rx_buffer.Pos, msg->ptr, (uint32_t)msg->arg1);
//					prv_uart_soft->rx_buffer.Pos += (uint32_t)msg->arg1;
//				}
//				else
//				{
//					LLOGE("soft uart resize no mem!");
//				}
//			}
//			else
//			{
//				memcpy(prv_uart_soft->rx_buffer.Data + prv_uart_soft->rx_buffer.Pos, msg->ptr, (uint32_t)msg->arg1);
//				prv_uart_soft->rx_buffer.Pos += (uint32_t)msg->arg1;
//			}
//		}
//		LLOGD("%d,%d", prv_uart_soft->rx_buffer.Pos, msg->arg2);
		if (prv_uart_soft->rx_buffer.Pos || msg->arg2)
		{
			if (uart_app_recvs[prv_uart_soft->uart_id]) {
				uart_app_recvs[prv_uart_soft->uart_id](prv_uart_soft->uart_id, msg->arg2);
			}
			if (uart_cbs[prv_uart_soft->uart_id].received) {
				lua_geti(L, LUA_REGISTRYINDEX, uart_cbs[prv_uart_soft->uart_id].received);
				if (lua_isfunction(L, -1)) {
					lua_pushinteger(L, prv_uart_soft->uart_id);
					lua_pushinteger(L, prv_uart_soft->rx_buffer.Pos);
					lua_call(L, 2, 0);
				}
			}
		}
		if (msg->arg2)
		{
			prv_uart_soft->is_rx_busy = 0;
			if (!prv_uart_soft->is_tx_busy)
			{
				luat_uart_soft_sleep_enable(1);
			}
		}

	}
//	if (msg->ptr)
//	{
//		luat_heap_alloc(NULL, msg->ptr, 0, 0);
//	}
    lua_pushinteger(L, 0);
    return 1;
}

static void __LUAT_C_CODE_IN_RAM__ luat_uart_soft_send_hwtimer_irq(void)
{
	if (prv_uart_soft->tx_shift_bits >= prv_uart_soft->total_bits)
	{
		//Sent completed
		if (prv_uart_soft->tx_buffer.Pos >= prv_uart_soft->tx_buffer.MaxLen)
		{
			luat_uart_soft_hwtimer_onoff(prv_uart_soft->tx_hwtimer_id, 0);
			rtos_msg_t msg;
			msg.handler = l_uart_soft_handler_tx_done;
			msg.ptr = NULL;
			msg.arg1 = NULL;
			msg.arg2 = NULL;
			luat_msgbus_put(&msg, 0);
		}
		else
		{
			//Send the starting bit of the new byte
			luat_uart_soft_gpio_fast_output(prv_uart_soft->tx_pin, 0);
			luat_uart_soft_hwtimer_onoff(prv_uart_soft->tx_hwtimer_id, prv_uart_soft->tx_period);
			prv_uart_soft->tx_shift_bits = 0;
		}
		return;
	}

	luat_uart_soft_gpio_fast_output(prv_uart_soft->tx_pin, (prv_uart_soft->tx_bit >> prv_uart_soft->tx_shift_bits) & 0x01);
	prv_uart_soft->tx_shift_bits++;

	if (prv_uart_soft->tx_shift_bits > prv_uart_soft->data_bits)
	{
		luat_uart_soft_hwtimer_onoff(prv_uart_soft->tx_hwtimer_id, prv_uart_soft->stop_period);
		prv_uart_soft->tx_shift_bits = prv_uart_soft->total_bits;
		prv_uart_soft->tx_buffer.Pos++;
		if (prv_uart_soft->tx_buffer.Pos < prv_uart_soft->tx_buffer.MaxLen)
		{
			if (prv_uart_soft->parity)
			{
				prv_uart_soft->tx_bit = (0xffff << prv_uart_soft->data_bits) | luat_uart_soft_check_party(prv_uart_soft->tx_buffer.Data[prv_uart_soft->tx_buffer.Pos], prv_uart_soft->data_bits, prv_uart_soft->parity_odd) | prv_uart_soft->tx_buffer.Data[prv_uart_soft->tx_buffer.Pos];
			}
			else
			{
				prv_uart_soft->tx_bit = (0xffff << prv_uart_soft->data_bits) | prv_uart_soft->tx_buffer.Data[prv_uart_soft->tx_buffer.Pos];
			}
		}

	}
}

static void __LUAT_C_CODE_IN_RAM__ luat_uart_soft_recv_hwtimer_irq(void)
{
	uint8_t bit = luat_uart_soft_gpio_fast_input(prv_uart_soft->rx_pin);
	uint8_t is_end = 0;
	if (!prv_uart_soft->rx_shift_bits) //Detect the starting bit
	{
		luat_uart_soft_hwtimer_onoff(prv_uart_soft->rx_hwtimer_id, prv_uart_soft->rx_period);
		prv_uart_soft->rx_bit = bit;
		prv_uart_soft->rx_shift_bits++;
		prv_uart_soft->rx_parity_bit += bit;
		return ;
	}
	else if (0xef == prv_uart_soft->rx_shift_bits)	//RX detection timed out, there is no new start bit
	{
		is_end = 1;
		goto UART_SOFT_RX_DONE;
	}
	if (prv_uart_soft->rx_shift_bits < prv_uart_soft->data_bits)
	{
		prv_uart_soft->rx_bit |= (bit << prv_uart_soft->rx_shift_bits);
		prv_uart_soft->rx_shift_bits++;
		prv_uart_soft->rx_parity_bit += bit;
		if (prv_uart_soft->rx_shift_bits >= prv_uart_soft->data_bits)
		{
			if (!prv_uart_soft->parity)	//If no parity check is performed, start the next byte directly
			{
				goto UART_SOFT_RX_BYTE_DONE;
			}
		}
		return;
	}
	if ((prv_uart_soft->rx_parity_bit & 0x01) != bit) //parity check error
	{
		is_end = 1;
		goto UART_SOFT_RX_DONE;
	}
UART_SOFT_RX_BYTE_DONE:
	prv_uart_soft->rx_fifo[prv_uart_soft->rx_fifo_cnt] = prv_uart_soft->rx_bit;
	prv_uart_soft->rx_fifo_cnt++;
	luat_uart_soft_gpio_fast_irq_set(prv_uart_soft->rx_pin, 1);
	prv_uart_soft->rx_shift_bits = 0xef;
	luat_uart_soft_hwtimer_onoff(prv_uart_soft->rx_hwtimer_id, prv_uart_soft->stop_period * 20);	//Do reception timeout detection here
	if (prv_uart_soft->rx_fifo_cnt < LUAT_UART_SOFT_FIFO_CNT)	//The receiving fifo is not full, continue to receive
	{
		return;
	}
UART_SOFT_RX_DONE:

	if (prv_uart_soft->rx_fifo_cnt || is_end)
	{
        rtos_msg_t msg = {0};
        msg.handler = l_uart_soft_handler_rx_done;
//        msg.ptr = luat_heap_alloc(0, 0, 0, prv_uart_soft->rx_fifo_cnt);
        msg.arg1 = prv_uart_soft->rx_fifo_cnt;
        msg.arg2 = is_end;
//        if (msg.ptr)
//        {
//        	memcpy(msg.ptr, prv_uart_soft->rx_fifo, prv_uart_soft->rx_fifo_cnt);
//        }
        OS_BufferWriteLimit(&prv_uart_soft->rx_buffer, prv_uart_soft->rx_fifo, prv_uart_soft->rx_fifo_cnt);
        prv_uart_soft->rx_fifo_cnt = 0;
        luat_msgbus_put(&msg, 0);
	}
	if (is_end)
	{
		luat_uart_soft_gpio_fast_irq_set(prv_uart_soft->rx_pin, 1);
		luat_uart_soft_hwtimer_onoff(prv_uart_soft->rx_hwtimer_id, 0);
	}
	return;
}
#endif

/**
Set the hardware configuration of the software uart. Only SOCs that support hardware timers can be used. Currently, only one can be set. The baud rate has different limits according to the software and hardware configuration of the platform. It is recommended to be 9600. The receive buffer does not exceed 65535. MSB is not supported. Support 485 automatic control. Subsequent setup operations are still required.
@api uart.createSoft(tx_pin, tx_hwtimer_id, rx_pin, rx_hwtimer_id, adjust_period)
@int Send pin number
@int Hardware timer ID used for sending
@int Receive pin number
@int Hardware timer ID for receiving
@int Send timing adjustment, the unit is the timer clock cycle, the default is 0, it needs to be fine-tuned according to the oscilloscope or logic analyzer
@int Receive timing adjustment, the unit is the timer clock cycle, the default is 0, it needs to be fine-tuned according to the oscilloscope or logic analyzer
@return int The id of the software uart, or nil if it fails
@usage
-- Initialization software uart
local uart_id = uart.createSoft(21, 0, 1, 2) --air780e recommends using timers 0 and 2. It is best to use AGPIO for tx_pin to prevent the opposite end RX from being accidentally triggered during sleep.*/
static int l_uart_soft(lua_State *L) {
#ifdef LUAT_USE_SOFT_UART
	if (!prv_uart_soft)
	{
		prv_uart_soft = luat_heap_alloc(NULL, NULL, 0, sizeof(luat_uart_soft_t));
		if (prv_uart_soft)
		{
			memset(prv_uart_soft, 0, sizeof(luat_uart_soft_t));
			INIT_LLIST_HEAD(&prv_uart_soft->tx_queue_head);
			prv_uart_soft->uart_id = 0xff;
		}
		else
		{
			lua_pushnil(L);
			goto CREATE_DONE;
		}
	}
	if (prv_uart_soft->is_inited)
	{
		lua_pushnil(L);
		goto CREATE_DONE;
	}
	for(int uart_id = 1; uart_id < MAX_DEVICE_COUNT; uart_id++)
	{
		if (!luat_uart_exist(uart_id))
		{
			LLOGD("find free uart id, %d", uart_id);
			prv_uart_soft->is_inited = 1;
			prv_uart_soft->uart_id = uart_id;
			break;
		}
	}
	if (!prv_uart_soft->is_inited)
	{
		lua_pushnil(L);
		goto CREATE_DONE;
	}

	prv_uart_soft->tx_pin = luaL_optinteger(L, 1, 0xff);
	prv_uart_soft->tx_hwtimer_id = luaL_optinteger(L, 2, 0xff);
	prv_uart_soft->rx_pin = luaL_optinteger(L, 3, 0xff);
	prv_uart_soft->rx_hwtimer_id = luaL_optinteger(L, 4, 0xff);
	prv_uart_soft->tx_adjust_period = luaL_optinteger(L, 5, 0);
	prv_uart_soft->rx_adjust_period = luaL_optinteger(L, 6, 0);
	if (luat_uart_soft_setup_hwtimer_callback(prv_uart_soft->tx_hwtimer_id, luat_uart_soft_send_hwtimer_irq))
	{
		prv_uart_soft->is_inited = 0;
	}
	if (luat_uart_soft_setup_hwtimer_callback(prv_uart_soft->rx_hwtimer_id, luat_uart_soft_recv_hwtimer_irq))
	{
		luat_uart_soft_setup_hwtimer_callback(prv_uart_soft->tx_hwtimer_id, NULL);
		prv_uart_soft->is_inited = 0;
	}
	if (!prv_uart_soft->is_inited)
	{
		lua_pushnil(L);
		goto CREATE_DONE;
	}
	lua_pushinteger(L, prv_uart_soft->uart_id);
#else
	LLOGE("not support soft uart");
	lua_pushnil(L);
#endif
#ifdef LUAT_USE_SOFT_UART
CREATE_DONE:
#endif
    return 1;
}

/*Get the list of available serial numbers, currently only win32
@api uart.list(max)
@int optional, default 256, how many serial ports can be obtained at most
@return table The list of available serial numbers obtained*/
#ifdef LUAT_FORCE_WIN32
static int l_uart_list(lua_State *L)
{
    size_t len = luaL_optinteger(L,1,256);
    lua_newtable(L);//Return the table used
    uint8_t* buff = (uint8_t*)luat_heap_malloc(len);
    if (!buff)
        return 1;
    int rlen = luat_uart_list(buff, len);
    for(int i = 0;i<rlen;i++)
    {
        lua_pushinteger(L,i+1);
        lua_pushinteger(L,buff[i]);
        lua_settable(L,-3);
    }
    luat_heap_free(buff);
    return 1;
}
#endif

#include "rotable2.h"
static const rotable_Reg_t reg_uart[] =
{
    { "write",      ROREG_FUNC(l_uart_write)},
    { "read",       ROREG_FUNC(l_uart_read)},
    { "wait485",    ROREG_FUNC(l_uart_wait485_tx_done)},
    { "tx",      	ROREG_FUNC(l_uart_tx)},
    { "rx",       	ROREG_FUNC(l_uart_rx)},
	{ "rxClear",	ROREG_FUNC(l_uart_rx_clear)},
	{ "rxSize",		ROREG_FUNC(l_uart_rx_size)},
	{ "rx_size",	ROREG_FUNC(l_uart_rx_size)},
	{ "createSoft",	ROREG_FUNC(l_uart_soft)},
    { "close",      ROREG_FUNC(l_uart_close)},
    { "on",         ROREG_FUNC(l_uart_on)},
    { "setup",      ROREG_FUNC(l_uart_setup)},
    { "exist",      ROREG_FUNC(l_uart_exist)},
#ifdef LUAT_FORCE_WIN32
    { "list",       ROREG_FUNC(l_uart_list)},
#endif
	//@const Odd number odd parity, case compatibility
    { "Odd",        ROREG_INT(LUAT_PARITY_ODD)},
	//@const Even number even parity, case compatibility
    { "Even",       ROREG_INT(LUAT_PARITY_EVEN)},
	//@const None number No verification, case compatibility
    { "None",       ROREG_INT(LUAT_PARITY_NONE)},
    //@const ODD number odd parity
    { "ODD",        ROREG_INT(LUAT_PARITY_ODD)},
    //@const EVEN number even parity
    { "EVEN",       ROREG_INT(LUAT_PARITY_EVEN)},
    //@const NONE number no check
    { "NONE",       ROREG_INT(LUAT_PARITY_NONE)},
    //High and low bit order
    //@const LSB number little endian mode
    { "LSB",        ROREG_INT(LUAT_BIT_ORDER_LSB)},
    //@const MSB number big endian mode
    { "MSB",        ROREG_INT(LUAT_BIT_ORDER_MSB)},

    //@const VUART_0 number virtual serial port 0
	{ "VUART_0",       ROREG_INT(LUAT_VUART_ID_0)},
    //@const ERROR_DROP number discards cached data when an error is encountered
	{ "ERROR_DROP",       ROREG_INT(LUAT_UART_RX_ERROR_DROP_DATA)},
    //@const DEBUG number turns on debugging function
	{ "DEBUG",       ROREG_INT(LUAT_UART_DEBUG_ENABLE)},

    { NULL,         ROREG_INT(0) }
};

LUAMOD_API int luaopen_uart(lua_State *L)
{
    luat_newlib2(L, reg_uart);
    return 1;
}
