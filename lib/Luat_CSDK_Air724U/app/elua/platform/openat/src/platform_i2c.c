
#include <string.h>

#include "lplatform.h"
#include "platform_conf.h"
#include "am_openat.h"

static const E_AMOPENAT_I2C_PORT i2cIdMap[OPENAT_I2C_QTY] =
{
    OPENAT_I2C_1, //id = 0
    OPENAT_I2C_2, //id = 1
    OPENAT_I2C_3,
/*+\BUG3555\zhuwangbin\2020.11.11\ÐÞ¸ÄI2C1ºÍI2C3²»ÄÜÓÃµÄÎÊÌâ,id 1,2,3¶ÔÓ¦I2C1,2,3*/
	OPENAT_I2C_QTY
/*-\BUG3555555555555555555555555555555555555555555555555555555555555555555555555555555555555555.1jâÐç1º2C1º2C1º2C3¼¼,id 1.2.3´gI2C1,3,3,3.*/
};

static u16 i2cSlaveAddr[OPENAT_I2C_QTY];

static u16 gpioi2cSlaveAddr[OPENAT_I2C_QTY];

/*-\ENEW\zhuwangbin\2016.4.4.6\¼æìèÝ»ÍÍì³»æ±æ±±µüµÄg_sensor, ÌnµÓµÉµÉÉÉÉÉÉÉóµûûûûûûóûûóûûô values.*/
/*A13	00111101b	01000100b£¨SLI3108£©
A14/A15	00111011b	01000100b£¨SLI3108£©
A14/A15	00111011b	00111101b£¨PA22401001)*/


#define PLATFORM_G_SENSOR_SLI3108_ID 0X21
#define PLATFORM_G_SENSOR_SLI3108_REG 0X00
#define PLATFORM_G_SENSOR_LIS2DS12_ID 0X43
#define PLATFORM_G_SENSOR_LIS2DS12_ID_REG 0X0F
#define PLATFORM_A13_SLAVE_ADDR 0X1E
#define PLATFORM_A14_SLAVE_ADDR 0X1D
#define PLATFORM_A15_SLAVE_ADDR PLATFORM_A14_SLAVE_ADDR

void platform_i2c_gSensorParam_get(u8 id, u8 *slave_addr, u8 *slave_id)
{
  u8 pRegAddr;

  /*° Æ ± ¾a13 Ðnºålis2ds12*/
  pRegAddr = PLATFORM_G_SENSOR_LIS2DS12_ID_REG;
  *slave_addr = PLATFORM_A13_SLAVE_ADDR;
  IVTBL(read_i2c)(i2cIdMap[id], *slave_addr, &pRegAddr, slave_id, 1);

  if (*slave_id == PLATFORM_G_SENSOR_LIS2DS12_ID)
  {
    OPENAT_print("platform_i2c_gSensorParam_get %x, %x", *slave_id, *slave_addr);
    return;
  }
  
  /*° Æ ± ¾a14 Ðnºålis2ds12*/
  pRegAddr = PLATFORM_G_SENSOR_LIS2DS12_ID_REG;
  *slave_addr = PLATFORM_A14_SLAVE_ADDR;
  IVTBL(read_i2c)(i2cIdMap[id], *slave_addr, &pRegAddr, slave_id, 1);

  if (*slave_id == PLATFORM_G_SENSOR_LIS2DS12_ID)
  {
    OPENAT_print("platform_i2c_gSensorParam_get %x, %x", *slave_id, *slave_addr);
    return;
  }

  /*° Æ ± ¾A15 Ðnºåsli3108*/
  
  pRegAddr = PLATFORM_G_SENSOR_SLI3108_REG;
  *slave_addr = PLATFORM_A15_SLAVE_ADDR;
  IVTBL(read_i2c)(i2cIdMap[id], *slave_addr, &pRegAddr, slave_id, 1);

  if (*slave_id == PLATFORM_G_SENSOR_SLI3108_ID)
  {
    OPENAT_print("platform_i2c_gSensorParam_get %x, %x", *slave_id, *slave_addr);
    return;
  }

  /*° æ ± ¾a15 ðnºåæäëû*/
  *slave_id = 0;
  *slave_addr = PLATFORM_A15_SLAVE_ADDR;
    
  OPENAT_print("platform_i2c_gSensorParam_get %x, %x", *slave_id, *slave_addr);
   
  return;
}
/*-\ENEW\zhuwangbin\2016.4.4.6\¼æìèÝ»ÍÍì³»æ±æ±±µüµÄg_sensor, ÌnµÓµÉµÉÉÉÉÉÉÉóµûûûûûûóûûóûûô values.*/

int platform_i2c_exists( unsigned id ) 
{
/*+\BUG3555\zhuwangbin\2020.11.11\ÐÞ¸ÄI2C1ºÍI2C3²»ÄÜÓÃµÄÎÊÌâ,id 1,2,3¶ÔÓ¦I2C1,2,3*/
    if((id > OPENAT_I2C_QTY) || id == 0) // ½ööz§³öi2c 2
        return PLATFORM_ERR;
/*-\BUG3555555555555555555555555555555555555555555555555555555555555555555555555555555555555555.1jâÐç1º2C1º2C1º2C3¼¼,id 1.2.3´gI2C1,3,3,3.*/
    return PLATFORM_OK;
}

int platform_i2c_setup( unsigned id, PlatformI2CParam *pParam ) 
{
    T_AMOPENAT_I2C_PARAM openatI2CParam;
    BOOL ret;

    memset(&openatI2CParam, 0, sizeof(openatI2CParam));

    openatI2CParam.freq = pParam->speed;
    i2cSlaveAddr[id] = pParam->slaveAddr;
    
    ret = IVTBL(open_i2c)(i2cIdMap[id], &openatI2CParam);
    
    return ret ? pParam->speed : 0;
}
/*+\ New \ Wangjian \ 2019.4.10 \ · â × ° I2c.close½ó¿ú*/
int platform_i2c_close( unsigned id )
{
    return IVTBL(close_i2c)(i2cIdMap[id]);
}
/*-\ New \ Wangjian \ 2019.4.10 \ · â × ° I2c.close½ó¿ú*/

int platform_i2c_send_data( unsigned id, u16 slave_addr, const u8 *pRegAddr, const u8 *buf, u32 len )
{
    //- I è-2 I «« ón · ª Yan · *'li ·
    if(slave_addr == I2C_NULL_SLAVE_ADDR) {
        slave_addr = i2cSlaveAddr[id];
    }

    // Èce´è´ÖµÖµèÖ«¿Õ¿à»´â ´“WExough
    if(slave_addr == I2C_NULL_SLAVE_ADDR){
        return 0;
    }
    
    return IVTBL(write_i2c)(i2cIdMap[id], slave_addr, pRegAddr, buf, len);
}

int platform_i2c_recv_data( unsigned id, u16 slave_addr, const u8 *pRegAddr, u8 *buf, u32 len  )
{
    // - I è-2 I «« ón · ª Yan · *'li ·
    if(slave_addr == I2C_NULL_SLAVE_ADDR) {
        slave_addr = i2cSlaveAddr[id];
    }

    // Èce´è´ÖµÖµèÖ«¿Õ¿à»´â ´“WExough
    if(slave_addr == I2C_NULL_SLAVE_ADDR){
        return 0;
    }
    
    return IVTBL(read_i2c)(i2cIdMap[id], slave_addr, pRegAddr, buf, len);
}


#ifdef LUA_GPIO_I2C
int platform_gpio_i2c_send_data( unsigned id, u16 slave_addr, const u8 *pRegAddr, const u8 *buf, u32 len )
{
    //- I è-2 I «« ón · ª Yan · *'li ·
    if(slave_addr == I2C_NULL_SLAVE_ADDR) {
        slave_addr = gpioi2cSlaveAddr[id];
    }

    // Èce´è´ÖµÖµèÖ«¿Õ¿à»´â ´“WExough
    if(slave_addr == I2C_NULL_SLAVE_ADDR){
        return 0;
    }
    
    return IVTBL(write_gpio_i2c)(i2cIdMap[id], slave_addr, pRegAddr, buf, len);
}

int platform_gpio_i2c_recv_data( unsigned id, u16 slave_addr, const u8 *pRegAddr, u8 *buf, u32 len  )
{
    // - I è-2 I «« ón · ª Yan · *'li ·
    if(slave_addr == I2C_NULL_SLAVE_ADDR) {
        slave_addr = gpioi2cSlaveAddr[id];
    }

    // Èce´è´ÖµÖµèÖ«¿Õ¿à»´â ´“WExough
    if(slave_addr == I2C_NULL_SLAVE_ADDR){
        return 0;
    }
    
    return IVTBL(read_gpio_i2c)(i2cIdMap[id], slave_addr, pRegAddr, buf, len);
}


int platform_gpio_i2c_setup( unsigned id, T_AMOPENAT_GPIO_I2C_PARAM *pParam ) 
{
    BOOL ret;
    gpioi2cSlaveAddr[id] = pParam->slaveAddr;
    
    ret = IVTBL(open_gpio_i2c)(i2cIdMap[id], pParam->sda_port, pParam->scl_port);
    
    return ret ;
}
#endif

