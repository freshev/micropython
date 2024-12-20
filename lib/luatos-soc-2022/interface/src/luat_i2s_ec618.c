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

#include "luat_i2s_ec618.h"
#include "driver_i2s.h"
#include "driver_gpio.h"
extern void I2S_GetConfig(uint8_t I2SID, i2sDataFmt_t *DataFmt, i2sSlotCtrl_t *SlotCtrl, i2sBclkFsCtrl_t *BclkFsCtrl, i2sDmaCtrl_t *DmaCtrl);

void luat_i2s_init(void)
{

}

void luat_i2s_base_setup(uint8_t bus_id, uint8_t mode,  uint8_t frame_size)
{
	I2S_BaseConfig(bus_id, mode, frame_size);
	switch(bus_id)
	{
	case I2S_ID0:
		GPIO_IomuxEC618(39, 1, 1, 0);
		GPIO_IomuxEC618(35, 1, 1, 0);
		GPIO_IomuxEC618(36, 1, 1, 0);
		GPIO_IomuxEC618(37, 1, 1, 0);
		GPIO_IomuxEC618(38, 1, 1, 0);
		break;
	case I2S_ID1:
		GPIO_IomuxEC618(18, 1, 1, 0);
		GPIO_IomuxEC618(19, 1, 1, 0);
		GPIO_IomuxEC618(20, 1, 1, 0);
		GPIO_IomuxEC618(21, 1, 1, 0);
		GPIO_IomuxEC618(22, 1, 1, 0);
		break;
	}
}

void luat_i2s_set_lr_channel(uint8_t bus_id, uint8_t lr_channel)
{
	i2sDataFmt_t DataFmt;
	i2sSlotCtrl_t  SlotCtrl;
	i2sBclkFsCtrl_t BclkFsCtrl;
	i2sDmaCtrl_t DmaCtrl;
	I2S_GetConfig(bus_id, &DataFmt, &SlotCtrl, &BclkFsCtrl, &DmaCtrl);
	BclkFsCtrl.fsPolarity = lr_channel;
	I2S_FullConfig(bus_id, DataFmt, SlotCtrl,  BclkFsCtrl,  DmaCtrl);
}

int luat_i2s_start(uint8_t bus_id, uint8_t is_play, uint32_t sample, uint8_t channel_num)
{
	return I2S_Start(bus_id, is_play, sample, channel_num);
}

void luat_i2s_no_block_tx(uint8_t bus_id, uint8_t* address, uint32_t byte_len, void *cb, void *param)
{
	I2S_Tx(bus_id, address, byte_len, cb, param);
}
void luat_i2s_no_block_rx(uint8_t bus_id, uint32_t byte_len, void *cb, void *param)
{
	I2S_Rx(bus_id,byte_len, cb, param);
}


void luat_i2s_tx_stop(uint8_t bus_id)
{
	I2S_TxStop(bus_id);
}

void luat_i2s_rx_stop(uint8_t bus_id)
{
	I2S_RxStop(bus_id);
}

void luat_i2s_deinit(uint8_t bus_id)
{
	I2S_TxStop(bus_id);
}
void luat_i2s_pause(uint8_t bus_id)
{
	I2S_TxPause(bus_id);
}

int luat_i2s_tx_stat(uint8_t id, size_t *buffsize, size_t* remain) {
    (void)id;
    (void)buffsize;
    (void)remain;
    return -1;
}

int luat_i2s_txbuff_info(uint8_t id, size_t *buffsize, size_t* remain) {
    (void)id;
    (void)buffsize;
    (void)remain;
    return -1;
}

void luat_i2s_transfer_loop(uint8_t bus_id, uint8_t* address, uint32_t one_truck_byte_len, uint32_t total_trunk_cnt, uint8_t need_irq)
{
	I2S_TransferLoop(bus_id, address, one_truck_byte_len, total_trunk_cnt, need_irq);
}

void luat_i2s_transfer_loop_stop(uint8_t bus_id)
{
	I2S_StopTransferLoop(bus_id);
}
