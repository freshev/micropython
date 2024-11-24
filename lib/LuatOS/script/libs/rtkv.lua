--[[
@module rtkv
@summary 远程KV数据库
@version 1.0
@date    2023.07.17
@author  wendal
@tag LUAT_USE_NETWORK
@usage

--Are you still worried about reporting several data values?
--Are you still having headaches about storing data in the database?
--There is no external network server, and intranet penetration is very troublesome?
--Don’t understand mqtt, and have no distribution requirements, but just want to report some values?

--That API is right for you
--It can:
--Save data to the server, such as temperature and humidity, GPS coordinates, GPIO status
--Read server data, such as OTA information
--The server will save historical records and also support drawing into charts.
--It cannot:
--Send data to devices in real time
--Upload huge amounts of data

--Home page of the website, enter the device identification number to see the data https://rtkv.air32.cn
--Sample device http://rtkv.air32.cn/d/6055F9779010

--Scenario example 1, report temperature and humidity data to the server, and then the website viewing address is XXX
rtkv.setup()
sys.taskInit(function()
    sys.waitUntil("IP_READY")
    while 1 do
        local val,result = sensor.ds18b20(17, true) 
        if result then
            rtkv.set("ds18b20_temp", val)
        end
        sys.wait(60*1000) --Report once a minute
    end
end)

--Scenario example 2, simple version OTA
rtkv.setup()
sys.taskInit(function()
    sys.waitUntil("IP_READY")
    sys.wait(1000)
    while 1 do
        local ota_version = rtkv.get("ota_version")
        if ota_version and ota_version ~= _G.VERSION then
            local ota_url = rtkv.get("ota_url")
            if ota_url then
                --Execute OTA, taking esp32c3 as an example
                local code = http.request("GET", ota_url, nil, nil, {dst="/update.bin"}).wait()
                if code and code == 200 then
                    log.info("ota", "ota包下载完成, 5秒后重启")
                    sys.wait(5000)
                    rtos.reboot()
                end
            end
        end
        sys.wait(4*3600*1000) --Check once every 4 hours
    end
end)

--Scenario example 3, non-real-time delivery control
rtkv.setup()
sys.taskInit(function()
    local LED = gpio.setup(27, 0, nil, gpio.PULLUP)
    local INPUT = gpio.setup(22, nil)
    sys.waitUntil("IP_READY")
    sys.wait(1000)
    while 1 do
        local gpio27 = rtkv.get("gpio27")
        if gpio27 then
            LED(gpio27 == "1" and 1 or 0)
        end
        rtkv.set("gpio22", INPUT()) --Report the status of GPIO22
        sys.wait(15*1000) --Query once every 15 seconds
    end
end)
]]

local rtkv = {}

--[[
rtkv初始化
@api rtkv.setup(conf)
@table 配置信息,详细说明看下面的示例
@return nil 没有返回值
@usage
--This function only needs to be called once, usually in main.lua

--Default initialization, debug log enabled
rtkv.setup()
--Initialize and close debug logs
rtkv.setup({nodebug=true})
--Detailed initialization, you can only fill in the items that need to be configured
rtkv.setup({
    apiurl = "http://rtkv.air32.cn", --Server address, you can deploy it yourself https://gitee.com/openLuat/luatos-service-rtkv
    device = "abc", --The device identification number can only be English characters + numerical values, case-sensitive
    token = "123456", --Device key, the default is the unique id of the device, that is, mcu.unique_id()
    nodebug = false,  --Turn off debugging log, default false
    timeout = 3000, --Request timeout, in milliseconds, default 3000 milliseconds
})

--About the default value of device value
--If 4G is supported, IMEI will be luat
--If wifi is supported, MAC will be used
--In other cases, use mcu.unique_id(), which is the unique id of the device.
]]
function rtkv.setup(conf)
    if not conf then
        conf = {}
    end
    rtkv.conf = conf
    if not rtkv.conf.apiurl then
        conf.apiurl = "http://rtkv.air32.cn"
    end
    if not conf.device then
        if mobile then
            conf.device = mobile.imei()
        elseif wlan then
            conf.device = wlan.getMac()
        else
            conf.device = mcu.unique_id():toHex()
        end
    end
    if not conf.token then
        conf.token = mcu.unique_id():toHex()
    end
    if not conf.timeout then
        conf.timeout = 3000
    end
    if not conf.nodebug then
        --log.info("rtkv", "apiurl", conf.apiurl)
        log.info("rtkv", "device", conf.device)
        log.info("rtkv", "token", conf.token)
        log.info("rtkv", "pls visit", conf.apiurl .. "/d/" .. conf.device)
    end
    return true
end

--[[
设置指定键对应的值
@api rtkv.set(key, value)
@string 键, 不能为nil,建议只使用英文字母/数字
@string 值, 不能为nil,一般建议不超过512字节
@return bool   成功返回true, 否则返回nil
@usage

--If you care about the execution result, you need to execute it in the task
--Non-task context, will return nil, and then execute in the background
rtkv.set("age", "18")
rtkv.set("version", _G.VERSION)
rtkv.set("project", _G.PROJECT)

--A note about value types
--Supports passing in strings, Boolean values, integers, and floating point numbers, which will eventually be converted to strings for upload.
--When getting a value through rtkv.get, the type of the returned value will also be a string.
]]
function rtkv.set(key, value)
    if not rtkv.conf or not key or not value then
        return
    end
    local url = rtkv.conf.apiurl .. "/api/rtkv/set?"
    url = url .. "device=" .. rtkv.conf.device
    url = url .. "&token=" .. rtkv.conf.token
    url = url .. "&key=" .. tostring(key):urlEncode()
    url = url .. "&value=" .. tostring(value):urlEncode()
    if rtkv.conf.debug then
        log.debug("rtkv", url)
    end
    local co, ismain = coroutine.running()
    if ismain then
        sys.taskInit(http.request, "GET", url)
    else
        local code, headers, body = http.request("GET", url, nil, nil, {timeout=rtkv.conf.timeout}).wait()
        if rtkv.conf.debug then
            log.info("rtkv", code, body)
        end
        if code and code == 200 and body == "ok" then
            return true
        end
    end
end

--[[
批量设置键值
@api rtkv.sets(datas)
@table 需要设置的键值对
@return bool   成功返回true, 否则返回nil
@usage
--If you care about the execution result, you need to execute it in the task
--Non-task context, will return nil, and then execute in the background
rtkv.sets({
    age = "18",
    vbat = 4193,
    temp = 23423
})
]]
function rtkv.sets(datas)
    local conf = rtkv.conf
    if not conf or not datas then
        return
    end
    local url = conf.apiurl .. "/api/rtkv/sets"
    local rbody = json.encode({
        device = conf.device,
        token  = conf.token,
        data = datas
    })
    if not rbody then
        log.info("rtkv", "rbody is nil")
        return
    end
    if not conf.nodebug then
        log.debug("rtkv", url, rbody)
    end
    local rheaders = {}
    rheaders["Content-Type"] = "application/json"
    local co, ismain = coroutine.running()
    if ismain then
        sys.taskInit(http.request, "POST", url, rheaders, rbody, {timeout=conf.timeout})
    else
        local code, headers, body = http.request("POST", url, rheaders, rbody, {timeout=conf.timeout}).wait()
        if not conf.nodebug then
            log.info("rtkv", code, body)
        end
        if code and code == 200 and body == "ok" then
            return true
        end
    end
end

--[[
获取指定键对应的值
@api rtkv.get(key)
@string 键, 不能为nil,长度需要2字节以上
@return string 成功返回字符,其他情况返回nil
@usage
--Note that it must be executed in a task, otherwise nil will be returned.
local age = rtkv.get("age")
]]
function rtkv.get(key)
    local conf = rtkv.conf
    if not conf or key then
        return
    end
    local url = conf.apiurl .. "/api/rtkv/get?"
    url = url .. "device=" .. conf.device
    url = url .. "&token=" .. conf.token
    url = url .. "&key=" .. tostring(key):urlEncode()
    if not conf.nodebug then
        log.debug("rtkv", "url", url)
    end
    local co, ismain = coroutine.running()
    if ismain then
        log.warn("rtkv", "must call in a task/thread")
        return
    else
        local code, headers, body = http.request("GET", url, nil, nil, {timeout=conf.timeout}).wait()
        if not conf.nodebug then
            log.info("rtkv", code, body)
        end
        if code and code == 200 and body == "ok" then
            return true
        end
    end
end

return rtkv
