/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_conf.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2012/10/8
 *
 * Description:
 * 
 **************************************************************************/

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#include "auxmods.h"

// *****************************************************************************
// ¶ Williams have in love allottedôææ¹¹|ÄÜÜÜs
#define BUILD_LUA_INT_HANDLERS
#define BUILD_C_INT_HANDLERS

/*+\NEW a siving\liger\2013.12.6\´t availability of the 1000´´½KO´½ ´s , 100‐down swelling silence selves and skulloced silence snacks .*/
#if DLMALLOC_DEFAULT_GRANULARITY > (200*1024)
#define USE_DLMALLOC_ALLOCATOR
#else
#define USE_PLATFORM_ALLOCATOR
#endif
/*-\ENTurang\li\2013.12.6\´s a 1»´´½´½KA´â€Odt ´s worshiped, DOK and sweeping sweeping sweeping sweeping sweeping sku .*/

// *****************************************************************************
// Configuration data

// Virtual timers (0 if not used)
#define VTMR_NUM_TIMERS       0

// Number of resources (0 if not available/not implemented)
#define NUM_PIO               3 // port 0:gpio0-31; port 1:gpio32-gpio55; port 2: gpio ex;
#define NUM_SPI               0
#define NUM_UART              4 //Éµ¼êö »Óð2çöîïàn´® ° ID0-¼æèý¾é ° æ ± ¾îªuart2 ID1 -UART1 ID2 -UART2 ID3-MOSTUART
#define NUM_TIMER             2
#define NUM_PWM               0
#define NUM_ADC               8
#define NUM_CAN               0
#define NUM_I2C               3

#define PIO_PIN_EX            9 /*gpio ex 0~6,7,8*/
#define PIO_PIN_ARRAY         {32 /* gpio_num 32 */, 32/* gpio_num 56 */, 32}

//ÐéÄâatÃüÁîÍ¨µÀ
#define PLATFORM_UART_ID_ATC              0x7f

//host uart debugÍ¨µÀ
#define PLATFORM_PORT_ID_DEBUG            0x80

/*+\NEW\Nigaguesayan\2019.5.8\«outh AT¬èëb AT±¨ëëhöëbèëbèëbôëbönôëbôëëhôëëhôëëhôëëhôëëhôëëhôëëhôëëhôëëhôëëhôëëhôëôëhôëëhôëëhôëëhôëëhôëëhôëëhôëëhôôôôôôôôôv-ôs.*/
#define PLATFORM_PORT_ID_USB              0x81
/*-\NOW\NOTY)9.5.8\« Externity°â₱´s |*/

//ÃüÁîÐÐÍ¨µÀ
#define CON_UART_ID           (platform_get_console_port())
#define CON_UART_SPEED        115200
#define CON_TIMER_ID          0

// PIO prefix ('0' for P0, P1, ... or 'A' for PA, PB, ...)
#define PIO_PREFIX            '0'

/*+\NEW\liweiqiang\2013.7.16\Ôö¼Óiconv×Ö·û±àÂë×ª»»¿â*/
#ifdef LUA_ICONV_LIB
#define ICONV_LINE   _ROM( AUXLIB_ICONV, luaopen_iconv, iconv_map )
#else
#define ICONV_LINE   
#endif
/*-\NEW\liweiqiang\2013.7.16\Ôö¼Óiconv×Ö·û±àÂë×ª»»¿â*/

/*+\NEW\liweiqiang\2014.2.9\Ôö¼Ózlib¿â*/
#ifdef LUA_ZLIB_LIB
#define ZLIB_LINE   _ROM( AUXLIB_ZLIB, luaopen_zlib, zlib_map )
#else
#define ZLIB_LINE
#endif
/*-\NEW\liweiqiang\2014.2.9\Ôö¼Ózlib¿â*/

#ifdef LUA_LVGL_SUPPORT
#define LVGL_LIB_LINE   _ROM( AUXLIB_LVGL, luaopen_lvgl, NULL )
#else
#define LVGL_LIB_LINE
#endif

/*+\ New \ liweiqiang, panjun \ 2015.04.21 \ am002_lua² »ö§³öïield*/
#ifdef LUA_DISP_LIB
#define DISP_LIB_LINE   _ROM( AUXLIB_DISP, luaopen_disp, disp_map )
#else
#define DISP_LIB_LINE
#endif
/*-\ New \ liweiqiang, panjun \ 2015.04.21 \ am002_lua² »ö§³öïield*/

#define JSON_LIB_LINE   _ROM( AUXLIB_JSON, luaopen_cjson, json_map )

#ifdef AM_CRYPTO_SUPPORT
#define CRYPTO_LIB_LINE _ROM( AUXLIB_CRYPTO,luaopen_crypto, crypto_map)
#else
#define CRYPTO_LIB_LINE
#endif

#ifdef HRD_SENSOR_SUPPORT
#define HRD_SENSOR_LINE   _ROM( AUXLIB_HRSENSOR, luaopen_hrsensorcore, hrsensor_map )
#else
#define HRD_SENSOR_LINE
#endif

#if defined(AM_PBC_SUPPORT)
#define PBC_LIB_LINE   _ROM( AUXLIB_PBC, luaopen_protobuf_c, pbc_map )
#else
#define PBC_LIB_LINE
#endif

/*+\NEW\NEW\senawoes\2020.31\¬·un άvicious · Vences of burdens*/
#ifdef LUA_QRENCODE_SUPPORT
#define QRENCODE_LIB_LINE	_ROM( AUXLIB_QRENCODE, luaopen_qr_encode, qr_encode_map )	 
#else
#define QRENCODE_LIB_LINE
#endif
/*-\NEW\NEW\senyafried\20*/

#if defined(LUA_WIFISCAN_SUPPORT)
#define WIFI_LIB_LINE	_ROM( AUXLIB_WIFI, luaopen_wificore, wifi_map)	 
#else
#define WIFI_LIB_LINE
#endif

#ifdef LUA_GPIO_I2C
#define GPIO_I2C_LINE _ROM( AUXLIB_GPIO_I2C, luaopen_gpio_i2c, gpio_i2c_map ) 
#else
#define GPIO_I2C_LINE
#endif

#ifdef LUA_GPS_LIB
#define GPS_LIB_LINE _ROM( AUXLIB_GPSCORE, luaopen_gpscore, gpscore_map ) 
#else
#define GPS_LIB_LINE
#endif
/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/
#ifdef LUA_BLUETOOTH_LIB
#define BLUETOOTH_LIB_LINE _ROM( AUXLIB_BLUETOOTH, luaopen_bluetooth, bluetooth_map ) 
#else
#define BLUETOOTH_LIB_LINE
#endif
/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/


#define LUA_PLATFORM_LIBS_ROM \
    _ROM( AUXLIB_BIT, luaopen_bit, bit_map ) \
    _ROM( AUXLIB_SPI, luaopen_spi, spi_map) \
    _ROM( AUXLIB_BITARRAY, luaopen_bitarray, bitarray_map ) \
    _ROM( AUXLIB_PACK, luaopen_pack, pack_map ) \
    _ROM( AUXLIB_PIO, luaopen_pio, pio_map ) \
    _ROM( AUXLIB_UART, luaopen_uart, uart_map ) \
    _ROM( AUXLIB_I2C, luaopen_i2c, i2c_map ) \
    _ROM( AUXLIB_RTOS, luaopen_rtos, rtos_map ) \
    _ROM( AUXLIB_PMD, luaopen_pmd, pmd_map ) \
    _ROM( AUXLIB_ADC, luaopen_adc, adc_map ) \
    _ROM( AUXLIB_PWM, luaopen_pwm, pwm_map ) \
    _ROM( AUXLIB_TTSPLYCORE, luaopen_ttsplycore, ttsplycore_map) \
    _ROM( AUXLIB_AUDIOCORE, luaopen_audiocore, audiocore_map ) \
    DISP_LIB_LINE \
    BLUETOOTH_LIB_LINE\
    ICONV_LINE \
    ZLIB_LINE \
    HRD_SENSOR_LINE \
    JSON_LIB_LINE \
    CRYPTO_LIB_LINE \
    PBC_LIB_LINE \
    QRENCODE_LIB_LINE \
    WIFI_LIB_LINE \
    GPIO_I2C_LINE \
    GPS_LIB_LINE \
    LVGL_LIB_LINE \
    _ROM( AUXLIB_CPU, luaopen_cpu, cpu_map) \
    _ROM( AUXLIB_TCPIPSOCK, luaopen_tcpipsock, tcpipsock_map) \
    _ROM( AUXLIB_WATCHDOG, luaopen_watchdog, watchdog_map ) \
    _ROM( AUXLIB_FACTORY, luaopen_factorycore, factorycore_map) \
    _ROM( AUXLIB_OneWire, luaopen_onewire, onewire_map) 
 
 
    // Interrupt queue size
#define PLATFORM_INT_QUEUE_LOG_SIZE 5

#define CPU_FREQUENCY         ( 26 * 1000 * 1000 )

// Interrupt list
#define INT_GPIO_POSEDGE      ELUA_INT_FIRST_ID
#define INT_GPIO_NEGEDGE      ( ELUA_INT_FIRST_ID + 1 )
#define INT_ELUA_LAST         INT_GPIO_NEGEDGE
    
#define PLATFORM_CPU_CONSTANTS \
     _C( INT_GPIO_POSEDGE ),\
     _C( INT_GPIO_NEGEDGE )

#endif //__PLATFORM_CONF_H__
