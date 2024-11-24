local home = {}

local obj_list = nil--Store newly created objects
local selected = 1--currently selected button

--Show content of this page
function home.show()
    local theme_btn = lvgl.btn_create(scr)
    lvgl.obj_set_size(theme_btn,70,46)
    lvgl.obj_set_x(theme_btn,5)
    lvgl.obj_set_y(theme_btn,-50)
    local theme_label = lvgl.label_create(theme_btn)
    lvgl.label_set_text(theme_label, "theme")

    local game_btn = lvgl.btn_create(scr)
    lvgl.obj_set_size(game_btn,70,46)
    lvgl.obj_set_x(game_btn,5)
    lvgl.obj_set_y(game_btn,-50)
    local game_label = lvgl.label_create(game_btn)
    lvgl.label_set_text(game_label, "reboot")

    local about_btn = lvgl.btn_create(scr)
    lvgl.obj_set_size(about_btn,70,46)
    lvgl.obj_set_x(about_btn,5)
    lvgl.obj_set_y(about_btn,-50)
    local about_label = lvgl.label_create(about_btn)
    lvgl.label_set_text(about_label, "about")

    if not obj_list then--The newly created object is saved for later destruction.
        obj_list = {--Store in order, even-numbered subscripts are button objects
            theme_label,
            theme_btn,
            game_label,
            game_btn,
            about_label,
            about_btn,
        }
    end

    home.selectChange()--Show the currently pressed keys

    --Let’s do an animation
    animation.once(theme_btn,lvgl.obj_set_y,-50,5,nil,900)
    animation.once(game_btn,lvgl.obj_set_y,-50,56,nil,700)
    animation.once(about_btn,lvgl.obj_set_y,-50,109,nil,500)
end

--Uninstall this page
function home.unload()
    if not obj_list then return end
    for i=1,#obj_list do
        lvgl.obj_del(obj_list[i])
    end
    obj_list = nil
end

--Change selected button
function home.selectChange(n)
    if not n then n = 0 end
    --Not shown yet
    if not obj_list then home.show() end
    --Which one have you chosen now?
    selected = selected + n
    if selected > 3 then selected = 1 end
    if selected < 1 then selected = 3 end
    --Set the state of each button
    for i=2,#obj_list,2 do
        if i/2 == selected then
            lvgl.btn_set_state(obj_list[i],lvgl.BTN_STATE_PRESSED)
        else
            lvgl.btn_set_state(obj_list[i],lvgl.BTN_STATE_RELEASED)
        end
    end
end

--Theme control and switching
local themes = {"material_light","material_dark"}
local themeNow = 1
function home.changeTheme()
    themeNow = themeNow + 1
    if themeNow > #themes then themeNow = 1 end
    lvgl.theme_set_act(themes[themeNow])
end

--event list
local keyList = {
    U = function ()
        home.selectChange(-1)
    end,
    D = function ()
        home.selectChange(1)
    end,
    O = function ()
        --todo
        if selected == 1 then
            home.changeTheme()
        elseif selected == 2 then
            rtos.reboot()
        elseif selected == 3 then
            home.unload()
            about.show()
            key.setCb(about.keyCb)
        end
    end,
}
keyList.L = keyList.U
keyList.R = keyList.D

--Key event handling
function home.keyCb(k)
    if keyList[k] then keyList[k]() end
end


return home
