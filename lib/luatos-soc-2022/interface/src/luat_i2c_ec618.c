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

#if 0

#include "FreeRTOS.h"
#include "luat_i2c.h"
#include "common_api.h"
#include <stdio.h>
#include <string.h>
#include "bsp_custom.h"

#if RTE_I2C0
extern ARM_DRIVER_I2C Driver_I2C0;
static ARM_DRIVER_I2C   *i2cDrv0 = &CREATE_SYMBOL(Driver_I2C, 0);
#endif
#if RTE_I2C1
extern ARM_DRIVER_I2C Driver_I2C1;
static ARM_DRIVER_I2C   *i2cDrv1 = &CREATE_SYMBOL(Driver_I2C, 1);
#endif

int luat_i2c_exist(int id) {
    #if RTE_I2C0
    if (id == 0)
        return 1;
    #endif
    #if RTE_I2C1
    if (id == 1)
        return 1;
    #endif
    return 0;
}
int luat_i2c_setup(int id, int speed) {
    if (!luat_i2c_exist(id)) {
        return -1;
    }
    int ret = 0;
#if RTE_I2C0
    if (id==0)
    {
        ret = i2cDrv0->Initialize(NULL);
        if (ret)  DBG("i2c0 setup error -- Initialize - %ld", ret);
        ret = i2cDrv0->PowerControl(ARM_POWER_FULL);
        if (ret)  DBG("i2c0 setup error -- PowerControl - %ld", ret);
        if (speed == 0)//(100kHz)
            ret = i2cDrv0->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
        else if (speed == 1)//(400kHz)
            ret = i2cDrv0->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
        else if (speed == 2)//(  1MHz)
            ret = i2cDrv0->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST_PLUS);
        else if (speed == 3)//(3.4MHz)
            ret = i2cDrv0->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_HIGH);
        if (ret)  DBG("i2c0 setup error -- Control SPEED - %ld", ret);
        ret = i2cDrv0->Control(ARM_I2C_BUS_CLEAR, 0);
        if (ret)  DBG("i2c0 setup error -- Control CLEAR - %ld", ret);
        DBG("i2c0 setup complete");
    }
#endif
#if RTE_I2C1
    if (id==1)
    {
        ret = i2cDrv1->Initialize(NULL);
        if (ret)  DBG("i2c1 setup error -- Initialize - %ld", ret);
        ret = i2cDrv1->PowerControl(ARM_POWER_FULL);
        if (ret)  DBG("i2c1 setup error -- PowerControl - %ld", ret);
        if (speed == 0)//(100kHz)
            ret = i2cDrv1->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
        else if (speed == 1)//(400kHz)
            ret = i2cDrv1->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
        else if (speed == 2)//(  1MHz)
            ret = i2cDrv1->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST_PLUS);
        else if (speed == 3)//(3.4MHz)
            ret = i2cDrv1->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_HIGH);
        if (ret)  DBG("i2c1 setup error -- Control SPEED - %ld", ret);
        ret = i2cDrv1->Control(ARM_I2C_BUS_CLEAR, 0);
        if (ret)  DBG("i2c1 setup error -- Control CLEAR - %ld", ret);
        DBG("i2c1 setup complete");
    }
#endif
    return ret;
}

int luat_i2c_close(int id) {
    if (!luat_i2c_exist(id)) {
        return -1;
    }
#if RTE_I2C0
    if (id==0){
        i2cDrv0->PowerControl(ARM_POWER_OFF);
        i2cDrv0->Uninitialize();
    }
#endif
#if RTE_I2C1
    if (id==1){
        i2cDrv1->PowerControl(ARM_POWER_OFF);
        i2cDrv1->Uninitialize();
    }
#endif
    return 0;
}

int luat_i2c_send(int id, int addr, void* buff, size_t len, uint8_t stop) {
    if (!luat_i2c_exist(id)) {
        return -1;
    }
#if RTE_I2C0
    if (id==0){
        if (stop)
            return i2cDrv0->MasterTransmit(addr, buff, len, false);
        else
            return i2cDrv0->MasterTransmit(addr, buff, len, true);
    }
#endif
#if RTE_I2C1
    if (id==1){
        if (stop)
            return i2cDrv1->MasterTransmit(addr, buff, len, false);
        else
            return i2cDrv1->MasterTransmit(addr, buff, len, true);
    }
#endif
}

int luat_i2c_recv(int id, int addr, void* buff, size_t len) {
    if (!luat_i2c_exist(id)) {
        return -1;
    }
#if RTE_I2C0
    if (id==0){
        return i2cDrv0->MasterReceive(addr, buff, len, false);
    }
#endif
#if RTE_I2C1
    if (id==1){
        return i2cDrv1->MasterReceive(addr, buff, len, false);
    }
#endif
    
}

int luat_i2c_transfer(int id, int addr, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len)
{
    int result;
    result = luat_i2c_send(id, addr, reg, reg_len, 0);
    if (result != 0) return-1;
    return luat_i2c_recv(id, addr, buff, len);
}

#else
#include "luat_i2c.h"
#include "common_api.h"
#include "soc_i2c.h"
#include "driver_gpio.h"
#include "gpr_common.h"
extern void GPR_swResetModule(const ClockResetVector_t *v);
static uint32_t luat_i2c_global_timeout = 25;
static uint8_t luat_i2c_iomux[I2C_MAX];
static const ClockResetVector_t g_i2cResetVectors[] = {I2C0_RESET_VECTOR, I2C1_RESET_VECTOR};
int luat_i2c_exist(int id) {
    return (id < I2C_MAX);
}

int luat_i2c_set_polling_mode(int id, uint8_t on_off)
{
	if (!luat_i2c_exist(id)) return -1;
    I2C_UsePollingMode(id, on_off);
    return 0;
}

int luat_i2c_set_iomux(int id, uint8_t value)
{
	if (!luat_i2c_exist(id)) return -1;
	luat_i2c_iomux[id] = value;
}

int luat_i2c_setup(int id, int speed) {
    if (speed == 0) {
        speed = 100 * 1000; // SLOW
    }
    else if (speed == 1) {
        speed = 400 * 1000; // FAST
    }
    else if (speed == 2) {
        speed = 400 * 1000; // SuperFast
    }
    if (!luat_i2c_exist(id)) return -1;
    if (id)
    {
    	switch(luat_i2c_iomux[id])
    	{
    	case 1:
        	GPIO_IomuxEC618(19, 2, 1, 0);
        	GPIO_IomuxEC618(20, 2, 1, 0);
    		break;
    	default:
        	GPIO_IomuxEC618(23, 2, 1, 0);
        	GPIO_IomuxEC618(24, 2, 1, 0);
        	break;
    	}

    }
    else
    {
    	switch(luat_i2c_iomux[id])
    	{
    	case 1:
        	GPIO_IomuxEC618(27, 2, 1, 0);
        	GPIO_IomuxEC618(28, 2, 1, 0);
    		break;
    	case 2:
        	GPIO_IomuxEC618(31, 2, 1, 0);
        	GPIO_IomuxEC618(32, 2, 1, 0);
    		break;
    	default:
        	GPIO_IomuxEC618(13, 2, 1, 0);
        	GPIO_IomuxEC618(14, 2, 1, 0);
        	break;
    	}

    }
	I2C_MasterSetup(id, speed);
#ifdef __LUATOS__
	luat_i2c_set_polling_mode(id, 1);
#endif
    return 0;
}

int luat_i2c_close(int id) {
    if (!luat_i2c_exist(id)) return -1;
    if (id)
    {
    	switch(luat_i2c_iomux[id])
    	{
    	case 1:
        	GPIO_IomuxEC618(19, 0, 1, 0);
        	GPIO_IomuxEC618(20, 0, 1, 0);
    		break;
    	default:
        	GPIO_IomuxEC618(23, 0, 1, 0);
        	GPIO_IomuxEC618(24, 0, 1, 0);
        	break;
    	}

    }
    else
    {
    	switch(luat_i2c_iomux[id])
    	{
    	case 1:
        	GPIO_IomuxEC618(27, 0, 1, 0);
        	GPIO_IomuxEC618(28, 0, 1, 0);
    		break;
    	case 2:
        	GPIO_IomuxEC618(31, 0, 1, 0);
        	GPIO_IomuxEC618(32, 0, 1, 0);
    		break;
    	default:
        	GPIO_IomuxEC618(13, 0, 1, 0);
        	GPIO_IomuxEC618(14, 0, 1, 0);
        	break;
    	}

    }
    return 0;
}

void luat_i2c_set_global_timeout(uint32_t time)
{
	luat_i2c_global_timeout = time;
}

static void i2c_failed(int id, int addr, int code)
{
	GPR_swResetModule(&g_i2cResetVectors[id]);
	switch(code)
	{
	case -ERROR_OPERATION_FAILED:
		DBG("i2c%d slave address %x no response", id, addr);
		break;
	case -ERROR_TIMEOUT:
		DBG("i2c%d slave address %x transmission timeout", id, addr);
		break;
	default:
		DBG("i2c%d slave address %x transmission failed, error code %d", id, addr, code);
		break;
	}
}

int luat_i2c_send(int id, int addr, void* buff, size_t len, uint8_t stop) {
	if (!luat_i2c_exist(id)) return -1;
	int result = I2C_BlockWrite(id, addr, (const uint8_t *)buff, len, luat_i2c_global_timeout, NULL, NULL);
	if (result)
	{
		i2c_failed(id, addr, result);
	}
	return result;
}

int luat_i2c_recv(int id, int addr, void* buff, size_t len) {
	if (!luat_i2c_exist(id)) return -1;
	int result = I2C_BlockRead(id, addr, 0, 0, (uint8_t *)buff, len, luat_i2c_global_timeout, NULL, NULL);
	if (result)
	{
		i2c_failed(id, addr, result);
	}
	return result;
}

int luat_i2c_transfer(int id, int addr, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len) {
	if (!luat_i2c_exist(id)) return -1;
	int result;
	if (reg && reg_len) {
		result = I2C_BlockRead(id, addr, reg, reg_len, (uint8_t *)buff, len, luat_i2c_global_timeout, NULL, NULL);
	} else {
		result = I2C_BlockWrite(id, addr, (const uint8_t *)buff, len, luat_i2c_global_timeout, NULL, NULL);
	}
	if (result)
	{
		i2c_failed(id, addr, result);
	}
	return result;
}
extern void I2C_SetNoBlock(uint8_t I2CID);
int luat_i2c_no_block_transfer(int id, int addr, uint8_t is_read, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len, uint16_t Toms, void *CB, void *pParam) {
	if (!luat_i2c_exist(id)) return -1;
	int32_t Result;
	if (!I2C_WaitResult(id, &Result)) {
		return -1;
	}
	if (Result)
	{
		GPR_swResetModule(&g_i2cResetVectors[id]);
	}
	I2C_Prepare(id, addr, 2, CB, pParam);
#ifdef __LUATOS__
	luat_i2c_set_polling_mode(id, 0);
#endif
	I2C_SetNoBlock(id);
	if (reg && reg_len)
	{
		I2C_MasterXfer(id, I2C_OP_READ_REG, reg, reg_len, buff, len, Toms);
	}
	else if (is_read)
	{
		I2C_MasterXfer(id, I2C_OP_READ, NULL, 0, buff, len, Toms);
	}
	else
	{
		I2C_MasterXfer(id, I2C_OP_WRITE, NULL, 0, buff, len, Toms);
	}
#ifdef __LUATOS__
	luat_i2c_set_polling_mode(id, 0);
#endif
	return 0;
}


#endif
