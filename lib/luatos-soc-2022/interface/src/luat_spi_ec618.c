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


#include "luat_base.h"
#include "luat_spi.h"

#include "common_api.h"
#include <stdio.h>
#include <string.h>
#include "bsp_custom.h"
#include "soc_spi.h"
#include "driver_gpio.h"

static uint8_t g_s_luat_spi_mode[SPI_MAX] ={0};

static int spi_exist(int id) {
    if (id < SPI_MAX) return 1;
    return 0;
}

#ifdef __LUATOS__
#include "luat_lcd.h"
#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */
#endif

int luat_spi_device_config(luat_spi_device_t* spi_dev) {
    if (!spi_exist(spi_dev->bus_id))
        return -1;
    uint8_t spi_mode = SPI_MODE_0;
    if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
    else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
    else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
    SPI_SetNewConfig(spi_dev->bus_id, spi_dev->spi_config.bandrate, spi_mode);
    return 0;
}

int luat_spi_bus_setup(luat_spi_device_t* spi_dev){
    if (!spi_exist(spi_dev->bus_id))
        return -1;
    uint8_t spi_mode = SPI_MODE_0;
    if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
    else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
    else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
    if (spi_dev->bus_id)
    {
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_13, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_14, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_15, 0), 1, 1, 0);
    }
    else
    {
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_9, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_10, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_11, 0), 1, 1, 0);
    }
    g_s_luat_spi_mode[spi_dev->bus_id] = spi_dev->spi_config.mode;
    // LLOGD("SPI_MasterInit luat_bus_%d:%d dataw:%d spi_mode:%d bandrate:%d ",bus_id,luat_spi[bus_id].id, spi_dev->spi_config.dataw, spi_mode, spi_dev->spi_config.bandrate);
    SPI_MasterInit(spi_dev->bus_id, spi_dev->spi_config.dataw, spi_mode, spi_dev->spi_config.bandrate, NULL, NULL);
    return 0;
}

int luat_spi_change_speed(int spi_id, uint32_t speed)
{
    if (!spi_exist(spi_id))
        return -1;
    SPI_SetNewConfig(spi_id, speed, 0xff);
    return 0;
}


int luat_spi_setup(luat_spi_t* spi) {
    if (!spi_exist(spi->id))
        return -1;
    uint8_t spi_mode = SPI_MODE_0;
    if(spi->CPHA&&spi->CPOL)spi_mode = SPI_MODE_3;
    else if(spi->CPOL)spi_mode = SPI_MODE_2;
    else if(spi->CPHA)spi_mode = SPI_MODE_1;
    if (spi->id)
    {
        if (HAL_GPIO_12 == spi->cs)
        {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_12, 0), 1, 1, 0);
        }
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_13, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_14, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_15, 0), 1, 1, 0);
    }
    else
    {
        if (HAL_GPIO_8 == spi->cs)
        {
            GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_8, 0), 1, 1, 0);
        }
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_9, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_10, 0), 1, 1, 0);
        GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_11, 0), 1, 1, 0);
    }
    g_s_luat_spi_mode[spi->id] = spi->mode;
//    LLOGD("SPI_MasterInit luat_spi%d:%d dataw:%d spi_mode:%d bandrate:%d ",spi_id,luat_spi[spi_id].id, spi->dataw, spi_mode, spi->bandrate);
    SPI_MasterInit(spi->id, spi->dataw, spi_mode, spi->bandrate, NULL, NULL);
    return 0;
}

//Close SPI, return 0 successfully
int luat_spi_close(int spi_id) {
    return 0;
}

//Send and receive SPI data and return the number of bytes received
int luat_spi_transfer(int spi_id, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length) {
    if (!spi_exist(spi_id))
        return -1;
    if(g_s_luat_spi_mode[spi_id])
    {
        if (SPI_BlockTransfer(spi_id, (uint8_t*)send_buf, (uint8_t*)recv_buf, recv_length))
        {
            return 0;
        }
        else
        {
            return recv_length;
        }
    }
    else
    {
        if (SPI_FlashBlockTransfer(spi_id, (uint8_t*)send_buf, send_length, (uint8_t*)recv_buf, recv_length))
        {
            return 0;
        }
        else
        {
            return recv_length;
        }
    }
    return 0;
}

//Receive SPI data and return the number of bytes received
int luat_spi_recv(int spi_id, char* recv_buf, size_t length) {
    if (!spi_exist(spi_id))
        return -1;
    if (SPI_GetSpeed(spi_id) > 12800000)
    {
        SPI_SetDMAEnable(spi_id, 0);
    }

    if (SPI_BlockTransfer(spi_id, (uint8_t*)recv_buf, (uint8_t*)recv_buf, length))
    {
        if (SPI_GetSpeed(spi_id) > 12800000)
        {
            SPI_SetDMAEnable(spi_id, 1);
        }
        return 0;
    }
    else
    {
        if (SPI_GetSpeed(spi_id) > 12800000)
        {
            SPI_SetDMAEnable(spi_id, 1);
        }
        return length;
    }
}
//Send SPI data and return the number of bytes sent
int luat_spi_send(int spi_id, const char* send_buf, size_t length) {
    if (!spi_exist(spi_id))
        return -1;
    if (SPI_BlockTransfer(spi_id, (uint8_t*)send_buf, NULL, length))
    {
        return 0;
    }
    else
    {
        return length;
    }
}

int luat_spi_no_block_transfer(int spi_id, uint8_t *tx_buff, uint8_t *rx_buff, size_t len, void *CB, void *pParam)
{
    if (SPI_IsTransferBusy(spi_id)) return -1;
    SPI_SetCallbackFun(spi_id, CB, pParam);
    SPI_SetNoBlock(spi_id);
    return SPI_TransferEx(spi_id, tx_buff, rx_buff, len, 0, 1);
}

#if 0
#ifdef LUAT_USE_LCD_CUSTOM_DRAW
static uint8_t g_s_lcd_draw_done = 1;
static int32_t luat_lcd_spi_cb_handler(void *pdata, void *param)
{
    int cs_pin = (int)param;
    GPIO_Output(cs_pin, 1);
    g_s_lcd_draw_done = 1;
    return 0;
}

static void luat_lcd_cmd(uint8_t spi_id, uint8_t cs_pin, uint8_t dc_pin, uint8_t Cmd)
{
    GPIO_Output(dc_pin, 0);
    GPIO_Output(cs_pin, 0);
    SPI_BlockTransfer(spi_id, &Cmd, NULL, 1);
    GPIO_Output(cs_pin, 1);
    GPIO_Output(dc_pin, 1);
}

static void luat_lcd_data(uint8_t spi_id, uint8_t cs_pin, uint8_t *Data, uint32_t Len)
{
    GPIO_Output(cs_pin, 0);
    SPI_BlockTransfer(spi_id, Data, NULL, Len);
    GPIO_Output(cs_pin, 1);
}

int luat_lcd_draw_no_block(luat_lcd_conf_t* conf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t* color, uint8_t last_flush)
{
    PV_Union uPV;
    uint32_t retry_cnt = 0;
    if (conf->port == LUAT_LCD_SPI_DEVICE){
        luat_spi_device_t* spi_dev = (luat_spi_device_t*)conf->lcd_spi_device;
        while (!g_s_lcd_draw_done)
        {
            retry_cnt++;
            luat_timer_mdelay(1);
        }
        g_s_lcd_draw_done = 0;
        if (retry_cnt)
        {
            DBG("lcd flush delay %ums, status %u,%u,%d", retry_cnt, x2-x1+1, y2-y1+1, last_flush);
        }

        uint8_t spi_mode = SPI_MODE_0;
        if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
        else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
        else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
        SPI_SetNewConfig(spi_dev->bus_id, spi_dev->spi_config.bandrate, spi_mode);
        luat_lcd_cmd(spi_dev->bus_id, spi_dev->spi_config.cs, conf->pin_dc, 0x2a);
        BytesPutBe16(uPV.u8, x1 + conf->xoffset);
        BytesPutBe16(uPV.u8 + 2, x2 + conf->xoffset);
        luat_lcd_data(spi_dev->bus_id, spi_dev->spi_config.cs, uPV.u8, 4);
        luat_lcd_cmd(spi_dev->bus_id, spi_dev->spi_config.cs, conf->pin_dc, 0x2b);
        BytesPutBe16(uPV.u8, y1 + conf->yoffset);
        BytesPutBe16(uPV.u8 + 2, y2 + conf->yoffset);
        luat_lcd_data(spi_dev->bus_id, spi_dev->spi_config.cs, uPV.u8, 4);
        luat_lcd_cmd(spi_dev->bus_id, spi_dev->spi_config.cs, conf->pin_dc, 0x2c);
        GPIO_Output(spi_dev->spi_config.cs, 0);
        uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
        //uint32_t size = conf->w * (conf->flush_y_max - conf->flush_y_min + 1) * 2;
        SPI_SetCallbackFun(spi_dev->bus_id, luat_lcd_spi_cb_handler, spi_dev->spi_config.cs);
        return SPI_TransferEx(spi_dev->bus_id, color, color, size, 0, 1);
    }
    else
    {
        return -1;
    }
}


int luat_lcd_flush(luat_lcd_conf_t* conf) {
    if (conf->buff == NULL) {
        return 0;
    }
    //DBG("luat_lcd_flush range %d %d", conf->flush_y_min, conf->flush_y_max);
    if (conf->flush_y_max < conf->flush_y_min) {
        // There is no content that needs to be refreshed, skip it directly
        //DBG("luat_lcd_flush no need");
        return 0;
    }
    if (conf->is_init_done) {
        luat_lcd_draw_no_block(conf, 0, conf->flush_y_min, conf->w - 1, conf->flush_y_max, &conf->buff[conf->flush_y_min * conf->w], 0);
    }
    else {
        uint32_t size = conf->w * (conf->flush_y_max - conf->flush_y_min + 1) * 2;
        luat_lcd_set_address(conf, 0, conf->flush_y_min, conf->w - 1, conf->flush_y_max);
        const char* tmp = (const char*)(conf->buff + conf->flush_y_min * conf->w);
        if (conf->port == LUAT_LCD_SPI_DEVICE){
            luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), tmp, size);
        }else{
            luat_spi_send(conf->port, tmp, size);
        }
    }
    //Reset to a state that does not require refreshing
    conf->flush_y_max = 0;
    conf->flush_y_min = conf->h;

    return 0;
}

int luat_lcd_draw(luat_lcd_conf_t* conf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t* color) {
    //Direct swipe mode
    if (conf->buff == NULL) {
        if (conf->is_init_done)
        {
            return luat_lcd_draw_no_block(conf, x1, y1, x2, y2, color, 0);
        }
        uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
        luat_lcd_set_address(conf, x1, y1, x2, y2);
        if (conf->port == LUAT_LCD_SPI_DEVICE){
            luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), (const char*)color, size);
        }else{
            luat_spi_send(conf->port, (const char*)color, size);
        }
        return 0;
    }
    // buff mode
    if (x1 > conf->w || y1 > conf->h) {
        DBG("out of lcd range");
        return -1;
    }
    uint16_t x_end = x2 > conf->w?conf->w:x2;
    uint16_t y_end = y2 > conf->h?conf->h:y2;
    luat_color_t* dst = (conf->buff + x1 + conf->w * y1);
    luat_color_t* src = (color);
    size_t lsize = (x_end - x1 + 1);
    for (size_t i = y1; i <= y_end; i++) {
        memcpy(dst, src, lsize * sizeof(luat_color_t));
        dst += conf->w;  //Move to next line
        src += lsize;    //Move data
        if (x2 > conf->w){
            src+=x2 - conf->w;
        }
    }
    //Storage the area that needs to be refreshed
    if (y1 < conf->flush_y_min)
        conf->flush_y_min = y1;
    if (y_end > conf->flush_y_max)
        conf->flush_y_max = y_end;
    return 0;
}
#endif
#endif


#ifdef __LUATOS__
extern void luat_spi_set_sdhc_ctrl_default(
        block_disk_t *disk
);
void luat_spi_set_sdhc_ctrl(block_disk_t *disk) {
    luat_spi_set_sdhc_ctrl_default(disk);
}


void luat_sdio_set_sdhc_ctrl(block_disk_t *disk) {
}

#endif
