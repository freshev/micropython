
/*@Moduleslcd
@summary lcd driver Modules
@version 1.0
@date 2021.06.16
@demolcd
@tag LUAT_USE_LCD*/
#include "luat_base.h"
#include "luat_lcd.h"
#include "luat_mem.h"
#include "luat_zbuff.h"
#include "luat_fs.h"
#include "luat_gpio.h"

#define LUAT_LOG_TAG "lcd"
#include "luat_log.h"

#include "u8g2.h"
#include "u8g2_luat_fonts.h"
#include "luat_u8g2.h"

#include "qrcodegen.h"

int8_t u8g2_font_decode_get_signed_bits(u8g2_font_decode_t *f, uint8_t cnt);
uint8_t u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t *f, uint8_t cnt);

extern luat_color_t BACK_COLOR , FORE_COLOR ;

extern const luat_lcd_opts_t lcd_opts_custom;

typedef struct lcd_reg {
  const char *name;
  const luat_lcd_opts_t *lcd_opts;
}lcd_reg_t;

static const lcd_reg_t lcd_regs[] = {
  {"custom",  &lcd_opts_custom},   //0 is fixed as the zeroth
  {"st7735",  &lcd_opts_st7735},
  {"st7735v", &lcd_opts_st7735v},
  {"st7735s", &lcd_opts_st7735s},
  {"st7789",  &lcd_opts_st7789},
  {"st7796", &lcd_opts_st7796},
  {"gc9a01",  &lcd_opts_gc9a01},
  {"gc9106l", &lcd_opts_gc9106l},
  {"gc9306x", &lcd_opts_gc9306x},
  {"gc9306",  &lcd_opts_gc9306x},  //gc9306 is an alias of gc9306x
  {"ili9341", &lcd_opts_ili9341},
  {"ili9486", &lcd_opts_ili9486},
  {"nv3037", &lcd_opts_nv3037},
  {"", NULL} //The last one must be an empty string
};


static luat_lcd_conf_t *default_conf = NULL;
// static int dft_conf_lua_ref = 0;

// All drawing-related functions should call this function
static void lcd_auto_flush(luat_lcd_conf_t *conf) {
#ifndef LUAT_USE_LCD_SDL2
  if (conf == NULL || conf->buff == NULL || conf->auto_flush == 0)
    return;
#endif
  luat_lcd_flush(conf);
}

luat_color_t lcd_str_fg_color,lcd_str_bg_color;
luat_lcd_conf_t *l_lcd_get_default_conf(void) {return default_conf;}
LUAT_WEAK void luat_lcd_IF_init(luat_lcd_conf_t* conf){}
LUAT_WEAK int luat_lcd_init_in_service(luat_lcd_conf_t* conf){return -1;}
/*lcd display initialization
@api lcd.init(tp, args, spi_dev, init_in_service)
@string lcd type, currently supported:<br>st7796<br>st7789<br>st7735<br>st7735v<br>st7735s<br>gc9a01<br>gc9106l<br>gc9306x<br>ili9486<br>custom
@table additional parameters, related to the specific device:<br>pin_pwr (backlight) is optional and can not be set<br>port: spi port, such as 0, 1, 2... If it is device mode, it is "device"< br>pin_dc: LCD data/command selection pin<br>pin_rst: LCD reset pin<br>pin_pwr: LCD backlight pin optional, can not be set<br>direction: LCD screen direction 0:0° 1:180° 2:270° 3:90°<br>w: lcd horizontal resolution<br>h: lcd vertical resolution<br>xoffset: x offset (different screen ic and different screen directions will be different)<br>yoffset :y offset (different screen ic and different screen orientation will have differences)<br>direction0: 0° direction command, (different screen ic will have differences)<br>direction90: 90° direction command, (different screen ic will have differences) )<br>direction180: 180° direction command, (different screen ic will have differences)<br>direction270: 270° direction command, (different screen ic will have differences) <br>sleepcmd: sleep command, default 0X10<br> wakecmd: wake-up command, default 0X11 <br>interface_mode lcd mode, default lcd.WIRE_4_BIT_8_INTERFACE_I
@userdata spi device, valid when port = "device"
@boolean allows initialization to run in the lcd service, the default is false
@usage
-- Initialize st7735s of spi0 Note: spi needs to be initialized before LCD initialization
spi_lcd = spi.deviceSetup(0,20,0,0,8,2000000,spi.MSB,1,1)
log.info("lcd.init",
lcd.init("st7735s",{port = "device",pin_dc = 17, pin_pwr = 7,pin_rst = 19,direction = 2,w = 160,h = 80,xoffset = 1,yoffset = 26},spi_lcd) )*/
static int l_lcd_init(lua_State* L) {
    size_t len = 0;
    luat_lcd_conf_t *conf = luat_heap_malloc(sizeof(luat_lcd_conf_t));
    if (conf == NULL) {
      LLOGE("out of system memory!!!");
      return 0;
    }
    if (default_conf != NULL) {
      LLOGD("lcd was inited, skip");
      lua_pushboolean(L, 1);
      return 1;
    }
#if defined LUAT_USE_LCD_SERVICE
    uint8_t init_in_service = 0;
    int ret;
    if (lua_isboolean(L, 4)) {
    	init_in_service = lua_toboolean(L, 4);
    }
#endif
    memset(conf, 0, sizeof(luat_lcd_conf_t)); // Fill with 0 to ensure no dirty data
    conf->lcd_cs_pin = LUAT_GPIO_NONE;
    conf->pin_dc = LUAT_GPIO_NONE;
    conf->pin_pwr = LUAT_GPIO_NONE;
    conf->interface_mode = LUAT_LCD_IM_4_WIRE_8_BIT_INTERFACE_I;
    if (lua_type(L, 3) == LUA_TUSERDATA){
        // If it is SPI Device mode, there may be a possibility that the variable is local and will be GCed at a certain point in time.
        conf->lcd_spi_device = (luat_spi_device_t*)lua_touserdata(L, 3);
        lua_pushvalue(L, 3);
        // Therefore, in addition to direct references, mandatory references are added to avoid GC
        // Given that the LCD is unlikely to be initialized repeatedly, there is no problem with the reference
        conf->lcd_spi_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        conf->port = LUAT_LCD_SPI_DEVICE;
    }
    const char* tp = luaL_checklstring(L, 1, &len);
    int16_t s_index = -1;//Which screen, -1 means no match
    for(int i = 0; i < 100; i++){
        if (strlen(lcd_regs[i].name) == 0)
          break;
        if(strcmp(lcd_regs[i].name,tp) == 0){
            s_index = i;
            break;
        }
    }
    if (s_index != -1) {
        LLOGD("ic support: %s",tp);
        if (lua_gettop(L) > 1) {
            conf->opts = (struct luat_lcd_opts *)lcd_regs[s_index].lcd_opts;
            lua_settop(L, 2); // Discard redundant parameters

            lua_pushstring(L, "port");
            int port = lua_gettable(L, 2);
            if (conf->port == LUAT_LCD_SPI_DEVICE && port ==LUA_TNUMBER) {
              LLOGE("port is not device but find luat_spi_device_t");
              goto end;
            }else if (conf->port != LUAT_LCD_SPI_DEVICE && LUA_TSTRING == port){
              LLOGE("port is device but not find luat_spi_device_t");
              goto end;
            }else if (LUA_TNUMBER == port) {
                conf->port = luaL_checkinteger(L, -1);
            }else if (LUA_TSTRING == port){
                conf->port = LUAT_LCD_SPI_DEVICE;
            }
            lua_pop(L, 1);

            lua_pushstring(L, "pin_dc");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->pin_dc = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "pin_pwr");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->pin_pwr = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "pin_rst");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->pin_rst = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "direction");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->direction = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);
            lua_pushstring(L, "direction0");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->opts->direction0 = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);
            lua_pushstring(L, "direction90");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->opts->direction90 = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);
            lua_pushstring(L, "direction180");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->opts->direction180 = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);
            lua_pushstring(L, "direction270");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->opts->direction270 = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "w");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->w = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);
            lua_pushstring(L, "h");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->h = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);
            conf->buffer_size = (conf->w * conf->h) * 2;

            lua_pushstring(L, "xoffset");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->xoffset = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "yoffset");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->yoffset = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "sleepcmd");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->opts->sleep_cmd = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "wakecmd");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->opts->wakeup_cmd = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "interface_mode");
            if (LUA_TNUMBER == lua_gettable(L, 2)) {
                conf->interface_mode = luaL_checkinteger(L, -1);
            }
            lua_pop(L, 1);

        }
        if (s_index == 0){
            unsigned int cmd = 0;
            lua_pushstring(L, "initcmd");
            lua_gettable(L, 2);
            if (lua_istable(L, -1)) {
              conf->opts->init_cmds_len = lua_rawlen(L, -1);
              conf->opts->init_cmds = luat_heap_malloc( conf->opts->init_cmds_len * sizeof(uint16_t));
              for (size_t i = 1; i <= conf->opts->init_cmds_len; i++){
                  lua_geti(L, -1, i);
                  cmd = luaL_checkinteger(L, -1);
                  conf->opts->init_cmds[i-1] = ((cmd >> 8) & 0xFF00) | (cmd & 0xFF);
                  lua_pop(L, 1);
              }
            }else if(lua_isstring(L, -1)){
              size_t  len;
              const char *fail_name = luaL_checklstring(L, -1, &len);
              FILE* fd = (FILE *)luat_fs_fopen(fail_name, "rb");
              conf->opts->init_cmds_len = 0;
              if (fd){
                  #define INITCMD_BUFF_SIZE 128
                  char init_cmd_buff[INITCMD_BUFF_SIZE] ;
                  conf->opts->init_cmds = luat_heap_malloc(sizeof(uint16_t));
                  while (1) {
                      memset(init_cmd_buff, 0, INITCMD_BUFF_SIZE);
                      int readline_len = luat_fs_readline(init_cmd_buff, INITCMD_BUFF_SIZE-1, fd);
                      if (readline_len < 1)
                          break;
                      if (memcmp(init_cmd_buff, "#", 1)==0){
                          continue;
                      }
                      char *token = strtok(init_cmd_buff, ",");
                      if (sscanf(token,"%x",&cmd) < 1){
                          continue;
                      }
                      conf->opts->init_cmds_len = conf->opts->init_cmds_len + 1;
                      conf->opts->init_cmds = luat_heap_realloc(conf->opts->init_cmds,conf->opts->init_cmds_len * sizeof(uint16_t));
                      conf->opts->init_cmds[conf->opts->init_cmds_len-1]=((cmd >> 8) & 0xFF00) | (cmd & 0xFF);;
                      while( token != NULL ) {
                          token = strtok(NULL, ",");
                          if (sscanf(token,"%x",&cmd) < 1){
                              break;
                          }
                          conf->opts->init_cmds_len = conf->opts->init_cmds_len + 1;
                          conf->opts->init_cmds = luat_heap_realloc(conf->opts->init_cmds,conf->opts->init_cmds_len * sizeof(uint16_t));
                          conf->opts->init_cmds[conf->opts->init_cmds_len-1]=((cmd >> 8) & 0xFF00) | (cmd & 0xFF);;
                      }
                  }
                  conf->opts->init_cmds[conf->opts->init_cmds_len]= 0;
                  luat_fs_fclose(fd);
              }else{
                  LLOGE("init_cmd fail open error");
              }
            }
            lua_pop(L, 1);
        }
        // Automatically flush by default, even if there is no buff
        conf->auto_flush = 1;

#ifdef LUAT_USE_LCD_SDL2
        extern const luat_lcd_opts_t lcd_opts_sdl2;
        conf->opts = &lcd_opts_sdl2;
#endif
        if (conf->port == LUAT_LCD_HW_ID_0) luat_lcd_IF_init(conf);
#if defined LUAT_USE_LCD_SERVICE
        if (init_in_service) {
        	ret = luat_lcd_init_in_service(conf);
        } else {
        	ret = luat_lcd_init(conf);
        }

#else
        int ret = luat_lcd_init(conf);
#endif
        if (ret != 0) {
            LLOGE("lcd init fail %d", ret);
            luat_heap_free(conf);
            lua_pushboolean(L, 0);
            return 0;
        }
        // Initialization OK, configure additional parameters
        default_conf = conf;
        u8g2_SetFont(&(conf->luat_lcd_u8g2), u8g2_font_opposansm12);
        u8g2_SetFontMode(&(conf->luat_lcd_u8g2), 0);
        u8g2_SetFontDirection(&(conf->luat_lcd_u8g2), 0);
        lua_pushboolean(L, 1);
        return 1;
    }
    LLOGE("ic not support: %s",tp);
end:
    lua_pushboolean(L, 0);
    luat_heap_free(conf);
    return 1;
}

/*turn off lcd display
@apilcd.close()
@usage
-- turn off lcd
lcd.close()*/
static int l_lcd_close(lua_State* L) {
    int ret = luat_lcd_close(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Turn on LCD display backlight
@apilcd.on()
@usage
-- Turn on LCD display backlight
lcd.on()*/
static int l_lcd_display_on(lua_State* L) {
    int ret = luat_lcd_display_on(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Turn off lcd display backlight
@apilcd.off()
@usage
-- Turn off the LCD display backlight
lcd.off()*/
static int l_lcd_display_off(lua_State* L) {
    int ret = luat_lcd_display_off(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd sleep
@api lcd.sleep()
@usage
-- lcd sleep
lcd.sleep()*/
static int l_lcd_sleep(lua_State* L) {
    int ret = luat_lcd_sleep(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd wake up
@api lcd.wakeup()
@usage
-- lcd wake up
lcd.wakeup()*/
static int l_lcd_wakeup(lua_State* L) {
    int ret = luat_lcd_wakeup(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd reverse display
@api lcd.invon()
@usage
-- lcd reverse display
lcd.invon()*/
static int l_lcd_inv_on(lua_State* L) {
    int ret = luat_lcd_inv_on(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*LCD reverse display off
@api lcd.invoff()
@usage
--LCD reverse display off
lcd.invoff()*/
static int l_lcd_inv_off(lua_State* L) {
    int ret = luat_lcd_inv_off(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd command
@api lcd.cmd(cmd)
@int cmd
@usage
-- lcd command
lcd.cmd(0x21)*/
static int l_lcd_write_cmd(lua_State* L) {
    int ret = lcd_write_cmd_data(default_conf,(uint8_t)luaL_checkinteger(L, 1), NULL, 0);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*LCD data
@api lcd.data(data)
@int data
@usage
-- lcd data
lcd.data(0x21)*/
static int l_lcd_write_data(lua_State* L) {
    int ret = lcd_write_data(default_conf,(const uint8_t)luaL_checkinteger(L, 1));
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd color settings
@api lcd.setColor(back,fore)
@int background color
@int foreground color
@usage
-- lcd color settings
lcd.setColor(0xFFFF,0x0000)*/
static int l_lcd_set_color(lua_State* L) {
    luat_color_t back,fore;
    back = (luat_color_t)luaL_checkinteger(L, 1);
    fore = (luat_color_t)luaL_checkinteger(L, 2);
    int ret = luat_lcd_set_color(back, fore);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

static int l_lcd_set_direction(lua_State* L) {
    int ret = luat_lcd_set_direction(default_conf, (uint8_t)luaL_checkinteger(L, 1));
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd color fill
@api lcd.draw(x1, y1, x2, y2,color)
@int The X position of the upper left edge.
@int The Y position of the upper left edge.
@int X position of the lower right edge.
@int The Y position of the lower right edge.
@string string or zbuff object
@usage
-- lcd color fill
local buff = zbuff.create({201,1,16},0x001F)
lcd.draw(20,30,220,30,buff)*/
static int l_lcd_draw(lua_State* L) {
    int16_t x1, y1, x2, y2;
    int ret;
    // luat_color_t *color = NULL;
    luat_zbuff_t *buff;
    x1 = luaL_checkinteger(L, 1);
    y1 = luaL_checkinteger(L, 2);
    x2 = luaL_checkinteger(L, 3);
    y2 = luaL_checkinteger(L, 4);
    if (lua_isinteger(L, 5)) {
        // color = (luat_color_t *)luaL_checkstring(L, 5);
        luat_color_t color = (luat_color_t)luaL_checkinteger(L, 1);
        ret = luat_lcd_draw(default_conf, x1, y1, x2, y2, &color);
    }
    else if (lua_isuserdata(L, 5)) {
        buff = luaL_checkudata(L, 5, LUAT_ZBUFF_TYPE);
        luat_color_t *color = (luat_color_t *)buff->addr;
        ret = luat_lcd_draw(default_conf, x1, y1, x2, y2, color);
    }
    else if(lua_isstring(L, 5)) {
        luat_color_t *color = (luat_color_t *)luaL_checkstring(L, 5);
        ret = luat_lcd_draw(default_conf, x1, y1, x2, y2, color);
    }
    else {
        return 0;
    }
    lcd_auto_flush(default_conf);
    // int ret = luat_lcd_draw(default_conf, x1, y1, x2, y2, color);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*clear lcd screen
@api lcd.clear(color)
@int screen color optional parameter, default background color
@usage
-- clear lcd screen
lcd.clear()*/
static int l_lcd_clear(lua_State* L) {
    //size_t len = 0;
    luat_color_t color = BACK_COLOR;
    if (lua_gettop(L) > 0)
        color = (luat_color_t)luaL_checkinteger(L, 1);
    int ret = luat_lcd_clear(default_conf, color);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*lcd color fill
@api lcd.fill(x1, y1, x2, y2,color)
@int The X position of the upper left edge.
@int The Y position of the upper left edge.
@int X position of the lower right edge, not included
@int Y position of the lower right edge, not included
@int painting color optional parameter, default background color
@usage
-- lcd color fill
lcd.fill(20,30,220,30,0x0000)*/
static int l_lcd_draw_fill(lua_State* L) {
    int16_t x1, y1, x2, y2;
    luat_color_t color = BACK_COLOR;
    x1 = luaL_checkinteger(L, 1);
    y1 = luaL_checkinteger(L, 2);
    x2 = luaL_checkinteger(L, 3);
    y2 = luaL_checkinteger(L, 4);
    if (lua_gettop(L) > 4)
        color = (luat_color_t)luaL_checkinteger(L, 5);
    int ret = luat_lcd_draw_fill(default_conf, x1,  y1,  x2,  y2, color);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Draw a point.
@api lcd.drawPoint(x0,y0,color)
@int X position of point.
@int Y position of point.
@int painting color optional parameter, default foreground color
@usage
lcd.drawPoint(20,30,0x001F)*/
static int l_lcd_draw_point(lua_State* L) {
    int16_t x, y;
    luat_color_t color = FORE_COLOR;
    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    if (lua_gettop(L) > 2)
        color = (luat_color_t)luaL_checkinteger(L, 3);
    int ret = luat_lcd_draw_point(default_conf, x, y, color);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Draw a line between two points.
@api lcd.drawLine(x0,y0,x1,y1,color)
@int X position of the first point.
@int Y position of the first point.
@int X position of the second point.
@int Y position of the second point.
@int painting color optional parameter, default foreground color
@usage
lcd.drawLine(20,30,220,30,0x001F)*/
static int l_lcd_draw_line(lua_State* L) {
    int16_t x1, y1, x2, y2;
    luat_color_t color = FORE_COLOR;
    x1 = luaL_checkinteger(L, 1);
    y1 = luaL_checkinteger(L, 2);
    x2 = luaL_checkinteger(L, 3);
    y2 = luaL_checkinteger(L, 4);
    if (lua_gettop(L) > 4)
        color = (luat_color_t)luaL_checkinteger(L, 5);
    int ret = luat_lcd_draw_line(default_conf, x1,  y1,  x2,  y2, color);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Draw a box starting from x/y position (top left edge)
@api lcd.drawRectangle(x0,y0,x1,y1,color)
@int The X position of the upper left edge.
@int The Y position of the upper left edge.
@int X position of the lower right edge.
@int The Y position of the lower right edge.
@int painting color optional parameter, default foreground color
@usage
lcd.drawRectangle(20,40,220,80,0x001F)*/
static int l_lcd_draw_rectangle(lua_State* L) {
    int16_t x1, y1, x2, y2;
    luat_color_t color = FORE_COLOR;
    x1 = luaL_checkinteger(L, 1);
    y1 = luaL_checkinteger(L, 2);
    x2 = luaL_checkinteger(L, 3);
    y2 = luaL_checkinteger(L, 4);
    if (lua_gettop(L) > 4)
        color = (luat_color_t)luaL_checkinteger(L, 5);
    int ret = luat_lcd_draw_rectangle(default_conf, x1,  y1,  x2,  y2, color);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Draw a circle starting from x/y position (center of circle)
@api lcd.drawCircle(x0,y0,r,color)
@int X position of the center of the circle.
@int Y position of circle center.
@int radius.
@int painting color optional parameter, default foreground color
@usage
lcd.drawCircle(120,120,20,0x001F)*/
static int l_lcd_draw_circle(lua_State* L) {
    int16_t x0, y0, r;
    luat_color_t color = FORE_COLOR;
    x0 = luaL_checkinteger(L, 1);
    y0 = luaL_checkinteger(L, 2);
    r = luaL_checkinteger(L, 3);
    if (lua_gettop(L) > 3)
        color = (luat_color_t)luaL_checkinteger(L, 4);
    int ret = luat_lcd_draw_circle(default_conf, x0,  y0,  r, color);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Buffer drawing QRCode
@api lcd.drawQrcode(x, y, str, size)
@int x coordinate
@int y coordinate
@string The content of the QR code
@int Display size (note: the QR code generation size is related to the content to be displayed and the error correction level. The generated version is an indefinite size of 1-40 (corresponding to 21x21 - 177x177). If it is different from the set size, it will automatically be in the middle of the specified area. Display the QR code. If the QR code is not displayed, please check the log prompt)
@return nil no return value*/
static int l_lcd_drawQrcode(lua_State *L)
{
    size_t len;
    int x           = luaL_checkinteger(L, 1);
    int y           = luaL_checkinteger(L, 2);
    const char* text = luaL_checklstring(L, 3, &len);
    int size        = luaL_checkinteger(L, 4);
    uint8_t *qrcode = luat_heap_malloc(qrcodegen_BUFFER_LEN_MAX);
    uint8_t *tempBuffer = luat_heap_malloc(qrcodegen_BUFFER_LEN_MAX);
    if (qrcode == NULL || tempBuffer == NULL) {
        if (qrcode)
            luat_heap_free(qrcode);
        if (tempBuffer)
            luat_heap_free(tempBuffer);
        LLOGE("qrcode out of memory");
        return 0;
    }
    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok){
        int qr_size = qrcodegen_getSize(qrcode);
        if (size < qr_size){
            LLOGE("size must be greater than qr_size %d",qr_size);
            goto end;
        }
        int scale = size / qr_size ;
        if (!scale)scale = 1;
        int margin = (size - qr_size * scale) / 2;
        luat_lcd_draw_fill(default_conf,x,y,x+size,y+size,BACK_COLOR);
        x+=margin;
        y+=margin;
        for (int j = 0; j < qr_size; j++) {
            for (int i = 0; i < qr_size; i++) {
                if (qrcodegen_getModule(qrcode, i, j))
                    luat_lcd_draw_fill(default_conf,x+i*scale,y+j*scale,x+(i+1)*scale,y+(j+1)*scale,FORE_COLOR);
            }
        }
    }else{
        LLOGE("qrcodegen_encodeText false");
    }
end:
    if (qrcode)
        luat_heap_free(qrcode);
    if (tempBuffer)
        luat_heap_free(tempBuffer);
    lcd_auto_flush(default_conf);
    return 0;
}

static uint8_t utf8_state;
static uint16_t encoding;
static uint16_t utf8_next(uint8_t b)
{
  if ( b == 0 )  /* '\n' terminates the string to support the string list procedures */
    return 0x0ffff; /* end of string detected, pending UTF8 is discarded */
  if ( utf8_state == 0 )
  {
    if ( b >= 0xfc )  /* 6 byte sequence */
    {
      utf8_state = 5;
      b &= 1;
    }
    else if ( b >= 0xf8 )
    {
      utf8_state = 4;
      b &= 3;
    }
    else if ( b >= 0xf0 )
    {
      utf8_state = 3;
      b &= 7;
    }
    else if ( b >= 0xe0 )
    {
      utf8_state = 2;
      b &= 15;
    }
    else if ( b >= 0xc0 )
    {
      utf8_state = 1;
      b &= 0x01f;
    }
    else
    {
      /* do nothing, just use the value as encoding */
      return b;
    }
    encoding = b;
    return 0x0fffe;
  }
  else
  {
    utf8_state--;
    /* The case b < 0x080 (an illegal UTF8 encoding) is not checked here. */
    encoding<<=6;
    b &= 0x03f;
    encoding |= b;
    if ( utf8_state != 0 )
      return 0x0fffe; /* nothing to do yet */
  }
  return encoding;
}

static void u8g2_draw_hv_line(u8g2_t *u8g2, int16_t x, int16_t y, int16_t len, uint8_t dir, uint16_t color){
  switch(dir)
  {
    case 0:
      luat_lcd_draw_hline(default_conf,x,y,len,color);
      break;
    case 1:
      luat_lcd_draw_vline(default_conf,x,y,len,color);
      break;
    case 2:
        luat_lcd_draw_hline(default_conf,x-len+1,y,len,color);
      break;
    case 3:
      luat_lcd_draw_vline(default_conf,x,y-len+1,len,color);
      break;
  }
}

static void u8g2_font_decode_len(u8g2_t *u8g2, uint8_t len, uint8_t is_foreground){
  uint8_t cnt;  /* total number of remaining pixels, which have to be drawn */
  uint8_t rem;  /* remaining pixel to the right edge of the glyph */
  uint8_t current;  /* number of pixels, which need to be drawn for the draw procedure */
    /* current is either equal to cnt or equal to rem */
  /* local coordinates of the glyph */
  uint8_t lx,ly;
  /* target position on the screen */
  int16_t x, y;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  cnt = len;
  /* get the local position */
  lx = decode->x;
  ly = decode->y;
  for(;;){
    /* calculate the number of pixel to the right edge of the glyph */
    rem = decode->glyph_width;
    rem -= lx;
    /* calculate how many pixel to draw. This is either to the right edge */
    /* or lesser, if not enough pixel are left */
    current = rem;
    if ( cnt < rem )
      current = cnt;
    /* now draw the line, but apply the rotation around the glyph target position */
    //u8g2_font_decode_draw_pixel(u8g2, lx,ly,current, is_foreground);
    // printf("lx:%d,ly:%d,current:%d, is_foreground:%d \r\n",lx,ly,current, is_foreground);
    /* get target position */
    x = decode->target_x;
    y = decode->target_y;
    /* apply rotation */
    x = u8g2_add_vector_x(x, lx, ly, decode->dir);
    y = u8g2_add_vector_y(y, lx, ly, decode->dir);
    /* draw foreground and background (if required) */
    if ( current > 0 )		/* avoid drawing zero length lines, issue #4 */
    {
      if ( is_foreground )
      {
	    u8g2_draw_hv_line(u8g2, x, y, current, decode->dir, lcd_str_fg_color);
      }
      // else if ( decode->is_transparent == 0 )
      // {
	    // u8g2_draw_hv_line(u8g2, x, y, current, decode->dir, lcd_str_bg_color);
      // }
    }
    /* check, whether the end of the run length code has been reached */
    if ( cnt < rem )
      break;
    cnt -= rem;
    lx = 0;
    ly++;
  }
  lx += cnt;
  decode->x = lx;
  decode->y = ly;
}
static void u8g2_font_setup_decode(u8g2_t *u8g2, const uint8_t *glyph_data)
{
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  decode->decode_ptr = glyph_data;
  decode->decode_bit_pos = 0;

  /* 8 Nov 2015, this is already done in the glyph data search procedure */
  /*
  decode->decode_ptr += 1;
  decode->decode_ptr += 1;
  */

  decode->glyph_width = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
  decode->glyph_height = u8g2_font_decode_get_unsigned_bits(decode,u8g2->font_info.bits_per_char_height);

}
static int8_t u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data){
  uint8_t a, b;
  int8_t x, y;
  int8_t d;
  int8_t h;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  u8g2_font_setup_decode(u8g2, glyph_data);
  h = u8g2->font_decode.glyph_height;
  x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
  y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
  d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);

  if ( decode->glyph_width > 0 )
  {
    decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h+y), decode->dir);
    decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h+y), decode->dir);
    //u8g2_add_vector(&(decode->target_x), &(decode->target_y), x, -(h+y), decode->dir);
    /* reset local x/y position */
    decode->x = 0;
    decode->y = 0;
    /* decode glyph */
    for(;;){
      a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
      b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
      do{
        u8g2_font_decode_len(u8g2, a, 0);
        u8g2_font_decode_len(u8g2, b, 1);
      } while( u8g2_font_decode_get_unsigned_bits(decode, 1) != 0 );
      if ( decode->y >= h )
        break;
    }
  }
  return d;
}
const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding);
static int16_t u8g2_font_draw_glyph(u8g2_t *u8g2, int16_t x, int16_t y, uint16_t encoding){
  int16_t dx = 0;
  u8g2->font_decode.target_x = x;
  u8g2->font_decode.target_y = y;
  const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
  if ( glyph_data != NULL ){
    dx = u8g2_font_decode_glyph(u8g2, glyph_data);
  }
  return dx;
}
extern void luat_u8g2_set_ascii_indentation(uint8_t value);
/*Set font
@api lcd.setFont(font, indentation)
@int font lcd.font_XXX Please refer to the constant table
@int indentation, the right side of the ascii character in the monospaced font is indented 0~127 pixels. The ascii characters in the monospaced font may have a large blank space on the right side. The user can choose to delete the part. If left blank or exceeds 127, the right half will be deleted directly. Non-monospaced fonts will be invalid.
@usage
--Set as font, valid for subsequent drawStr, must be set before calling lcd.drawStr

-- If it prompts "only font pointer is allow", it means that the current firmware does not contain the corresponding font, and you can use the cloud compilation service to customize it for free.
-- Cloud compilation documentation: https://wiki.luatos.com/develop/compile/Cloud_compilation.html

--The default fonts of the lcd library all start with lcd.font_
lcd.setFont(lcd.font_opposansm12)
lcd.drawStr(40,10,"drawStr")
sys.wait(2000)
lcd.setFont(lcd.font_opposansm12_chinese) -- For specific values, please refer to the constant table of the api document
lcd.drawStr(40,40,"drawStr test")*/
static int l_lcd_set_font(lua_State *L) {
    if (!lua_islightuserdata(L, 1)) {
      LLOGE("only font pointer is allow");
      return 0;
    }
    const uint8_t *ptr = (const uint8_t *)lua_touserdata(L, 1);
    if (ptr == NULL) {
      LLOGE("only font pointer is allow");
      return 0;
    }
    luat_u8g2_set_ascii_indentation(0xff);
    u8g2_SetFont(&(default_conf->luat_lcd_u8g2), ptr);
    if (lua_isinteger(L, 2)) {
        int indentation = luaL_checkinteger(L, 2);
    	  luat_u8g2_set_ascii_indentation(indentation);
    }
    lua_pushboolean(L, 1);
    return 1;
}

/*display string
@api lcd.drawStr(x,y,str,fg_color)
@int x abscissa
@int y vertical coordinate Note: This (x, y) is the starting coordinate of the lower left
@string str file content
@int fg_color str color Note: This parameter is optional. If not filled in, the previously set color will be used. Only the font part will be drawn, and the background needs to be cleared by yourself.
@usage
-- Set to Chinese font before displaying, which will be effective for subsequent drawStr
lcd.setFont(lcd.font_opposansm12)
lcd.drawStr(40,10,"drawStr")
sys.wait(2000)
lcd.setFont(lcd.font_opposansm16_chinese)
lcd.drawStr(40,40,"drawStr test")*/
static int l_lcd_draw_str(lua_State* L) {
    int x, y;
    size_t sz;
    const uint8_t* data;
    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    data = (const uint8_t*)luaL_checklstring(L, 3, &sz);
    lcd_str_fg_color = (luat_color_t)luaL_optinteger(L, 4,FORE_COLOR);
    // lcd_str_bg_color = (uint32_t)luaL_optinteger(L, 5,BACK_COLOR);
    if (sz == 0)
        return 0;
    uint16_t e;
    int16_t delta;
    utf8_state = 0;

    for(;;){
        e = utf8_next((uint8_t)*data);
        if ( e == 0x0ffff )
        break;
        data++;
        if ( e != 0x0fffe ){
        delta = u8g2_font_draw_glyph(&(default_conf->luat_lcd_u8g2), x, y, e);
        if (e < 0x0080) delta = luat_u8g2_need_ascii_cut(delta);
        switch(default_conf->luat_lcd_u8g2.font_decode.dir){
            case 0:
            x += delta;
            break;
            case 1:
            y += delta;
            break;
            case 2:
            x -= delta;
            break;
            case 3:
            y -= delta;
            break;
        }
        }
    }
    lcd_auto_flush(default_conf);
    return 0;
}

#ifdef LUAT_USE_GTFONT

#include "GT5SLCD2E_1A.h"
extern unsigned int gtfont_draw_w(unsigned char *pBits,unsigned int x,unsigned int y,unsigned int size,unsigned int widt,unsigned int high,int(*point)(void*),void* userdata,int mode);
extern void gtfont_draw_gray_hz(unsigned char *data,unsigned short x,unsigned short y,unsigned short w ,unsigned short h,unsigned char grade, unsigned char HB_par,int(*point)(void*,uint16_t, uint16_t, uint32_t),void* userdata,int mode);

/*Display gb2312 string using gtfont
@api lcd.drawGtfontGb2312(str,size,x,y)
@string str display string
@int size font size (supports 16-192 size fonts)
@int x abscissa
@int y vertical coordinate
@usage
-- Note that gtfont is additional font chip hardware and needs to be plugged into the SPI bus to call this function.
lcd.drawGtfontGb2312("Ahhhhh",32,0,0)*/
/*Display gbk string using gtfont
@api lcd.drawGtfontGbk(str,size,x,y)
@string str display string
@int size font size (supports 16-192 size fonts)
@int x abscissa
@int y vertical coordinate
@usage
-- Note that gtfont is additional font chip hardware and needs to be plugged into the SPI bus to call this function.
lcd.drawGtfontGbk("Ahhhhh",32,0,0)*/
static int l_lcd_draw_gtfont_gbk(lua_State *L) {
    unsigned char buf[128];
	size_t len;
	int i = 0;
	uint8_t strhigh,strlow ;
	uint16_t str;
  const char *fontCode = luaL_checklstring(L, 1,&len);
  unsigned char size = luaL_checkinteger(L, 2);
	int x = luaL_checkinteger(L, 3);
	int y = luaL_checkinteger(L, 4);
  lcd_str_fg_color = (luat_color_t)luaL_optinteger(L, 5,FORE_COLOR);
  // lcd_str_bg_color = (luat_color_t)luaL_optinteger(L, 6,BACK_COLOR);
	while ( i < len){
		strhigh = *fontCode;
		fontCode++;
		strlow = *fontCode;
		str = (strhigh<<8)|strlow;
		fontCode++;
		int font_size = get_font(buf, str<0x80?VEC_HZ_ASCII_STY:VEC_BLACK_STY, str, size, size, size);
    if(font_size == 0){
      LLOGW("get gtfont error size:%d font_size:%d",size,font_size);
      return 0;
    }
		gtfont_draw_w(buf , x ,y , font_size,size , size,luat_lcd_draw_point,default_conf,0);
		x+=size;
		i+=2;
	}
    lcd_auto_flush(default_conf);
    return 0;
}

/*Display gb2312 string using gtfont grayscale
@api lcd.drawGtfontGb2312Gray(str,size,gray,x,y)
@string str display string
@int size font size (supports 16-192 size fonts)
@int gray grayscale [1st level/2nd level/3rd level/4th level]
@int x abscissa
@int y vertical coordinate
@usage
-- Note that gtfont is additional font chip hardware and needs to be plugged into the SPI bus to call this function.
lcd.drawGtfontGb2312Gray("Ahhhhh",32,4,0,40)*/
/*Display gbk string using gtfont grayscale
@api lcd.drawGtfontGbkGray(str,size,gray,x,y)
@string str display string
@int size font size (supports 16-192 size fonts)
@int gray grayscale [1st level/2nd level/3rd level/4th level]
@int x abscissa
@int y vertical coordinate
@usage
-- Note that gtfont is additional font chip hardware and needs to be plugged into the SPI bus to call this function.
lcd.drawGtfontGbkGray("Ahhhhh",32,4,0,40)*/
static int l_lcd_draw_gtfont_gbk_gray(lua_State* L) {
	unsigned char buf[2048];
	size_t len;
	int i = 0;
	uint8_t strhigh,strlow ;
	uint16_t str;
  const char *fontCode = luaL_checklstring(L, 1,&len);
  unsigned char size = luaL_checkinteger(L, 2);
	unsigned char font_g = luaL_checkinteger(L, 3);
	int x = luaL_checkinteger(L, 4);
	int y = luaL_checkinteger(L, 5);
  lcd_str_fg_color = (luat_color_t)luaL_optinteger(L, 6,FORE_COLOR);
  // lcd_str_bg_color = (luat_color_t)luaL_optinteger(L, 7,BACK_COLOR);
	while ( i < len){
		strhigh = *fontCode;
		fontCode++;
		strlow = *fontCode;
		str = (strhigh<<8)|strlow;
		fontCode++;
		int font_size = get_font(buf, str<0x80?VEC_HZ_ASCII_STY:VEC_BLACK_STY, str, size*font_g, size*font_g, size*font_g);
    if(font_size != size*font_g){
      LLOGW("get gtfont error size:%d font_size:%d",size,font_size);
      return 0;
    }
		Gray_Process(buf,size,size,font_g);
		gtfont_draw_gray_hz(buf, x, y, size , size, font_g, 1,luat_lcd_draw_point,default_conf,0);
		x+=size;
		i+=2;
	}
    lcd_auto_flush(default_conf);
    return 0;
}

#ifdef LUAT_USE_GTFONT_UTF8
extern unsigned short unicodetogb2312 ( unsigned short	chr);

/*Display UTF8 string using gtfont
@api lcd.drawGtfontUtf8(str,size,x,y)
@string str display string
@int size font size (supports 16-192 size fonts)
@int x abscissa
@int y vertical coordinate
@usage
lcd.drawGtfontUtf8("Ahhhhh",32,0,0)*/
static int l_lcd_draw_gtfont_utf8(lua_State *L) {
    unsigned char buf[128] = {0};
    size_t len;
    int i = 0;
    uint8_t strhigh,strlow ;
    uint16_t e,str;
    const char *fontCode = luaL_checklstring(L, 1,&len);
    unsigned char size = luaL_checkinteger(L, 2);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    lcd_str_fg_color = (luat_color_t)luaL_optinteger(L, 5,FORE_COLOR);
    // lcd_str_bg_color = (luat_color_t)luaL_optinteger(L, 6,BACK_COLOR);
    for(;;){
      e = utf8_next((uint8_t)*fontCode);
      if ( e == 0x0ffff )
      break;
      fontCode++;
      if ( e != 0x0fffe ){
        uint16_t str = unicodetogb2312(e);
        memset(buf,0,128);
        int font_size = get_font(buf, str<0x80?VEC_HZ_ASCII_STY:VEC_BLACK_STY, str, size, size, size);
        if(font_size == 0){
          LLOGW("get gtfont error size:%d font_size:%d",size,font_size);
          return 0;
        }
        unsigned int dw = gtfont_draw_w(buf , x ,y , font_size,size , size,luat_lcd_draw_point,default_conf,0);
        x+=str<0x80?dw:size;
      }
    }
    lcd_auto_flush(default_conf);
    return 0;
}

/*Display UTF8 string using gtfont grayscale
@api lcd.drawGtfontUtf8Gray(str,size,gray,x,y)
@string str display string
@int size font size (supports 16-192 size fonts)
@int gray grayscale [1st level/2nd level/3rd level/4th level]
@int x abscissa
@int y vertical coordinate
@usage
lcd.drawGtfontUtf8Gray("Ahhhhh",32,4,0,40)*/
static int l_lcd_draw_gtfont_utf8_gray(lua_State* L) {
	unsigned char buf[2048] = {0};
	size_t len;
	int i = 0;
	uint8_t strhigh,strlow ;
	uint16_t e,str;
  const char *fontCode = luaL_checklstring(L, 1,&len);
  unsigned char size = luaL_checkinteger(L, 2);
	unsigned char font_g = luaL_checkinteger(L, 3);
	int x = luaL_checkinteger(L, 4);
	int y = luaL_checkinteger(L, 5);
  lcd_str_fg_color = (luat_color_t)luaL_optinteger(L, 6,FORE_COLOR);
  // lcd_str_bg_color = (luat_color_t)luaL_optinteger(L, 7,BACK_COLOR);
	for(;;){
        e = utf8_next((uint8_t)*fontCode);
        if ( e == 0x0ffff )
        break;
        fontCode++;
        if ( e != 0x0fffe ){
			uint16_t str = unicodetogb2312(e);
			int font_size = get_font(buf, str<0x80?VEC_HZ_ASCII_STY:VEC_BLACK_STY, str, size*font_g, size*font_g, size*font_g);
      if(font_size != size*font_g){
        LLOGW("get gtfont error size:%d font_size:%d",size,font_size);
        return 0;
      }
			Gray_Process(buf,size,size,font_g);
      gtfont_draw_gray_hz(buf, x, y, size , size, font_g, 1,luat_lcd_draw_point,default_conf,0);
        	x+=size;
        }
    }
    lcd_auto_flush(default_conf);
    return 0;
}

#endif // LUAT_USE_GTFONT_UTF8

#endif // LUAT_USE_GTFONT

static int l_lcd_set_default(lua_State *L) {
    if (lua_gettop(L) == 1) {
        default_conf = lua_touserdata(L, 1);
        lua_pushboolean(L, 1);
        return 1;
    }
    return 1;
}

static int l_lcd_get_default(lua_State *L) {
    if (default_conf == NULL)
      return 0;
    lua_pushlightuserdata(L, default_conf);
    return 1;
}

/*Get screen size
@api lcd.getSize()
@return int width, 0 will be returned if not initialized
@return int high, 0 will be returned if not initialized
@usage
log.info("lcd", "size", lcd.getSize())*/
static int l_lcd_get_size(lua_State *L) {
  if (lua_gettop(L) == 1) {
    luat_lcd_conf_t * conf = lua_touserdata(L, 1);
    if (conf) {
      lua_pushinteger(L, conf->w);
      lua_pushinteger(L, conf->h);
      return 2;
    }
  }
  if (default_conf == NULL) {
    lua_pushinteger(L, 0);
    lua_pushinteger(L, 0);
  }
  else {
    lua_pushinteger(L, default_conf->w);
    lua_pushinteger(L, default_conf->h);
  }
  return 2;
}

/*Draw bitmap
@api lcd.drawXbm(x, y, w, h, data)
@int X coordinate
@int y coordinate
@int bitmap width
@int bitmap height
@int bitmap data, each bit represents a pixel
@usage
-- Use PCtoLCD2002 software to take the modulus, negative code, line by line, reverse
-- At (0,0) as the upper left corner, draw a 16x16 "today" bitmap
lcd.drawXbm(0, 0, 16,16, string.char(
    0x80,0x00,0x80,0x00,0x40,0x01,0x20,0x02,0x10,0x04,0x48,0x08,0x84,0x10,0x83,0x60,
    0x00,0x00,0xF8,0x0F,0x00,0x08,0x00,0x04,0x00,0x04,0x00,0x02,0x00,0x01,0x80,0x00
))*/
static int l_lcd_drawxbm(lua_State *L){
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    size_t w = luaL_checkinteger(L, 3);
    size_t h = luaL_checkinteger(L, 4);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 5, &len);
    uint8_t mask = 1;
    if (h < 1) return 0; //The number of rows must be greater than 0
    if (len*8/h < w) return 0; // At least fill one line
    int w1 = w/8;
    if (w%8)w1++;
    if (len != h*w1)return 0;
    luat_color_t* color_w = luat_heap_malloc(sizeof(luat_color_t) * w);
    for (size_t b = 0; b < h; b++){
      size_t a = 0;
      while (a < w){
        for (size_t c = 0; c < 8; c++){
          if (*data&(mask<<c)){
            color_w[a]=FORE_COLOR;
          }else{
            color_w[a]=BACK_COLOR;
          }
          a++;
          if (a == w)break;
        }
        data++;
      }
      luat_lcd_draw(default_conf, x, y+b, x+w-1, y+b, color_w);
    }
    luat_heap_free(color_w);
    lcd_auto_flush(default_conf);
    lua_pushboolean(L, 1);
    return 1;
}

#ifdef LUAT_USE_TJPGD
#include "tjpgd.h"
#include "tjpgdcnf.h"

#define N_BPP (3 - JD_FORMAT)

/* Session identifier for input/output functions (name, members and usage are as user defined) */
typedef struct {
    FILE *fp;               /* Input stream */
    int x;
    int y;
    // int width;
    // int height;
    uint16_t buff[16*16];
} IODEV;

static unsigned int file_in_func (JDEC* jd, uint8_t* buff, unsigned int nbyte){
    IODEV *dev = (IODEV*)jd->device;   /* Device identifier for the session (5th argument of jd_prepare function) */
    if (buff) {
        /* Read bytes from input stream */
        return luat_fs_fread(buff, 1, nbyte, dev->fp);
    } else {
        /* Remove bytes from input stream */
        return luat_fs_fseek(dev->fp, nbyte, SEEK_CUR) ? 0 : nbyte;
    }
}

static int lcd_out_func (JDEC* jd, void* bitmap, JRECT* rect){
    IODEV *dev = (IODEV*)jd->device;
    uint16_t* tmp = (uint16_t*)bitmap;

    // rgb high and low swap
    uint16_t count = (rect->right - rect->left + 1) * (rect->bottom - rect->top + 1);
    for (size_t i = 0; i < count; i++){
      if (default_conf->port == LUAT_LCD_HW_ID_0)
        dev->buff[i] = tmp[i];
      else
        dev->buff[i] = ((tmp[i] >> 8) & 0xFF)+ ((tmp[i] << 8) & 0xFF00);
    }
    
    // LLOGD("jpeg seg %dx%d %dx%d", rect->left, rect->top, rect->right, rect->bottom);
    // LLOGD("jpeg seg size %d %d %d", rect->right - rect->left + 1, rect->bottom - rect->top + 1, (rect->right - rect->left + 1) * (rect->bottom - rect->top + 1));
    luat_lcd_draw(default_conf, dev->x + rect->left, dev->y + rect->top,
                                dev->x + rect->right, dev->y + rect->bottom,
                                dev->buff);
    return 1;    /* Continue to decompress */
}

static int lcd_draw_jpeg(const char* path, int xpos, int ypos) {
  JRESULT res;      /* Result code of TJpgDec API */
  JDEC jdec;        /* Decompression object */
  void *work;       /* Pointer to the decompressor work area */
#if JD_FASTDECODE == 2
  size_t sz_work = 3500 * 3; /* Size of work area */
#else
  size_t sz_work = 3500; /* Size of work area */
#endif
  IODEV devid;      /* User defined device identifier */

  FILE* fd = luat_fs_fopen(path, "r");
  if (fd == NULL) {
    LLOGW("no such file %s", path);
    return -1;
  }

  devid.fp = fd;
  work = luat_heap_malloc(sz_work);
  if (work == NULL) {
    LLOGE("out of memory when malloc jpeg decode workbuff");
    return -3;
  }
  res = jd_prepare(&jdec, file_in_func, work, sz_work, &devid);
  if (res != JDR_OK) {
    luat_heap_free(work);
    luat_fs_fclose(fd);
    LLOGW("jd_prepare file %s error %d", path, res);
    return -2;
  }
  devid.x = xpos;
  devid.y = ypos;
  // devid.width = jdec.width;
  // devid.height = jdec.height;
  res = jd_decomp(&jdec, lcd_out_func, 0);
  luat_heap_free(work);
  luat_fs_fclose(fd);
  if (res != JDR_OK) {
    LLOGW("jd_decomp file %s error %d", path, res);
    return -2;
  }
  else {
    lcd_auto_flush(default_conf);
    return 0;
  }
}

/*Display pictures, currently only supports jpg, jpeg
@api lcd.showImage(x, y, file)
@int X coordinate
@int y coordinate
@string file path
@usage
lcd.showImage(0,0,"/luadb/logo.jpg")*/
static int l_lcd_showimage(lua_State *L){
    size_t size = 0;
    int ret = 0;
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    const char* input_file = luaL_checklstring(L, 3, &size);
    if (memcmp(input_file+size-4, ".jpg", 5) == 0 || memcmp(input_file+size-4, ".JPG", 5) == 0 || memcmp(input_file+size-5, ".jpeg", 6) == 0 || memcmp(input_file+size-5, ".JPEG", 6) == 0){
      ret = lcd_draw_jpeg(input_file, x, y);
      lua_pushboolean(L, ret == 0 ? 1 : 0);
    } else{
      LLOGE("input_file not support");
      lua_pushboolean(L, 0);
    }
    return 1;
}
#endif

/*Actively refresh data to the interface, only use after setting buff and disabling automatic attributes
@api lcd.flush()
@return bool returns true if successful, otherwise returns nil/false
@usage
-- This API is used in conjunction with lcd.setupBuff lcd.autoFlush
lcd.flush()*/
static int l_lcd_flush(lua_State* L) {
  luat_lcd_conf_t * conf = NULL;
  if (lua_gettop(L) == 1) {
    conf = lua_touserdata(L, 1);
  }
  else {
    conf = default_conf;
  }
  if (conf == NULL) {
    //LLOGW("lcd not init");
    return 0;
  }
  if (conf->buff == NULL) {
    //LLOGW("lcd without buff, not support flush");
    return 0;
  }
  if (conf->auto_flush) {
    //LLOGI("lcd auto flush is enable, no need for flush");
    return 0;
  }
  luat_lcd_flush(conf);
  lua_pushboolean(L, 1);
  return 0;
}

/*To set the display buffer, the required memory size is 2×width×height bytes. Please weigh the memory requirements and the refresh frequency required by the business.
@api lcd.setupBuff(conf, onheap)
@userdata conf pointer, no need to pass
@bool true uses heap memory, false uses vm memory, defaults to vm memory, no need to actively transfer
@return bool whether successful
@usage
-- Initialize the LCD's buff buffer, which can be understood as the FrameBuffer area.
lcd.setupBuff()*/
static int l_lcd_setup_buff(lua_State* L) {
  luat_lcd_conf_t * conf = NULL;
  if (lua_gettop(L) == 1) {
    conf = lua_touserdata(L, 1);
  }
  else {
    conf = default_conf;
  }
  if (conf == NULL) {
    LLOGW("lcd not init");
    return 0;
  }
  if (conf->buff != NULL) {
    LLOGW("lcd buff is ok");
    return 0;
  }
  if (lua_isboolean(L, 2) && lua_toboolean(L, 2)) {
    conf->buff = luat_heap_malloc(sizeof(luat_color_t) * conf->w * conf->h);
  }
  else {
    conf->buff = lua_newuserdata(L, sizeof(luat_color_t) * conf->w * conf->h);
    if (conf->buff) {
      conf->buff_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
  }
  if (conf->buff == NULL) {
    LLOGE("lcd buff malloc fail, out of memory?");
    return 0;
  }
  // Set to the unnecessary range first
  conf->flush_y_min = conf->h;
  conf->flush_y_max = 0;
  // luat_lcd_clear will expand the area to the entire screen
  luat_lcd_clear(default_conf, BACK_COLOR);
  lua_pushboolean(L, 1);
  return 1;
}

/*Set automatic refresh, need to be used with lcd.setupBuff
@api lcd.autoFlush(enable)
@bool Whether to refresh automatically, the default is true
@usage
-- Set buff and disable automatic updates
lcd.setupBuff()
lcd.autoFlush(false)
-- After disabling automatic updates, you need to use lcd.flush() to actively refresh data to the screen.*/
static int l_lcd_auto_flush(lua_State* L) {
  luat_lcd_conf_t * conf = default_conf;
  if (conf == NULL) {
    LLOGW("lcd not init");
    return 0;
  }
  conf->auto_flush = lua_toboolean(L, 1);
  lua_pushboolean(L, conf->auto_flush);
  return 1;
}

/*RGB565 color generation
@api lcd.rgb565(r, g, b, swap)
@int red, 0x00 ~ 0xFF
@int green, 0x00 ~ 0xFF
@int blue, 0x00 ~ 0xFF
@bool Whether to flip, true to flip, false not to flip. Default to flip
@return int color value
@usage
-- This API supports multiple modes, the number of parameters are 1, 2, 3, 4 respectively
-- 1. Single parameter form, 24bit RGB value, swap = true, recommended
local red = lcd.rgb565(0xFF0000)
local green = lcd.rgb565(0x00FF00)
local blue = lcd.rgb565(0x0000FF)

-- 2. Two parameter form, 24bit RGB value, add swap setting
local red = lcd.rgb565(0xFF0000, true)
local green = lcd.rgb565(0x00FF00, true)
local blue = lcd.rgb565(0x0000FF, true)

-- 3. Three-parameter form, red/green/blue, 8bit each
local red = lcd.rgb565(0xFF, 0x00, 0x00)
local green = lcd.rgb565(0x00, 0xFF, 0x00)
local blue = lcd.rgb565(0x00, 0x00, 0xFF)

-- 4. Four parameter form, red/green/blue, 8bit each, add swap setting
local red = lcd.rgb565(0xFF, 0x00, 0x00, true)
local green = lcd.rgb565(0x00, 0xFF, 0x00, true)
local blue = lcd.rgb565(0x00, 0x00, 0xFF, true)*/
static int l_lcd_rgb565(lua_State* L) {
  uint8_t r =0,g =0,b = 0;
  uint8_t swap = 0;
  uint16_t dst = 0;
  int top = 0 ;
  uint32_t rgb = 0;
  top = lua_gettop(L);
  if (top == 1 || top == 2) {
    rgb = luaL_checkinteger(L, 1);
    r = (uint8_t)((rgb >> 16 ) & 0xFF);
    g = (uint8_t)((rgb >> 8 ) & 0xFF);
    b = (uint8_t)((rgb >> 0 ) & 0xFF);
    swap = (lua_isboolean(L, 2) && !lua_toboolean(L, 2)) ? 0U : 1U;
  }
  else if (top == 3 || top == 4) {
    r = (uint8_t)luaL_checkinteger(L, 1);
    g = (uint8_t)luaL_checkinteger(L, 2);
    b = (uint8_t)luaL_checkinteger(L, 3);
    swap = (lua_isboolean(L, 4) && !lua_toboolean(L, 4)) ? 0U : 1U;
  }
  else {
    LLOGW("unkown args count %d", top);
    dst = 0;
  }
  dst = (uint16_t)((r&0xF8)<<8) | (uint16_t)((g&0xFC)<<3) | (uint16_t)(b>>3);

  if (swap) {
    dst = ((dst >> 8) & 0xFF) + ((dst & 0xFF) << 8);
  }
  lua_pushinteger(L, dst);
  return 1;
}
#ifdef LUAT_USE_UFONT
#include "luat_ufont.h"
static const int l_lcd_draw_utf8(lua_State *L) {
    size_t sz = 0;
    uint32_t letter = 0;
    uint32_t str_offset;
    int ret = 0;
    uint16_t draw_offset = 0;

    int draw_x = 0;
    int draw_y = 0;
    luat_font_char_desc_t desc = {0};
    //Coordinates x,y of the upper left corner
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    //String to be drawn
    const char* data = (const char*)luaL_checklstring(L, 3, &sz);
    // font pointer
    lv_font_t* lfont = (lv_font_t*)lua_touserdata(L, 4);
    if (lfont == NULL) {
       LLOGW("draw without font");
       return 0;
    }
    luat_font_header_t* font = (luat_font_header_t*)lfont->dsc;
    // Whether to fill the background
    bool draw_bg = lua_isboolean(L, 5) ? lua_toboolean(L, 5) : true;

    // No content, no need to draw
    if (sz == 0) {
      // Directly return to the original coordinates
      lua_pushinteger(L, x);
      return 1;
    }

    // There is no font, no need to draw
    if (font == NULL) {
      LLOGD("NULL font, skip draw");
      // Directly return to the original coordinates
      lua_pushinteger(L, x);
      return 1;
    }
    // Has it exceeded the boundary? If it has exceeded, there is no need to draw it.
    if (default_conf->h < y || default_conf->w < x) {
      //LLOGD("draw y %d h % font->line_height %d", y, default_conf->h, font->line_height);
      // Directly return to the original coordinates
      lua_pushinteger(L, x);
      return 1;
    }

    luat_color_t* buff = NULL;
    if (draw_bg)
      buff = luat_heap_malloc(font->line_height * font->line_height * 2);
    // if (buff == NULL)
    //   return 0;
    int offset = 0;
    uint8_t *data_ptr = data;
    uint8_t utf8_state = 0;
    uint16_t utf8_tmp = 0;
    uint16_t utf8_out = 0;
    luat_color_t color = FORE_COLOR;
    for (size_t i = 0; i < sz; i++)
    {
        utf8_out = luat_utf8_next(data[i], &utf8_state, &utf8_tmp);
        if (utf8_out == 0x0ffff)
          break; // it's over
        if (utf8_out == 0x0fffe)
          continue; // If a character has not been read, continue to the next loop
        letter = (uint32_t)utf8_out;

        //LLOGD("draw letter %04X", letter);
        int ret = luat_font_get_bitmap(font, &desc, letter);
        if (ret != 0) {
            LLOGD("not such char in font");
            draw_offset += font->line_height / 2; // Character not found, half character skipped by default
            continue;
        }
        offset = 0;
        // desc.data = tmp;
        memset(buff, 0, font->line_height * font->line_height * 2);
        draw_x = x + draw_offset;
        draw_offset += desc.char_w;
        if (draw_x >= 0 &&  draw_x + desc.char_w <= default_conf->w) {
          //if (default_conf->buff == NULL) {
            for (size_t j = 0; j < font->line_height; j++)
            {
              //LLOGD("draw char pix line %d", i);
              for (size_t k = 0; k < desc.char_w; k++)
              {
                if ((desc.data[offset / 8] >> (7 - (offset % 8))) & 0x01) {
                  color = FORE_COLOR;
                  if (buff)
                    buff[offset] = FORE_COLOR;
                  else
                    luat_lcd_draw_point(default_conf, draw_x + k, y + j, FORE_COLOR);
                  //LLOGD("draw char pix mark %d", offset);
                }
                else {
                  if (buff)
                    buff[offset] = BACK_COLOR;
                  //LLOGD("draw char pix offset %d color %04X", offset, FORE_COLOR);
                }
                offset ++;
              }
            }
            //LLOGD("luat_lcd_draw %d %d %d %d", draw_x, y, draw_x + desc.char_w, y + font->line_height);
            luat_lcd_draw(default_conf, draw_x, y, draw_x + desc.char_w - 1, y + font->line_height - 1, buff);
          //}
          //else {
          //
          //}
        }
    }
    if (buff)
      luat_heap_free(buff);

    lcd_auto_flush(default_conf);
    lua_pushinteger(L, draw_x + desc.char_w);
    return 1;
}
#endif

#include "rotable2.h"
static const rotable_Reg_t reg_lcd[] =
{
    { "init",       ROREG_FUNC(l_lcd_init)},
    { "clear",      ROREG_FUNC(l_lcd_clear)},
    { "fill",       ROREG_FUNC(l_lcd_draw_fill)},
    { "drawPoint",  ROREG_FUNC(l_lcd_draw_point)},
    { "drawLine",   ROREG_FUNC(l_lcd_draw_line)},
    { "drawRectangle",   ROREG_FUNC(l_lcd_draw_rectangle)},
    { "drawCircle", ROREG_FUNC(l_lcd_draw_circle)},
    { "drawQrcode", ROREG_FUNC(l_lcd_drawQrcode)},
    { "drawStr",    ROREG_FUNC(l_lcd_draw_str)},
    { "flush",      ROREG_FUNC(l_lcd_flush)},
    { "setupBuff",  ROREG_FUNC(l_lcd_setup_buff)},
    { "autoFlush",  ROREG_FUNC(l_lcd_auto_flush)},
    { "setFont",    ROREG_FUNC(l_lcd_set_font)},
    { "setDefault", ROREG_FUNC(l_lcd_set_default)},
    { "getDefault", ROREG_FUNC(l_lcd_get_default)},
    { "getSize",    ROREG_FUNC(l_lcd_get_size)},
    { "drawXbm",    ROREG_FUNC(l_lcd_drawxbm)},
    { "close",      ROREG_FUNC(l_lcd_close)},
    { "on",         ROREG_FUNC(l_lcd_display_on)},
    { "off",        ROREG_FUNC(l_lcd_display_off)},
    { "sleep",      ROREG_FUNC(l_lcd_sleep)},
    { "wakeup",     ROREG_FUNC(l_lcd_wakeup)},
    { "invon",      ROREG_FUNC(l_lcd_inv_on)},
    { "invoff",     ROREG_FUNC(l_lcd_inv_off)},
    { "cmd",        ROREG_FUNC(l_lcd_write_cmd)},
    { "data",       ROREG_FUNC(l_lcd_write_data)},
    { "setColor",   ROREG_FUNC(l_lcd_set_color)},
    { "draw",       ROREG_FUNC(l_lcd_draw)},
    { "rgb565",     ROREG_FUNC(l_lcd_rgb565)},
#ifdef LUAT_USE_UFONT
    { "drawUTF8",   ROREG_FUNC(l_lcd_draw_utf8)},
#endif
#ifdef LUAT_USE_TJPGD
    { "showImage",    ROREG_FUNC(l_lcd_showimage)},
#endif
#ifdef LUAT_USE_GTFONT
    { "drawGtfontGb2312", ROREG_FUNC(l_lcd_draw_gtfont_gbk)},
    { "drawGtfontGb2312Gray", ROREG_FUNC(l_lcd_draw_gtfont_gbk_gray)},
    { "drawGtfontGbk", ROREG_FUNC(l_lcd_draw_gtfont_gbk)},
    { "drawGtfontGbkGray", ROREG_FUNC(l_lcd_draw_gtfont_gbk_gray)},
#ifdef LUAT_USE_GTFONT_UTF8
    { "drawGtfontUtf8", ROREG_FUNC(l_lcd_draw_gtfont_utf8)},
    { "drawGtfontUtf8Gray", ROREG_FUNC(l_lcd_draw_gtfont_utf8_gray)},
#endif // LUAT_USE_GTFONT_UTF8
#endif // LUAT_USE_GTFONT
    //The default is only English size 12 font
    //@const font_opposansm12 font font size 12
    { "font_opposansm12", ROREG_PTR((void*)u8g2_font_opposansm12)},
#ifdef USE_U8G2_OPPOSANSM_ENGLISH
    //@const font_unifont_t_symbols font symbol font
    { "font_unifont_t_symbols",   ROREG_PTR((void*)u8g2_font_unifont_t_symbols)},
    //@const font_open_iconic_weather_6x_t font weather font
    { "font_open_iconic_weather_6x_t", ROREG_PTR((void*)u8g2_font_open_iconic_weather_6x_t)},

    //@const font_opposansm16 font font size 16
    //@const font_opposansm18 font font size 18
    //@const font_opposansm20 font font size 20
    //@const font_opposansm22 font font size 22
    //@const font_opposansm24 font font size 24
    //@const font_opposansm32 font font size 32
    { "font_opposansm16", ROREG_PTR((void*)u8g2_font_opposansm16)},
    { "font_opposansm18", ROREG_PTR((void*)u8g2_font_opposansm18)},
    { "font_opposansm20", ROREG_PTR((void*)u8g2_font_opposansm20)},
    { "font_opposansm22", ROREG_PTR((void*)u8g2_font_opposansm22)},
    { "font_opposansm24", ROREG_PTR((void*)u8g2_font_opposansm24)},
    { "font_opposansm32", ROREG_PTR((void*)u8g2_font_opposansm32)},
#endif
#ifdef USE_U8G2_OPPOSANSM12_CHINESE
    //@const font_opposansm12_chinese font No. 12 Chinese font
    { "font_opposansm12_chinese", ROREG_PTR((void*)u8g2_font_opposansm12_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM14_CHINESE
    //@const font_opposansm14_chinese font No. 14 Chinese font
    { "font_opposansm14_chinese", ROREG_PTR((void*)u8g2_font_opposansm14_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM16_CHINESE
    //@const font_opposansm16_chinese font No. 16 Chinese font
    { "font_opposansm16_chinese", ROREG_PTR((void*)u8g2_font_opposansm16_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM18_CHINESE
    //@const font_opposansm18_chinese font No. 18 Chinese font
    { "font_opposansm18_chinese", ROREG_PTR((void*)u8g2_font_opposansm18_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM20_CHINESE
    //@const font_opposansm20_chinese font No. 20 Chinese font
    { "font_opposansm20_chinese", ROREG_PTR((void*)u8g2_font_opposansm20_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM22_CHINESE
    //@const font_opposansm22_chinese font No. 22 Chinese font
    { "font_opposansm22_chinese", ROREG_PTR((void*)u8g2_font_opposansm22_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM24_CHINESE
    //@const font_opposansm24_chinese font No. 24 Chinese font
    { "font_opposansm24_chinese", ROREG_PTR((void*)u8g2_font_opposansm24_chinese)},
#endif
#ifdef USE_U8G2_OPPOSANSM32_CHINESE
    //@const font_opposansm32_chinese font No. 32 Chinese font
    { "font_opposansm32_chinese", ROREG_PTR((void*)u8g2_font_opposansm32_chinese)},
#endif
#ifdef USE_U8G2_SARASA_ENGLISH
    { "font_sarasa_m12_ascii", ROREG_PTR((void*)u8g2_font_sarasa_m12_ascii)},
    { "font_sarasa_m14_ascii", ROREG_PTR((void*)u8g2_font_sarasa_m14_ascii)},
    { "font_sarasa_m16_ascii", ROREG_PTR((void*)u8g2_font_sarasa_m16_ascii)},
    { "font_sarasa_m18_ascii", ROREG_PTR((void*)u8g2_font_sarasa_m18_ascii)},
    { "font_sarasa_m20_ascii", ROREG_PTR((void*)u8g2_font_sarasa_m20_ascii)},
    { "font_sarasa_m22_ascii", ROREG_PTR((void*)u8g2_font_sarasa_m22_ascii)},
    //No matter how big it is, it is rarely used so I won’t add it first.
#endif

// #ifdef USE_U8G2_SARASA_M8_CHINESE
//     { "font_sarasa_m8_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m8_chinese)},
// #endif
#ifdef USE_U8G2_SARASA_M10_CHINESE
    { "font_sarasa_m10_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m10_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M12_CHINESE
    { "font_sarasa_m12_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m12_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M14_CHINESE
    { "font_sarasa_m14_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m14_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M16_CHINESE
    { "font_sarasa_m16_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m16_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M18_CHINESE
    { "font_sarasa_m18_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m18_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M20_CHINESE
    { "font_sarasa_m20_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m20_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M22_CHINESE
    { "font_sarasa_m22_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m22_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M24_CHINESE
    { "font_sarasa_m24_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m24_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M26_CHINESE
    { "font_sarasa_m26_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m26_chinese)},
#endif
#ifdef USE_U8G2_SARASA_M28_CHINESE
    { "font_sarasa_m28_chinese", ROREG_PTR((void*)u8g2_font_sarasa_m28_chinese)},
#endif
    { "set_direction",  ROREG_FUNC(l_lcd_set_direction)},
    //@const direction_0 int 0° direction command
    { "direction_0",    ROREG_INT(0)},
    //@const direction_90 int 90° direction command
    { "direction_90",   ROREG_INT(1)},
    //@const direction_180 int 180° direction command
    { "direction_180",  ROREG_INT(2)},
    //@const direction_270 int 270° direction command
    { "direction_270",  ROREG_INT(3)},
    //@const HWID_0 hardware lcd driver id0 (selected based on chip support)
    { "HWID_0",         ROREG_INT(LUAT_LCD_HW_ID_0)},
    //@const WIRE_3_BIT_9_INTERFACE_I three-line spi 9bit mode I
    { "WIRE_3_BIT_9_INTERFACE_I",   ROREG_INT(LUAT_LCD_IM_3_WIRE_9_BIT_INTERFACE_I)},
    //@const WIRE_4_BIT_8_INTERFACE_I four-wire spi 8bit mode I
    { "WIRE_4_BIT_8_INTERFACE_I",   ROREG_INT(LUAT_LCD_IM_4_WIRE_8_BIT_INTERFACE_I)},
    //@const WIRE_3_BIT_9_INTERFACE_II three-line spi 9bit mode II
    { "WIRE_3_BIT_9_INTERFACE_II",  ROREG_INT(LUAT_LCD_IM_3_WIRE_9_BIT_INTERFACE_II)},
    //@const WIRE_4_BIT_8_INTERFACE_II four-wire spi 8bit mode II
    { "WIRE_4_BIT_8_INTERFACE_II",  ROREG_INT(LUAT_LCD_IM_4_WIRE_8_BIT_INTERFACE_II)},
    //@const DATA_2_LANE int dual channel mode
    { "DATA_2_LANE",                ROREG_INT(LUAT_LCD_IM_2_DATA_LANE)},
	  {NULL, ROREG_INT(0)}
};

LUAMOD_API int luaopen_lcd( lua_State *L ) {
    luat_newlib2(L, reg_lcd);
    return 1;
}
