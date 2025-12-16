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

#ifndef LUAT_I2C_H
#define LUAT_I2C_H

#include "luat_base.h"
#include "luat_i2c_legacy.h"

/**
 * @ingroup luatos_device peripheral interface
 * @{*/
/**
 * @defgroup luatos_device_i2c I2C interface
 * @{*/

/**
 * @brief Check if i2c exists
 *
 * @param id i2c_id
 * @return 1 exists 0 does not exist*/
int luat_i2c_exist(int id);


/**
 * @brief Set up i2c multiplexing
 * @attention If the id is 1 and the value is 1, it is multiplexed to GPIO4, GPIO5 (pin80, pin81); if the value is other, it is multiplexed to GPIO8, GPIO9 (pin52, pin50);
 * @attention If id is 0 and value is 1, it is multiplexed to GPIO12, GPIO13 (pin58, pin57); value 2, it is multiplexed to GPIO16, GPIO17 (pin22, pin23), value is other, it is multiplexed to Modules pin67, pin66
 * @param id i2c_id
 * @return -1 failure, other normal*/

int luat_i2c_set_iomux(int id, uint8_t value);

/**
 * @brief initialize i2c
 *
 * @param id i2c_id
 * @param speed i2c speed
 * @param slaveaddr i2c slave address
 * @return 0 success, other failure*/
int luat_i2c_setup(int id, int speed);

/**
 * @brief close i2c
 *
 * @param id i2c_id
 * @return 0 success, other failure*/
int luat_i2c_close(int id);

/**
 * @brief I2C sends data
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param buff data buff
 * @param len data length
 * @param stop whether to send stop bit
 * @return 0 success, other failure*/
int luat_i2c_send(int id, int addr, void* buff, size_t len, uint8_t stop);
/**
 * @brief I2C accepts data
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param buff data buff
 * @param len data length
 * @return 0 success, other failure*/
int luat_i2c_recv(int id, int addr, void* buff, size_t len);
/**
 * @brief I2C data transceiver function, if reg, reg_len exists, it means reading the value of the register, if it does not exist, it means sending data
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param reg The register to be read, which can be an array
 * @param reg_len the length of the register
 * @param buff data buff, if reg and reg_len exist, it is the received data, if not, it is the sent data
 * @param len data length
 * @return 0 success, other failure*/
int luat_i2c_transfer(int id, int addr, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len);
/** @}*/
/** @}*/
#endif
