/*
 * Copyright (c) 2023 OpenLuat & AirM2M
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

#ifndef PRIVATE_INCLUDE_LUAT_LCD_SERVICE_H_
#define PRIVATE_INCLUDE_LUAT_LCD_SERVICE_H_

#include "luat_base.h"
#include "soc_lcd.h"

/**
 * @brief The current LCD refresh takes up ram space
 * @return ram space size*/
uint32_t luat_lcd_draw_cache(void);

/**
 * @brief lcd refresh request, here the data in the refresh area will be copied, thus occupying additional RAM
 * @param spi_id spi bus id
 * @param spi_mode spi timing mode, 0~3
 * @param cs_pin cs pin
 * @param dc_pin dc pin
 * @param spi_speed spi speed
 * @param data refreshed data
 * @param w_start horizontal starting position
 * @param w_end horizontal end position, the maximum can only be total width -1
 * @param h_start vertical starting position
 * @param h_end Vertical end position, the maximum can only be total height -1
 * @param w_offset horizontal offset, related to the actual screen
 * @param h_offset vertical offset, related to the actual screen
 * @param color_mode color mode, see COLOR_MODE_XXX, currently only supports COLOR_MODE_RGB_565 and COLOR_MODE_GRAY
 * @return Returns 0 if successful, otherwise failed. Failure is usually caused by insufficient RAM.*/
int luat_lcd_draw_require(uint8_t spi_id, uint8_t spi_mode, uint8_t cs_pin, uint8_t dc_pin,  uint32_t spi_speed, void *data, uint16_t w_start, uint16_t w_end, uint16_t h_start, uint16_t h_end, uint16_t w_offset, uint16_t h_offset, uint8_t color_mode);

/**
 * @brief lcd refresh camera data request, the data in the refresh area will not be copied here. If the camera speed is faster than the lcd refresh, you need to pay attention to the data synchronization problem, the camera has to wait for the lcd refresh
 * @param spi_id spi bus id
 * @param spi_mode spi timing mode, 0~3
 * @param cs_pin cs pin
 * @param dc_pin dc pin
 * @param spi_speed spi speed
 * @param data refreshed data
 * @param w_start horizontal starting position
 * @param w_end horizontal end position, the maximum can only be total width -1
 * @param h_start vertical starting position
 * @param h_end Vertical end position, the maximum can only be total height -1
 * @param w_offset horizontal offset, related to the actual screen
 * @param h_offset vertical offset, related to the actual screen
 * @param color_mode color mode, see COLOR_MODE_XXX, currently only supports COLOR_MODE_RGB_565 and COLOR_MODE_GRAY
 * @return Returns 0 if successful, otherwise failed. Failure is usually caused by insufficient RAM.*/
int luat_lcd_draw_camera_require(uint8_t spi_id, uint8_t spi_mode, uint8_t cs_pin, uint8_t dc_pin,  uint32_t spi_speed, void *data, uint16_t w_start, uint16_t w_end, uint16_t h_start, uint16_t h_end, uint16_t w_offset, uint16_t h_offset, uint8_t color_mode);

/**
 * @brief lcd refresh service initialization
 * @param priority refresh task priority 0~100*/
void luat_lcd_service_init(uint8_t priority);

/**
 * @brief lcd task runs user API, which can be used for synchronization of camera and LCD refresh.
 * @param CB user api
 * @param data api data parameter
 * @param len api’s len parameter
 * @param timeout request timeout, reserved, invalid*/
void luat_lcd_run_user_api(CBDataFun_t CB, uint32_t data, uint32_t len, uint32_t timeout);

#endif /* PRIVATE_INCLUDE_LUAT_LCD_SERVICE_H_ */
