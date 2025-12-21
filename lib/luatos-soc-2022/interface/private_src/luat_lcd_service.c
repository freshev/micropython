#include "luat_lcd_service.h"


uint32_t luat_lcd_draw_cache(void) {return LCD_DrawCacheLen();}

int luat_lcd_draw_require(uint8_t spi_id, uint8_t spi_mode, uint8_t cs_pin, uint8_t dc_pin,  uint32_t spi_speed, void *data, uint16_t w_start, uint16_t w_end, uint16_t h_start, uint16_t h_end, uint16_t w_offset, uint16_t h_offset, uint8_t color_mode)
{
	LCD_DrawStruct *draw;
	draw = calloc(sizeof(LCD_DrawStruct), 1);
	if (!draw)
	{
		DBG("no enough memory!");
		return -1;
	}

	draw->Size = (w_end - w_start + 1) * (h_end - h_start + 1) * ((COLOR_MODE_GRAY == color_mode)?1:2);
	draw->Data = malloc(draw->Size);
	if (!draw->Data)
	{
		free(draw);
		DBG("no enough memory!");
		return -1;
	}
	memcpy(draw->Data, data, draw->Size);
	draw->x1 = w_start;
	draw->x2 = w_end;
	draw->y1 = h_start;
	draw->y2 = h_end;
	draw->xoffset = w_offset;
	draw->yoffset = h_offset;
	draw->SpiID = spi_id;
	draw->ColorMode = color_mode;
	draw->Speed = spi_speed;
	draw->CSPin = cs_pin;
	draw->DCPin = dc_pin;
	draw->Mode = spi_mode;
	LCD_Draw(draw);
}


int luat_lcd_draw_camera_require(uint8_t spi_id, uint8_t spi_mode, uint8_t cs_pin, uint8_t dc_pin,  uint32_t spi_speed, void *data, uint16_t w_start, uint16_t w_end, uint16_t h_start, uint16_t h_end, uint16_t w_offset, uint16_t h_offset, uint8_t color_mode)
{
	LCD_DrawStruct *draw;
	draw = calloc(sizeof(LCD_DrawStruct), 1);
	if (!draw)
	{
		DBG("no enough memory!");
		return -1;
	}

	draw->Size = (w_end - w_start + 1) * (h_end - h_start + 1) * ((COLOR_MODE_GRAY == color_mode)?1:2);
	draw->Data = data;
	draw->x1 = w_start;
	draw->x2 = w_end;
	draw->y1 = h_start;
	draw->y2 = h_end;
	draw->xoffset = w_offset;
	draw->yoffset = h_offset;
	draw->SpiID = spi_id;
	draw->ColorMode = color_mode;
	draw->Speed = spi_speed;
	draw->CSPin = cs_pin;
	draw->DCPin = dc_pin;
	draw->Mode = spi_mode;
	LCD_CameraDraw(draw);
}


void luat_lcd_service_init(uint8_t priority) {LCD_ServiceInit(priority);}

void luat_lcd_run_user_api(CBDataFun_t CB, uint32_t data, uint32_t len, uint32_t timeout) {LCD_RunUserAPI(CB, data, len, timeout);}
