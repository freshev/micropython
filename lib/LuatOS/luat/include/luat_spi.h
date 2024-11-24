
#ifndef LUAT_SPI_H
#define LUAT_SPI_H
#include "luat_base.h"
/**
 * @defgroup luatos_device_spi SPI interface
 * @{*/
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

typedef struct luat_spi_device
{
    uint8_t  bus_id;
    luat_spi_t spi_config;
    void* user_data;
} luat_spi_device_t;

typedef struct luat_fatfs_spi
{
	uint8_t type;
	uint8_t spi_id;
	uint8_t spi_cs;
	uint8_t nop;
	uint32_t fast_speed;
    uint8_t transfer_buf[7];
	luat_spi_device_t * spi_device;
}luat_fatfs_spi_t;

/**
    spiId,--serial port id
    cs,
    0,--CPHA
    0,--CPOL
    8,--data width
    20000000,--maximum frequency 20M
    spi.MSB,--high and low order, optional, high-end first by default
    spi.master,--master mode optional, default master
    spi.full,--full duplex optional, default full duplex*/
/**
 * @brief Initialize and configure SPI parameters and open SPI
 *
 * @param spi spi structure
 * @return int Returns 0 successfully*/
int luat_spi_setup(luat_spi_t* spi);
/**
 * @brief SPI sends and receives data and tries to start DMA mode
 *
 * @param spi_id spi id
 * @param tx_channel sending channel
 * @param rx_channel receiving channel
 * @return int*/
int luat_spi_config_dma(int spi_id, uint32_t tx_channel, uint32_t rx_channel);
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
/**
 * @brief SPI rate modification
 *
 * @param spi_id spi id
 * @param speed speed
 * @return int Returns the number of bytes sent*/
int luat_spi_change_speed(int spi_id, uint32_t speed);
/**
 * @brief SPI sends and receives data (asynchronous)
 *
 * @param spi_id spi id
 * @param tx_buff send data
 * @param rx_buff receives data
 * @param len data length
 * @param CB callback function
 * @param pParam callback parameter
 * @return int Returns the number of bytes sent*/
int luat_spi_no_block_transfer(int spi_id, uint8_t *tx_buff, uint8_t *rx_buff, size_t len, void *CB, void *pParam);
/**
 * @brief SPI mode acquisition
 *
 * @param spi_id spi id
 * @return int mode*/
int luat_spi_get_mode(int spi_id);
/**
 * @brief SPI mode modification
 *
 * @param spi_id spi id
 * @param mode mode
 * @return int Returns the number of bytes sent*/
int luat_spi_set_mode(int spi_id, uint8_t mode);

/**
 * @brief spi bus initialization
 *
 * @param spi_dev luat_spi_device_t structure
 * @return int*/
int luat_spi_bus_setup(luat_spi_device_t* spi_dev);
/**
 * @brief spi device initialization
 *
 * @param spi_dev luat_spi_device_t structure
 * @return int*/
int luat_spi_device_setup(luat_spi_device_t* spi_dev);
/**
 * @brief spi device configuration
 *
 * @param spi_dev luat_spi_device_t structure
 * @return int*/
int luat_spi_device_config(luat_spi_device_t* spi_dev);
/**
 * @brief spi device is closed
 *
 * @param spi_dev luat_spi_device_t structure
 * @return int*/
int luat_spi_device_close(luat_spi_device_t* spi_dev);
/**
 * @brief spi device sends and receives data and returns the number of bytes received
 *
 * @param spi_dev luat_spi_device_t structure
 * @param send_buf send data
 * @param send_length send data length
 * @param recv_buf receive data
 * @param recv_length received data length
 * @return int*/
int luat_spi_device_transfer(luat_spi_device_t* spi_dev, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length);
/**
 * @brief spi device receives data and returns the number of bytes received
 *
 * @param spi_dev luat_spi_device_t structure
 * @param recv_buf receive data
 * @param length data length
 * @return int Returns the number of bytes received*/
int luat_spi_device_recv(luat_spi_device_t* spi_dev, char* recv_buf, size_t length);
/**
 * @brief spi device sends data and returns the number of bytes received
 *
 * @param spi_dev luat_spi_device_t structure
 * @param send_buf send data
 * @param length data length
 * @return int Returns the number of bytes sent*/
int luat_spi_device_send(luat_spi_device_t* spi_dev, const char* send_buf, size_t length);

/**@}*/
#endif
