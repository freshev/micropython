/**
 * @file driver_gpio.h
 * @brief When using the API of this Modules, you cannot use the original I2C API. All APIs are task unsafe and interrupt unsafe! ! !
 * @version 0.1
 * @date 2022-10-25
 *
 * @copyright
 **/

#ifndef __CORE_I2C_H__
#define __CORE_I2C_H__
#include "bsp_common.h"
/**
 * @brief i2c host configuration
 *
 * @param I2CID I2C channel number
 * @param Speed   speed, only 100000 and 400000*/
void I2C_MasterSetup(uint8_t I2CID, uint32_t Speed);
/** @brief i2c pre-transmission configuration, if the configuration is consistent with the last time, there is no need to set it
 *
 * @param I2CID I2C channel number
 * @param ChipAddress I2C device address
 * @param ChipAddressLen I2C device address length, 1 or 2
 * @param CB callback function after completion
 * @param pParam pParam in the callback function after completion*/
void I2C_Prepare(uint8_t I2CID, uint16_t ChipAddress, uint8_t ChipAddressLen, CBFuncEx_t CB, void *pParam);
/**
 * @brief i2c host transmission, compatible with direct reading and writing and writing the register address first and then reading the data
 *
 * @param I2CID I2C channel number
 * @param Operate operation type
 * I2C_OP_READ_REG = 0, //i2c general read register, one write and one read, automatically with start signal
I2C_OP_READ, //i2c general read, read only
I2C_OP_WRITE, //i2c universal write, write only
 * @param RegAddress The data cache of the register address, ignored during general reading and general writing
 * @param RegLen register address length, ignored during general reading and general writing
 * @param Data Read and write data cache, directly using the user's space, the space cannot be released before completion
 * @param Len read and write data length
 * @param Toms transmission single byte timeout, unit ms*/
void I2C_MasterXfer(uint8_t I2CID, uint8_t Operate, uint8_t *RegAddress, uint32_t RegLen, uint8_t *Data, uint32_t Len, uint16_t Toms);

/**
 * @brief i2c host transmission result query
 *
 * @param I2CID I2C channel number
 * @param Result The transmission result =0 is successful, otherwise it fails. Only return != 0 is valid.
 * @return =0 The transfer has not been completed, others have been completed*/
int I2C_WaitResult(uint8_t I2CID, int32_t *Result);

int32_t I2C_BlockWrite(uint8_t I2CID, uint16_t ChipAddress, uint8_t *Data, uint32_t Len, uint16_t Toms, CBFuncEx_t CB, void *pParam);

int32_t I2C_BlockRead(uint8_t I2CID, uint16_t ChipAddress, uint8_t *Reg, uint32_t RegLen, uint8_t *Data, uint32_t Len, uint16_t Toms, CBFuncEx_t CB, void *pParam);

void I2C_ChangeBR(uint8_t I2CID, uint32_t Baudrate);

void I2C_Reset(uint8_t I2CID);

void I2C_UsePollingMode(uint8_t I2CID, uint8_t OnOff);
#endif
