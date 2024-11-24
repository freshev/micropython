--[[
@module libfota2
@summary fota升级v2
@version 1.0
@date    2024.04.09
@author  wendal
@demo    fota2
@usage
--Usage examples
local libfota2 = require("libfota2")

--Function: Get the callback function of fota
--parameter:
--result:number type
--0 means success
--1 means the connection failed
--2 means url error
--3 means the server is disconnected
--4 indicates an error in receiving the message.
--5 indicates that using the IoT platform VERSION requires the use of xxx.yyy.zzz form
function libfota_cb(result)
    log.info("fota", "result", result)
    --success
    if result == 0 then
        rtos.reboot()   --If you have other things to do, decide the timing of reboot by yourself.
    end
end

--The example below is the Hezhou IoT platform, address: http://iot.openluat.com
libfota2.request(libfota_cb)

--If you use a self-built server, change the URL yourself
--The requirements for a custom server are:
--If an upgrade is required, respond with http 200, and the body is the content of the upgrade file.
--If you do not need to upgrade, please pay attention to the response code of 300 or above.
local opts = {url="http://xxxxxx.com/xxx/upgrade"}
--For detailed description of opts, see the function API documentation later.
libfota2.request(libfota_cb, opts)

--If you need to upgrade regularly
--Hezhou IoT platform
sys.timerLoopStart(libfota2.request, 4*3600*1000, libfota_cb)
--Self-built platform
sys.timerLoopStart(libfota2.request, 4*3600*1000, libfota_cb, opts)
]]

local sys = require "sys"
require "sysplus"

local libfota2 = {}


local function fota_task(cbFnc, opts)
    local ret = 0
    local code, headers, body = http.request(opts.method, opts.url, opts.headers, opts.body, opts, opts.server_cert, opts.client_cert, opts.client_key, opts.client_password).wait()
    --log.info("http fota", code, headers, body)
    if code == 200 or code == 206 then
        if body == 0 then
            ret = 4
        else
            ret = 0
        end
    elseif code == -4 then
        ret = 1
    elseif code == -5 then
        ret = 3
    else
        log.info("fota", code, body)
        ret = 4
    end
    cbFnc(ret)
end

--[[
fota升级
@api libfota.request(cbFnc, opts)
@table fota参数, 后面有详细描述
@function cbFnc 用户回调函数，回调函数的调用形式为：cbFnc(result) , 必须传
@return nil 无返回值
@usaga

--opts parameter description, all parameters are optional
--1. opts.url string The URL required for upgrade. If you use the Hezhou IoT platform, you do not need to fill it in.
--2. opts.version string version number, the default is BSP version number.x.z format
--3. opts.timeout int request timeout, default 300000 milliseconds, unit milliseconds
--4. opts.project_key string is the project key of Hezhou IOT platform. By default, it takes the global variable PRODUCT_KEY. It is not necessary to fill in the self-built server.
--5. opts.imei string device identification code, the default is IMEI (Cat.1 Modules) or WLAN MAC address (wifi Modules) or MCU unique ID
--6. opts.firmware_name string firmware name, the default is _G.PROJECT.. "_LuatOS-SoC_" .. rtos.bsp()
--7. opts.server_cert string server certificate, not used by default
--8. opts.client_cert string client certificate, not used by default
--9. opts.client_key string client private key, not used by default
--10. opts.client_password string client private key password, not used by default
--11. opts.method string request method, the default is GET
--12. opts.headers table additional request headers, not required by default
--13. opts.body string additional request body, not required by default
]]
function libfota2.request(cbFnc, opts)
    if not opts then
        opts = {}
    end
    if fota then
        opts.fota = true
    else
        os.remove("/update.bin")
        opts.dst = "/update.bin"
    end
    if not cbFnc then
        cbFnc = function() end
    end
    --Handle URL
    if not opts.url then
        opts.url = "http://iot.openluat.com/api/site/firmware_upgrade"
    end
    if opts.url:sub(1, 4) ~= "###" and not opts.url_done then
        --Complete the project_key function
        if not opts.project_key then
            opts.project_key = _G.PRODUCT_KEY
            if not opts.project_key then
                log.error("fota", "iot.openluat.com need PRODUCT_KEY!!!")
                cbFnc(5)
                return
            end
        end
        --Complete the version parameter
        if not opts.version then
            local x,y,z = string.match(_G.VERSION,"(%d+).(%d+).(%d+)")
            opts.version = rtos.version():sub(2) .. "." .. x.."."..z
        end
        --Complete the firmware_name parameter
        if not opts.firmware_name then
            opts.firmware_name = _G.PROJECT.. "_LuatOS-SoC_" .. rtos.bsp()
        end
        --Complete imei parameters
        if not opts.imei then
            local imei = ""
            if mobile then
                imei = mobile.imei()
            elseif wlan and wlan.getMac then
                imei = wlan.getMac()
            else
                imei = mcu.unique_id():toHex()
            end
            opts.imei = imei
        end

        --Then spliced   into the final url
        opts.url = string.format("%s?imei=%s&project_key=%s&firmware_name=%s&version=%s", opts.url, opts.imei, opts.project_key, opts.firmware_name, opts.version)
    else
        opts.url = opts.url:sub(4)
        opts.url_done = true
    end
    --Processing method
    if not opts.method then
        opts.method = "GET"
    end
    log.info("fota.url", opts.method, opts.url)
    log.info("fota.imei", opts.imei)
    log.info("fota.project_key", opts.project_key)
    log.info("fota.firmware_name", opts.firmware_name)
    log.info("fota.version", opts.version)
    sys.taskInit(fota_task, cbFnc, opts)
end

return libfota2
