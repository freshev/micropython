
--Low power consumption demonstration
sys.taskInit(function()
    sys.waitUntil("aliyun_ready")
    log.info("aliyun.pm", "阿里云已经连接成功, 5秒后请求进入低功耗模式, USB功能会断开")
    sys.wait(5000)
    local bsp = rtos.bsp():upper()
    --Enter low power mode
    if bsp == "EC618" then
        log.info("aliyun.pm", "EC618方案进入低功耗模式")
        -- gpio.setup(23,nil)
        -- gpio.close(33)
        --mobile.rtime(2) -- RRC quick release to reduce the connection time can greatly reduce power consumption, but it will bring possible offline risks. You can choose to delay the time or not
        pm.power(pm.USB, false)
        pm.force(pm.LIGHT)
    elseif bsp == "EC718P" or bsp == "EC718PV" then
        log.info("aliyun.pm", "EC718P/EC718PV方案进入低功耗模式")
        --mobile.rtime(2) -- RRC quick release to reduce the connection time can greatly reduce power consumption, but it will bring possible offline risks. You can choose to delay the time or not
        pm.power(pm.USB, false)
        pm.force(pm.LIGHT)
    elseif bsp == "AIR101" or bsp == "AIR601" or bsp == "AIR103" then
        log.info("aliyun.pm", "XT804方案进入低功耗模式")
        while 1 do
            pm.dtimerStart(0, 30000)
            pm.request(pm.LIGHT)
            sys.wait(30000)
        end
    end
end)
