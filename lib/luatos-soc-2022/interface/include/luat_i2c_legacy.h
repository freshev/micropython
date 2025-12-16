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

#ifndef LUAT_I2C_LEGACY_H
#define LUAT_I2C_LEGACY_H

#include "luat_base.h"



/**
 * @brief I2C sends data
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param buff data buff
 * @param len data length
 * @param stop whether to send stop bit
 * @return int*/
int luat_i2c_write_reg(int id, int addr, int reg, uint16_t value, uint8_t stop);

/**
 * @brief I2C accepts data
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param buff data buff
 * @param len data length
 * @return int*/
int luat_i2c_read_reg(int id, int addr, int reg, uint16_t* value);
/**
 * @brief Non-blocking I2C sending and receiving data*/
int luat_i2c_no_block_transfer(int id, int addr, uint8_t is_read, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len, uint16_t Toms, void *CB, void *pParam);

#ifdef __LUATOS__

#define LUAT_EI2C_TYPE "EI2C*"
#define toei2c(L) ((luat_ei2c *)luaL_checkudata(L, 1, LUAT_EI2C_TYPE))

typedef struct luat_ei2c {
    int sda;
    int scl;
} luat_ei2c;//Software i2c

void i2c_soft_start(luat_ei2c *ei2c);
char i2c_soft_recv(luat_ei2c *ei2c, unsigned char addr, char *buff, size_t len);
char i2c_soft_send(luat_ei2c *ei2c, unsigned char addr, char *data, size_t len, uint8_t stop);

#endif

#endif
