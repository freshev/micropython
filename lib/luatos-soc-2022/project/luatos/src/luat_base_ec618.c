#include "common_api.h"
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "luat_fs.h"
#include "luat_timer.h"
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "mbedtls/cipher.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md5.h"
#include "plat_config.h"
#include "reset.h"
#define LUAT_LOG_TAG "base"
#include "luat_log.h"

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
void luat_lv_fs_init(void);
void lv_bmp_init(void);
void lv_png_init(void);
void lv_split_jpeg_init(void);
#endif

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base}, // _G
  {LUA_LOADLIBNAME, luaopen_package}, // require
  {LUA_COLIBNAME, luaopen_coroutine}, // coroutine coroutine library
  {LUA_TABLIBNAME, luaopen_table},    // table library, operates table type data structures
  {LUA_IOLIBNAME, luaopen_io},        // io library, operating files
  {LUA_OSLIBNAME, luaopen_os},        // os library, streamlined
  {LUA_STRLIBNAME, luaopen_string},   // string library, string operations
  {LUA_MATHLIBNAME, luaopen_math},    // math numerical calculation
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},     // debug library, streamlined
#ifdef LUAT_USE_DBG
#ifndef LUAT_USE_SHELL
#define LUAT_USE_SHELL
#endif
  {"dbg",  luaopen_dbg},               //debugging library
#endif
#if defined(LUA_COMPAT_BITLIB)
  {LUA_BITLIBNAME, luaopen_bit32},    // unlikely to be enabled
#endif
// Below is the library customized for LuatOS. If you need to simplify it, please test it carefully.
//----------------------------------------------------------------------
// Core support library, cannot be disabled!!
  {"rtos",    luaopen_rtos},              // rtos underlying library, the core function is queue and timer
  {"log",     luaopen_log},               //Log library
  {"timer",   luaopen_timer},             //delay library
//-----------------------------------------------------------------------
  {"mobile", luaopen_mobile},
  {"sms",    luaopen_sms},
  {"errDump",luaopen_errdump},
#ifdef LUAT_USE_NETWORK
  {"socket", luaopen_socket_adapter},
#ifdef LUAT_USE_MQTT
  {"mqtt", luaopen_mqtt},
#endif
#ifdef LUAT_USE_WEBSOCKET
  {"websocket", luaopen_websocket},
#endif
  {"http", luaopen_http},
#ifdef LUAT_USE_FTP
  {"ftp", luaopen_ftp},
#endif
#endif
#ifdef LUAT_USE_W5500
  {"w5500", luaopen_w5500},
#endif
#ifdef LUAT_USE_WLAN
  {"wlan", luaopen_wlan},
#endif
// The device driver class can be deleted according to the actual situation. Even in the most streamlined firmware, it is strongly recommended to retain the uart library
#ifdef LUAT_USE_UART
  {"uart",    luaopen_uart},              // Serial port operation
#endif
#ifdef LUAT_USE_GPIO
  {"gpio",    luaopen_gpio},              //Operation of GPIO pin
#endif
#ifdef LUAT_USE_I2C
  {"i2c",     luaopen_i2c},               //I2C operation
#endif
#ifdef LUAT_USE_SPI
  {"spi",     luaopen_spi},               //SPI operation
#endif
#ifdef LUAT_USE_ADC
  {"adc",     luaopen_adc},               // ADC Modules
#endif
#ifdef LUAT_USE_PWM
  {"pwm",     luaopen_pwm},               //PWM Modules
#endif
#ifdef LUAT_USE_WDT
  {"wdt",     luaopen_wdt},               // watchdog Modules
#endif
#ifdef LUAT_USE_PM
  {"pm",      luaopen_pm},                //Power management Modules
#endif
#ifdef LUAT_USE_MCU
  {"mcu",     luaopen_mcu},               // Some operations unique to MCU
#endif
#ifdef LUAT_USE_RTC
  {"rtc", luaopen_rtc},                   // real-time clock
#endif
#ifdef LUAT_USE_OTP
  {"otp", luaopen_otp},                   // OTP
#endif
//-----------------------------------------------------------------------
// Tool library, select as needed
#ifdef LUAT_USE_CRYPTO
  {"crypto",luaopen_crypto},            // Encryption and hash Modules
#endif
#ifdef LUAT_USE_CJSON
  {"json",    luaopen_cjson},          // Serialization and deserialization of json
#endif
#ifdef LUAT_USE_ZBUFF
  {"zbuff",   luaopen_zbuff},             // Manipulate memory blocks like C language language
#endif
#ifdef LUAT_USE_PACK
  {"pack",    luaopen_pack},              // pack.pack/pack.unpack
#endif
#ifdef LUAT_USE_MQTTCORE
  {"mqttcore",luaopen_mqttcore},          // MQTT protocol encapsulation
#endif
#ifdef LUAT_USE_LIBCOAP
  {"libcoap", luaopen_libcoap},           //Process COAP message
#endif
#ifdef LUAT_USE_LIBGNSS
  {"libgnss", luaopen_libgnss},           // Process GNSS positioning data
#endif
#ifdef LUAT_USE_FS
  {"fs",      luaopen_fs},                // The file system library provides some methods in addition to the io library.
#endif
#ifdef LUAT_USE_SENSOR
  {"sensor",  luaopen_sensor},            // Sensor library, supports DS18B20
#endif
#ifdef LUAT_USE_SFUD
  {"sfud", luaopen_sfud},              // sfud
#endif
#ifdef LUAT_USE_SFD
  {"sfd", luaopen_sfd},              // sfud
#endif
#ifdef LUAT_USE_DISP
  {"disp",  luaopen_disp},              // OLED display Modules
#endif
#ifdef LUAT_USE_U8G2
  {"u8g2", luaopen_u8g2},              // u8g2
#endif

#ifdef LUAT_USE_EINK
  {"eink",  luaopen_eink},              // E-ink screen
#endif
#ifdef LUAT_USE_FATFS
  {"fatfs",  luaopen_fatfs},              // SD card/tf card
#endif

#ifdef LUAT_USE_LVGL
// #ifndef LUAT_USE_LCD
// #define LUAT_USE_LCD
// #endif
  {"lvgl",   luaopen_lvgl},
#endif

#ifdef LUAT_USE_LCD
  {"lcd",    luaopen_lcd},
#endif
//#ifdef LUAT_USE_STATEM
//  {"statem",    luaopen_statem},
//#endif
#ifdef LUAT_USE_GTFONT
  {"gtfont",    luaopen_gtfont},
#endif
#ifdef LUAT_USE_FSKV
  {"fskv", luaopen_fskv},
// When FSKV is enabled, FDB is automatically disabled
#ifdef LUAT_USE_FDB
#undef LUAT_USE_FDB
#endif
#endif
#ifdef LUAT_USE_FDB
  {"fdb",       luaopen_fdb},
#endif
#ifdef LUAT_USE_VMX
  {"vmx",       luaopen_vmx},
#endif
#ifdef LUAT_USE_NES   
  {"nes", luaopen_nes}, 
#endif
#ifdef LUAT_USE_COREMARK
  {"coremark", luaopen_coremark},
#endif
#ifdef LUAT_USE_FONTS
  {"fonts", luaopen_fonts},
#endif
//#ifdef LUAT_USE_ZLIB
//  {"zlib", luaopen_zlib},
//#endif
#ifdef LUAT_USE_MLX90640
  {"mlx90640", luaopen_mlx90640},
#endif
#ifdef LUAT_USE_IR
  {"ir", luaopen_ir},
#endif
#ifdef LUAT_USE_YMODEM
  {"ymodem", luaopen_ymodem},
#endif
#ifdef LUAT_USE_LORA
  {"lora", luaopen_lora},
#endif
#ifdef LUAT_USE_LORA2
  {"lora2", luaopen_lora2},
#endif
#ifdef LUAT_USE_MINIZ
  {"miniz", luaopen_miniz},
#endif
#ifdef LUAT_USE_PROTOBUF
  {"protobuf", luaopen_protobuf},
#endif
#ifdef LUAT_USE_IOTAUTH
  {"iotauth", luaopen_iotauth},
#endif
#ifdef LUAT_USE_HTTPSRV
  {"httpsrv", luaopen_httpsrv},
#endif
#ifdef LUAT_USE_RSA
  {"rsa", luaopen_rsa},
#endif
#ifdef LUAT_USE_MEDIA
  {"i2s", luaopen_i2s},
  {"audio", luaopen_multimedia_audio},
#ifndef LUAT_USE_TTS_ONLY
  {"codec", luaopen_multimedia_codec},
#endif
#endif
#ifdef LUAT_USE_HMETA
  {"hmeta", luaopen_hmeta},
#endif
#ifdef LUAT_USE_FOTA
  {"fota", luaopen_fota},
#endif
#ifdef LUAT_USE_PROFILER
  {"profiler", luaopen_profiler},
#endif
#ifdef LUAT_USE_ICONV
  {"iconv", luaopen_iconv},
#endif
#ifdef LUAT_USE_GMSSL
  {"gmssl",luaopen_gmssl},
#endif
#ifdef LUAT_USE_MAX30102
  {"max30102", luaopen_max30102},
#endif
#ifdef LUAT_USE_BIT64
  {"bit64", luaopen_bit64},
#endif
#ifdef LUAT_USE_HTTPSRV
  {"httpsrv", luaopen_httpsrv},
#endif
#ifdef LUAT_USE_REPL
  {"repl", luaopen_repl},
#endif
#ifdef LUAT_USE_STATEM
  {"statem",    luaopen_statem},
#endif
#ifdef LUAT_USE_FASTLZ
  {"fastlz",    luaopen_fastlz},
#endif
#ifdef LUAT_USE_ERCOAP
  {"ercoap", luaopen_ercoap},
#endif
#ifdef LUAT_USE_SQLITE3
  {"sqlite3", luaopen_sqlite3},
#endif
#ifdef LUAT_USE_WS2812
  {"ws2812", luaopen_ws2812},
#endif
#ifdef LUAT_USE_ANTBOT
  {"antbot", luaopen_antbot},
#endif
#ifdef LUAT_USE_XXTEA
  {"xxtea", luaopen_xxtea},
#endif
#ifdef LUAT_USE_ULWIP
  {"ulwip", luaopen_ulwip},
  {"napt",  luaopen_napt},
#endif
#ifdef LUAT_USE_HT1621
  {"ht1621", luaopen_ht1621},
#endif
  {NULL, NULL}
};

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
#else
void mbedtls_md5( const unsigned char *input,
                  size_t ilen,
                  unsigned char output[16] )
{
    mbedtls_md5_ret( input, ilen, output );
}
#endif

// Load different library functions according to different rtconfig
void luat_openlibs(lua_State *L) {
    //Initialize queue service
    luat_msgbus_init();
    //print_list_mem("done>luat_msgbus_init");
    //Load system library
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
        //extern void print_list_mem(const char* name);
        //print_list_mem(lib->name);
    }
}

void ResetECSystemReset(void);

void luat_os_reboot(int code){
    (void)code;
    ResetECSystemReset();
}

extern const char *soc_get_chip_name(void);
const char* luat_os_bsp(void) {
#if 0
    return soc_get_chip_name();
#else
    return "EC618";
#endif
}

/** The device enters standby mode*/
void luat_os_standy(int timeout) {
    (void)timeout;
    return; // nop
}

// from reset.h

//void luat_ota_reboot(int timeout_ms) {
//  if (timeout_ms > 0)
//    luat_timer_mdelay(timeout_ms);
//  // nop
//}

uint32_t luat_get_utc(uint32_t *tamp)
{
	uint32_t sec = soc_get_utc();
	if (tamp)
	{
		*tamp = sec;
	}
	return sec;
}

void luat_os_entry_cri(void) {
  portENTER_CRITICAL();
}

void luat_os_exit_cri(void) {
  portEXIT_CRITICAL();
}

// delay_us is a system function
extern void delay_us(uint32_t us);
void luat_timer_us_delay(size_t us) {
  if (us > 0)
    delay_us(us);
}

extern void soc_vsprintf(uint8_t no_print, const char *fmt, va_list ap);
void DBG_Printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	soc_vsprintf(0, fmt, ap);
	va_end(ap);
}

extern const uint8_t ByteToAsciiTable[16];
void DBG_HexPrintf(void *Data, unsigned int len)
{
	uint8_t *data = (uint8_t *)Data;
	uint8_t *uart_buf;
    uint32_t i,j;
    j = 0;
    if (!len) return;
    uart_buf = luat_heap_malloc(len * 3 + 2);
    if (!uart_buf) return;
    memset(uart_buf, 0, len * 3 + 2);
    for (i = 0; i < len; i++){
		uart_buf[j++] = ByteToAsciiTable[(data[i] & 0xf0) >> 4];
		uart_buf[j++] = ByteToAsciiTable[data[i] & 0x0f];
		uart_buf[j++] = ' ';
    }
    uart_buf[j++] = '\r';
    uart_buf[j++] = '\n';
	luat_log_write((char*)uart_buf, len * 3 + 2);
	luat_heap_free(uart_buf);
}

struct tm *mbedtls_platform_gmtime_r( const mbedtls_time_t *tt,
                                      struct tm *tm_buf )
{
	Date_UserDataStruct Date;
	Time_UserDataStruct Time;
	Tamp2UTC(*tt, &Date, &Time, 0);
	tm_buf->tm_year = Date.Year - 1900;
	tm_buf->tm_mon = Date.Mon - 1;
	tm_buf->tm_mday = Date.Day;
	tm_buf->tm_hour = Time.Hour;
	tm_buf->tm_min = Time.Min;
	tm_buf->tm_sec = Time.Sec;
	return tm_buf;

}

void luat_mcu_set_hardfault_mode(int mode)
{
	uint8_t new_mode = EXCEP_OPTION_SILENT_RESET;
	switch (mode)
	{
	case 0:
		new_mode = EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP;
		break;
	case 1:
		new_mode = EXCEP_OPTION_SILENT_RESET;
		break;
	case 2:
		new_mode = EXCEP_OPTION_DUMP_FLASH_EPAT_RESET;
		break;
	case 3:
		new_mode = EXCEP_OPTION_DUMP_FLASH_RESET;
		break;
	default:
		return;
	}
	BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION, new_mode);
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION) == EXCEP_OPTION_SILENT_RESET)
        ResetLockupCfg(true, true);
    else
        ResetLockupCfg(false, false);
}
