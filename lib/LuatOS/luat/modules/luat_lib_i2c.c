
/*@Modules  i2c
@summary I2C操作
@version 1.0
@date    2020.03.30
@demo i2c
@tag LUAT_USE_I2C*/
#include "luat_base.h"
#include "luat_log.h"
#include "luat_timer.h"
#include "luat_mem.h"
#include "luat_i2c.h"
#include "luat_gpio.h"
#include "luat_zbuff.h"
#include "luat_irq.h"
#define LUAT_LOG_TAG "i2c"
#include "luat_log.h"


void i2c_soft_start(luat_ei2c_t *ei2c)
{
    luat_gpio_mode(ei2c->sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->sda, Luat_GPIO_LOW);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
    luat_timer_us_delay(ei2c->udelay);
}
static void i2c_soft_stop(luat_ei2c_t *ei2c)
{
    luat_gpio_mode(ei2c->sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
    luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->sda, Luat_GPIO_LOW);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->sda, Luat_GPIO_HIGH);
    luat_timer_us_delay(ei2c->udelay);
}
static unsigned char i2c_soft_wait_ack(luat_ei2c_t *ei2c)
{
    luat_gpio_set(ei2c->sda, Luat_GPIO_HIGH);
    luat_gpio_mode(ei2c->sda, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 1);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(ei2c->udelay * 3);
    int max_wait_time = 3000;
    while (max_wait_time--)
    {
        if (luat_gpio_get(ei2c->sda) == Luat_GPIO_LOW)
        {
            luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
            return 1;
        }
        luat_timer_us_delay(1);
    }
    i2c_soft_stop(ei2c);
    return 0;
}
static void i2c_soft_send_ack(luat_ei2c_t *ei2c)
{
    luat_gpio_mode(ei2c->sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 0);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
}
static void i2c_soft_send_noack(luat_ei2c_t *ei2c)
{
    luat_gpio_mode(ei2c->sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(ei2c->udelay);
    luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
}
static void i2c_soft_send_byte(luat_ei2c_t *ei2c, unsigned char data)
{
    unsigned char i = 8;
    luat_gpio_mode(ei2c->sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 0);
    while (i--)
    {
        luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
        luat_timer_us_delay(ei2c->udelay * 2);
        if (data & 0x80)
        {
            luat_gpio_set(ei2c->sda, Luat_GPIO_HIGH);
        }
        else
        {
            luat_gpio_set(ei2c->sda, Luat_GPIO_LOW);
        }
        luat_timer_us_delay(ei2c->udelay);
        data <<= 1;
        luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
        luat_timer_us_delay(ei2c->udelay);
        luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
        luat_timer_us_delay(ei2c->udelay);
    }
}
static char i2c_soft_recv_byte(luat_ei2c_t *ei2c)
{
    unsigned char i = 8;
    unsigned char data = 0;
    luat_gpio_set(ei2c->sda, Luat_GPIO_HIGH);
    luat_gpio_mode(ei2c->sda, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 1);
    while (i--)
    {
        data <<= 1;
        luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
        luat_timer_us_delay(ei2c->udelay);
        luat_gpio_set(ei2c->scl, Luat_GPIO_HIGH);
        luat_timer_us_delay(ei2c->udelay);
        if (luat_gpio_get(ei2c->sda))
            data |= 0x01;
    }
    luat_gpio_set(ei2c->scl, Luat_GPIO_LOW);
    return (data);
}
char i2c_soft_recv(luat_ei2c_t *ei2c, unsigned char addr, char *buff, size_t len)
{
    size_t i;
    i2c_soft_start(ei2c);
    i2c_soft_send_byte(ei2c, (addr << 1) + 1);
    if (!i2c_soft_wait_ack(ei2c))
    {
        i2c_soft_stop(ei2c);
        return -1;
    }
    luat_timer_us_delay(ei2c->udelay * 10);
    for (i = 0; i < len; i++)
    {
        *buff++ = i2c_soft_recv_byte(ei2c);
        if (i < (len - 1))
            i2c_soft_send_ack(ei2c);
    }
    i2c_soft_send_noack(ei2c);
    i2c_soft_stop(ei2c);
    return 0;
}
char i2c_soft_send(luat_ei2c_t *ei2c, unsigned char addr, char *data, size_t len, uint8_t stop)
{
    size_t i;
    i2c_soft_start(ei2c);
    i2c_soft_send_byte(ei2c, addr << 1);
    if (!i2c_soft_wait_ack(ei2c))
    {
        i2c_soft_stop(ei2c);
        return -1;
    }
    for (i = 0; i < len; i++)
    {
        i2c_soft_send_byte(ei2c, data[i]);
        if (!i2c_soft_wait_ack(ei2c))
        {
            i2c_soft_stop(ei2c);
            return -1;
        }
    }
    if (stop){
        i2c_soft_stop(ei2c);
    }
    return 0;
}

/*Does the i2c number exist?
@api i2c.exist(id)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@return bool returns true if it exists, otherwise returns false
@usage
-- Check if i2c1 exists
if i2c.exist(1) then
    log.info("i2c1 exists")
end*/
static int l_i2c_exist(lua_State *L)
{
    int re = luat_i2c_exist(luaL_checkinteger(L, 1));
    lua_pushboolean(L, re == 0 ? 0 : 1);
    return 1;
}

LUAT_WEAK int luat_i2c_set_polling_mode(int id, uint8_t on_off){
    return -1;
}

/*i2c initialization
@api i2c.setup(id, speed, pullup)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int I2C speed, e.g. i2c.FAST
@bool Whether software pull-up is enabled. It is not enabled by default and requires hardware support.
@return int Returns 1 if successful, otherwise returns 0
@usage
--Initialize i2c1
i2c.setup(1, i2c.FAST) -- If the id is correct, it will be successful.
-- To determine whether the i2c id is legal, please use the i2c.exist function*/
static int l_i2c_setup(lua_State *L)
{
    if (lua_isuserdata(L, 1)) {
        LLOGD("Software i2c does not need to execute i2c.setup anymore");
        lua_pushinteger(L, 1);
        return 1;
    }
    int re = luat_i2c_setup(luaL_checkinteger(L, 1), luaL_optinteger(L, 2, 0));
    if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
        luat_i2c_set_polling_mode(luaL_checkinteger(L, 1), lua_toboolean(L, 3));
    }
    lua_pushinteger(L, re == 0 ? 1 : 0);
    return 1;
}

/*Create a new software i2c object
@api i2c.createSoft(scl,sda,delay)
@int i2c SCL pin number (GPIO number)
@int i2c SDA pin number (GPIO number)
@int Delay of each operation, unit us, default 5
@return software I2C object can be used as i2c id
@usage
-- Notice! This interface is a software simulation of i2c, and the speed may be slower than that of the hardware.
-- No need to call i2c.close interface
-- Initialization software i2c
local softI2C = i2c.createSoft(1, 2, 5)
i2c.send(softI2C, 0x5C, string.char(0x0F, 0x2F))
-- Note that the third parameter is the delay added on 2023.06.19
-- By adjusting the value of the delay parameter, the speed of I2C can be increased or decreased*/
static int l_i2c_soft(lua_State *L)
{
    luat_ei2c_t *ei2c = (luat_ei2c_t *)lua_newuserdata(L, sizeof(luat_ei2c_t));
    ei2c->scl = luaL_checkinteger(L, 1);
    ei2c->sda = luaL_checkinteger(L, 2);
    if (lua_isinteger(L, 3)) {
        ei2c->udelay = luaL_checkinteger(L, 3);
        if (ei2c->udelay < 1)
            ei2c->udelay = 1;
        }
    else {
        ei2c->udelay = 5;
    }
    luat_gpio_mode(ei2c->scl, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
    luat_gpio_mode(ei2c->sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
    i2c_soft_stop(ei2c);
    luaL_setmetatable(L, LUAT_EI2C_TYPE);
    return 1;
}

/*i2c send data
@api i2c.send(id, addr, data,stop)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int Address of I2C subdevice, 7-bit address
@integer/string/table Data to be sent, adaptive parameter type
@integer optional parameter whether to send stop bit 1 to send 0 not to send default to send (105 is not supported)
@return true/false whether the sending is successful
@usage
--Send 1 byte of data to i2c0
i2c.send(0, 0x68, 0x75)
--Send 2 bytes of data to i2c1
i2c.send(1, 0x5C, string.char(0x0F, 0x2F))
i2c.send(1, 0x5C, {0x0F, 0x2F})*/
static int l_i2c_send(lua_State *L)
{
    int id = 0;
    if (!lua_isuserdata(L, 1))
    {
        id = luaL_checkinteger(L, 1);
    }
    int addr = luaL_checkinteger(L, 2);
    size_t len = 0;
    int result = 0;

    int stop = luaL_optnumber(L, 4 , 1);
    if (lua_isinteger(L, 3))
    {
        char buff = (char)luaL_checkinteger(L, 3);
        if (lua_isuserdata(L, 1))
        {
            luat_ei2c_t *ei2c = toei2c(L);
            result = i2c_soft_send(ei2c, addr, &buff, 1,stop);
        }
        else
        {
            result = luat_i2c_send(id, addr, &buff, 1,stop);
        }
        // luat_heap_free(buff);
    }
    else if (lua_isstring(L, 3))
    {
        const char *buff = luaL_checklstring(L, 3, &len);
        if (lua_isuserdata(L, 1))
        {
            luat_ei2c_t *ei2c = toei2c(L);
            result = i2c_soft_send(ei2c, addr, (char *)buff, len,stop);
        }
        else
        {
            result = luat_i2c_send(id, addr, (char *)buff, len,stop);
        }
    }
    else if (lua_istable(L, 3))
    {
        const int len = lua_rawlen(L, 3); //Return the length of the array
        char *buff = (char *)luat_heap_malloc(len);

        for (size_t i = 0; i < len; i++)
        {
            lua_rawgeti(L, 3, 1 + i);
            buff[i] = (char)lua_tointeger(L, -1);
            lua_pop(L, 1); //Pop the element value just obtained from the stack
        }
        if (lua_isuserdata(L, 1))
        {
            luat_ei2c_t *ei2c = toei2c(L);
            result = i2c_soft_send(ei2c, addr, buff, len,stop);
        }
        else
        {
            result = luat_i2c_send(id, addr, buff, len,stop);
        }
        luat_heap_free(buff);
    }
    else
    {
        if (lua_isuserdata(L, 1))
        {
            luat_ei2c_t *ei2c = toei2c(L);
            result = i2c_soft_send(ei2c, addr, NULL, 0,stop);
        }
        else
        {
            result = luat_i2c_send(id, addr, NULL, 0,stop);
        }
    }
    lua_pushboolean(L, result == 0);
    return 1;
}

/*i2c receive data
@api i2c.recv(id, addr, len)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int Address of I2C subdevice, 7-bit address
@int length of received data
@return string received data
@usage
-- Read 2 bytes of data from i2c1
local data = i2c.recv(1, 0x5C, 2)*/
static int l_i2c_recv(lua_State *L)
{
    int id = 0;
    if (!lua_isuserdata(L, 1))
    {
        id = luaL_checkinteger(L, 1);
    }
    int addr = luaL_checkinteger(L, 2);
    int len = luaL_checkinteger(L, 3);
    char *buff = (char *)luat_heap_malloc(len);
    int result;
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);
        result = i2c_soft_recv(ei2c, addr, buff, len);
    }
    else
    {
        result = luat_i2c_recv(id, addr, buff, len);
    }
    if (result != 0)
    { //If the return value is not 0, it means the collection failed
        len = 0;
        LLOGD("i2c receive result %d", result);
    }
    lua_pushlstring(L, buff, len);
    luat_heap_free(buff);
    return 1;
}

/*i2c write register data
@api i2c.writeReg(id, addr, reg, data,stop)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int Address of I2C subdevice, 7-bit address
@int register address
@string data to be sent
@integer optional parameter whether to send stop bit 1 to send 0 not to send default to send (105 is not supported)
@return true/false whether the sending is successful
@usage
-- Write 2 bytes of data from register 0x01 of the device with i2c1 address 0x5C
i2c.writeReg(1, 0x5C, 0x01, string.char(0x00, 0xF2))*/
static int l_i2c_write_reg(lua_State *L)
{
    int id = 0;
    if (!lua_isuserdata(L, 1))
    {
        id = luaL_checkinteger(L, 1);
    }
    int addr = luaL_checkinteger(L, 2);
    int reg = luaL_checkinteger(L, 3);
    size_t len;
    const char *lb = luaL_checklstring(L, 4, &len);
    int stop = luaL_optnumber(L, 5 , 1);
    char *buff = (char *)luat_heap_malloc(sizeof(char) * len + 1);
    *buff = (char)reg;
    memcpy(buff + 1, lb, sizeof(char) + len + 1);
    int result;
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);
        result = i2c_soft_send(ei2c, addr, buff, len + 1,stop);
    }
    else
    {
        result = luat_i2c_send(id, addr, buff, len + 1,stop);
    }
    luat_heap_free(buff);
    lua_pushboolean(L, result == 0);
    return 1;
}

/*i2c read register data
@api i2c.readReg(id, addr, reg, len)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int Address of I2C subdevice, 7-bit address
@int register address
@int length of data to be received
@integer optional parameter whether to send stop bit 1 to send 0 not to send default to send (105 is not supported)
@return string received data
@usage
-- Read 2 bytes of data from register 0x01 of the device with i2c1 address 0x5C
i2c.readReg(1, 0x5C, 0x01, 2)*/
static int l_i2c_read_reg(lua_State *L)
{
    int id = 0;
    if (!lua_isuserdata(L, 1))
    {
        id = luaL_checkinteger(L, 1);
    }
    int addr = luaL_checkinteger(L, 2);
    int reg = luaL_checkinteger(L, 3);
    int len = luaL_checkinteger(L, 4);
    int stop = luaL_optnumber(L, 5 , 0);
    char temp = (char)reg;
    int result;
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);
        result = i2c_soft_send(ei2c, addr, &temp, 1,stop);
    }
    else
    {
        result = luat_i2c_send(id, addr, &temp, 1,stop);
    }
    if (result != 0)
    { //If the return value is not 0, it means the collection failed
        LLOGD("i2c send result %d", result);
        lua_pushlstring(L, NULL, 0);
        return 1;
    }
    char *buff = (char *)luat_heap_malloc(sizeof(char) * len);
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);
        result = i2c_soft_recv(ei2c, addr, buff, len);
    }
    else
    {
        result = luat_i2c_recv(id, addr, buff, len);
    }
    if (result != 0)
    { //If the return value is not 0, it means the collection failed
        len = 0;
        LLOGD("i2c receive result %d", result);
    }
    lua_pushlstring(L, buff, len);
    luat_heap_free(buff);
    return 1;
}

/*Close i2c device
@api i2c.close(id)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@return nil no return value
@usage
-- Close i2c1
i2c.close(1)*/
static int l_i2c_close(lua_State *L)
{
    int id = luaL_checkinteger(L, 1);
    luat_i2c_close(id);
    return 0;
}

/*Read DHT12 temperature and humidity data from i2c bus
@api i2c.readDHT12(id)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int DHT12 device address, default 0x5C
@return boolean returns true if the reading is successful, otherwise returns false
@return int Humidity value, unit 0.1%, for example, 591 represents 59.1%
@return int temperature value, unit 0.1 degrees Celsius, for example, 292 represents 29.2 degrees Celsius
@usage
-- Read DHT12 from i2c0
i2c.setup(0)
local re, H, T = i2c.readDHT12(0)
if re then
    log.info("dht12", H, T)
end*/
static int l_i2c_readDHT12(lua_State *L)
{
    int id = 0;
    if (!lua_isuserdata(L, 1))
    {
        id = luaL_checkinteger(L, 1);
    }
    int addr = luaL_optinteger(L, 2, 0x5C);
    char buff[5] = {0};
    char temp = 0x00;
    int result = -1;
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);
        result = i2c_soft_send(ei2c, addr, &temp, 1,1);
    }
    else
    {
        result = luat_i2c_send(id, addr, &temp, 1,1);
    }
    if (result != 0)
    {
        LLOGD("DHT12 i2c bus write fail");
        lua_pushboolean(L, 0);
        return 1;
    }
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);
        result = i2c_soft_recv(ei2c, addr, buff, 5);
    }
    else
    {
        result = luat_i2c_recv(id, addr, buff, 5);
    }
    if (result != 0)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    else
    {
        if (buff[0] == 0 && buff[1] == 0 && buff[2] == 0 && buff[3] == 0 && buff[4] == 0)
        {
            LLOGD("DHT12 DATA emtry");
            lua_pushboolean(L, 0);
            return 1;
        }
        // Check crc value
        LLOGD("DHT12 DATA %02X%02X%02X%02X%02X", buff[0], buff[1], buff[2], buff[3], buff[4]);
        uint8_t crc_act = (uint8_t)buff[0] + (uint8_t)buff[1] + (uint8_t)buff[2] + (uint8_t)buff[3];
        uint8_t crc_exp = (uint8_t)buff[4];
        if (crc_act != crc_exp)
        {
            LLOGD("DATA12 DATA crc not ok");
            lua_pushboolean(L, 0);
            return 1;
        }
        lua_pushboolean(L, 1);
        lua_pushinteger(L, (uint8_t)buff[0] * 10 + (uint8_t)buff[1]);
        if (((uint8_t)buff[2]) > 127)
            lua_pushinteger(L, ((uint8_t)buff[2] - 128) * -10 + (uint8_t)buff[3]);
        else
            lua_pushinteger(L, (uint8_t)buff[2] * 10 + (uint8_t)buff[3]);
        return 3;
    }
}

/*Read DHT30 temperature and humidity data from i2c bus (contributed by "curious star")
@api i2c.readSHT30(id,addr)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int device addr, device address of SHT30, default 0x44 bit7
@return boolean returns true if the reading is successful, otherwise returns false
@return int Humidity value, unit 0.1%, for example, 591 represents 59.1%
@return int temperature value, unit 0.1 degrees Celsius, for example, 292 represents 29.2 degrees Celsius
@usage
-- Read SHT30 from i2c0
i2c.setup(0)
local re, H, T = i2c.readSHT30(0)
if re then
    log.info("sht30", H, T)
end*/
static int l_i2c_readSHT30(lua_State *L)
{
    char buff[7] = {0x2c, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00};
    float temp = 0x00;
    float hum = 0x00;

    int result = -1;
    int addr = luaL_optinteger(L, 2, 0x44);
    if (lua_isuserdata(L, 1))
    {
        luat_ei2c_t *ei2c = toei2c(L);

        i2c_soft_send(ei2c, addr, buff, 2,1);
        luat_timer_mdelay(13);

        result = i2c_soft_recv(ei2c, addr, buff, 6);
    }
    else
    {
        int id = luaL_optinteger(L, 1, 0);
        luat_i2c_send(id, addr, buff, 2, 1);
        luat_timer_mdelay(1);
        result = luat_i2c_recv(id, addr, buff, 6);
    }

    if (result != 0)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    else
    {
        if (buff[0] == 0 && buff[1] == 0 && buff[2] == 0 && buff[3] == 0 && buff[4] == 0)
        {
            LLOGD("SHT30 DATA emtry");
            lua_pushboolean(L, 0);
            return 1;
        }
        // Check crc value
        // LLOGD("SHT30 DATA %02X %02X %02X %02X %02X %02X", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);

        temp = ((((buff[0] * 256) + buff[1]) * 175) / 6553.5) - 450;
        hum = ((((buff[3] * 256) + buff[4]) * 100) / 6553.5);

        // LLOGD("\r\n SHT30  %d deg  %d Hum ", temp, hum);
        // Skip CRC

        // uint8_t crc_act = (uint8_t)buff[0] + (uint8_t)buff[1] + (uint8_t)buff[2] + (uint8_t)buff [3];
        // uint8_t crc_exp = (uint8_t)buff[4];
        // if (crc_act != crc_exp) {
        //     LLOGD("DATA12 DATA crc not ok");
        //     lua_pushboolean(L, 0);
        //     return 1;
        // }

        // Convert the data
        lua_pushboolean(L, 1);
        lua_pushinteger(L, (int)hum);
        lua_pushinteger(L, (int)temp);

        return 3;
        //Fahrenheit
        // fTemp = (cTemp * 1.8) + 32;
    }
}

#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK int luat_i2c_transfer(int id, int addr, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len)
{
    int result;
    result = luat_i2c_send(id, addr, reg, reg_len, 0);
    if (result != 0) return-1;
    return luat_i2c_recv(id, addr, buff, len);
}

LUAT_WEAK int luat_i2c_no_block_transfer(int id, int addr, uint8_t is_read, uint8_t *reg, size_t reg_len, uint8_t *buff, size_t len, uint16_t Toms, void *CB, void *pParam)
{
    return -1;
}
#endif

/**
i2c universal transmission includes three functions: sending N bytes, sending N bytes + receiving N bytes, and receiving N bytes. It sends a reStart signal during the sending and receiving process to solve the problem that mlx90614 must have a restart signal, but it cannot be used. Controlled by i2c.send, such as air105
@api i2c.transfer(id, addr, txBuff, rxBuff, rxLen)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int Address of I2C subdevice, 7-bit address
@integer/string/zbuff The data to be sent, adaptive parameter type. If it is nil, no data will be sent.
@zbuff The zbuff of the data to be received. If zbuff is not used, the received data will be returned in return
@int The length of data to be received. If it is 0 or nil, no data will be received.
@return boolean true/false whether the sending was successful
@return string or nil If parameter 5 is interger, return the received data
@usage
local result, _ = i2c.transfer(0, 0x11, txbuff, rxbuff, 1)
local result, _ = i2c.transfer(0, 0x11, txbuff, nil, 0) --Only send the data in txbuff, do not receive data. The typical application is to write the register, where the register address and value are placed in txbuff.
local result, _ = i2c.transfer(0, 0x11, "\x01\x02\x03", nil, 1) --Send 0x01, 0x02, 0x03, do not receive data, if it is eeprom, write 02 to the address of 0x01 and 03, or write 03 to the address of 0x0102, it depends on the specific chip.
local result, rxdata = i2c.transfer(0, 0x11, "\x01\x02", nil, 1) --Send 0x01, 0x02, and then receive 1 byte. The typical application is eeprom
local result, rxdata = i2c.transfer(0, 0x11, 0x00, nil, 1) --send 0x00, then receive 1 byte, typical applications for various sensors*/
static int l_i2c_transfer(lua_State *L)
{
	int addr = luaL_checkinteger(L, 2);
	size_t tx_len = 0;
	size_t rx_len = 0;
	int result = -1;
	uint8_t temp[1];
	uint8_t *tx_buff = NULL;
	uint8_t *rx_buff = NULL;
	uint8_t tx_heap_flag = 0;
	if (lua_isnil(L, 3)) {
		tx_len = 0;
	}
	else if (lua_isinteger(L, 3)) {
		temp[0] = luaL_checkinteger(L, 3);
		tx_buff = temp;
		tx_len = 1;
	}
	else if (lua_isstring(L, 3)) {
		tx_buff = (uint8_t*)luaL_checklstring(L, 3, &tx_len);
	}
    else if (lua_istable(L, 3)) {
        const int tx_len = lua_rawlen(L, 3); //Return the length of the array
        tx_buff = (uint8_t *)luat_heap_malloc(tx_len);
        tx_heap_flag = 1;
        for (size_t i = 0; i < tx_len; i++)
        {
            lua_rawgeti(L, 3, 1 + i);
            tx_buff[i] = (char)lua_tointeger(L, -1);
            lua_pop(L, 1); //Pop the element value just obtained from the stack
        }
    }
	else {
		luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE));
		tx_buff = buff->addr;
		tx_len = buff->used;
	}
	luat_zbuff_t *rbuff = ((luat_zbuff_t *)luaL_testudata(L, 4, LUAT_ZBUFF_TYPE));
	if (lua_isnil(L, 5)) {
		rx_len = 0;
	}
	else if (lua_isinteger(L, 5)) {
		rx_len = luaL_checkinteger(L, 5);
		if (rx_len) {
			if (!rbuff) {
				rx_buff = luat_heap_malloc(rx_len);
			}
			else {
				if ((rbuff->used + rx_len) > rbuff->len) {
					__zbuff_resize(rbuff, rbuff->len + rx_len);
				}
				rx_buff = rbuff->addr + rbuff->used;
			}
		}
	}

	int id = 0;
	if (!lua_isuserdata(L, 1)) {
		id = luaL_checkinteger(L, 1);
		if (rx_buff && rx_len) {
			result = luat_i2c_transfer(id, addr, tx_buff, tx_len, rx_buff, rx_len);
		} else {
			result = luat_i2c_transfer(id, addr, NULL, 0, tx_buff, tx_len);
		}
	}else if (lua_isuserdata(L, 1)){
        luat_ei2c_t *ei2c = toei2c(L);
		if (tx_buff && tx_len) {
            result = i2c_soft_send(ei2c, addr, (char*)tx_buff, tx_len,0);
		} 
        result = i2c_soft_recv(ei2c, addr, (char*)rx_buff, rx_len);
    }
	if (tx_heap_flag) {
		luat_heap_free(tx_buff);
	}
	lua_pushboolean(L, !result);
	if (rx_buff && rx_len) {
		if (rbuff) {
			rbuff->used += rx_len;
			lua_pushnil(L);
		} else {
            lua_pushlstring(L, (const char *)rx_buff, rx_len);
            luat_heap_free(rx_buff);
		}
	} else {
		lua_pushnil(L);
	}
	return 2;

}

/**
i2c non-blocking universal transmission, similar to transfer, but will not wait until the I2C transfer is completed before returning. Calling this function will return immediately. After the I2C transfer is completed, a message callback
@api i2c.xfer(id, addr, txBuff, rxBuff, rxLen, transfer_done_topic, timeout)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int Address of I2C subdevice, 7-bit address
@zbuff The data to be sent, due to the non-blocking model, can only use zbuff to ensure the validity of dynamic data. The data sent starts from zbuff.addr and has a length of zbuff.used
@zbuff The zbuff of the data to be received. If it is nil, the following parameters will be ignored and no data will be received. The received data will be placed at the beginning of zbuff.addr and will overwrite the previous data. Please note that the reserved space for zhuff must be sufficient.
@int The length of data to be received. If it is 0 or nil, no data will be received.
@string The callback message after the transfer is completed
@int timeout time, if filled in nil, it is 100ms
@return boolean true/false Whether this transmission is started correctly, true, it is started, false, there is an error and it cannot be started. When the transfer is completed, the message transfer_done_topic and boolean result will be released.
@usage
local result = i2c.xfer(0, 0x11, txbuff, rxbuff, 1, "I2CDONE") if result then result, i2c_id, succ, error_code = sys.waitUntil("I2CDONE") end if not result or not succ then log. info("i2c fail, error code", error_code) else log.info("i2c ok") end*/
static int l_i2c_no_block_transfer(lua_State *L)
{
	size_t topic_len = 0;
	const char *topic = luaL_checklstring(L, 6, &topic_len);
	int id = luaL_checkinteger(L, 1);
	int addr = luaL_checkinteger(L, 2);


	size_t tx_len = 0;
	size_t rx_len = 0;
	int result = -1;
	uint8_t *tx_buff = NULL;
	uint8_t *rx_buff = NULL;

	luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_testudata(L, 3, LUAT_ZBUFF_TYPE));
	if (buff) {
		tx_buff = buff->addr;
		tx_len = buff->used;
	}

	luat_zbuff_t *rbuff = ((luat_zbuff_t *)luaL_testudata(L, 4, LUAT_ZBUFF_TYPE));
	if (lua_isinteger(L, 5) && rbuff) {
		rx_len = luaL_checkinteger(L, 5);
		rx_buff = rbuff->addr;
		rbuff->used = rx_len;
	}

	uint32_t timeout = luaL_optinteger(L, 7, 100);

	char *cb_topic = luat_heap_malloc(topic_len + 1);
	memcpy(cb_topic, topic, topic_len);
	cb_topic[topic_len] = 0;
	if (rx_buff && rx_len) {
		result = luat_i2c_no_block_transfer(id, addr, 1, tx_buff, tx_len, rx_buff, rx_len, timeout, luat_irq_hardware_cb_handler, cb_topic);
	} else {
		result = luat_i2c_no_block_transfer(id, addr, 0, NULL, 0, tx_buff, tx_len, timeout, luat_irq_hardware_cb_handler, cb_topic);
	}
	if (result) {
		luat_heap_free(cb_topic);
	}

	lua_pushboolean(L, !result);
	return 1;

}


/**
Scan i2c devices
@api i2c.scan(id,speed)
@int device id, for example, the id of i2c1 is 1, the id of i2c2 is 2
@int speed, optional i2c.SLOW i2c.FAST i2c.PLUS i2c.HSMODE defaults to i2c.SLOW, modify this if not detected
@return nil currently has no return value
@usage
-- This function was added on 2023.07.04
-- The main goal of this function is to scan i2c devices during development
-- Some BSPs will output logs when the specified addr does not respond, causing the output to be disrupted.
i2c.scan()*/
#if defined(LUA_USE_WINDOWS) || defined(LUA_USE_LINUX) || defined(LUA_USE_MACOS)
static int l_i2c_scan(lua_State *L) {
    return 0;
}
#else
#include "i2c_utils.h"
static int l_i2c_scan(lua_State *L) {
    int id = luaL_optinteger(L, 1, 0);
    i2c_init(id,luaL_optinteger(L, 2, 0));
    i2c_scan();
    return 0;
}
#endif

#include "rotable2.h"
static const rotable_Reg_t reg_i2c[] =
{
    { "exist",      ROREG_FUNC(l_i2c_exist)},
    { "setup",      ROREG_FUNC(l_i2c_setup)},
    { "createSoft", ROREG_FUNC(l_i2c_soft)},
#ifdef __F1C100S__
#else
    { "send",       ROREG_FUNC(l_i2c_send)},
    { "recv",       ROREG_FUNC(l_i2c_recv)},

#endif
	{ "transfer",	ROREG_FUNC(l_i2c_transfer)},
    { "writeReg",   ROREG_FUNC(l_i2c_write_reg)},
    { "readReg",    ROREG_FUNC(l_i2c_read_reg)},
    { "close",      ROREG_FUNC(l_i2c_close)},

    { "readDHT12",  ROREG_FUNC(l_i2c_readDHT12)},
    { "readSHT30",  ROREG_FUNC(l_i2c_readSHT30)},

	{ "xfer",	    ROREG_FUNC(l_i2c_no_block_transfer)},

    
	{ "scan",	    ROREG_FUNC(l_i2c_scan)},
	{ "HSMODE",     ROREG_INT(3)},
	{ "PLUS",       ROREG_INT(2)},
    //@const FAST number high speed
    { "FAST",       ROREG_INT(1)},
    //@const SLOW number Slow speed
    { "SLOW",       ROREG_INT(0)},
	{ NULL,         ROREG_INT(0) }
};

LUAMOD_API int luaopen_i2c(lua_State *L)
{
    luat_newlib2(L, reg_i2c);
    luaL_newmetatable(L, LUAT_EI2C_TYPE);
    lua_pop(L, 1);
    return 1;
}