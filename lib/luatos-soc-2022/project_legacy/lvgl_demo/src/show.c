#include "common_api.h"
#include "soc_lcd.h"

#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_mcu.h"
#include "luat_spi.h"
#include "luat_debug.h"
#include "luat_gpio.h"

#include "spi_lcd_common.h"

#include "lv_conf.h"
#include "lvgl.h"

#define SPI_LCD_FPS		(16)		//The highest can only be
#define SPI_LCD_BUS_ID	0
#define SPI_LCD_SPEED	25600000
#define SPI_LCD_CS_PIN	HAL_GPIO_24
#define SPI_LCD_DC_PIN	HAL_GPIO_8
#define SPI_LCD_POWER_PIN HAL_GPIO_25
#define SPI_LCD_BL_PIN HAL_GPIO_26
#define SPI_LCD_SDA_PIN HAL_GPIO_9
#define SPI_LCD_SCL_PIN HAL_GPIO_11
#define SPI_LCD_W	240
#define SPI_LCD_H	320
#define SPI_LCD_X_OFFSET	0
#define SPI_LCD_Y_OFFSET	0
#define SPI_LCD_RAM_CACHE_MAX	(SPI_LCD_W * SPI_LCD_H * 2)
#if LV_USE_DEMO_BENCHMARK
#define LVGL_FLUSH_TIME	(5)
#define LVGL_FLUSH_BUF_LINE	(160) //buf is opened to 20 lines in size, and can also be modified by yourself
#define LVGL_FLUSH_WAIT_TIME (3)
#else
#define LVGL_FLUSH_TIME	(50)
#define LVGL_FLUSH_BUF_LINE	(20) //buf is opened to 20 lines in size, and can also be modified by yourself
#define LVGL_FLUSH_WAIT_TIME (5)
#endif

enum
{
	LVGL_FLUSH_EVENT = 1,
};

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

static const LCD_DrawStruct g_s_draw =
{
		.Speed = SPI_LCD_SPEED,
		.SpiID = SPI_LCD_BUS_ID,
		.Mode = 0,
		.ColorMode = 0,
		.DCPin = SPI_LCD_DC_PIN,
		.CSPin = SPI_LCD_CS_PIN,
		.xoffset = SPI_LCD_X_OFFSET,
		.yoffset = SPI_LCD_Y_OFFSET,
};

typedef struct
{
	lv_disp_draw_buf_t draw_buf_dsc;
	lv_disp_drv_t disp_drv;
	luat_rtos_task_handle h_lvgl_task;
	luat_rtos_timer_t h_lvgl_timer;
	lv_color_t *draw_buf;
	uint8_t is_sleep;
	uint8_t wait_flush;
}lvgl_ctrl_t;

static lvgl_ctrl_t g_s_lvgl;

static LUAT_RT_RET_TYPE lvgl_flush_timer_cb(LUAT_RT_CB_PARAM)
{
	if (g_s_lvgl.wait_flush < 2)
	{
		g_s_lvgl.wait_flush++;
		luat_send_event_to_task(g_s_lvgl.h_lvgl_task, LVGL_FLUSH_EVENT, 0, 0, 0);
	}
}

static void lvgl_flush_cb(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
	LCD_DrawStruct *draw;
	uint32_t retry_cnt = 0;
	while (LCD_DrawCacheLen() >= SPI_LCD_RAM_CACHE_MAX)
	{
		retry_cnt++;
		luat_rtos_task_sleep(LVGL_FLUSH_WAIT_TIME);
	}
	draw = calloc(sizeof(LCD_DrawStruct), 1);
	if (!draw)
	{
		DBG("no mem!");
		goto FLUSH_END;
	}
	*draw = g_s_draw;
	draw->x1 = area->x1;
	draw->x2 = area->x2;
	draw->y1 = area->y1;
	draw->y2 = area->y2;
	draw->Size = (draw->x2 - draw->x1 + 1) * (draw->y2 - draw->y1 + 1) * sizeof(lv_color_t);
	draw->Data = malloc(draw->Size);
	if (!draw->Data)
	{
		free(draw);
		DBG("no mem!");
		goto FLUSH_END;
	}
    memcpy(draw->Data, color_p, draw->Size);
    LCD_Draw(draw);
FLUSH_END:
	lv_disp_flush_ready(disp_drv);
}

static void lvgl_lcd_init(void)
{
	int result = spi_lcd_detect(SPI_LCD_SDA_PIN, SPI_LCD_SCL_PIN, SPI_LCD_CS_PIN, SPI_LCD_DC_PIN, SPI_LCD_POWER_PIN);
	DBG("%d", result);
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
	spi_lcd_init_auto(&g_s_spi_lcd, result);

}

static void lvgl_draw_init(void)
{
	g_s_lvgl.draw_buf = malloc(g_s_spi_lcd.w_max * LVGL_FLUSH_BUF_LINE * sizeof(lv_color_t));
    lv_disp_draw_buf_init(&g_s_lvgl.draw_buf_dsc, g_s_lvgl.draw_buf, NULL, g_s_spi_lcd.w_max * LVGL_FLUSH_BUF_LINE);   /*Initialize the display buffer*/
    lv_disp_drv_init(&g_s_lvgl.disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    g_s_lvgl.disp_drv.hor_res = g_s_spi_lcd.w_max;
    g_s_lvgl.disp_drv.ver_res = g_s_spi_lcd.h_max;

    /*Used to copy the buffer's content to the display*/
    g_s_lvgl.disp_drv.flush_cb = lvgl_flush_cb;

    /*Set a display buffer*/
    g_s_lvgl.disp_drv.draw_buf = &g_s_lvgl.draw_buf_dsc;

    /*Finally register the driver*/
    lv_disp_drv_register(&g_s_lvgl.disp_drv);
}

//static void lvgl_task_test(void *param)
//{
//	size_t total, used, max_used;
//	uint16_t pos, color[3], i,j;
//	LCD_DrawStruct *draw;
//	PV_Union u_color;
//	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
//	lvgl_lcd_init();
//	color[0] = 0xf800;
//	color[1] = 0x07e0;
//	color[2] = 0x001f;
//	i = 0;
//	while(1)
//	{
//
//    	luat_meminfo_sys(&total, &used, &max_used);
//    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);
//		for(pos = 0; pos < g_s_spi_lcd.h_max; pos+=40)
//		{
//			draw = calloc(sizeof(LCD_DrawStruct), 1);
//			*draw = g_s_draw;
//			u_color.p = malloc(g_s_spi_lcd.w_max * 40 * 2);
//			draw->Data = u_color.pu8;
//			for(j = 0; j < g_s_spi_lcd.w_max * 40; j++)
//			{
//				u_color.pu16[j] = color[i];
//			}
//			draw->x1 = 0;
//			draw->x2 = g_s_spi_lcd.w_max - 1;
//			draw->y1 = pos;
//			draw->y2 = pos + 39;
//			draw->Size = g_s_spi_lcd.w_max * 40 * 2;
//			LCD_Draw(draw);
//		}
//		luat_rtos_task_sleep(55);
//		i++;
//		i = i%3;
//	}
//}

static void lvgl_task(void *param)
{
	luat_event_t event;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
	lvgl_lcd_init();
	lv_init();
	lvgl_draw_init();
	luat_start_rtos_timer(g_s_lvgl.h_lvgl_timer, LVGL_FLUSH_TIME, 1);
#if LV_USE_DEMO_BENCHMARK
	luat_wdt_close();
	lv_demo_benchmark();
#endif
	while(1)
	{
		luat_wait_event_from_task(g_s_lvgl.h_lvgl_task, 0, &event, NULL, LUAT_WAIT_FOREVER);
		if (g_s_lvgl.wait_flush) g_s_lvgl.wait_flush--;
		lv_timer_handler();
	}
}


void lvgl_init(void)
{

	luat_gpio_cfg_t gpio_cfg;

	LCD_ServiceInit(60);
	luat_rtos_task_create(&g_s_lvgl.h_lvgl_task, 8196, 50, "lvgl", lvgl_task, NULL, 16);
	g_s_lvgl.h_lvgl_timer = luat_create_rtos_timer(lvgl_flush_timer_cb, NULL, NULL);

	luat_gpio_set_default_cfg(&gpio_cfg);
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


}

INIT_TASK_EXPORT(lvgl_init, "1");
