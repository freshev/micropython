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

#ifndef LUAT_SPI_H
#define LUAT_SPI_H

#include "luat_base.h"

/**
 * @ingroup luatos_device peripheral interface
 * @{*/

/**
 * @defgroup luatos_device_spi SPI interface
 * @{*/

/**
 * @brief SPI function configuration*/
typedef struct luat_spi
{
    int  id;            /**< spi id optional 1, 0*/
    int  CPHA;          /**< CPHA optional 1,0*/  
    int  CPOL;          /**< CPOL optional 1,0*/  
    int  dataw;         /**< Data width 8: 8bit*/
    int  bit_dict;      /**< High and low order optional 1: MSB, 0: LSB*/  
    int  master;        /**< Set master-slave mode optional 1: master, 0: slave*/  
    int  mode;          /**< Set full\half duplex optional 1: full duplex, 0: half duplex*/  
    int bandrate;       /**< Frequency minimum 100000, maximum 25600000*/  
    int cs;             /**< cs control pin The chip select of SPI0 is GPIO8. When configured to 8, it means that SPI0's built-in chip select is enabled; for other configurations, you need to code and control the chip select by yourself.*/  
} luat_spi_t;


/**
 * @brief Initialize and configure SPI parameters and open SPI
 *
 * @param spi spi structure
 * @return int Returns 0 successfully*/
int luat_spi_setup(luat_spi_t* spi);

/**
 * @brief Turn off SPI
 *
 * @param spi_id spi id
 * @return int Returns 0 successfully*/
int luat_spi_close(int spi_id);

/**
 * @brief Send and receive SPI data
 *
 * @param spi_id spi id
 * @param send_buf send data
 * @param send_length send data length
 * @param recv_buf receive data
 * @param recv_length received data length
 * @return int Returns the number of bytes received*/
int luat_spi_transfer(int spi_id, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length);

/**
 * @brief Receive SPI data
 *
 * @param spi_id spi id
 * @param recv_buf receive data
 * @param length data length
 * @return int Returns the number of bytes received*/
int luat_spi_recv(int spi_id, char* recv_buf, size_t length);

/**
 * @brief Send SPI data
 *
 * @param spi_id spi id
 * @param send_buf send data
 * @param length data length
 * @return int Returns the number of bytes sent*/
int luat_spi_send(int spi_id, const char* send_buf, size_t length);

#include "luat_spi_legacy.h"


/**@}*/
/**@}*/
#endif
