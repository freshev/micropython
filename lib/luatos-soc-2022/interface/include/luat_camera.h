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
#ifndef INCLUDE_LUAT_CAMERA_H_
#define INCLUDE_LUAT_CAMERA_H_
#include "luat_base.h"
#include "common_api.h"

typedef struct
{
	size_t  camera_speed;			//Provide camera clock frequency
    uint16_t sensor_width;			//Maximum width of camera
    uint16_t sensor_height;			//The maximum height of the camera
    uint16_t one_buf_height;		//The height of 1 receive buffer, receive cache size = sensor_width * one_buf_height * (1 or 2, only_y=1), the bottom layer will make adjustments according to the actual situation, thereby modifying this value
    uint8_t only_y;
	uint8_t rowScaleRatio;
	uint8_t colScaleRatio;
	uint8_t scaleBytes;
	uint8_t spi_mode;
	uint8_t is_msb;	//0 or 1;
	uint8_t is_two_line_rx; //0 or 1;
	uint8_t seq_type;	//0 or 1
} luat_spi_camera_t;

/**
 * @brief configure camera and initialize camera
 * @param id camera receives data bus ID, there are 2 on ec618, 0 and 1
 * @param conf camera related configuration
 * @param callback camera receives interrupt callback, note that this is the callback in the interrupt
 * @param param User parameters when interrupting callback
 * @return 0 success, others failure*/
int luat_camera_setup(int id, luat_spi_camera_t *conf, void *callback, void *param);

/**
 * @brief Start receiving camera data
 * @param id camera receives data bus ID
 * @return 0 success, others failure*/
int luat_camera_start(int id);

/**
 * @brief Stop receiving camera data
 * @param id camera receives data bus ID
 * @return 0 success, others failure*/
int luat_camera_stop(int id);

/**
 * @brief Close camera and release resources
 * @param id camera receives data bus ID
 * @return 0 success, others failure*/
int luat_camera_close(int id);

#endif /* INCLUDE_LUAT_CAMERA_H_ */
