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
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_i2c.h"

luat_rtos_task_handle i2c_task_handle;

#define I2C_ID                  0
#define AHT10_ADDRESS_ADR_LOW   0x38

#define AHT10_INIT              0xE1 //Initialization command
#define AHT10_MEASURE           0xAC //Trigger measurement command
#define AHT10_SOFT_RESET        0xBA
#define AHT10_STATE             0x71 //Status word.

/*1. I2C reuse, the latest Hezhou standard CSDK no longer needs to be controlled through RTE_Device.h, the original driver still needs
    2.I2C reuse can be achieved through the int luat_i2c_set_iomux(int   id, uint8_t value) function
    If the id is 1 and the value is 1, it is multiplexed to GPIO4 and GPIO5; if the value is other, it is multiplexed to GPIO8 and GPIO9 (default interface);
    If id is 0 and value is 1, it is multiplexed to GPIO12 and GPIO13; value 2 is multiplexed to GPIO16 and GPIO17; value is other and multiplexed to Modules pin67 and pin66.*/
static void task_test_i2c(void *param)
{
    char soft_reset[] = {AHT10_SOFT_RESET}; 
    char init_cmd[] = {AHT10_INIT,0x08,0x00}; 
    char measure_cmd[] = {AHT10_MEASURE, 0x33, 0x00}; 
    char read_cmd[] = {AHT10_STATE}; 
    char recv_date[7] = {0};
    luat_i2c_setup(I2C_ID,1);
    luat_rtos_task_sleep(40);
    luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, soft_reset, 1, 1);
    luat_rtos_task_sleep(20);

    luat_i2c_recv(I2C_ID, AHT10_ADDRESS_ADR_LOW, recv_date, 1);

    if (recv_date[0]&(1<<3) == 0){
        luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, init_cmd, 3, 1);
    }
    while (1)
    {
        luat_rtos_task_sleep(1000);
        luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, measure_cmd, 3, 1);
        luat_rtos_task_sleep(80);
        luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, read_cmd, 1, 1);
        luat_i2c_recv(I2C_ID, AHT10_ADDRESS_ADR_LOW, recv_date, 6);

        float cur_temp = ((recv_date[3] & 0xf) << 16 | recv_date[4] << 8 | recv_date[5]) * 200.0 / (1 << 20) - 50;
        LUAT_DEBUG_PRINT("temp: %f",cur_temp);

    }
    
}

static void task_demo_i2c(void)
{
    luat_rtos_task_create(&i2c_task_handle, 1024, 20, "i2c", task_test_i2c, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_i2c,"1");



