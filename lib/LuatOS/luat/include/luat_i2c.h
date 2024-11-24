#ifndef LUAT_I2C_H
#define LUAT_I2C_H

#include "luat_base.h"
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
 * @brief initialize i2c
 *
 * @param id i2c_id
 * @param speed i2c speed
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
 * @brief I2C write register
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param reg register address
 * @param buff data
 * @param len data length
 * @param stop whether to send stop bit
 * @return 0 success, other failure*/
int luat_i2c_write_reg(int id, int addr, int reg, void* buff, size_t len, uint8_t stop);
/**
 * @brief I2C read register
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param reg register address
 * @param buff data
 * @param len data length
 * @return 0 success, other failure*/
int luat_i2c_read_reg(int id, int addr, int reg, void* buff, size_t len);
/**
 * @brief I2C sends and receives data
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param reg register to read
 * @param reg_len register length
 * @param buff If reg, reg_len is not NULL, buff saves the buffer for the read data, otherwise it is the write buffer.
 * @param len data length
 * @return 0 success, other failure*/
int luat_i2c_transfer(int id, int addr, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len);

/**
 * @brief I2C sends and receives data (asynchronous)
 *
 * @param id i2c_id
 * @param addr 7-bit device address
 * @param is_read whether it is read and write
 * @param reg register to read
 * @param reg_len register length
 * @param buff If reg, reg_len is not NULL, buff saves the buffer for the read data, otherwise it is the write buffer.
 * @param len data length
 * @param Toms data length
 * @param CB callback function
 * @param pParam callback function parameters
 * @return 0 success, other failure*/
int luat_i2c_no_block_transfer(int id, int addr, uint8_t is_read, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len, uint16_t Toms, void *CB, void *pParam);

/**
 * @brief Set up i2c multiplexing
 *@attention
 *@attention
 * @param id i2c_id
 * @return -1 failure, other normal*/
int luat_i2c_set_iomux(int id, uint8_t value);

int luat_i2c_set_polling_mode(int id, uint8_t on_off);
/** @}*/

#define LUAT_EI2C_TYPE "EI2C*"

typedef struct luat_ei2c {
    int8_t sda;
    int8_t scl;
    int16_t udelay;
} luat_ei2c_t;//Software i2c

void i2c_soft_start(luat_ei2c_t *ei2c);
char i2c_soft_recv(luat_ei2c_t *ei2c, unsigned char addr, char *buff, size_t len);
char i2c_soft_send(luat_ei2c_t *ei2c, unsigned char addr, char *data, size_t len, uint8_t stop);


#define toei2c(L) ((luat_ei2c_t *)luaL_checkudata(L, 1, LUAT_EI2C_TYPE))

#endif
