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

#ifndef __LUAT_I2S_EC618_H__
#define __LUAT_I2S_EC618_H__
#include "common_api.h"
void luat_i2s_init(void);
int luat_i2s_start(uint8_t bus_id, uint8_t is_play, uint32_t sample, uint8_t channel_num);
void luat_i2s_base_setup(uint8_t bus_id, uint8_t mode,  uint8_t frame_size);
void luat_i2s_no_block_tx(uint8_t bus_id, uint8_t* address, uint32_t byte_len, void *cb, void *param);
void luat_i2s_no_block_rx(uint8_t bus_id, uint32_t byte_len, void *cb, void *param);
void luat_i2s_tx_stop(uint8_t bus_id);
void luat_i2s_rx_stop(uint8_t bus_id);
void luat_i2s_deinit(uint8_t bus_id);
void luat_i2s_pause(uint8_t bus_id);
/**
 * @brief i2s tx loop output, N data blocks are output in loop
 * @param bus_id i2s bus ID
 * @param address The data output by the loop. If it is NULL, it is mute. In the case of mute, the total length cannot exceed 8000.
 * @param one_truck_byte_len 1 data block length byte, cannot exceed 8000
 * @param total_trunk_cnt The number of data blocks, the total length is one_truck_byte_len * total_trunk_cnt
 * @param need_irq Whether to interrupt, currently not supported, no interrupt callback*/
void luat_i2s_transfer_loop(uint8_t bus_id, uint8_t* address, uint32_t one_truck_byte_len, uint32_t total_trunk_cnt, uint8_t need_irq);
/**
 * @brief Stop i2s tx loop output
 * @param bus_id i2s bus ID*/
void luat_i2s_transfer_loop_stop(uint8_t bus_id);

#endif
