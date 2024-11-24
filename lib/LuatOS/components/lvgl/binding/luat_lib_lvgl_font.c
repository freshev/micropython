/*@Moduleslvgl
@summary LVGL Image Library
@version 1.0
@date 2021.06.01*/

#include "luat_base.h"
#include "luat_lvgl.h"
#include "lvgl.h"
#include "luat_mem.h"

#include "luat_lvgl_fonts.h"
#include "luat_spi.h"

#ifdef LUAT_USE_GTFONT
    extern luat_spi_device_t* gt_spi_dev;
#endif

typedef struct lvfont
{
    const char* name;
    const void* data;
}lvfont_t;

static const lvfont_t fonts[] = {
    {.name="montserrat_14", .data=&lv_font_montserrat_14},
#ifdef LV_FONT_OPPOSANS_M_8
    {.name="opposans_m_8", .data=&lv_font_opposans_m_8},
#endif
#ifdef LV_FONT_OPPOSANS_M_10
    {.name="opposans_m_10", .data=&lv_font_opposans_m_10},
#endif
#ifdef LV_FONT_OPPOSANS_M_12
    {.name="opposans_m_12", .data=&lv_font_opposans_m_12},
#endif
#ifdef LV_FONT_OPPOSANS_M_14
    {.name="opposans_m_14", .data=&lv_font_opposans_m_14},
#endif
#ifdef LV_FONT_OPPOSANS_M_16
    {.name="opposans_m_16", .data=&lv_font_opposans_m_16},
#endif
#ifdef LV_FONT_OPPOSANS_M_18
    {.name="opposans_m_18", .data=&lv_font_opposans_m_18},
#endif
#ifdef LV_FONT_OPPOSANS_M_20
    {.name="opposans_m_20", .data=&lv_font_opposans_m_20},
#endif
#ifdef LV_FONT_OPPOSANS_M_22
    {.name="opposans_m_22", .data=&lv_font_opposans_m_22},
#endif
    {.name=NULL, .data=NULL}
};

/*Get built-in fonts
@api lvgl.font_get(name)
@string font name + font size, for example opposans_m_10
@return userdata font pointer
@usage

local font = lvgl.font_get("opposans_m_12")*/
int luat_lv_font_get(lua_State *L) {
    lv_font_t* font = NULL;
    const char* fontname = luaL_checkstring(L, 1);
    if (!strcmp("", fontname)) {
        LLOGE("Font name cannot be an empty string");
        return 0;
    }
    for (size_t i = 0; i < sizeof(fonts) / sizeof(lvfont_t); i++)
    {
        if (fonts[i].name == NULL) {
            break;
        }
        if (!strcmp(fontname, fonts[i].name)) {
            lua_pushlightuserdata(L, fonts[i].data);
            return 1;
        }
    }
    LLOGE("The corresponding font name %s was not found", fontname);
    for (size_t i = 0; i < sizeof(fonts) / sizeof(lvfont_t); i++)
    {
        if (fonts[i].name == NULL) {
            break;
        }
        LLOGI("Font names supported by this firmware: %s", fonts[i].name);
    }
    return 0;
}

/*Load fonts from file system
@api lvgl.font_load(path/spi_device,size,bpp,thickness,cache_size,sty_zh,sty_en)
@string/userdata font path/spi_device (spi_device uses an external Qualcomm vector font chip)
@number size optional, font size 16-192, default 16 (using Qualcomm vector font library)
@number bpp optional depth default 4 (using Qualcomm vector font library)
@number thickness optional thickness value default size * bpp (using Qualcomm vector font library)
@number cache_size optional default 0 (use Qualcomm vector font library)
@number sty_zh Optional Select font Default 1 (use Qualcomm vector font library)
@number sty_en Optional Select font Default 3 (use Qualcomm vector font library)
@return userdata font pointer
@usage
local font = lvgl.font_load("/font_32.bin")
--local font = lvgl.font_load(spi_device,16)(Qualcomm vector font library)*/
int luat_lv_font_load(lua_State *L) {
    lv_font_t *font = NULL;
    if (lua_isuserdata(L, 1)) {
        #ifdef LUAT_USE_GTFONT
            luat_spi_device_t *spi = lua_touserdata(L, 1);
            uint8_t size = luaL_optinteger(L, 2, 16);
            uint8_t bpp = luaL_optinteger(L, 3, 4);
            uint16_t thickness = luaL_optinteger(L, 4, size * bpp);
            uint8_t cache_size = luaL_optinteger(L, 5, 0);
            uint8_t sty_zh = luaL_optinteger(L, 6, 1);
            uint8_t sty_en = luaL_optinteger(L, 7, 3);

            if (!(bpp >= 1 && bpp <= 4 && bpp != 3)) {
                return 0;
            }
            if (gt_spi_dev == NULL) {
                gt_spi_dev = lua_touserdata(L, 1);
            }
            font = lv_font_new_gt(sty_zh, sty_en, size, bpp, thickness, cache_size);
        #else
        LLOGE("This firmware does not contain the driver for the Qualcomm font chip");
        #endif
    } else {
        const char* path = luaL_checkstring(L, 1);
        LLOGD("Load lvgl font %s from file", path);
        font = lv_font_load(path);
        if (!font) {
            LLOGE("Loading lvgl font %s from file failed", path);
        }
    }
    if (font) {
        lua_pushlightuserdata(L, font);
        return 1;
    }
    return 0;
}

/*Release fonts, use with caution!!! Only fonts loaded through font_load are allowed to be uninstalled, fonts obtained through font_get are not allowed to be uninstalled
@api lvgl.font_free(font)
@string font path
@return userdata font pointer
@usage
local font = lvgl.font_load("/font_32.bin")
-- N N N N operations
-- Make sure the font is not in use or referenced, and the memory is tight and needs to be released
lvgl.font_free(font)*/
int luat_lv_font_free(lua_State *L) {
    lv_font_t* font = lua_touserdata(L, 1);
    if (font) {
        if (lv_font_is_gt(font)) lv_font_del_gt(font);
        else lv_font_free(font);
    }
    return 0;
}
