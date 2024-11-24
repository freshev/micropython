/*@Moduleslvgl
@summary LVGL Image Library
@version 1.0
@date 2021.06.01*/

#include "luat_base.h"
#include "luat_lvgl.h"
#include "lvgl.h"
#include "../exts/lv_gif/lv_gif.h"

/*Create gif component
@api lvgl.gif_create(parent, path)
@userdata parent component, can be nil, but usually will not be nil
@string file path
@return userdata component pointer, if it fails, it will return nil. It is recommended to check
@usage
local gif = lvgl.gif_create(scr, "S/emtry.gif")
if gif then
    log.info("gif", "create ok")
end*/
int luat_lv_gif_create(lua_State *L) {
    lv_obj_t * parent = lua_touserdata(L, 1);
    const char* path = luaL_checkstring(L, 2);
    lv_obj_t *gif = lv_gif_create_from_file(parent, path);
    lua_pushlightuserdata(L, gif);
    return 1;
}

/*Replay gif component
@api lvgl.gif_restart(gif)
@userdata gif component support, returned by gif_create method
@return nil no return value
@usage
local gif = lvgl.gif_create(scr, "S/emtry.gif")
if gif then
    log.info("gif", "create ok")
end*/
int luat_lv_gif_restart(lua_State *L) {
    lv_obj_t * gif = lua_touserdata(L, 1);
    lv_gif_restart(gif);
    return 0;
}

int luat_lv_gif_delete(lua_State *L);
