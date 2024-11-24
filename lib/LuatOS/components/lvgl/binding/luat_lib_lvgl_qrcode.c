/*@Moduleslvgl
@summary LVGL Image Library
@version 1.0
@date 2021.06.01*/


#include "luat_base.h"
#include "luat_lvgl.h"
#include "lvgl.h"
#include "../exts/lv_qrcode/lv_qrcode.h"

/**
Create qrcode component
@api lvgl.qrcode_create(parent, size, dark_color, light_color)
@userdata parent component
@int length, because qrcode is a square
@int Color of data points in QR code, RGB color, default 0x3333ff
@int The color of the background point in the QR code, RGB color, default 0xeeeeff
@return userdata qrcode component
@usage
--Create and display qrcode
local qrcode = lvgl.qrcode_create(scr, 100)
lvgl.qrcode_update(qrcode, "https://luatos.com")
lvgl.obj_align(qrcode, lvgl.scr_act(), lvgl.ALIGN_CENTER, -100, -100)*/
int luat_lv_qrcode_create(lua_State *L) {
    lv_obj_t * parent = lua_touserdata(L, 1);
    lv_coord_t size = luaL_checkinteger(L, 2);
    int32_t dark_color = luaL_optinteger(L, 3, 0x3333ff);
    int32_t light_color = luaL_optinteger(L, 4, 0xeeeeff);
    lv_obj_t * qrcode = lv_qrcode_create(parent, size, lv_color_hex(dark_color), lv_color_hex(light_color));
    lua_pushlightuserdata(L, qrcode);
    return 1;
}

/**
Set the QR code content of the qrcode component and use it with qrcode_create
@api lvgl.qrcode_update(qrcode, cnt)
@userdata qrcode component, created by qrcode_create
@string QR code content data
@return bool Returns true if the update is successful, otherwise returns false. Usually, false will be returned only if the data is too long to be accommodated.*/
int luat_lv_qrcode_update(lua_State *L) {
    lv_obj_t * qrcode = lua_touserdata(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    lv_res_t ret = lv_qrcode_update(qrcode, data, len);
    lua_pushboolean(L, ret == LV_RES_OK ? 1 : 0);
    return 1;
}

/**
Delete qrcode component
@api lvgl.qrcode_delete(qrcode)
@userdata qrcode component, created by qrcode_create
@return nil no return value*/
int luat_lv_qrcode_delete(lua_State *L) {
    lv_obj_t * qrcode = lua_touserdata(L, 1);
    lv_qrcode_delete(qrcode);
    return 0;
}
