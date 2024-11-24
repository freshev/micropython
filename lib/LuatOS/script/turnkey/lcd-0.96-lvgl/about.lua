local about = {}

local obj_list = nil--Store newly created objects

--Show content of this page
function about.show()
    local w = lvgl.win_create(scr)
    lvgl.win_set_title(w, "About")
    local t = lvgl.label_create(w)
    lvgl.label_set_text(t, 
[[Air10x core screen
press OK return
by luatos]])
    obj_list = {t,w}
end

--Uninstall this page
function about.unload()
    if not obj_list then return end
    for i=1,#obj_list do
        lvgl.obj_del(obj_list[i])
    end
    obj_list = nil
end

--Key event handling
function about.keyCb(k)
    if k == "O" then--Return to homepage
        about.unload()
        home.show()
        key.setCb(home.keyCb)
    elseif k == "L" or k =="U" then--Scroll left and right
        lvgl.win_scroll_hor(obj_list[2],30)
    elseif k == "R" or k =="D" then
        lvgl.win_scroll_hor(obj_list[2],-30)
    end
end

return about
