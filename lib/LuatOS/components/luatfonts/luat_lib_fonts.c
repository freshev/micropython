/*@Modules  fonts
@summary 字体库
@version 1.0
@date    2022.07.11
@tag LUAT_USE_FONTS*/

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_fs.h"

#define LUAT_LOG_TAG "fonts"
#include "luat_log.h"

#include "u8g2.h"
#include "u8g2_luat_fonts.h"

#include "luat_fonts_custom.h"

typedef struct u8g2_font
{
    const char* name;
    const uint8_t* font;
}u8g2_font_t;

static const u8g2_font_t u8g2_fonts[] = {
#ifdef USE_U8G2_OPPOSANSM_ENGLISH
    {.name="unifont_t_symbols", .font=u8g2_font_unifont_t_symbols},
    {.name="open_iconic_weather_6x_t", .font=u8g2_font_open_iconic_weather_6x_t},
    {.name="opposansm10", .font=u8g2_font_opposansm10},
    {.name="opposansm12", .font=u8g2_font_opposansm12},
    {.name="opposansm16", .font=u8g2_font_opposansm16},
    {.name="opposansm20", .font=u8g2_font_opposansm20},
    {.name="opposansm24", .font=u8g2_font_opposansm24},
    {.name="opposansm32", .font=u8g2_font_opposansm32},
#endif
#ifdef USE_U8G2_OPPOSANSM12_CHINESE
    {.name="opposansm12_chinese", .font=u8g2_font_opposansm12_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM14_CHINESE
    {.name="opposansm14_chinese", .font=u8g2_font_opposansm14_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM16_CHINESE
    {.name="opposansm16_chinese", .font=u8g2_font_opposansm16_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM18_CHINESE
    {.name="opposansm18_chinese", .font=u8g2_font_opposansm18_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM20_CHINESE
    {.name="opposansm20_chinese", .font=u8g2_font_opposansm20_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM22_CHINESE
    {.name="opposansm22_chinese", .font=u8g2_font_opposansm22_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM24_CHINESE
    {.name="opposansm24_chinese", .font=u8g2_font_opposansm24_chinese},
#endif
#ifdef USE_U8G2_OPPOSANSM32_CHINESE
    {.name="opposansm32_chinese", .font=u8g2_font_opposansm32_chinese},
#endif
#ifdef USE_U8G2_SARASA_ENGLISH
    { .name="sarasa_m12_ascii", .font=u8g2_font_sarasa_m12_ascii},
    { .name="sarasa_m14_ascii", .font=u8g2_font_sarasa_m14_ascii},
    { .name="sarasa_m16_ascii", .font=u8g2_font_sarasa_m16_ascii},
    { .name="sarasa_m18_ascii", .font=u8g2_font_sarasa_m18_ascii},
    { .name="sarasa_m20_ascii", .font=u8g2_font_sarasa_m20_ascii},
    { .name="sarasa_m22_ascii", .font=u8g2_font_sarasa_m22_ascii},
    //No matter how big it is, it is rarely used so I won’t add it first.
#endif

// #ifdef USE_U8G2_SARASA_M8_CHINESE
//     { .name="sarasa_m8_chinese", .font=u8g2_font_sarasa_m8_chinese},
// #endif
#ifdef USE_U8G2_SARASA_M10_CHINESE
    { .name="sarasa_m10_chinese", .font=u8g2_font_sarasa_m10_chinese},
#endif
#ifdef USE_U8G2_SARASA_M12_CHINESE
    { .name="sarasa_m12_chinese", .font=u8g2_font_sarasa_m12_chinese},
#endif
#ifdef USE_U8G2_SARASA_M14_CHINESE
    { .name="sarasa_m14_chinese", .font=u8g2_font_sarasa_m14_chinese},
#endif
#ifdef USE_U8G2_SARASA_M16_CHINESE
    { .name="sarasa_m16_chinese", .font=u8g2_font_sarasa_m16_chinese},
#endif
#ifdef USE_U8G2_SARASA_M18_CHINESE
    { .name="sarasa_m18_chinese", .font=u8g2_font_sarasa_m18_chinese},
#endif
#ifdef USE_U8G2_SARASA_M20_CHINESE
    { .name="sarasa_m20_chinese", .font=u8g2_font_sarasa_m20_chinese},
#endif
#ifdef USE_U8G2_SARASA_M22_CHINESE
    { .name="sarasa_m22_chinese", .font=u8g2_font_sarasa_m22_chinese},
#endif
#ifdef USE_U8G2_SARASA_M24_CHINESE
    { .name="sarasa_m24_chinese", .font=u8g2_font_sarasa_m24_chinese},
#endif
#ifdef USE_U8G2_SARASA_M26_CHINESE
    { .name="sarasa_m26_chinese", .font=u8g2_font_sarasa_m26_chinese},
#endif
#ifdef USE_U8G2_SARASA_M28_CHINESE
    { .name="sarasa_m28_chinese", .font=u8g2_font_sarasa_m28_chinese},
#endif
#ifdef LUAT_FONTS_CUSTOM_U8G2
    LUAT_FONTS_CUSTOM_U8G2
#endif
    {.name="", .font=NULL},
};


static int l_fonts_u8g2_get(lua_State *L) {
    const char* name = luaL_checkstring(L,  1);
    const u8g2_font_t *font = u8g2_fonts;
    while (font->font != NULL) {
        if (!strcmp(name, font->name)) {
            lua_pushlightuserdata(L, font->font);
            return 1;
        }
        font ++;
    }
    return 0;
}

static int l_fonts_u8g2_load(lua_State *L) {
    char* ptr = NULL;
    //Load from file
    const char* path = luaL_checkstring(L, 1);
    size_t flen = luat_fs_fsize(path);
    if (flen < 16) {
        LLOGE("not a good font file %s", path);
        return 0;
    }
    FILE* fd = luat_fs_fopen(path, "rb");
    if (fd == NULL) {
        LLOGE("no such file %s", path);
        return 0;
    }
#ifdef LUAT_USE_FS_VFS
  //LLOGD("try mmap");
  ptr = (char*)luat_fs_mmap(fd);
  if (ptr != NULL) {
    LLOGD("load by mmap %s %p", path, ptr);
    lua_pushlightuserdata(L, ptr);
    luat_fs_fclose(fd);
    return 1;
  }
#endif
    ptr = lua_newuserdata(L, flen);
    if (ptr == NULL) {
        luat_fs_fclose(fd);
        LLOGE("no engouh memory for font %s", path);
        return 0;
    }
    char buff[256];
    int len = 0;
    int count = 0;
    while (count < flen)
    {
        len = luat_fs_fread(buff, 256, 1, fd);
        if (len < 0)
            break;
        if (len > 0) {
            memcpy(ptr + count, buff, len);
            count += len;
        }
    }
    luat_fs_fclose(fd);
    lua_pushlightuserdata(L, ptr);
    return 1;
}


static int l_fonts_u8g2_list(lua_State *L) {
    const u8g2_font_t *font = u8g2_fonts;
    lua_createtable(L, 10, 0);
    int index = 1;
    while (font->font != NULL) {
        lua_pushinteger(L, index);
        lua_pushstring(L, font->name);
        lua_settable(L, -3);
        index ++;
        font ++;
    }
    return 1;
}

//----------------------------------------------
// LVGL related
//----------------------------------------------

#ifdef LUAT_USE_LVGL

#include "lvgl.h"
#include "lv_font/lv_font.h"

typedef struct lv_font_reg
{
    const char* name;
    lv_font_t* font;
}lv_font_reg_t;

static const lv_font_reg_t lv_regs[] = {
#ifdef LV_FONT_MONTSERRAT_14
    {.name="montserrat_14", .font=&lv_font_montserrat_14},
#endif
#ifdef LV_FONT_OPPOSANS_M_8
    {.name="opposans_m_8", .font=&lv_font_opposans_m_8},
#endif
#ifdef LV_FONT_OPPOSANS_M_10
    {.name="opposans_m_10", .font=&lv_font_opposans_m_10},
#endif
#ifdef LV_FONT_OPPOSANS_M_12
    {.name="opposans_m_12", .font=&lv_font_opposans_m_12},
#endif
#ifdef LV_FONT_OPPOSANS_M_14
    {.name="opposans_m_14", .font=&lv_font_opposans_m_14},
#endif
#ifdef LV_FONT_OPPOSANS_M_16
    {.name="opposans_m_16", .font=&lv_font_opposans_m_16},
#endif
#ifdef LV_FONT_OPPOSANS_M_18
    {.name="opposans_m_18", .font=&lv_font_opposans_m_18},
#endif
#ifdef LV_FONT_OPPOSANS_M_20
    {.name="opposans_m_20", .font=&lv_font_opposans_m_20},
#endif
#ifdef LV_FONT_OPPOSANS_M_22
    {.name="opposans_m_22", .font=&lv_font_opposans_m_22},
#endif

#ifdef LUAT_FONTS_CUSTOM_LVGL
    LUAT_FONTS_CUSTOM_LVGL
#endif
    {.name="", .font=NULL},
};

static int l_fonts_lvgl_get(lua_State *L) {
    const char* name = luaL_checkstring(L,  1);
    const lv_font_reg_t *font = lv_regs;
    while (font->font != NULL) {
        if (!strcmp(name, font->name)) {
            lua_pushlightuserdata(L, font->font);
            return 1;
        }
        font ++;
    }
    return 0;
}

static int l_fonts_lvgl_list(lua_State *L) {
    const lv_font_reg_t *font = lv_regs;
    lua_createtable(L, 10, 0);
    int index = 1;
    while (font->font != NULL) {
        lua_pushinteger(L, index);
        lua_pushstring(L, font->name);
        lua_settable(L, -3);
        index ++;
    }
    return 1;
}

#endif

/*Returns the list of fonts supported by the firmware
@api fonts.list(tp)
@string type, default u8g2, can also be lvgl
@return table font list
@usage
-- API added on 2022-07-12
if fonts.list then
    log.info("fonts", "u8g2", json.encode(fonts.list("u8g2")))
end*/
static int l_fonts_list(lua_State *L) {
#ifdef LUAT_USE_LVGL
    const char* tp = luaL_optstring(L, 1, "u8g2");
    if (!strcmp("lvgl", tp)) {
        return l_fonts_lvgl_list(L);
    }
#endif
    return l_fonts_u8g2_list(L);
}

/*Get font
@api fonts.u8g2_get(name, tp)
@string font name, such as opposansm8_chinese unifont_t_symbols
@string type, default u8g2, can also be lvgl
@return userdata If the font is stored, return the font pointer, otherwise return nil
@usage
oppo_8 = fonts.get("opposansm8_chinese", "u8g2")
if oppo_8 then
    u8g2.SetFont(oppo_8)
else
    log.warn("fonts", "no such font opposansm8_chinese")
end
-- If you use a cloud-compiled custom font, use it as follows:
oppo_8 = fonts.get("oppo_bold_8", "u8g2") -- oppo_bold_8 is the font name of the cloud compilation interface
if oppo_8 then
    u8g2.SetFont(oppo_8)
else
    log.warn("fonts", "no such font opposansm8_chinese")
end*/
static int l_fonts_get(lua_State *L) {
#ifdef LUAT_USE_LVGL
    const char* tp = luaL_optstring(L, 2, "u8g2");
    if (!strcmp("lvgl", tp)) {
        return l_fonts_lvgl_get(L);
    }
#endif
    return l_fonts_u8g2_get(L);
}

/*Load fonts from file
@api fonts.u8g2_load(path, path)
@string font path, for example /luadb/abc.bin
@string type, default u8g2. Also supports lvgl
@return userdata If the font is stored, return the font pointer, otherwise return nil
@usage
-- API added on 2022-07-11
-- Reminder: If the file is located under /luadb, it does not need to occupy memory
-- If the file is in other paths, such as tf/sd card, spi flash, it will be automatically loaded into the memory, consuming the memory space of Lua vm.
-- Please quote appropriately after loading. Do not load the same font file repeatedly.
oppo12 = fonts.load("/luadb/oppo12.bin")
if oppo12 then
    u8g2.SetFont(oppo12)
else
    log.warn("fonts", "no such font file oppo12.bin")
end*/
static int l_fonts_load(lua_State *L) {
#ifdef LUAT_USE_LVGL
    const char* tp = luaL_optstring(L, 2, "u8g2");
    if (!strcmp("lvgl", tp)) {
        const char* fontname = luaL_checkstring(L, 1);
        lv_font_t* font = lv_font_load(fontname);
        if (font == NULL) {
            return 0;
        }
        lua_pushlightuserdata(L, font);
        return 1;
    }
#endif
    return l_fonts_u8g2_load(L);
}

#include "rotable2.h"
static const rotable_Reg_t reg_fonts[] =
{
    { "get" ,       ROREG_FUNC(l_fonts_get)},
    { "list" ,      ROREG_FUNC(l_fonts_list)},
    { "load" ,      ROREG_FUNC(l_fonts_load)},
	{ NULL,         ROREG_INT(0)},
};

LUAMOD_API int luaopen_fonts( lua_State *L ) {
    luat_newlib2(L, reg_fonts);
    return 1;
}
