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

/** The demo requires air780E_core_v1.6 + BF30A2, using I2S1 and I2C0 buses. If low power consumption is required, the camera needs to be completely turned off before sleeping, and I2C0 reuse configuration 1.
 *LCD preview requires 2.4-inch TFT LCD resolution: 240X320 screen ic: GC9306. If low power consumption is required, RST, backlight control and CS pin must be controlled by AONIO
 * Since the external camera power supply LDO on the development board does not necessarily exist, Air780EC is required and the internal LDO output is used.
 * Since more ram is required, the low-speed network compilation mode must be turned on
 * Enable relevant functions according to actual needs
 *
 * 780E has limited resources and is not suitable for camera development. Please use 780EP for development! ! ! ! ! !
 **/

#include "common_api.h"
#include "luat_camera.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_lcd_service.h"
#include "luat_uart.h"
#include "luat_i2c.h"

//#define CAMERA_TEST_QRCODE //Scan code + preview
//#define CAMERA_TEST_CAPTURE //Camera + preview
#define CAMERA_TEST_VIDEO		//The camera is output from the USB serial port to the computer

#ifdef CAMERA_TEST_QRCODE
#undef CAMERA_TEST_CAPTURE
#undef CAMERA_TEST_VIDEO
#define LCD_ENABLE
#endif

#ifdef CAMERA_TEST_CAPTURE
#undef CAMERA_TEST_QRCODE
#undef CAMERA_TEST_VIDEO
#define LCD_ENABLE
#endif
#ifdef CAMERA_TEST_VIDEO
#undef CAMERA_TEST_QRCODE
#undef CAMERA_TEST_CAPTURE
#define USB_UART_ENABLE
#endif

#define CAMERA_I2C_ID	0
#define CAMERA_SPI_ID	1	//It is I2S1
#define CAMERA_PD_PIN	HAL_GPIO_5
//#define CAMERA_POWER_PIN HAL_GPIO_25   //External LDO control
//#define CAMERA_POWER_PIN_ALT 0
#define CAMERA_POWER_PIN HAL_GPIO_13	//Internal LDO control
#define CAMERA_POWER_PIN_ALT 4
#define CAMERA_SPEED	25500000
#define CAMERA_W 240
#define CAMERA_H 320
#define BF30A2_I2C_ADDRESS	(0x6e)

enum
{
	CAMERA_FRAME_START = USER_EVENT_ID_START,
	CAMERA_FRAME_DATA,
	CAMERA_FRAME_END
};

typedef struct
{
	Buffer_Struct pic_data_buffer;
	uint16_t one_buf_height;
}luat_camera_app_t;

static luat_camera_app_t g_s_camera_app;
luat_rtos_task_handle g_s_task_handle;

typedef struct
{
	uint8_t reg;
	uint8_t data;
}camera_reg_t;

static camera_reg_t g_s_bf302a_reg_table[] =
{
	{0xf2,0x01},//soft reset
	{0xcf,0xb0},//POWER UP
	{0x12,0x20},//MTK:20 ZX:10 RDA:40
	{0x15,0x80},
	{0x6b,0x71},
	{0x00,0x40},
	{0x04,0x00},
	{0x06,0x26},
	{0x08,0x07},
	{0x1c,0x12},
	{0x20,0x20},
	{0x21,0x20},
	{0x34,0x02},
	{0x35,0x02},
	{0x36,0x21},
	{0x37,0x13},
	{0x03,0x23},
	{0xcb,0x22},
	{0xcc,0x89},
	{0xcd,0x4c},
	{0xce,0x6b},
	{0xa0,0x8e},
	{0x01,0x1b},
	{0x02,0x1d},
	{0x13,0x08},
	{0x87,0x13},
	{0x8b,0x08},
	{0x70,0x17},
	{0x71,0x43},
	{0x72,0x0a},
	{0x73,0x62},
	{0x74,0xa2},
	{0x75,0xbf},
	{0x76,0x00},
	{0x77,0xcc},
	{0x40,0x32},
	{0x41,0x28},
	{0x42,0x26},
	{0x43,0x1d},
	{0x44,0x1a},
	{0x45,0x14},
	{0x46,0x11},
	{0x47,0x0f},
	{0x48,0x0e},
	{0x49,0x0d},
	{0x4B,0x0c},
	{0x4C,0x0b},
	{0x4E,0x0a},
	{0x4F,0x09},
	{0x50,0x09},
	{0x24,0x30},
	{0x25,0x36},
	{0x80,0x00},
	{0x81,0x20},
	{0x82,0x40},
	{0x83,0x30},
	{0x84,0x50},
	{0x85,0x30},
	{0x86,0xd8},
	{0x89,0x45},
	{0x8a,0x33},
	{0x8f,0x81},
	{0x91,0xff},
	{0x92,0x08},
	{0x94,0x82},
	{0x95,0xfd},
	{0x9a,0x20},
	{0x9e,0xbc},
	{0xf0,0x87},
	{0x51,0x06},
	{0x52,0x25},
	{0x53,0x2b},
	{0x54,0x0f},
	{0x57,0x2a},
	{0x58,0x22},
	{0x59,0x2c},
	{0x23,0x33},
	{0xa1,0x93},
	{0xa2,0x0f},
	{0xa3,0x2a},
	{0xa4,0x08},
	{0xa5,0x26},
	{0xa7,0x80},
	{0xa8,0x80},
	{0xa9,0x1e},
	{0xaa,0x19},
	{0xab,0x18},
	{0xae,0x50},
	{0xaf,0x04},
	{0xc8,0x10},
	{0xc9,0x15},
	{0xd3,0x0c},
	{0xd4,0x16},
	{0xee,0x06},
	{0xef,0x04},
	{0x55,0x34},
	{0x56,0x9c},
	{0xb1,0x98},
	{0xb2,0x98},
	{0xb3,0xc4},
	{0xb4,0x0c},
	{0xa0,0x8f},
	{0x13,0x07},
};

#ifdef LCD_ENABLE

#define SPI_LCD_BUS_ID	0
#define SPI_LCD_SPEED	25600000
//#define SPI_LCD_CS_PIN	HAL_GPIO_24
//#define SPI_LCD_DC_PIN	HAL_GPIO_8
//#define SPI_LCD_POWER_PIN HAL_GPIO_27
//#define SPI_LCD_BL_PIN HAL_GPIO_22
#define SPI_LCD_CS_PIN	HAL_GPIO_8
#define SPI_LCD_DC_PIN	HAL_GPIO_10
#define SPI_LCD_POWER_PIN HAL_GPIO_24
#define SPI_LCD_BL_PIN HAL_GPIO_NONE	//Connect directly to 3.3V
#define SPI_LCD_SDA_PIN HAL_GPIO_9
#define SPI_LCD_SCL_PIN HAL_GPIO_11
#define SPI_LCD_W	240
#define SPI_LCD_H	320
#define SPI_LCD_X_OFFSET	0
#define SPI_LCD_Y_OFFSET	0
#define SPI_LCD_RAM_CACHE_MAX	(SPI_LCD_W * SPI_LCD_H * 2)


static spi_lcd_ctrl_t g_s_spi_lcd =
{
		SPI_LCD_W,
		SPI_LCD_H,
		SPI_LCD_BUS_ID,
		SPI_LCD_CS_PIN,
		SPI_LCD_DC_PIN,
		SPI_LCD_POWER_PIN,
		SPI_LCD_BL_PIN,
		0,
		SPI_LCD_X_OFFSET,
		SPI_LCD_Y_OFFSET
};

#endif


#ifdef LCD_ENABLE
static int luat_spi_lcd_init(uint8_t *data, uint32_t len)
{

	luat_gpio_cfg_t gpio_cfg;
	gpio_cfg.pin = SPI_LCD_CS_PIN;
	gpio_cfg.output_level = LUAT_GPIO_HIGH;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = SPI_LCD_DC_PIN;
	gpio_cfg.output_level = LUAT_GPIO_LOW;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = SPI_LCD_POWER_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = SPI_LCD_BL_PIN;
	luat_gpio_open(&gpio_cfg);


	int result = spi_lcd_detect(SPI_LCD_SDA_PIN, SPI_LCD_SCL_PIN, SPI_LCD_CS_PIN, SPI_LCD_DC_PIN, SPI_LCD_POWER_PIN);
	if (result < 0)
	{
		LUAT_DEBUG_PRINT("no find gc9306x!");
	}
	luat_spi_t spi_cfg = {
			.id = SPI_LCD_BUS_ID,
			.bandrate = SPI_LCD_SPEED,
			.CPHA = 0,
			.CPOL = 0,
			.mode = 1,
			.dataw = 8,
			.cs = SPI_LCD_CS_PIN,
	};
	luat_spi_setup(&spi_cfg);

	gpio_cfg.pin = SPI_LCD_CS_PIN;
	gpio_cfg.output_level = LUAT_GPIO_HIGH;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = SPI_LCD_DC_PIN;
	luat_gpio_open(&gpio_cfg);

	spi_lcd_init_gc9306x(&g_s_spi_lcd);
}
#endif

#ifdef USB_UART_ENABLE
static void luat_usb_recv_cb(int uart_id, uint32_t data_len)
{
    char* data_buff = malloc(data_len);
    luat_uart_read(uart_id, data_buff, data_len);
    LUAT_DEBUG_PRINT("luat_uart_cb uart_id:%d data:%.*s data_len:%d",uart_id, data_len, data_buff,data_len);
    free(data_buff);
}
#endif


static int luat_camera_irq_callback(void *pdata, void *param)
{
	if (!pdata)
	{
		luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_START, 0, 0, 0, 0);
		g_s_camera_app.pic_data_buffer.Pos = 0;
		return 0;
	}

	if (INVALID_HANDLE_VALUE == pdata)
	{
		luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_END, 0, 0, 0, 0);
		return 0;
	}
	Buffer_Struct *buf = (Buffer_Struct *)pdata;
	OS_BufferWriteLimit(&g_s_camera_app.pic_data_buffer, buf->Data, buf->MaxLen);
	luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_DATA, 0, 0, 0, 0);
	return 0;
}

static int luat_bf30a2_init(void)
{
	uint8_t id[2];
	luat_spi_camera_t spi_camera =
	{
			.camera_speed = CAMERA_SPEED,
			.sensor_width = CAMERA_W,
			.sensor_height = CAMERA_H,
#ifdef CAMERA_TEST_QRCODE
			.only_y = 1,
#else
			.only_y = 0,
#endif
			.rowScaleRatio = 0,
			.colScaleRatio  = 0,
			.scaleBytes = 0,
			.spi_mode = SPI_MODE_0,
			.is_msb = 0,
			.is_two_line_rx = 0,
			.seq_type = 0,
	};
	luat_gpio_set(CAMERA_PD_PIN, LUAT_GPIO_LOW);
	luat_camera_setup(CAMERA_SPI_ID, &spi_camera, luat_camera_irq_callback, NULL);	//Output MCLK to supply camera clock
	luat_rtos_task_sleep(1);
	g_s_camera_app.one_buf_height = spi_camera.one_buf_height;
	LUAT_DEBUG_PRINT("%u", g_s_camera_app.one_buf_height);
	luat_i2c_set_iomux(CAMERA_I2C_ID, 1);
    luat_i2c_setup(CAMERA_I2C_ID, 1);
	luat_gpio_set(CAMERA_POWER_PIN, LUAT_GPIO_HIGH);
	luat_rtos_task_sleep(1);
	luat_i2c_set_polling_mode(CAMERA_I2C_ID, 1);
	id[0] = 0xfc;
	if (luat_i2c_transfer(CAMERA_I2C_ID, BF30A2_I2C_ADDRESS, id, 1, id, 2))
	{
		LUAT_DEBUG_PRINT("BF30A2 not i2c response");
		goto CAM_OPEN_FAIL;
	}
	if (id[0] != 0x3b || id[1] != 0x02)
	{
		LUAT_DEBUG_PRINT("not BF30A2 %x,%x", id[0], id[1]);
		goto CAM_OPEN_FAIL;
	}
	for(int i = 0; i < sizeof(g_s_bf302a_reg_table)/sizeof(camera_reg_t); i++)
	{
		if (luat_i2c_send(CAMERA_I2C_ID, BF30A2_I2C_ADDRESS, &g_s_bf302a_reg_table[i].reg, 2, 1))
		{
			LUAT_DEBUG_PRINT("write %d %x,%x failed", i, g_s_bf302a_reg_table[i].reg, g_s_bf302a_reg_table[i].data);
			goto CAM_OPEN_FAIL;
		}
	}
	luat_i2c_set_polling_mode(CAMERA_I2C_ID, 0);
	luat_camera_start(CAMERA_SPI_ID);
	return 0;
CAM_OPEN_FAIL:
	luat_camera_close(CAMERA_SPI_ID);
	luat_i2c_close(CAMERA_I2C_ID);
	luat_gpio_set(CAMERA_POWER_PIN, LUAT_GPIO_LOW);
	return -1;
}

static void luat_camera_task(void *param)
{
	luat_event_t event;
	uint32_t all,now_free_block,min_free_block;
	uint32_t pos, block_len;
	uint16_t v_len;
	uint8_t *cache;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
	luat_rtos_task_sleep(1000);
#ifdef LCD_ENABLE
	luat_lcd_run_user_api(luat_spi_lcd_init, NULL, 0, 0);
#endif
#ifdef USB_UART_ENABLE
    luat_uart_t uart = {
        .id = LUAT_VUART_ID_0,
    };
    luat_uart_setup(&uart);
    luat_uart_ctrl(LUAT_VUART_ID_0, LUAT_UART_SET_RECV_CALLBACK, luat_usb_recv_cb);
#endif

    OS_InitBuffer(&g_s_camera_app.pic_data_buffer, CAMERA_W *CAMERA_H*2);


    // BF30A2 initialization process, if there is a low-power sleep requirement, you need to go through it again after waking up
    if (luat_bf30a2_init())
    {
    	while(1)
    	{
    		LUAT_DEBUG_PRINT("no find camera bf20a2, test stop");
    		luat_rtos_task_sleep(5000);
    	}
    }
#ifdef CAMERA_TEST_QRCODE
    cache = malloc(16 + g_s_camera_app.one_buf_height * CAMERA_H);
    block_len = g_s_camera_app.one_buf_height * CAMERA_H;
#else
    cache = malloc(16 + g_s_camera_app.one_buf_height * CAMERA_W * 2);
    block_len = g_s_camera_app.one_buf_height * CAMERA_W * 2;
#endif

#ifdef USB_UART_ENABLE
    cache[0] = 'V';
    cache[1] = 'C';
    cache[2] = 'A';
    cache[3] = 'M';
    BytesPutLe16(cache + 4, CAMERA_W);
    BytesPutLe16(cache + 6, CAMERA_H);
    BytesPutLe16(cache + 12, block_len);
    BytesPutLe16(cache + 14, block_len);
    DBG("%d", block_len);
#endif
	luat_meminfo_sys(&all, &now_free_block, &min_free_block);
	LUAT_DEBUG_PRINT("meminfo %d,%d,%d",all,now_free_block,min_free_block);
	while(1)
	{
		luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch(event.id)
		{
		case CAMERA_FRAME_START:
			pos = 0;
			v_len = 0;
			break;
		case CAMERA_FRAME_DATA:
#ifdef USB_UART_ENABLE
			BytesPutLe32(cache + 8, v_len);
			memcpy(cache + 16, g_s_camera_app.pic_data_buffer.Data + pos, block_len);
			luat_uart_write(LUAT_VUART_ID_0, cache, 16 + block_len);
			pos += block_len;
			v_len += g_s_camera_app.one_buf_height;
//			DBG("%d,%d", pos, v_len);
#endif
			break;
		case CAMERA_FRAME_END:
			break;
		}
	}
}

static void camera_demo_init(void)
{
#ifdef LCD_ENABLE
	luat_lcd_service_init(90);
#endif
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = CAMERA_PD_PIN;
	gpio_cfg.output_level = LUAT_GPIO_LOW;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = CAMERA_POWER_PIN;
	gpio_cfg.output_level = LUAT_GPIO_LOW;
	gpio_cfg.alt_fun = CAMERA_POWER_PIN_ALT;
	luat_gpio_open(&gpio_cfg);
	luat_rtos_task_create(&g_s_task_handle, 4 * 1024, 50, "camera", luat_camera_task, NULL, 64);
}

INIT_TASK_EXPORT(camera_demo_init, "1");

