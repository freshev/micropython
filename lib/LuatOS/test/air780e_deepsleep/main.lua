--[[
    对应的issue https://gitee.com/openLuat/LuatOS/issues/I6GZGQ
]]

PROJECT = "gnss"
VERSION = "1.0.0"

--sys library is standard
local sys = require("sys")

uart.setup(2, 115200)
uart.on(2, "recv", function()
    while 1 do
        local data = uart.read(2, 1024)
        if not data or #data == 0 then
            return
        end
        log.info("uart2", data)
    end
end)
function gpsinit()
    log.info("GPS", "开始启动")
    pm.power(pm.GPS, true)
    log.info("GPS", "启动完成")
end

mobile.flymode(0, false)

--hibernation process
sys.taskInit(function()
    gpsinit()
    sys.wait(20 * 1000)
    log.info("开始休眠")
    mobile.flymode(0, true)
    pm.dtimerStart(0, 15 * 1000)
    pm.power(pm.GPS, false)
    pm.power(pm.USB, false) --If you are testing with the USB plugged in, you need to turn off the USB
    pm.force(pm.DEEP) --You can also use pm.HIB mode
    sys.wait(1000)
    log.info("休眠失败")
end)

sys.run()
