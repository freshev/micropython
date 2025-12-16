/**
 * @file driver_i2s.h
 * @brief When using the API of this Modules, the original I2S API cannot be used. All APIs are task unsafe and interrupt unsafe! ! !
 * @version 0.1
 * @date 2023-1-9
 *
 * @copyright
 **/

#ifndef __CORE_I2S_H__
#define __CORE_I2S_H__
#include "bsp_common.h"
#include "i2s.h"
void I2S_FullConfig(uint8_t I2SID, i2sDataFmt_t DataFmt, i2sSlotCtrl_t SlotCtrl, i2sBclkFsCtrl_t BclkFsCtrl, i2sDmaCtrl_t DmaCtrl);
void I2S_BaseConfig(uint8_t I2SID, uint8_t Mode, uint8_t FrameSize);
int32_t I2S_Start(uint8_t I2SID, uint8_t IsPlay, uint32_t SampleRate, uint8_t ChannelNum);
void I2S_TransferLoop(uint8_t I2SID, uint8_t* Data, uint32_t OneTrunkByteLen, uint32_t TotalTrunkCnt, uint8_t NeedIrq);
void I2S_StopTransferLoop(uint8_t I2SID);
void I2S_Tx(uint8_t I2SID, uint8_t* Data, uint32_t ByteLen, CBFuncEx_t cb, void *param);
void I2S_Rx(uint8_t I2SID, uint32_t ByteLen, CBFuncEx_t cb, void *param);
void I2S_TxStop(uint8_t I2SID);
void I2S_RxStop(uint8_t I2SID);
void I2S_TxPause(uint8_t I2SID);
void I2S_TxDebug(uint8_t I2SID);
void I2S_RxDebug(uint8_t I2SID);
#endif
