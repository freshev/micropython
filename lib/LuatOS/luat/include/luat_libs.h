
#ifndef LUAT_LIBS_H
#define LUAT_LIBS_H
#include "lua.h"
#include "lauxlib.h"


/** sys library, reserved, actually unavailable state*/
LUAMOD_API int luaopen_sys( lua_State *L );
/** rtos library*/
LUAMOD_API int luaopen_rtos( lua_State *L );
/** timer library*/
LUAMOD_API int luaopen_timer( lua_State *L );
/** msgbus library, reserved, actually unavailable state*/
// LUAMOD_API int luaopen_msgbus( lua_State *L );
/** gpio library*/
LUAMOD_API int luaopen_gpio( lua_State *L );
/** adc library*/
LUAMOD_API int luaopen_adc( lua_State *L );
/** pwm library*/
LUAMOD_API int luaopen_pwm( lua_State *L );
/** uart library*/
LUAMOD_API int luaopen_uart( lua_State *L );
/** pm library*/
LUAMOD_API int luaopen_pm( lua_State *L );
/** fs library*/
LUAMOD_API int luaopen_fs( lua_State *L );
/** wlan library*/
LUAMOD_API int luaopen_wlan( lua_State *L );
/** socket library*/
LUAMOD_API int luaopen_socket( lua_State *L );
/** sensor library*/
LUAMOD_API int luaopen_sensor( lua_State *L );
/** log library*/
LUAMOD_API int luaopen_log( lua_State *L );
/** json library*/
LUAMOD_API int luaopen_cjson( lua_State *L );
/** i2c library*/
LUAMOD_API int luaopen_i2c( lua_State *L );
/** spi library*/
LUAMOD_API int luaopen_spi( lua_State *L );
/** disp library*/
LUAMOD_API int luaopen_disp( lua_State *L );
/** u8g2 library*/
LUAMOD_API int luaopen_u8g2( lua_State *L );
/** sfud library*/
LUAMOD_API int luaopen_sfud( lua_State *L );
/** little_flash库*/
LUAMOD_API int luaopen_little_flash( lua_State *L );
/** utest library*/
// LUAMOD_API int luaopen_utest( lua_State *L );
/** mqtt library*/
LUAMOD_API int luaopen_mqtt( lua_State *L );
/** http library*/
LUAMOD_API int luaopen_http( lua_State *L );
/** pack library*/
LUAMOD_API int luaopen_pack( lua_State *L );
/** mqttcore library*/
LUAMOD_API int luaopen_mqttcore( lua_State *L );
/** crypto library*/
LUAMOD_API int luaopen_crypto( lua_State *L );
LUAMOD_API int luaopen_gmssl( lua_State *L );
/** Power consumption adjustment*/
LUAMOD_API int luaopen_pm( lua_State *L);
LUAMOD_API int luaopen_m2m( lua_State *L);
LUAMOD_API int luaopen_libcoap( lua_State *L);
LUAMOD_API int luaopen_lpmem( lua_State *L);
LUAMOD_API int luaopen_ctiot( lua_State *L);
LUAMOD_API int luaopen_iconv(lua_State *L);
LUAMOD_API int luaopen_nbiot( lua_State *L );
LUAMOD_API int luaopen_libgnss( lua_State *L ) ;
LUAMOD_API int luaopen_fatfs( lua_State *L );
LUAMOD_API int luaopen_eink( lua_State *L);
LUAMOD_API int luaopen_dbg( lua_State *L );
/** zbuff library*/
LUAMOD_API int luaopen_zbuff( lua_State *L );

LUAMOD_API int luaopen_sfd( lua_State *L );
LUAMOD_API int luaopen_lfs2( lua_State *L );
LUAMOD_API int luaopen_lvgl( lua_State *L );

/** ir library, dependent on gpio library*/
LUAMOD_API int luaopen_ir( lua_State *L );

LUAMOD_API int luaopen_lcd( lua_State *L );
LUAMOD_API int luaopen_lwip( lua_State *L );

LUAMOD_API int luaopen_wdt( lua_State *L );
LUAMOD_API int luaopen_mcu( lua_State *L );
LUAMOD_API int luaopen_hwtimer( lua_State *L );
LUAMOD_API int luaopen_rtc( lua_State *L );
LUAMOD_API int luaopen_sdio( lua_State *L );

LUAMOD_API int luaopen_statem( lua_State *L );
LUAMOD_API int luaopen_vmx( lua_State *L );
LUAMOD_API int luaopen_lcdseg( lua_State *L );

LUAMOD_API int luaopen_fdb( lua_State *L );

LUAMOD_API int luaopen_keyboard( lua_State *L );
LUAMOD_API int luaopen_coremark( lua_State *L );

LUAMOD_API int luaopen_fonts( lua_State *L );
LUAMOD_API int luaopen_gtfont( lua_State *L );

LUAMOD_API int luaopen_pin( lua_State *L );
LUAMOD_API int luaopen_dac( lua_State *L );
LUAMOD_API int luaopen_otp( lua_State *L );
LUAMOD_API int luaopen_mlx90640( lua_State *L );
LUAMOD_API int luaopen_zlib( lua_State *L );
LUAMOD_API int luaopen_camera( lua_State *L );
LUAMOD_API int luaopen_multimedia_audio( lua_State *L );
LUAMOD_API int luaopen_multimedia_video( lua_State *L );
LUAMOD_API int luaopen_multimedia_codec( lua_State *L );
LUAMOD_API int luaopen_luf( lua_State *L );

LUAMOD_API int luaopen_touchkey(lua_State *L);
LUAMOD_API int luaopen_softkb( lua_State *L );
LUAMOD_API int luaopen_nes( lua_State *L );

LUAMOD_API int luaopen_io_queue( lua_State *L );
LUAMOD_API int luaopen_ymodem( lua_State *L );
LUAMOD_API int luaopen_w5500( lua_State *L );
LUAMOD_API int luaopen_socket_adapter( lua_State *L );

LUAMOD_API int luaopen_airui( lua_State *L );
LUAMOD_API int luaopen_fota( lua_State *L );
LUAMOD_API int luaopen_i2s( lua_State *L );
LUAMOD_API int luaopen_lora( lua_State *L );
LUAMOD_API int luaopen_lora2( lua_State *L );
LUAMOD_API int luaopen_iotauth( lua_State *L );
LUAMOD_API int luaopen_ufont( lua_State *L );
LUAMOD_API int luaopen_miniz( lua_State *L );
LUAMOD_API int luaopen_mobile( lua_State *L );

LUAMOD_API int luaopen_protobuf( lua_State *L );

LUAMOD_API int luaopen_httpsrv( lua_State *L );
LUAMOD_API int luaopen_rsa( lua_State *L );

LUAMOD_API int luaopen_websocket( lua_State *L );


LUAMOD_API int luaopen_ftp( lua_State *L );
LUAMOD_API int luaopen_hmeta( lua_State *L );
LUAMOD_API int luaopen_sms( lua_State *L );
LUAMOD_API int luaopen_errdump( lua_State *L );
LUAMOD_API int luaopen_profiler( lua_State *L );
LUAMOD_API int luaopen_fskv( lua_State *L );
LUAMOD_API int luaopen_max30102( lua_State *L );

LUAMOD_API int luaopen_bit64( lua_State *L );

LUAMOD_API int luaopen_repl( lua_State *L );


LUAMOD_API int luaopen_fastlz( lua_State *L );

LUAMOD_API int luaopen_usernet( lua_State *L );

LUAMOD_API int luaopen_ercoap( lua_State *L );

LUAMOD_API int luaopen_sqlite3( lua_State *L );

LUAMOD_API int luaopen_ws2812( lua_State *L );

LUAMOD_API int luaopen_onewire( lua_State *L );

// Ant chain
LUAMOD_API int luaopen_antbot( lua_State *L );

// xxtea encryption and decryption, the strength is actually very low
LUAMOD_API int luaopen_xxtea( lua_State *L );

// phone function
LUAMOD_API int luaopen_cc( lua_State *L );

//User-side LWIP integration, used to connect to third-party network equipment such as Ethernet, WiFi, etc., usually SPI or SDIO protocol
LUAMOD_API int luaopen_ulwip( lua_State *L );

// json parsing library based on real cjson, unfinished
LUAMOD_API int luaopen_json2( lua_State *L );

// SPI slave
LUAMOD_API int luaopen_spislave( lua_State *L );

//WLAN naked data sending and receiving
LUAMOD_API int luaopen_wlan_raw(lua_State *L);

//LCD screen driver
LUAMOD_API int luaopen_ht1621(lua_State *L);

// NAPT
LUAMOD_API int luaopen_napt(lua_State *L);

#endif
