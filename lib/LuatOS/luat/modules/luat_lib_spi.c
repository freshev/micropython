/*@Modules  spi
@summary spi操作库
@version 1.0
@date    2020.04.23
@demo spi
@video https://www.bilibili.com/video/BV1VY411M7YH
@tag LUAT_USE_SPI*/
#include "luat_base.h"
#include "luat_log.h"
#include "luat_sys.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_mem.h"
#include "luat_spi.h"
#include "luat_zbuff.h"
#include "luat_gpio.h"
#include "luat_irq.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "spi"

#define META_SPI "SPI*"

#define LUAT_ESPI_TYPE "ESPI*"
#define toespi(L) ((luat_espi_t *)luaL_checkudata(L, 1, LUAT_ESPI_TYPE))

// software spi
typedef struct luat_espi {
    uint8_t cs;
    uint8_t mosi;
    uint8_t miso;
    uint32_t clk;
    uint8_t CPHA;
    uint8_t CPOL;
    uint8_t dataw;
    uint8_t bit_dict;
    uint8_t master;
    uint8_t mode;
} luat_espi_t;

//Soft SPI sends a byte
static void spi_soft_send_byte(luat_espi_t *espi, uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (data&0x80)
        {
            luat_gpio_set(espi->mosi, Luat_GPIO_HIGH);
        }
        else
        {
            luat_gpio_set(espi->mosi, Luat_GPIO_LOW);
        }
        data<<=1;
        if (espi->CPOL == 0)
        {
            luat_gpio_set(espi->clk, Luat_GPIO_HIGH);
            luat_gpio_set(espi->clk, Luat_GPIO_LOW);
        }
        else
        {
            luat_gpio_set(espi->clk, Luat_GPIO_LOW);
            luat_gpio_set(espi->clk, Luat_GPIO_HIGH);
        }
    }
}

//Soft SPI receives a byte
static char spi_soft_recv_byte(luat_espi_t *espi)
{
    unsigned char i = 8;
    unsigned char data = 0;
    while (i--)
    {
        data <<= 1;
        if (luat_gpio_get(espi->miso))
        {
            data |= 0x01;
        }
        if (espi->CPOL == 0)
        {
            luat_gpio_set(espi->clk, Luat_GPIO_HIGH);
            luat_gpio_set(espi->clk, Luat_GPIO_LOW);
        }else{
            luat_gpio_set(espi->clk, Luat_GPIO_LOW);
            luat_gpio_set(espi->clk, Luat_GPIO_HIGH);
        }
    }
    return data;
}

//Soft SPI sends and receives one byte
static char spi_soft_xfer_byte(luat_espi_t *espi, uint8_t send_data)
{
    unsigned char i = 8;
    unsigned char data = 0;
    while (i--)
    {
        // send
        if (send_data&0x80)
        {
            luat_gpio_set(espi->mosi, Luat_GPIO_HIGH);
        }
        else
        {
            luat_gpio_set(espi->mosi, Luat_GPIO_LOW);
        }
        send_data<<=1;
        // take over
        data <<= 1;
        if (luat_gpio_get(espi->miso))
        {
            data |= 0x01;
        }
        if (espi->CPOL == 0)
        {
            luat_gpio_set(espi->clk, Luat_GPIO_HIGH);
            luat_gpio_set(espi->clk, Luat_GPIO_LOW);
        }else{
            luat_gpio_set(espi->clk, Luat_GPIO_LOW);
            luat_gpio_set(espi->clk, Luat_GPIO_HIGH);
        }
    }
    return data;
}

int luat_spi_soft_send(luat_espi_t *espi, const char*data, size_t len)
{
    size_t i = 0;
    if (espi->cs != Luat_GPIO_MAX_ID)
    {
        luat_gpio_set(espi->cs, Luat_GPIO_LOW);
    }
    for (i = 0; i < len; i++)
    {
        spi_soft_send_byte(espi, data[i]);
    }
    if (espi->cs != Luat_GPIO_MAX_ID)
    {
        luat_gpio_set(espi->cs, Luat_GPIO_HIGH);
    }
    return 0;
}


int luat_spi_soft_recv(luat_espi_t *espi, char *buff, size_t len)
{
    size_t i = 0;
    if (espi->cs != Luat_GPIO_MAX_ID)
    {
        luat_gpio_set(espi->cs, Luat_GPIO_LOW);
    }
    luat_gpio_set(espi->mosi, Luat_GPIO_LOW);
    for (i = 0; i < len; i++)
    {
        *buff++ = spi_soft_recv_byte(espi);
    }
    if (espi->cs != Luat_GPIO_MAX_ID)
    {
        luat_gpio_set(espi->cs, Luat_GPIO_HIGH);
    }
    luat_gpio_set(espi->mosi, Luat_GPIO_HIGH);
    return 0;
}

int luat_spi_soft_xfer(luat_espi_t *espi, const char *send_buff, char* recv_buff, size_t len)
{
    size_t i = 0;
    if (espi->cs != Luat_GPIO_MAX_ID)
    {
        luat_gpio_set(espi->cs, Luat_GPIO_LOW);
    }
    luat_gpio_set(espi->mosi, Luat_GPIO_LOW);
    for (i = 0; i < len; i++)
    {
        *recv_buff++ = spi_soft_xfer_byte(espi, (uint8_t)send_buff[i]);
    }
    if (espi->cs != Luat_GPIO_MAX_ID)
    {
        luat_gpio_set(espi->cs, Luat_GPIO_HIGH);
    }
    luat_gpio_set(espi->mosi, Luat_GPIO_HIGH);
    return 0;
}

/**
Set up and enable SPI
@api spi.setup(id, cs, CPHA, CPOL, dataw, bandrate, bitdict, ms, mode)
@int SPI number, for example 0
@int CS chip selector, if it is not available in w600, please fill in nil
@int CPHA default 0, optional 0/1
@int CPOL default 0, optional 0/1
@int data width, default 8bit
@int baud rate, default 2M=2000000
@int Big and small endian, default spi.MSB, optional spi.LSB
@int Master-slave setting, default master 1, optional slave 0. Usually only host mode is supported
@int working mode, full duplex 1, half duplex 0, default full duplex
@return int Returns 0 successfully, otherwise returns other value
@usage
--Initialize spi
spi.setup(0,20,0,0,8,2000000,spi.MSB,1,1)*/
static int l_spi_setup(lua_State *L) {
    luat_spi_t spi_config = {0};

    spi_config.id = luaL_checkinteger(L, 1);
    spi_config.cs = luaL_optinteger(L, 2, Luat_GPIO_MAX_ID); // None by default
    spi_config.CPHA = luaL_optinteger(L, 3, 0); // CPHA0
    spi_config.CPOL = luaL_optinteger(L, 4, 0); // CPOL0
    spi_config.dataw = luaL_optinteger(L, 5, 8); // 8bit
    spi_config.bandrate = luaL_optinteger(L, 6, 2000000U); // 2000000U
    spi_config.bit_dict = luaL_optinteger(L, 7, 1); // MSB=1, LSB=0
    spi_config.master = luaL_optinteger(L, 8, 1); // master=1,slave=0
    spi_config.mode = luaL_optinteger(L, 9, 1); // FULL=1, half=0

    lua_pushinteger(L, luat_spi_setup(&spi_config));

    return 1;
}

/**
Set up and enable software SPI
@api spi.createSoft(cs, mosi, miso, clk, CPHA, CPOL, dataw, bitdict, ms, mode)
@int cs pin number, passing in nil means Lua controls the cs pin
@int mosi pin number
@int miso pin number
@int clk pin number
@int default 0, optional 0/1
@int default 0, optional 0/1
@int data width, default 8bit
@int Big and small endian, default spi.MSB, optional spi.LSB
@int Master-slave setting, default master 1, optional slave 0. Usually only host mode is supported
@int Working mode, full duplex 1, half duplex 0, default half duplex
@return software SPI object can be used as SPI ID
@usage
-- Initialize software spi
local softSpiDevice = spi.createSoft(0, 1, 2, 3, 0, 0, 8, spi.MSB, 1, 1)
local result = spi.send(softSpiDevice, string.char(0x9f))*/
static int l_spi_soft(lua_State *L) {
    luat_espi_t *espi = (luat_espi_t *)lua_newuserdata(L, sizeof(luat_espi_t));
    espi->cs = luaL_optinteger(L, 1, Luat_GPIO_MAX_ID);
    espi->mosi = luaL_checkinteger(L, 2);
    espi->miso = luaL_checkinteger(L, 3);
    espi->clk = luaL_checkinteger(L, 4);
    espi->CPHA = luaL_optinteger(L, 5, 0);
    espi->CPOL = luaL_optinteger(L, 6, 0);
    espi->dataw = luaL_optinteger(L, 7, 8);
    espi->bit_dict = luaL_optinteger(L, 8, 1);
    espi->master = luaL_optinteger(L, 9, 1);
    espi->mode = luaL_optinteger(L, 10, 0);
    luat_gpio_mode(espi->cs, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_HIGH);
    luat_gpio_mode(espi->mosi, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_HIGH);
    luat_gpio_mode(espi->miso, Luat_GPIO_INPUT, Luat_GPIO_DEFAULT, Luat_GPIO_LOW);
    if (espi->CPOL == 0)
    {
        luat_gpio_mode(espi->clk, Luat_GPIO_OUTPUT, Luat_GPIO_PULLDOWN, Luat_GPIO_LOW);
    }
    else if (espi->CPOL == 1)
    {
        luat_gpio_mode(espi->clk, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_HIGH);
    }
    luaL_setmetatable(L, LUAT_ESPI_TYPE);
    return 1;
}

/**
Close the specified SPI
@api spi.close(id)
@int SPI number, for example 0
@return int Returns 0 successfully, otherwise returns other value
@usage
--Initialize spi
spi.close(0)*/
static int l_spi_close(lua_State *L) {
    if (lua_isinteger(L, 1))
    {
        int id = luaL_checkinteger(L, 1);
        lua_pushinteger(L, luat_spi_close(id));
    }
    else if (lua_isuserdata(L, 1))
    {
        luat_spi_device_t *spi_device = (luat_spi_device_t *)luaL_testudata(L, 1, META_SPI);
        if (spi_device){
            int ret = luat_spi_device_close(spi_device);
            lua_pushinteger(L, ret);
        }
        else {
            lua_pushinteger(L, 0);
        }
    }
    else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

/**
Transmitting SPI data
@api spi.transfer(id, send_data, send_len, recv_len)
@int SPI number (e.g. 0) or software SPI object
@string/zbuff The data to be sent. If it is zbuff data, it will start reading from the pointer where the object is located.
@int Optional. The length of the data to be sent, the default is data length
@int Optional. The length of read data, default is 1
@return string Returns a string if the reading is successful, otherwise returns nil
@usage
--Initialize spi
spi.setup(0,nil,0,0,8,2000000,spi.MSB,1,1)
local recv = spi.transfer(0, "123")--Send 123 and read the data

local buff = zbuff.create(1024, 0x33) --Create a memory area with initial values     all 0x33
local recv = spi.transfer(0, buff)--Start the zbuff data from the pointer, send it all out, and read the data*/
static int l_spi_transfer(lua_State *L) {
    size_t send_length = 0;
    const char* send_buff = NULL;
    char* recv_buff = NULL;
    if(lua_isuserdata(L, 2)){//Special handling of zbuff objects
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        send_buff = (const char*)(buff->addr+buff->cursor);
        send_length = buff->len - buff->cursor;
    }else{
        send_buff = luaL_checklstring(L, 2, &send_length);
    }
    size_t length = luaL_optinteger(L,3,send_length);
    if(length <= send_length)
        send_length = length;
    size_t recv_length = luaL_optinteger(L,4,1);
    //When the length is 0, return the empty string directly.
    if(send_length == 0){
        lua_pushlstring(L, "",0);
        return 1;
    }
    if (recv_length > 0) {
        recv_buff = luat_heap_malloc(recv_length);
        if(recv_buff == NULL)
            return 0;
    }
    if (lua_isinteger(L, 1))
    {
        int id = luaL_checkinteger(L, 1);
        int ret = luat_spi_transfer(id, send_buff, send_length, recv_buff, recv_length);
        if (ret > 0) {
            lua_pushlstring(L, recv_buff, ret);
            luat_heap_free(recv_buff);
            return 1;
        }
    }
    else
    {
        //softSPI
        luat_espi_t *espi = toespi(L);
        uint8_t csPin = Luat_GPIO_MAX_ID;
        if (espi->cs != Luat_GPIO_MAX_ID)
        {
            csPin = espi->cs;
            espi->cs = Luat_GPIO_MAX_ID;
            luat_gpio_set(csPin, Luat_GPIO_LOW);
        }
        // Half-duplex, send first and read later
        if (espi->mode == 0 || (send_length != recv_length)) {
            luat_spi_soft_send(espi, send_buff, send_length);
            luat_spi_soft_recv(espi, recv_buff, recv_length);
        }
        // Full duplex, reading while sending, only when the length of sending and receiving is the same, it is full duplex
        else {
            luat_spi_soft_xfer(espi, send_buff, recv_buff, send_length);
        }
        if (csPin!= Luat_GPIO_MAX_ID)
        {
            luat_gpio_set(csPin, Luat_GPIO_HIGH);
            espi->cs = csPin;
        }
        lua_pushlstring(L, recv_buff, recv_length);
        luat_heap_free(recv_buff);
        return 1;
    }

    luat_heap_free(recv_buff);
    return 0;
}

/**
Receive SPI data of specified length
@api spi.recv(id, size, buff)
@int SPI number, for example 0
@int data length
@userdata zbuff object, optional, new on 2024.3.29
@return string/int Returns a string if the reading is successful. If zbuff is passed in, the read size will be returned. If an error occurs, nil will be returned.
@usage
--Initialize spi
spi.setup(0,nil,0,0,8,2000000,spi.MSB,1,1)
--Receive data
local recv = spi.recv(0, 4)--Receive 4 bytes of data

-- When the zbuff parameter is passed in, the return value is different. Added on 2024.3.29
-- After the read is successful, the pointer will move back len bytes
-- The writing position starts from the current used() position. Please make sure there is enough space to write len length data.
local len = spi.recv(0, 4, buff)*/
static int l_spi_recv(lua_State *L) {
    luat_zbuff_t* buff = NULL;
    char* recv_buff = NULL;
    luaL_Buffer b = {0};
    int ret = 0;
    int len = luaL_optinteger(L, 2, 1);
    if (len <= 0) {
        return 0;
    }
    if (lua_isuserdata(L, 3)) {
        buff = (luat_zbuff_t*)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE);
        recv_buff = (char*)buff->addr + buff->used;
        if (buff->len - buff->used < len) {
            LLOGW("Can't put down zbuff %d %d %d", buff->len - buff->used, len);
            return 0;
        }
    }
    else {
        luaL_buffinitsize(L, &b, len);
        recv_buff = b.b;
    }
    if(recv_buff == NULL) {
        LLOGW("out of memory when malloc spi buff %d", len);
        return 0;
    }
    
    if (lua_isinteger(L, 1))
    {
        int id = luaL_checkinteger(L, 1);
        ret = luat_spi_recv(id, recv_buff, len);
        b.n = ret;
    }
    else if (lua_isuserdata(L, 1))
    {
        luat_spi_device_t *spi_device = (luat_spi_device_t *)luaL_testudata(L, 1, META_SPI);
        if (spi_device){
            luat_spi_device_recv(spi_device, recv_buff, len);
            ret = len;
        }
        else {
            luat_espi_t *espi = (luat_espi_t *)luaL_testudata(L, 1, LUAT_ESPI_TYPE);
            if (espi){
                luat_spi_soft_recv(espi, recv_buff, len);
                ret = len;
            }
        }
    }
    if (ret <= 0) {
        return 0;
    }
    
    if (buff == NULL) {
        luaL_pushresult(&b);
    }
    else {
        buff->used += len;
        lua_pushinteger(L, len);
    }
    return 1;
}

/**
Send SPI data
@api spi.send(id, data[, len])
@int SPI number, for example 0
@string/zbuff The data to be sent. If it is zbuff data, it will start reading from the pointer where the object is located.
@int Optional. The length of the data to be sent, the default is data length
@return int send result
@usage
--Initialize spi
spi.setup(0,nil,0,0,8,2000000,spi.MSB,1,1)
local result = spi.send(0, "123")--Send 123

local buff = zbuff.create(1024, 0x33) --Create a memory area with initial values     all 0x33
local result = spi.send(0, buff)--send all zbuff data starting from the pointer*/
static int l_spi_send(lua_State *L) {
    size_t len = 0;
    const char* send_buff = NULL;
    if(lua_isuserdata(L, 2)){//Special handling of zbuff objects
        luat_zbuff_t *buff = (luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE);
        send_buff = (const char*)(buff->addr+buff->cursor);
        len = buff->len - buff->cursor;
    }
    else{
        send_buff = luaL_checklstring(L, 2, &len);
    }
    if(lua_isinteger(L,3)){//Length parameter
        size_t len_temp = luaL_checkinteger(L,3);
        if(len_temp < len)
            len = len_temp;
    }
    //When the length is 0, return directly
    if(len <= 0){
        lua_pushinteger(L,0);
        return 1;
    }
    if (lua_isinteger(L, 1))
    {
        int id = luaL_checkinteger(L, 1);
        lua_pushinteger(L, luat_spi_send(id, send_buff, len));
    }
    else
    {
        luat_espi_t *espi = toespi(L);
        lua_pushinteger(L, luat_spi_soft_send(espi, send_buff, len));
    }
    return 1;
}

/**
Set up and enable SPI (object mode)
@api spi.deviceSetup(id, cs, CPHA, CPOL, dataw, bandrate, bitdict, ms, mode)
@int SPI number, for example 0
@int CS chip selector, if it is not available in w600, please fill in nil
@int CPHA default 0, optional 0/1
@int CPOL default 0, optional 0/1
@int data width, default 8bit
@int baud rate, default 20M=20000000
@int Big and small endian, default spi.MSB, optional spi.LSB
@int Master-slave setting, default master 1, optional slave 0. Usually only host mode is supported
@int working mode, full duplex 1, half duplex 0, default full duplex
@return userdata spi_device
@usage
--Initialize spi
local spi_device = spi.deviceSetup(0,17,0,0,8,2000000,spi.MSB,1,1)*/
static int l_spi_device_setup(lua_State *L) {
    int bus_id = luaL_checkinteger(L, 1);
    int cs = luaL_optinteger(L, 2, 255); // None by default
    int CPHA = luaL_optinteger(L, 3, 0); // CPHA0
    int CPOL = luaL_optinteger(L, 4, 0); // CPOL0
    int dataw = luaL_optinteger(L, 5, 8); // 8bit
    int bandrate = luaL_optinteger(L, 6, 20000000U); // 20000000U
    int bit_dict = luaL_optinteger(L, 7, 1); // MSB=1, LSB=0
    int master = luaL_optinteger(L, 8, 1); // master=1,slave=0
    int mode = luaL_optinteger(L, 9, 1); // FULL=1, half=0
    luat_spi_device_t* spi_device = lua_newuserdata(L, sizeof(luat_spi_device_t));
    if (spi_device == NULL)return 0;
    memset(spi_device, 0, sizeof(luat_spi_device_t));
    spi_device->bus_id = bus_id;
    spi_device->spi_config.cs = cs;
    spi_device->spi_config.CPHA = CPHA;
    spi_device->spi_config.CPOL = CPOL;
    spi_device->spi_config.dataw = dataw;
    spi_device->spi_config.bandrate = bandrate;
    spi_device->spi_config.bit_dict = bit_dict;
    spi_device->spi_config.master = master;
    spi_device->spi_config.mode = mode;
    luat_spi_device_setup(spi_device);
    luaL_setmetatable(L, META_SPI);
    return 1;
}

/**
Close the specified SPI (object mode)
@api spi_device:close()
@userdata spi_device
@return int Returns 0 successfully, otherwise returns other value
@usage
--Initialize spi
spi_device.close()*/
static int l_spi_device_close(lua_State *L) {
    luat_spi_device_t* spi_device = (luat_spi_device_t*)lua_touserdata(L, 1);
    int ret = luat_spi_device_close(spi_device);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/**
Transmit SPI data (object mode)
@api spi_device:transfer(send_data[, len])
@userdata spi_device
@string/zbuff The data to be sent. If it is zbuff data, it will start reading from the pointer where the object is located.
@int Optional. The length of the data to be sent, the default is data length
@int Optional. The length of read data, default is 1
@return string Returns a string if the reading is successful, otherwise returns nil
@usage
--Initialize spi
local spi_device = spi.device_setup(0,17,0,0,8,2000000,spi.MSB,1,1)
local recv = spi_device:transfer("123")--Send 123 and read the data
local result = spi_device:transfer({0x00,0x01})--send 0x00,0x01 and read the data

local buff = zbuff.create(1024, 0x33) --Create a memory area with initial values     all 0x33
local recv = spi_device:transfer(buff)--Start the zbuff data from the pointer, send it all out, and read the data*/
static int l_spi_device_transfer(lua_State *L) {
    luat_spi_device_t* spi_device = (luat_spi_device_t*)lua_touserdata(L, 1);
    size_t send_length = 0;
    char* send_buff = NULL;
    char* recv_buff = NULL;
    uint8_t send_mode = 0;
    if(lua_isuserdata(L, 2)){//Special handling of zbuff objects
        luat_zbuff_t *buff = (luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE);
        send_buff = (const char*)(buff->addr+buff->cursor);
        send_length = buff->len - buff->cursor;
    }else if (lua_istable(L, 2)){
        send_mode = LUA_TTABLE;
        send_length = lua_rawlen(L, 2); //Return the length of the array
        send_buff = (char*)luat_heap_malloc(send_length);
        for (size_t i = 0; i < send_length; i++){
            lua_rawgeti(L, 2, 1 + i);
            send_buff[i] = (char)lua_tointeger(L, -1);
            lua_pop(L, 1); //Pop the element value just obtained from the stack
        }
    }else if(lua_isstring(L, 2)){
        send_buff = luaL_checklstring(L, 2, &send_length);
    }else{
        LLOGE("spi_device transfer first arg error");
        return 0;
    }
    if (lua_isinteger(L, 3)){
        send_length = (size_t)lua_tointeger(L, 3);
    }
    size_t recv_length = luaL_optinteger(L,4,1);
    //When the length is 0, return the empty string directly.
    if(recv_length == 0){
        lua_pushlstring(L,NULL,0);
        return 1;
    }
    if (recv_length > 0) {
        recv_buff = luat_heap_malloc(recv_length);
        if(recv_buff == NULL)
            return 0;
    }
    int ret = luat_spi_device_transfer(spi_device, send_buff, send_length, recv_buff, recv_length);
    if (send_mode == LUA_TTABLE){
        luat_heap_free(send_buff);
    }
    if (ret > 0) {
        lua_pushlstring(L, recv_buff, ret);
        luat_heap_free(recv_buff);
        return 1;
    }
    luat_heap_free(recv_buff);
    return 0;
}

/**
Send SPI data (object mode)
@api spi_device:send(data[, len])
@userdata spi_device
@string/zbuff The data to be sent. If it is zbuff data, it will start reading from the pointer where the object is located.
@return int send result
@usage
--Initialize spi
local spi_device = spi.device_setup(0,17,0,0,8,2000000,spi.MSB,1,1)
local result = spi_device:send("123")--send 123
local result = spi_device:send({0x00,0x01})--Send 0x00,0x01

local buff = zbuff.create(1024, 0x33) --Create a memory area with initial values     all 0x33
local result = spi_device:send(buff)--send all zbuff data starting from the pointer*/
static int l_spi_device_send(lua_State *L) {
    luat_spi_device_t* spi_device = (luat_spi_device_t*)lua_touserdata(L, 1);
    size_t len = 0;
    char* send_buff = NULL;
    if(lua_isuserdata(L, 2)){//Special handling of zbuff objects
        luat_zbuff_t *buff = (luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE);
        send_buff = (char*)(buff->addr+buff->cursor);
        len = buff->len - buff->cursor;
        lua_pushinteger(L, luat_spi_device_send(spi_device, send_buff, len));
    }else if (lua_istable(L, 2)){
        len = lua_rawlen(L, 2); //Return the length of the array
        send_buff = (char*)luat_heap_malloc(len);
        for (size_t i = 0; i < len; i++){
            lua_rawgeti(L, 2, 1 + i);
            send_buff[i] = (char)lua_tointeger(L, -1);
            lua_pop(L, 1); //Pop the element value just obtained from the stack
        }
        lua_pushinteger(L, luat_spi_device_send(spi_device, send_buff, len));
        luat_heap_free(send_buff);
    }else if(lua_isstring(L, 2)){
        send_buff = (char*)luaL_checklstring(L, 2, &len);
        lua_pushinteger(L, luat_spi_device_send(spi_device, send_buff, len));
    }else{
        LLOGE("spi_device transfer first arg error");
        return 0;
    }
    return 1;
}

/**
Receive SPI data of specified length (object mode)
@api spi_device:recv(size)
@userdata spi_device
@int data length
@return string Returns a string if the reading is successful, otherwise returns nil
@usage
--Initialize spi
local spi_device = spi.device_setup(0,17,0,0,8,2000000,spi.MSB,1,1)
local recv = spi_device:recv(4)--Receive 4 bytes of data*/
static int l_spi_device_recv(lua_State *L) {
    luat_spi_device_t* spi_device = (luat_spi_device_t*)lua_touserdata(L, 1);
    int len = luaL_optinteger(L, 2,1);
    char* recv_buff = luat_heap_malloc(len);
    if(recv_buff == NULL) return 0;
    int ret = luat_spi_device_recv(spi_device, recv_buff, len);
    if (ret > 0) {
        lua_pushlstring(L, recv_buff, ret);
        luat_heap_free(recv_buff);
        return 1;
    }
    luat_heap_free(recv_buff);
    return 0;
}

static int _spi_struct_gc(lua_State *L) {
    luat_spi_device_t* spi_device = (luat_spi_device_t*)lua_touserdata(L, 1);
    LLOGI("The spi_device object is being recycled, and the related memory will be released %p, spi id=%d", spi_device, spi_device->bus_id);
    return 0;
}

int _spi_struct_newindex(lua_State *L) {
    const char* key = luaL_checkstring(L, 2);
    if (!strcmp("close", key)) {
        lua_pushcfunction(L, l_spi_device_close);
        return 1;
    }
    else if (!strcmp("transfer", key)) {
        lua_pushcfunction(L, l_spi_device_transfer);
        return 1;
    }
    else if (!strcmp("send", key)) {
        lua_pushcfunction(L, l_spi_device_send);
        return 1;
    }
    else if (!strcmp("recv", key)) {
        lua_pushcfunction(L, l_spi_device_recv);
        return 1;
    }
    return 0;
}

void luat_spi_struct_init(lua_State *L) {
    luaL_newmetatable(L, META_SPI);
    lua_pushcfunction(L, _spi_struct_gc);
    lua_setfield( L, -2, "__gc" );
    lua_pushcfunction(L, _spi_struct_newindex);
    lua_setfield( L, -2, "__index" );
    lua_pop(L, 1);
}

void luat_soft_spi_struct_init(lua_State *L) {
    luaL_newmetatable(L, LUAT_ESPI_TYPE);
    lua_pop(L, 1);
}


int LUAT_WEAK luat_spi_no_block_transfer(int id, uint8_t *tx_buff, uint8_t *rx_buff, size_t len, void *CB, void *pParam)
{
    return -1;
}

/**
Non-blocking hardware SPI transmits SPI data in order to improve core utilization. The API directly returns whether to start the transmission. After the transmission is completed, it calls back through the topic. This API is suitable for hardware SPI transmission of large amounts of data. The SPI and software SPI occupied by peripheral functions (LCD SPI, W5500 SPI, etc.) cannot be used. A small amount of data transmission is recommended. Use traditional blocking APIs
@api spi.xfer(id, txbuff, rxbuff, rx_len, transfer_done_topic)
@userdata or int spi_device or spi_id. Note that if it is spi_device, you need to manually raise cs after the transmission is completed!!!!!!
@zbuff If the data to be sent is nil, only the data will be received. Due to the non-blocking model used, in order to ensure the validity of dynamic data, only zbuff can be used. The data sent is from zbuff.addr
@zbuff receives data. If it is nil, only data is sent. Due to the non-blocking model used, in order to ensure the validity of dynamic data, only zbuff can be used. The received data is stored starting from zbuff.addr
@int Transmission data length, especially if it is half-duplex, send first and then receive, such as SPI flash operation, then the length = sent bytes + received bytes. Note that enough data must be left in the above sending and receiving buffs. Subsequent receive data processing needs to skip sending data length bytes
@string topic to be called back after the transfer is completed
@return boolean true/false Whether this transmission is started correctly, true, it is started, false, there is an error and it cannot be started. When the transfer is completed, the message transfer_done_topic and boolean result will be released.
@usage
local result = spi.xfer(spi.SPI_0, txbuff, rxbuff, 1024, "SPIDONE") if result then result, spi_id, succ, error_code = sys.waitUntil("SPIDONE") end if not result or not succ then log. info("spi fail, error code", error_code) else log.info("spi ok") end*/
static int l_spi_no_block_transfer(lua_State *L)
{
	size_t topic_len = 0;
	const char *topic = luaL_checklstring(L, 5, &topic_len);
	size_t len = luaL_optinteger(L, 4, 0);

	int result = -1;
	uint8_t *tx_buff = NULL;
	uint8_t *rx_buff = NULL;
	luat_zbuff_t *txbuff = ((luat_zbuff_t *)luaL_testudata(L, 2, LUAT_ZBUFF_TYPE));
	luat_zbuff_t *rxbuff = ((luat_zbuff_t *)luaL_testudata(L, 3, LUAT_ZBUFF_TYPE));
	if ((!txbuff && !rxbuff) || !len) {
		lua_pushboolean(L, 0);
		return 1;
	}
	if (txbuff) tx_buff = txbuff->addr;
	if (rxbuff) rx_buff = rxbuff->addr;

	char *cb_topic = luat_heap_malloc(topic_len + 1);
	memcpy(cb_topic, topic, topic_len);
	cb_topic[topic_len] = 0;

    if (lua_isinteger(L, 1)) {
        int id = luaL_checkinteger(L, 1);
        result = luat_spi_no_block_transfer(id, tx_buff, rx_buff, len, luat_irq_hardware_cb_handler, cb_topic);
    }
    else {
    	luat_spi_device_t* spi_dev = (luat_spi_device_t*)lua_touserdata(L, 1);
    	luat_spi_device_config(spi_dev);
    	luat_gpio_set(spi_dev->spi_config.cs, 0);
    	result = luat_spi_no_block_transfer(spi_dev->bus_id, tx_buff, rx_buff, len, luat_irq_hardware_cb_handler, cb_topic);
    }
    if (result) {
    	luat_heap_free(cb_topic);
    }
	lua_pushboolean(L, !result);
	return 1;
}
//------------------------------------------------------------------
#include "rotable2.h"
static const rotable_Reg_t reg_spi[] =
{
    { "setup" ,           ROREG_FUNC(l_spi_setup)},
    { "createSoft",       ROREG_FUNC(l_spi_soft) },
    { "close",            ROREG_FUNC(l_spi_close)},
    { "transfer",         ROREG_FUNC(l_spi_transfer)},
    { "recv",             ROREG_FUNC(l_spi_recv)},
    { "send",             ROREG_FUNC(l_spi_send)},
    { "deviceSetup",      ROREG_FUNC(l_spi_device_setup)},
    { "deviceTransfer",   ROREG_FUNC(l_spi_device_transfer)},
    { "deviceSend",       ROREG_FUNC(l_spi_device_send)},
	{ "xfer",   		  ROREG_FUNC(l_spi_no_block_transfer)},

    //@const MSB number big endian mode
    { "MSB",               ROREG_INT(1)},
    //@const LSB number little endian mode
    { "LSB",               ROREG_INT(0)},
    //@const master number host mode
    { "master",            ROREG_INT(1)},
    //@const slave number slave mode
    { "slave",             ROREG_INT(0)},
    //@const full number full duplex
    { "full",              ROREG_INT(1)},
    //@const half number half duplex
    { "half",              ROREG_INT(0)},
	//@const SPI_0 number SPI0
    { "SPI_0",             ROREG_INT(0)},
	//@const SPI_1 number SPI1
    { "SPI_1",             ROREG_INT(1)},
	//@const SPI_2 number SPI2
    { "SPI_2",             ROREG_INT(2)},
	//@const SPI_3 number SPI3
    { "SPI_3",             ROREG_INT(3)},
	//@const SPI_4 number SPI4
    { "SPI_4",             ROREG_INT(4)},
	//@const HSPI_0 number High-speed SPI0, currently dedicated to 105
	{ "HSPI_0",             ROREG_INT(5)},
	{ NULL,                ROREG_INT(0) }
};

LUAMOD_API int luaopen_spi( lua_State *L ) {
    luat_newlib2(L, reg_spi);
    luat_spi_struct_init(L);
    luat_soft_spi_struct_init(L);
    return 1;
}
