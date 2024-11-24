
#ifndef LUAT_CONF_BSP
#define LUAT_CONF_BSP

#define LUAT_BSP_VERSION "V1113"

//------------------------------------------------------
// The following content between custom --> and <-- custom is for user configuration.
// It is also a configurable part of cloud compilation. Do not delete or modify the logo when submitting the code.
//custom -->
//------------------------------------------------------

// Peripherals, enable on demand, at least enable uart and wdt libraries
#define LUAT_USE_UART 1
#define LUAT_USE_GPIO 1
#define LUAT_USE_I2C  1
#define LUAT_USE_SPI  1
#define LUAT_USE_ADC  1
#define LUAT_USE_PWM  1
#define LUAT_USE_WDT  1
#define LUAT_USE_PM  1
#define LUAT_USE_MCU  1
#define LUAT_USE_RTC 1
#define LUAT_USE_OTP 1

#define LUAT_USE_HMETA 1

#define LUAT_USE_IOTAUTH 1
#define LUAT_USE_WEBSOCKET 1
#define LUAT_USE_MQTT 1
// #define LUAT_USE_ULWIP 1

#define LUAT_USE_SOFT_UART 1
//----------------------------
//Commonly used tool libraries can be enabled on demand. Cjson and pack are highly recommended.
#define LUAT_USE_CRYPTO  1
#define LUAT_USE_CJSON  1
#define LUAT_USE_ZBUFF  1
#define LUAT_USE_PACK  1
#define LUAT_USE_LIBGNSS  1
#define LUAT_USE_FS  1
#define LUAT_USE_SENSOR  1
#define LUAT_USE_SFUD  1
// #define LUAT_USE_SFD   1
// #define LUAT_USE_STATEM 1
//Performance test
//#define LUAT_USE_COREMARK 1
#define LUAT_USE_IR 1
// FDB provides kv database, similar to nvm library
#define LUAT_USE_FDB 1
// fskv provides an API compatible with fdb and is designed to replace the fdb library
#define LUAT_USE_FSKV 1
#define LUAT_USE_I2CTOOLS 1
#define LUAT_USE_LORA 1
// #define LUAT_USE_LORA2 1
// #define LUAT_USE_MAX30102 1
// #define LUAT_USE_MLX90640 1
// zlib compression, faster and smaller implementation
#define LUAT_USE_MINIZ 1
// #define LUAT_USE_FASTLZ 1
#define LUAT_USE_FTP 1
// #define LUAT_USE_HTTPSRV 1
//32bitluavm additional support for 64bit data
#define LUAT_USE_BIT64 1
#define LUAT_USE_WLAN 1
#define LUAT_USE_W5500 1
//---------------FATFS special configuration
// Fatfs's long file name and non-English file name support requires 180k ROM, which is very luxurious
#define LUAT_USE_FATFS 1
//#define LUAT_USE_FATFS_CHINESE

// #define LUAT_USE_PROFILER 1
// #define LUAT_USE_MQTTCORE 1
// #define LUAT_USE_LIBCOAP 1
// #define LUAT_USE_ERCOAP 1
// #define LUAT_USE_SQLITE3 1
// #define LUAT_USE_WS2812 1
// #define LUAT_USE_YMODEM 1
// #define LUAT_USE_HT1621 1

//----------------------------
// Qualcomm font, needs to be used with the chip
// #define LUAT_USE_GTFONT 1
// #define LUAT_USE_GTFONT_UTF8

//----------------------------
//Advanced functions
#define LUAT_USE_REPL 1
//Multi-virtual machine support, experimental, generally not enabled
// #define LUAT_USE_VMX 1
// #define LUAT_USE_NES 1
#define LUAT_USE_PROTOBUF 1
#define LUAT_USE_RSA      1
// #define LUAT_USE_XXTEA    1

// Ant chain
// #define LUAT_USE_ANTBOT 1

// Encoding conversion requires about 40k space, and incomplete GBK mapping is turned off by default
// #define LUAT_USE_ICONV 1

// National secret algorithm SM2/SM3/SM4
// #define LUAT_USE_GMSSL 1

// ------------------------------
// Audio related
//Special reminder for TTS:
// TTS is implemented in 2 ways:
// 1. Enable LUAT_USE_TTS_ONCHIP, and put resource files in on-chip Flash
// 2. Disable LUAT_USE_TTS_ONCHIP, and put resource files into off-chip SPI Flash
//
// The resource data is very large, requiring nearly 800k. It cannot be used to directly enable TTS_ONCHIP in the default configuration!!!
// 1. Disable all UI libraries, including fonts and fonts libraries
// 2. Disable most tool libraries, such as rsa, lora, etc.
// 3. Reduce the script area to 64+48 mode, or even 32+24 mode
// 4. Using 8k resource files can save 100k. The sound quality is a matter of opinion and is optional.
// ------------------------------
#define LUAT_USE_MEDIA    1
#define LUAT_SUPPORT_AMR  1
// #define LUAT_USE_TTS    1
// #define LUAT_USE_TTS_ONCHIP    1
// #define LUAT_USE_TTS_8K 1
// // Only enable TTS, disable AMR/MP3 decoding
// #define LUAT_USE_TTS_ONLY 1
//---------------------
// UI
// LCD is a color screen, if you use LVGL you must enable LCD
#define LUAT_USE_LCD
#define LUAT_USE_TJPGD
// EINK is the ink screen
#define LUAT_USE_EINK

//---------------------
// U8G2
// Monochrome screen, supports i2c/spi
// #define LUAT_USE_DISP
#define LUAT_USE_U8G2

/**************FONT*****************/
#define LUAT_USE_FONTS
/**********U8G2&LCD&EINK FONT*************/
// OPPOSANS
#define USE_U8G2_OPPOSANSM_ENGLISH 1
// #define USE_U8G2_OPPOSANSM12_CHINESE
// #define USE_U8G2_OPPOSANSM16_CHINESE
// #define USE_U8G2_OPPOSANSM24_CHINESE
// #define USE_U8G2_OPPOSANSM32_CHINESE
// SARASA
// #define USE_U8G2_SARASA_M8_CHINESE
// #define USE_U8G2_SARASA_M10_CHINESE
// #define USE_U8G2_SARASA_M12_CHINESE
// #define USE_U8G2_SARASA_M14_CHINESE
// #define USE_U8G2_SARASA_M16_CHINESE
// #define USE_U8G2_SARASA_M18_CHINESE
// #define USE_U8G2_SARASA_M20_CHINESE
// #define USE_U8G2_SARASA_M22_CHINESE
// #define USE_U8G2_SARASA_M24_CHINESE
// #define USE_U8G2_SARASA_M26_CHINESE
// #define USE_U8G2_SARASA_M28_CHINESE

/**********LVGL FONT*************/
// #define LV_FONT_OPPOSANS_M_8
// #define LV_FONT_OPPOSANS_M_10
// #define LV_FONT_OPPOSANS_M_12
// #define LV_FONT_OPPOSANS_M_16

//---------------------
// LVGL
// Mainly recommended UI library, powerful but cumbersome API
//#define LUAT_USE_LVGL

#define LUAT_USE_LVGL_JPG 1 // Enable JPG decoding support
// #define LUAT_USE_LVGL_PNG 1 // Enable PNG decoding support
// #define LUAT_USE_LVGL_BMP 1 // Enable BMP decoding support

#define LUAT_USE_LVGL_ARC   //Arc no dependency
#define LUAT_USE_LVGL_BAR   //Progress bar no dependencies
#define LUAT_USE_LVGL_BTN   //Button depends on container CONT
#define LUAT_USE_LVGL_BTNMATRIX   //Button matrix no dependencies
#define LUAT_USE_LVGL_CALENDAR   //Calendar no dependencies
#define LUAT_USE_LVGL_CANVAS   //Canvas depends on image IMG
#define LUAT_USE_LVGL_CHECKBOX   //Check box depends on button BTN label LABEL
#define LUAT_USE_LVGL_CHART   //Chart has no dependencies
#define LUAT_USE_LVGL_CONT   //Container has no dependencies
#define LUAT_USE_LVGL_CPICKER   //Color selector no dependencies
#define LUAT_USE_LVGL_DROPDOWN   //Drop-down list depends on page PAGE label LABEL
#define LUAT_USE_LVGL_GAUGE   //Instrument depends on progress bar BAR instrument (arc scale) LINEMETER
#define LUAT_USE_LVGL_IMG   //Picture depends on label LABEL
#define LUAT_USE_LVGL_IMGBTN   //Picture button depends on button BTN
#define LUAT_USE_LVGL_KEYBOARD   //Keyboard relies on image button IMGBTN
#define LUAT_USE_LVGL_LABEL   //Tag no dependencies
#define LUAT_USE_LVGL_LED   //LED has no dependencies
#define LUAT_USE_LVGL_LINE   //Line has no dependencies
#define LUAT_USE_LVGL_LIST   //List dependent page PAGE button BTN label LABEL
#define LUAT_USE_LVGL_LINEMETER   //Instrument (arc scale) no dependencies
#define LUAT_USE_LVGL_OBJMASK   //Object mask no dependencies
#define LUAT_USE_LVGL_MSGBOX   //Message box depends on image button IMGBTN label LABEL
#define LUAT_USE_LVGL_PAGE   //Page depends on container CONT
#define LUAT_USE_LVGL_SPINNER   //Rotator relies on arc ARC animation ANIM
#define LUAT_USE_LVGL_ROLLER   //Roller no dependencies
#define LUAT_USE_LVGL_SLIDER   //Slider depends on progress bar BAR
#define LUAT_USE_LVGL_SPINBOX   //Digital adjustment box no dependencies
#define LUAT_USE_LVGL_SWITCH   //Switch depends on slider SLIDER
#define LUAT_USE_LVGL_TEXTAREA   //Text box depends on label LABEL page PAGE
#define LUAT_USE_LVGL_TABLE   //Table depends on label LABEL
#define LUAT_USE_LVGL_TABVIEW   //Page dependent page PAGE picture button IMGBTN
#define LUAT_USE_LVGL_TILEVIEW   //Tile view depends on page PAGE
#define LUAT_USE_LVGL_WIN   //Window dependent container CONT button BTN label LABEL picture IMG page PAGE

//-------------------------------------------------
//Advanced configuration
//-------------------------------------------------


// Enable 64-bit virtual machine
// #define LUAT_CONF_VM_64bit

// LITE mode, digital transmission firmware configuration:
// 1. In order to differentiate the packet size, close some libraries
// 2. The script area and script OTA area are set to 448 + 284 layout, the same as V1103
// #define LUAT_EC618_LITE_MODE
// #define LUAT_SCRIPT_SIZE 448
// #define LUAT_SCRIPT_OTA_SIZE 284


#ifndef LUAT_SCRIPT_SIZE

#define LUAT_SCRIPT_SIZE 128
#define LUAT_SCRIPT_OTA_SIZE 96

// Suitable for extreme operations of tts_onchip, TTS is supported without external SPI FLASH.
// Be sure to read the description of LUAT_USE_TTS_ONCHIP
// #define LUAT_SCRIPT_SIZE 64
// #define LUAT_SCRIPT_OTA_SIZE 48
// #define LUAT_SCRIPT_SIZE 32
// #define LUAT_SCRIPT_OTA_SIZE 24


// Encryption algorithm related
// #define LUAT_CONF_TLS_DISABLE_ECP_ECDHE 1
// #define LUAT_CONF_TLS_DTLS_DISABLE 1
// #define LUAT_CONF_TLS_DISABLE_NC 1
#endif

//-------------------------------------------------------------------------------
//<-- custom
//------------------------------------------------------------------------------


// The following options can only be modified by developers. General users should not modify them by themselves.
//-----------------------------
// Memory configuration, default 200k, 128 ~ 324k adjustable. 324k is the limit value, audio cannot be used, and the number of TLS connections is limited to no more than 2
#ifdef LUAT_HEAP_SIZE_324K
#define LUAT_HEAP_SIZE (324*1024)
#endif
#ifdef LUAT_HEAP_SIZE_300K
#define LUAT_HEAP_SIZE (300*1024)
#endif
#ifdef LUAT_HEAP_SIZE_200K
#define LUAT_HEAP_SIZE (200*1024)
#endif
// Generally no modification is required. If you do not need to use SSL/TLS/TTS, you can increase it appropriately, but it should not exceed 256k
#ifndef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE (256*1024)
#endif

#ifdef LUAT_EC618_RNDIS_ENABLED
#undef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE (160*1024)
#endif
//-----------------------------

//Switch UART0 to user mode, the default is UNILOG mode
// Using UART0, the log will completely rely on USB output. If the USB is not pulled out or is invalid, the underlying log will not be obtained.
// This feature is only available to users who fully understand the risks.
// #define LUAT_UART0_FORCE_USER     1
// #define LUAT_UART0_FORCE_ALT1     1
// #define LUAT_UART0_LOG_BR_12M     1

#define LUAT_GPIO_PIN_MAX 36
#define LUAT__UART_TX_NEED_WAIT_DONE
// Memory optimization: Reduce memory consumption, which will slightly reduce performance
#define LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP 1

//----------------------------------
// Use VFS (virtual file system) and built-in library files, which must be enabled
#define LUAT_USE_FS_VFS 1
#define LUAT_USE_VFS_INLINE_LIB 1
#define LUA_USE_VFS_FILENAME_OFFSET 1
//----------------------------------

#define LV_DISP_DEF_REFR_PERIOD 30
#define LUAT_LV_DEBUG 0

#define LV_MEM_CUSTOM 1

#define LUAT_USE_LVGL_INDEV 1 // input device

#define LV_HOR_RES_MAX          (160)
#define LV_VER_RES_MAX          (80)
#define LV_COLOR_DEPTH          16

#define LV_COLOR_16_SWAP   1
#define __LVGL_SLEEP_ENABLE__

#undef LV_DISP_DEF_REFR_PERIOD
#define LV_DISP_DEF_REFR_PERIOD g_lvgl_flash_time

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE  "common_api.h"         /*Header for the system time function*/
#define LV_TICK_CUSTOM_SYS_TIME_EXPR ((uint32_t)GetSysTickMS())     /*Expression evaluating to current system time in ms*/

#define LUAT_LOG_NO_NEWLINE
#define __LUATOS_TICK_64BIT__

#define LUAT_RET int
#define LUAT_RT_RET_TYPE	void
#define LUAT_RT_CB_PARAM void *param

#define LUAT_USE_NETWORK 1
#define LUAT_USE_TLS 1
#define LUAT_USE_LWIP 1
#define LUAT_USE_DNS 1
#define LUAT_USE_ERR_DUMP 1
#define LUAT_USE_DHCP  1
#define LUAT_USE_ERRDUMP 1
#define LUAT_USE_FOTA 1
#define LUAT_SUPPORT_AMR 1
#define LUAT_USE_MOBILE 1
#define LUAT_USE_WLAN_SCANONLY 1
#define LUAT_USE_SNTP 1
//Macros not used yet, but they have to be written here
#define LUAT_USE_I2S

#ifndef LUAT_USE_HMETA
#define LUAT_USE_HMETA 1
#endif

#ifdef LUAT_EC618_LITE_MODE
#undef LUAT_USE_LCD
#undef LUAT_USE_TJPGD
#undef LUAT_USE_LORA
#undef LUAT_USE_IR
#undef USE_U8G2_OPPOSANSM_ENGLISH
#undef LUAT_USE_EINK
#undef LUAT_USE_FONTS
#undef LUAT_USE_LVGL
#undef LUAT_USE_DISP
#undef LUAT_USE_GTFONT
#undef LUAT_USE_FATFS
#undef LUAT_USE_I2CTOOLS
#undef LUAT_USE_SFUD
#undef LUAT_USE_SFD
#undef LUAT_USE_W5500
#undef LUAT_USE_SOFT_UART
#undef LUAT_USE_MINIZ
#undef LUAT_USE_OTP
#undef LUAT_SUPPORT_AMR

#ifdef LUAT_USE_TTS
#undef LUAT_USE_LCD
#undef LUAT_USE_TJPGD
#undef LUAT_USE_LORA
#undef LUAT_USE_IR
#undef USE_U8G2_OPPOSANSM_ENGLISH
#undef LUAT_USE_EINK
#undef LUAT_USE_FONTS
#undef LUAT_USE_LVGL
#undef LUAT_USE_DISP
#define LUAT_USE_TTS_ONLY 1
#endif

#endif // LUAT_EC618_LITE_MODE

// TTS related
#ifdef LUAT_USE_TTS

#ifndef LUAT_USE_TTS_8K
#define LUAT_USE_TTS_16K 1
#endif // LUAT_USE_TTS_8K

#ifndef LUAT_USE_MEDIA
#define LUAT_USE_MEDIA 1
#endif

#ifdef LUAT_USE_TTS_ONCHIP
#ifdef LUAT_EC618_LITE_MODE
#ifdef LUAT_USE_WEBSOCKET
#undef LUAT_USE_WEBSOCKET
#endif
#ifdef LUAT_USE_FTP
#undef LUAT_USE_FTP
#endif
#endif
#ifdef LUAT_USE_REPL
#undef LUAT_USE_REPL
#endif
#ifdef LUAT_USE_YMODEM
#undef LUAT_USE_YMODEM
#endif
#undef LUAT_USE_SFUD
#else
#ifndef LUAT_USE_SFUD
#define LUAT_USE_SFUD  1
#endif // LUAT_USE_SFUD
#endif // LUAT_USE_TTS_ONCHIP

#endif // LUAT_USE_TTS

#define LUAT_WS2812B_MAX_CNT	(8)

#ifdef LUAT_USE_TTS_ONCHIP
#ifndef LUAT_USE_TLS_DISABLE
#undef LUAT_SCRIPT_SIZE
#undef LUAT_SCRIPT_OTA_SIZE
#define LUAT_SCRIPT_SIZE 64
#define LUAT_SCRIPT_OTA_SIZE 48
#endif
#endif

#define LUA_SCRIPT_ADDR (FLASH_FOTA_REGION_START - (LUAT_SCRIPT_SIZE + LUAT_SCRIPT_OTA_SIZE) * 1024)
#define LUA_SCRIPT_OTA_ADDR FLASH_FOTA_REGION_START - (LUAT_SCRIPT_OTA_SIZE * 1024)

#define __LUAT_C_CODE_IN_RAM__ __attribute__((__section__(".platFMRamcode")))

#ifdef LUAT_USE_SHELL
#undef LUAT_USE_REPL
#endif

#ifdef LUAT_USE_TLS_DISABLE
#undef LUAT_USE_TLS
#endif


#endif
