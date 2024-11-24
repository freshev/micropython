sys = require("sys")

log.info("sys", "from win32")

sys.taskInit(function()
    log.info("lvgl", lvgl.init(480, 320))
    -- gui.setup_ui()

    --local scr = lvgl.obj_create()
    local btn = lvgl.btn_create(lvgl.scr_act())
    lvgl.obj_set_x(btn, 10)
    lvgl.obj_set_y(btn, 100)
    --lvgl.btn_set_title("Hi, LuatOS-Soc")

    local anim = lvgl.anim_create()
    lvgl.anim_set_var(anim, btn)
    --Note that this method requires 3 parameters, and the third parameter can be a custom function (obj, val)
    lvgl.anim_set_exec_cb(anim, lvgl.obj_set_x)
    --lvgl.anim_set_exec_cb(anim, function(obj, val)
    --lvgl.obj_set_x(obj, val)
    -- end)

    lvgl.anim_set_values(anim, 10, 240);

    while true do --For demonstration purposes, here we restore the object to its default position and then restart the animation.
        sys.wait(5000)
        lvgl.obj_set_x(btn, 10)
        lvgl.anim_start(anim)
    end

    lvgl.anim_free(anim) --Different from C, the anim here needs to be actively released
end)

sys.run()
