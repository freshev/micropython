
/*@Modules sensor
@summary Sensor operation library
@version 1.0
@date 2020.03.30
@tag LUAT_USE_SENSOR
@demo dht11
@usage
-- Please check demo/dht11 demo/ds18b20*/
#include "luat_base.h"
#include "luat_timer.h"
#include "luat_mem.h"
#include "luat_gpio.h"
#include "luat_zbuff.h"

#define LUAT_LOG_TAG "sensor"
#include "luat_log.h"

#define CONNECT_SUCCESS 0
#define CONNECT_FAILED 1
static void w1_reset(int pin)
{
  luat_gpio_mode(pin, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);
  luat_gpio_set(pin, Luat_GPIO_LOW);
  luat_timer_us_delay(550); /* 480us - 960us */
  luat_gpio_set(pin, Luat_GPIO_HIGH);
  luat_timer_us_delay(40); /* 15us - 60us*/
}

static uint8_t w1_connect(int pin)
{
  uint8_t retry = 0;
  luat_gpio_mode(pin, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 0);

  while (luat_gpio_get(pin) && retry < 200)
  {
    retry++;
    luat_timer_us_delay(1);
  };

  if (retry >= 200)
    return CONNECT_FAILED;
  else
    retry = 0;

  while (!luat_gpio_get(pin) && retry < 240)
  {
    retry++;
    luat_timer_us_delay(1);
  };

  if (retry >= 240)
    return CONNECT_FAILED;

  return CONNECT_SUCCESS;
}

static uint8_t w1_read_bit(int pin){
  uint8_t data;
  luat_gpio_mode(pin, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 0);
  luat_gpio_set(pin, Luat_GPIO_LOW);
  luat_timer_us_delay(2);
  // luat_gpio_set(pin, Luat_GPIO_HIGH);
  luat_gpio_mode(pin, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 0);
  data = (uint8_t)luat_gpio_get(pin);
  luat_timer_us_delay(60);
  return data;
}

static uint8_t w1_read_byte(int pin)
{
  uint8_t i, j, dat;
  dat = 0;
  luat_timer_us_delay(90);
  //rt_base_t level;
  //level = rt_hw_interrupt_disable();
  for (i = 1; i <= 8; i++)
  {
    j = w1_read_bit(pin);
    dat = (j << 7) | (dat >> 1);
  }
  //rt_hw_interrupt_enable(level);

  return dat;
}

static void w1_write_byte(int pin, uint8_t dat)
{
  uint8_t j;
  uint8_t testb;
  luat_gpio_mode(pin, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);

  for (j = 1; j <= 8; j++)
  {
    testb = dat & 0x01;
    dat = dat >> 1;

    if (testb)
    {
      luat_gpio_set(pin, Luat_GPIO_LOW);
      luat_timer_us_delay(2);
      luat_gpio_set(pin, Luat_GPIO_HIGH);
      luat_timer_us_delay(60);
    }
    else
    {
      luat_gpio_set(pin, Luat_GPIO_LOW);
      luat_timer_us_delay(60);
      luat_gpio_set(pin, Luat_GPIO_HIGH);
      luat_timer_us_delay(2);
    }
  }
}

static const uint8_t crc8_maxim[256] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

static int32_t ds18b20_get_temperature(int pin, int32_t *val, int check_crc)
{
  uint8_t TL, TH;
  int32_t tem;

  uint8_t data[9] = {0};

  //ds18b20_start(pin);
  w1_reset(pin);
  if (w1_connect(pin)){
    // LLOGD("ds18b20 connect fail");
    return -1;
  }
  w1_write_byte(pin, 0xcc); /* skip rom */
  w1_write_byte(pin, 0x44); /* convert */

  //ds18b20_init(pin);
  w1_reset(pin);
  if (w1_connect(pin)){
    // LLOGD("ds18b20 connect fail");
    return -2;
  }

  w1_write_byte(pin, 0xcc);
  w1_write_byte(pin, 0xbe);
  data[0] = w1_read_byte(pin); /* LSB first */
  data[1] = w1_read_byte(pin);

  // if (data[0] == 0xFF || data[1] == 0xFF)
  // {
  //   //LLOGD("ds18b20 bad data, skip");
  //   return -3;
  // }

  // Read all 9 bytes and check CRC
  if (check_crc)
  {
    for (size_t i = 2; i < 9; i++){
      data[i] = w1_read_byte(pin);
    }
    uint8_t crc = 0;
    for (size_t i = 0; i < 8; i++){
      crc = crc8_maxim[crc ^ data[i]];
    }
    // LLOGD("ds18b20 %02X%02X%02X%02X%02X%02X%02X%02X [%02X %02X]",
    //            data[0], data[1], data[2], data[3],
    //            data[4], data[5], data[6], data[7],
    //            data[8], crc);
    if (data[8] != crc){
      // LLOGD("ds18b20 bad crc");
      return -4;
    }
  }
  TL = data[0];
  TH = data[1];

  if (TH > 7){
    TH = ~TH;
    TL = ~TL;
    tem = TH;
    tem <<= 8;
    tem += TL;
    tem = (int32_t)(tem * 0.0625 * 10 + 0.5);
    *val = -tem;
    return 0;
  }else{
    tem = TH;
    tem <<= 8;
    tem += TL;
    tem = (int32_t)(tem * 0.0625 * 10 + 0.5);
    *val = tem;
    return 0;
  }
}

/*Get the temperature data of DS18B20
@api sensor.ds18b20(pin, check_crc)
@int gpio port number
@boolean Whether to verify the crc value, the default is true. Not verifying the crc value can increase the probability of successful reading, but the wrong value may be read
@return int Temperature data, unit 0.1 degrees Celsius, error code returned when reading fails
@return boolean returns true if successful, otherwise returns false
@usage
while 1 do
    sys.wait(5000)
    local val,result = sensor.ds18b20(17, true) -- GPIO17 and check CRC value
    -- val 301 == 30.1 degrees Celsius
    -- result true reading successful
    log.info("ds18b20", val, result)
end*/
static int l_sensor_ds18b20(lua_State *L)
{
  int32_t val = 0;
  int check_crc = lua_gettop(L) > 1 ? lua_toboolean(L, 2) : 1;
  int pin = luaL_checkinteger(L, 1);
#ifdef LUAT_USE_MOBILE
#else
  luat_os_entry_cri();
#endif
  int32_t ret = ds18b20_get_temperature(pin, &val, check_crc);
#ifdef LUAT_USE_MOBILE
#else
  luat_os_exit_cri();
#endif
  // -55°C ~ 125°C
  if (ret || !(val <= 1250 && val >= -550))
  {
    LLOGI("ds18b20 read fail");
    lua_pushinteger(L, ret);
    lua_pushboolean(L, 0);
    return 2;
  }
  //rt_kprintf("temp:%3d.%dC\n", temp/10, temp%10);
  lua_pushinteger(L, val);
  lua_pushboolean(L, 1);
  return 2;
}

//-------------------------------------
// W1 single bus protocol, expose w1_xxx method
//-------------------------------------

/*Single bus protocol, reset device
@api sensor.w1_reset(pin)
@int gpio port number
@return nil no return*/
static int l_w1_reset(lua_State *L)
{
  w1_reset((int)luaL_checkinteger(L, 1));
  return 0;
}

/*Single bus protocol, connecting devices
@api sensor.w1_connect(pin)
@int gpio port number
@return boolean returns true if successful, false if failed*/
static int l_w1_connect(lua_State *L)
{
  if (w1_connect((int)luaL_checkinteger(L, 1)) == CONNECT_SUCCESS)
  {
    lua_pushboolean(L, 1);
  }
  else
  {
    lua_pushboolean(L, 0);
  }
  return 1;
}

/*Single bus protocol, writing data to the bus
@api sensor.w1_write(pin, data1,data2)
@int gpio port number
@int first data
@int The second data, you can write N data
@return nil no return value*/
static int l_w1_write_byte(lua_State *L)
{
  int pin = luaL_checkinteger(L, 1);
  int top = lua_gettop(L);
  if (top > 1)
  {
    for (int i = 2; i <= top; i++)
    {
      uint8_t data = luaL_checkinteger(L, i);
      w1_write_byte(pin, data);
    }
  }
  return 0;
}

/*Single bus protocol, reading data from the bus
@api sensor.w1_read(pin, len)
@int gpio port number
@int read length
@return int Returns N integers according to the read length*/
static int l_w1_read_byte(lua_State *L)
{
  int pin = luaL_checkinteger(L, 1);
  int len = luaL_checkinteger(L, 2);
  for (int i = 0; i < len; i++)
  {
    lua_pushinteger(L, w1_read_byte(pin));
  }
  return len;
}
unsigned long ReadCount(int date,int clk) //Gain 128
{
  unsigned long count = 0;
  unsigned char i = 0;
  luat_gpio_mode(date, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 0);
  luat_gpio_set(date, Luat_GPIO_HIGH);
  luat_timer_us_delay(1);
  luat_gpio_mode(clk, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 0);
  luat_gpio_set(clk, Luat_GPIO_LOW);
  luat_gpio_mode(date, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 0);
  luat_timer_us_delay(1);
  count = 0;
  int retry = 50; // Wait up to 5ms == 50 * 100us
  while (retry > 0 && luat_gpio_get(date)) {
    luat_timer_us_delay(100);
    retry --;
  }
  for (i = 0; i < 24; i++)
  {
    luat_gpio_set(clk, Luat_GPIO_HIGH);
    count = count << 1;
    luat_gpio_set(clk, Luat_GPIO_LOW);
    if (luat_gpio_get(date))
      count++;
  }
  luat_gpio_set(clk, Luat_GPIO_HIGH);
  luat_timer_us_delay(1);
  luat_gpio_set(clk, Luat_GPIO_LOW);

  return count;
}
/*Get pressure sensing data of Hx711
@api sensor.hx711(pin_date,pin_clk)
@int gpio port number of data
@int clock gpio port number
@return int The data read by hx711
@usage
-- If the device does not exist, it will be stuck on the reading interface.
sys.taskInit(
    function()
        sys.wait(1000)
        local maopi = sensor.hx711(0,7)
        while true do
            sys.wait(2000)
            a = sensor.hx711(0,7) - maopi
            if a > 0 then
                log.info("tag", a / 4.6)
            end
        end
    end
)*/
static int l_sensor_hx711(lua_State *L)
{
  // unsigned int j;
  unsigned long hx711_dat = 0;
  unsigned int temp = 0;
  int date = luaL_checkinteger(L, 1);
  int clk = luaL_checkinteger(L, 2);
  //for (j = 0; j < 5; j++)
  //  luat_timer_us_delay(5000);
  hx711_dat = ReadCount(date,clk);                //HX711AD conversion data processing
  temp = (unsigned int)(hx711_dat / 100); //Scale long data to int type for easy processing
  //LLOGI("hx711:%d",temp);
  lua_pushinteger(L, temp);

  return 1;
}
/*Get cs1237 sensing data
@api sensor.cs1237(pin_date,pin_clk)
@int gpio port number of data
@int clock gpio port number
@return int cs1237 read data
@usage
-- If the device does not exist, it will be stuck on the reading interface.
sys.taskInit(
    function()
        sys.wait(1000)
        local cs1237_data = sensor.cs1237(0,7)
        while true do
            sys.wait(2000)
            cs1237_data = sensor.cs1237(0,7) - maopi
            log.info("cs1237_data:", cs1237_data)--get the original data
        end
    end
)*/
static int l_sensor_cs1237(lua_State *L)
{
  // unsigned int j;
  unsigned long cs1237_dat = 0;
  unsigned int temp = 0;
  int date = luaL_checkinteger(L, 1);
  int clk = luaL_checkinteger(L, 2);
  //for (j = 0; j < 5; j++)
  //  luat_timer_us_delay(5000);
  cs1237_dat = ReadCount(date,clk);              
  temp = (unsigned int)cs1237_dat;
  //LLOGI("cs1237:%d",temp);
  lua_pushinteger(L, temp);

  return 1;
}
/*Set ws2812b output (gpio driver mode)
@api sensor.ws2812b(pin,data,T0H,T0L,T1H,T1L)
@int gpio port number of ws2812b
@string/zbuff The data to be sent (if it is zbuff data, the pointer position will always start from offset 0 regardless of the pointer position)
@int T0H time, indicating how many nops are delayed. Each model is different. Adjust it yourself.
@int T0L time, indicating how many nops are delayed
@int T1H time, indicating how many nops are delayed
@int T1L time, indicating how many nops are delayed
@usage
local buff = zbuff.create({8,8,24})
buff:drawLine(1,2,5,6,0x00ffff)
sensor.ws2812b(7,buff,300,700,700,700)*/

const unsigned char xor_bit_table[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

#define WS2812B_BIT_0() \
  t0h_temp = t0h; t0l_temp = t0l; \
  luat_gpio_pulse(pin,&pulse_level,2,t0h); \
  while(t0l_temp--)
#define WS2812B_BIT_1() \
  t1h_temp = t1h; t1l_temp = t1l; \
  luat_gpio_pulse(pin,&pulse_level,2,t1h); \
  while(t1l_temp--)
static int l_sensor_ws2812b(lua_State *L)
{
  int j;
  size_t len,i;
  uint8_t pulse_level = 0x80;
  const char *send_buff = NULL;
  int pin = luaL_checkinteger(L, 1);
  if (lua_isuserdata(L, 2))
  {
    luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
    send_buff = (const char*)buff->addr;
    len = buff->len;
  }
  else
  {
    send_buff = lua_tolstring(L, 2, &len);
  }
#ifdef LUAT_WS2812B_MAX_CNT
  luat_gpio_mode(pin, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_LOW);
  uint8_t bit0h = luaL_optinteger(L, 3, 10);
  uint8_t bit0l = luaL_optinteger(L, 4, 0);
  uint8_t bit1h = luaL_optinteger(L, 5, 10);
  uint8_t bit1l = luaL_optinteger(L, 6, 0);
  uint32_t frame_cnt = luaL_optinteger(L, 7, 0);
  luat_gpio_driver_ws2812b(pin, send_buff, len, frame_cnt,  bit0h, bit0l, bit1h, bit1l);
#else
  volatile uint32_t t0h_temp,t0h = luaL_checkinteger(L, 3);
  volatile uint32_t t0l_temp,t0l = luaL_checkinteger(L, 4);
  volatile uint32_t t1h_temp,t1h = luaL_checkinteger(L, 5);
  volatile uint32_t t1l_temp,t1l = luaL_checkinteger(L, 6);
  
  luat_gpio_mode(pin, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);
  pulse_level = 0 ;
  luat_gpio_pulse(pin,&pulse_level,2,t0h);
  pulse_level = 0x80;
  luat_os_entry_cri();

  for(i=0;i<len;i++)
  {
    for(j=7;j>=0;j--)
    {
      if(send_buff[i]&xor_bit_table[j])
      {
        WS2812B_BIT_1();
      }
      else
      {
        WS2812B_BIT_0();
      }
    }
  }
  luat_os_exit_cri();
#endif
  return 0;
}

#ifdef LUAT_USE_PWM

#include "luat_pwm.h"
static luat_pwm_conf_t ws2812b_pwm_conf = {
  .pnum = 1,
  .period = 800*1000,
  .precision = 100
};
/*Set ws2812b output (pwm drive mode, the pwm needs to be able to output 800k frequency, otherwise this method cannot be used)
@api sensor.ws2812b_pwm(pin,data)
@int pwm port number
@string/zbuff The data to be sent (if it is zbuff data, the pointer position will always start from offset 0 regardless of the pointer position)
@usage
local buff = zbuff.create({8,8,24})
buff:setFrameBuffer(8,8,24,0x0000ff)
sensor.ws2812b_pwm(7,buff)*/
static int l_sensor_ws2812b_pwm(lua_State *L)
{
  int ret = 0;
  int j = 0;
  size_t len,i;
  const char *send_buff = NULL;

  ws2812b_pwm_conf.channel = luaL_checkinteger(L, 1);
  if (lua_isuserdata(L, 2)){
    luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
    send_buff = (const char*)buff->addr;
    len = buff->len;
  }else if(lua_isstring(L, 2)){
    send_buff = lua_tolstring(L, 2, &len);
  }else return 0;

  //ws2812b_pwm_conf.pulse = 0;
  //luat_pwm_setup(&ws2812b_pwm_conf);

  for(i=0;i<len;i++){
    for(j=7;j>=0;j--){
      if(send_buff[i]>>j&0x01){
        ws2812b_pwm_conf.pulse = 200/3;
        ret = luat_pwm_setup(&ws2812b_pwm_conf);
      }else{
        ws2812b_pwm_conf.pulse = 100/3;
        ret = luat_pwm_setup(&ws2812b_pwm_conf);
      }
      if (ret) {
        LLOGW("luat_pwm_setup ret %d, end of PWM output", ret);
        return 0;
      }
    }
  }

  return 0;
}
#endif

#ifdef LUAT_USE_SPI
#include "luat_spi.h"
static luat_spi_t ws2812b_spi_conf = {
  .cs = 255,
  .CPHA = 0,
  .CPOL = 0,
  .dataw = 8,
  .bandrate = 5*1000*1000,
  .bit_dict = 1,
  .master = 1,
  .mode = 1,
};
/*Set ws2812b output (spi driver mode, spi needs to be able to output 5M frequency, otherwise this method cannot be used)
@api sensor.ws2812b_spi(pin,data)
@int spi port number
@string/zbuff The data to be sent (if it is zbuff data, the pointer position will always start from offset 0 regardless of the pointer position)
@usage
local buff = zbuff.create({8,8,24})
buff:setFrameBuffer(8,8,24,0x0000ff)
sensor.ws2812b_spi(2,buff)*/
static int l_sensor_ws2812b_spi(lua_State *L)
{
  int j,m;
  size_t len,i;
  const char *send_buff = NULL;

  ws2812b_spi_conf.id = luaL_checkinteger(L, 1);
  if (lua_isuserdata(L, 2)){
    luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
    send_buff = (const char*)buff->addr;
    len = buff->len;
  }else if(lua_isstring(L, 2)){
    send_buff = lua_tolstring(L, 2, &len);
  }else return 0;

  const char res_buff = 0x00;
  const char low_buff = 0xc0;
  const char high_buff = 0xf8;
  luat_spi_setup(&ws2812b_spi_conf);

  luat_spi_send(ws2812b_spi_conf.id, &res_buff, 1);

  char *gbr_buff = luat_heap_malloc(len*8);
  m=0;
  for(i=0;i<len;i++){
    for(j=7;j>=0;j--){
      if(send_buff[i]>>j&0x01){
        gbr_buff[m]=high_buff;
      }else{
        gbr_buff[m]=low_buff;
      }
      m++;
    }
  }
  luat_spi_send(ws2812b_spi_conf.id, gbr_buff, len*8);
  luat_spi_close(ws2812b_spi_conf.id);
  luat_heap_free(gbr_buff);
  return 0;
}
#endif

//The idle state of the bus is high. The host pulls the bus low and waits for DHT11 response. The host pulls the bus low for more than 18 milliseconds.
static void dht_reset(int pin)
{
  luat_gpio_mode(pin, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_HIGH);
  luat_gpio_set(pin, Luat_GPIO_HIGH);
  luat_timer_mdelay(100);
  luat_gpio_set(pin, Luat_GPIO_LOW);
  luat_timer_mdelay(20);
  luat_gpio_set(pin, Luat_GPIO_HIGH);
}

//After the host sends the start signal, wait for 20-40us and then read the response signal of DHT11
static uint8_t dht_connect(int pin)
{
  luat_gpio_mode(pin, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, Luat_GPIO_HIGH);
  luat_timer_us_delay(10);
  uint8_t retry = 0;
  while (luat_gpio_get(pin) && retry < 100)
  {
    retry++;
    luat_timer_us_delay(1);
  };
  if (retry >= 100)
    return CONNECT_FAILED;

  retry = 0;
  while (!luat_gpio_get(pin) && retry < 100)
  {
    retry++;
    luat_timer_us_delay(1);
  };
  if (retry >= 100)
    return CONNECT_FAILED;

  //wait to become low after the response is completed
  while (luat_gpio_get(pin) && retry < 200)
  {
    retry++;
    luat_timer_us_delay(1);
  };
  return CONNECT_SUCCESS;
}

//The bus is low level, indicating that DHT11 sends a response signal. After DHT11 sends the response signal, it pulls the bus high for 80us and prepares to send data. Each bit of data starts with a 50us low level time slot. The length of the high level determines The data bit is 0 or 1
static uint8_t dht_read_bit(int pin)
{
  uint8_t retry=0,d=0;
  while (!luat_gpio_get(pin) && retry < 200)
  {
    retry++;
    luat_timer_us_delay(1);
  };
  luat_timer_us_delay(30);
  d = luat_gpio_get(pin);
  retry=0;
  while (luat_gpio_get(pin) && retry < 200)
  {
    retry++;
    luat_timer_us_delay(1);
  };
  return d;
}
static uint8_t dht_read_byte(int pin)
{
  uint8_t i, dat;
  dat = 0;
  for (i = 0; i < 8; i++)//MSB
  {
    dat<<=1;
    dat+=dht_read_bit(pin);
  }
  return dat;
}

/*Get temperature and humidity data of DHT11/DHT12
@api sensor.dht1x(pin)
@int gpio port number
@boolean Whether to verify the crc value, the default is true. Not verifying the crc value can increase the probability of successful reading, but the wrong value may be read
@return int Humidity data, unit 0.01%, error value returned when reading fails
@return int Temperature data, unit 0.01 degrees Celsius, error value returned when reading fails
@return boolean returns true if successful, otherwise returns false
@usage
while 1 do
    sys.wait(1000)
    local h,t,r = sensor.dht1x(17, true) -- GPIO17 and check CRC value
    log.info("dht11", h/100,t/100,r)--90.1 23.22
end*/
static int dht1x_read(lua_State *L)
{
  int pin = luaL_checkinteger(L,1);
  int check = lua_toboolean(L,2);

  dht_reset(pin);
  luat_os_entry_cri();
  if(dht_connect(pin) != CONNECT_SUCCESS)//Not connected
  {
    luat_os_exit_cri();
    lua_pushinteger(L,0);
    lua_pushinteger(L,0);
    lua_pushboolean(L,0);
    return 3;
  }
  
  uint8_t buff[5];
  buff[0] = dht_read_byte(pin);
  buff[1] = dht_read_byte(pin);//decimal part
  buff[2] = dht_read_byte(pin);
  buff[3] = dht_read_byte(pin);//decimal part
  buff[4] = dht_read_byte(pin);//This is crc
  luat_os_exit_cri();

  int temp = (buff[0] & 0x7F) *100 + buff[1];
  int humi = (buff[2] & 0x7F) *100 + buff[3];
  if (buff[0] & 0x80)
  {
    temp = -temp;
  }

  lua_pushinteger(L, temp);
  lua_pushinteger(L, humi);
  if(check)
  {
    uint8_t check_r = 0;
    check_r = buff[0]+buff[1]+buff[2]+buff[3];
    lua_pushboolean(L,check_r == buff[4]);
  }
  else
  {
    lua_pushboolean(L,1);
  }
  return 3;
}

static unsigned char sc12a_wait_ack(int sda, int scl)
{
  luat_gpio_set(sda, Luat_GPIO_HIGH);
  luat_gpio_mode(sda, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 1);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_HIGH);
  luat_timer_us_delay(5 * 3);
  int max_wait_time = 3000;
  while (max_wait_time--)
  {
    if (luat_gpio_get(sda) == Luat_GPIO_LOW)
    {
      luat_gpio_set(scl, Luat_GPIO_LOW);
      return 1;
    }
    luat_timer_us_delay(1);
  }
  // stop signal
  luat_gpio_mode(sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
  luat_gpio_set(scl, Luat_GPIO_LOW);
  luat_timer_us_delay(5);
  luat_gpio_set(sda, Luat_GPIO_LOW);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_HIGH);
  luat_timer_us_delay(5);
  luat_gpio_set(sda, Luat_GPIO_HIGH);
  luat_timer_us_delay(5);
  return 0;
}

static unsigned int read_sc12akey(int sda, int scl)
{
  unsigned int data=0 ;
  unsigned char sc12a_addr = 0x81;
  //Send start signal
  luat_gpio_mode(scl, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
  luat_gpio_mode(sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_HIGH);
  luat_timer_us_delay(5);
  luat_gpio_set(sda, Luat_GPIO_LOW);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_LOW);
  luat_timer_us_delay(5);
  //Sending address
  unsigned char i = 8;

  luat_gpio_mode(sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 0);
  while (i--)
  {
    luat_gpio_set(scl, Luat_GPIO_LOW);
    luat_timer_us_delay(5 * 2);
    if (sc12a_addr & 0x80)
    {
      luat_gpio_set(sda, Luat_GPIO_HIGH);
    }
    else
    {
      luat_gpio_set(sda, Luat_GPIO_LOW);
    }
    luat_timer_us_delay(5);
    sc12a_addr <<= 1;
    luat_gpio_set(scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(5);
    luat_gpio_set(scl, Luat_GPIO_LOW);
    luat_timer_us_delay(5);
  }
  // Wait for response ack
  if (!sc12a_wait_ack(sda, scl))
  {
    luat_gpio_mode(sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
    luat_gpio_set(scl, Luat_GPIO_LOW);
    luat_timer_us_delay(5);
    luat_gpio_set(sda, Luat_GPIO_LOW);
    luat_timer_us_delay(5);
    luat_gpio_set(scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(5);
    luat_gpio_set(sda, Luat_GPIO_HIGH);
    luat_timer_us_delay(5);
    return -1;
  }

  i = 16;
  luat_gpio_set(sda, Luat_GPIO_HIGH);
  luat_gpio_mode(sda, Luat_GPIO_INPUT, Luat_GPIO_PULLUP, 1);
  while (i--)
  {
    data <<= 1;
    luat_gpio_set(scl, Luat_GPIO_LOW);
    luat_timer_us_delay(5);
    luat_gpio_set(scl, Luat_GPIO_HIGH);
    luat_timer_us_delay(5);
    if (luat_gpio_get(sda))
      data |= 0x01;
  }
  //Send NOACK
  luat_gpio_set(scl, Luat_GPIO_LOW);
  luat_gpio_mode(sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_HIGH);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_LOW);

  //Send STOP
  luat_gpio_mode(sda, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
  luat_gpio_set(scl, Luat_GPIO_LOW);
  luat_timer_us_delay(5);
  luat_gpio_set(sda, Luat_GPIO_LOW);
  luat_timer_us_delay(5);
  luat_gpio_set(scl, Luat_GPIO_HIGH);
  luat_timer_us_delay(5);
  luat_gpio_set(sda, Luat_GPIO_HIGH);
  luat_timer_us_delay(5);

  return (data);
}
/*Get the channel data of sc12a being touched
@api sensor.sc12a(sda,scl)
@int gpio port number of data
@int clock gpio port number
@return int Returns integer data if the reading is successful, and returns an error value if the reading fails.
@usage
while true do
  local temp1=sensor.sc12a(4,7)
  if bit.rshift(bit.band( temp1, 0x8000), 15 )==0x01 then
    log.info("Channel 0 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x4000), 14 )==0x01 then
    log.info("Channel 1 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x2000), 13 )==0x01 then
    log.info("Channel 2 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x1000), 12 )==0x01 then
    log.info("Channel 3 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x800), 11 )==0x01 then
    log.info("Channel 4 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x400), 10 )==0x01 then
    log.info("Channel 5 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x200), 9 )==0x01 then
    log.info("Channel 6 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x100), 8 )==0x01 then
    log.info("Channel 7 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x80), 7 )==0x01 then
    log.info("Channel 8 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x40), 6 )==0x01 then
    log.info("Channel 9 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x20), 5 )==0x01 then
    log.info("Channel 10 is pressed")
  end
  if bit.rshift(bit.band( temp1, 0x10), 4 )==0x01 then
    log.info("Channel 11 is pressed")
  end
  sys.wait(200)
end*/
static int l_sensor_sc12a(lua_State *L)
{ 
  unsigned int sc12a_data = 0xffff;
 
  int sda = luaL_checkinteger(L, 1);
  int scl = luaL_checkinteger(L, 2);
  
  sc12a_data = read_sc12akey(sda,scl);
  sc12a_data =sc12a_data^0xffff; 
  lua_pushinteger(L, sc12a_data);
  return 1;
}

/*Single bus command to read and write YHM27XX
@api sensor.yhm27xxx(pin, chip_id, reg, data)
@int gpio port number
@int chip ID
@int register address
@int The data to be written. If not filled in, it means reading data from the register.
@return boolean returns true if successful, false if failed
@return int Returns the register value if the reading is successful, and no return is received if the writing is successful.
@usage
while 1 do
    sys.wait(1000)
    local result, data = sensor.yhm27xxx(15, 0x04, 0x05)
    log.info("yhm27xxx", result, data)
end*/
#ifdef LUAT_USE_YHM27XX
static int l_sensor_yhm27xx(lua_State *L)
{
  uint8_t pin = luaL_checkinteger(L, 1);
  uint8_t chip_id = luaL_checkinteger(L, 2);
  uint8_t reg = luaL_checkinteger(L, 3);
  uint8_t data = 0;
  uint8_t is_read = 1;
  if (!lua_isnone(L, 4))
  {
    is_read = 0;
    data = luaL_checkinteger(L, 4);
  }
  if(luat_gpio_driver_yhm27xx(pin, chip_id, reg, is_read, &data))
  {
    lua_pushboolean(L, 0);
    return 1;
  }
  lua_pushboolean(L, 1);
  if (is_read)
  {
    lua_pushinteger(L, data);
    return 2;
  }
  return 1;
}
#endif

#include "rotable2.h"
static const rotable_Reg_t reg_sensor[] =
    {
        {"w1_reset",    ROREG_FUNC(l_w1_reset)},
        {"w1_connect",  ROREG_FUNC(l_w1_connect)},
        {"w1_write",    ROREG_FUNC(l_w1_write_byte)},
        {"w1_read",     ROREG_FUNC(l_w1_read_byte)},
        {"ds18b20",     ROREG_FUNC(l_sensor_ds18b20)},
        {"hx711",       ROREG_FUNC(l_sensor_hx711)},
        {"cs1237",       ROREG_FUNC(l_sensor_cs1237)},
        {"ws2812b",     ROREG_FUNC(l_sensor_ws2812b)},
        {"dht1x",       ROREG_FUNC(dht1x_read)},
        {"sc12a",       ROREG_FUNC(l_sensor_sc12a)},
#ifdef LUAT_USE_PWM
        {"ws2812b_pwm", ROREG_FUNC(l_sensor_ws2812b_pwm)},
#endif
#ifdef LUAT_USE_SPI
        {"ws2812b_spi",     ROREG_FUNC(l_sensor_ws2812b_spi)},
#endif
#ifdef LUAT_USE_YHM27XX
        {"yhm27xx",     ROREG_FUNC(l_sensor_yhm27xx)},
#endif
        {NULL,          ROREG_INT(0) }
};

LUAMOD_API int luaopen_sensor(lua_State *L)
{
  luat_newlib2(L, reg_sensor);
  return 1;
}
