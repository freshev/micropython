--[[
@module libfota
@summary libfota fota升级
@version 1.0
@date    2023.02.01
@author  Dozingfiretruck
@demo    fota
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
local libfota = require("libfota")

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
        rtos.reboot()   --If you have other things to do, don’t reboot immediately
    end
end

--Note!!!: When using the Hezhou IoT platform, you must use luatools to generate the .bin file!!! Self-built servers can use .ota files!!!
--Note!!!: When using the Hezhou IoT platform, you must use luatools to generate the .bin file!!! Self-built servers can use .ota files!!!
--Note!!!: When using the Hezhou IoT platform, you must use luatools to generate the .bin file!!! Self-built servers can use .ota files!!!

--The example below is the Hezhou IoT platform, address: http://iot.openluat.com
libfota.request(libfota_cb)

--If you use a self-built server, change the URL yourself
--The requirements for a custom server are:
--If an upgrade is required, respond with http 200, and the body is the content of the upgrade file.
--If you do not need to upgrade, please pay attention to the response code of 300 or above.
libfota.request(libfota_cb,"http://xxxxxx.com/xxx/upgrade?version=" .. _G.VERSION)

--If you need to upgrade regularly
--Hezhou IoT platform
sys.timerLoopStart(libfota.request, 4*3600*1000, libfota_cb)
--Self-built platform
sys.timerLoopStart(libfota.request, 4*3600*1000, libfota_cb, "http://xxxxxx.com/xxx/upgrade?version=" .. _G.VERSION)
]]

local sys = require "sys"
local sysplus = require "sysplus"

local libfota = {}


local function fota_task(cbFnc,storge_location, len, param1,ota_url,ota_port,libfota_timeout,server_cert, client_cert, client_key, client_password, show_otaurl)
    if cbFnc == nil then
        cbFnc = function() end
    end
    --If ota_url is not passed, then the Hezhou IoT platform is used.
    if ota_url == nil then
        if _G.PRODUCT_KEY == nil then
            --PRODUCT_KEY = "xxx" must be defined in main.lua
            --After creating a new project on the IoT platform, you can find it in the project details.
            log.error("fota", "iot.openluat.com need PRODUCT_KEY!!!")
            cbFnc(5)
            return
        else
            local x,y,z = string.match(_G.VERSION,"(%d+).(%d+).(%d+)")
            if x and y and z then
                version = x.."."..z
                local imei = ""
                if mobile then
                    imei = mobile.imei()
                elseif wlan and wlan.getMac then
                    imei = wlan.getMac()
                else
                    imei = mcu.unique_id():toHex()
                end
                ota_url = "http://iot.openluat.com/api/site/firmware_upgrade?project_key=" .. _G.PRODUCT_KEY .. "&imei=".. imei .. "&device_key=&firmware_name=" .. _G.PROJECT.. "_LuatOS-SoC_" .. rtos.bsp() .. "&version=" .. rtos.version():sub(2) .. "." .. version
            else
                log.error("fota", "_G.VERSION must be xxx.yyy.zzz!!!")
                cbFnc(5)
                return
            end
        end
    end
    local ret
    local opts = {timeout = libfota_timeout}
    if fota then
        opts.fota = true
    else
        os.remove("/update.bin")
        opts.dst = "/update.bin"
    end
    if show_otaurl == nil or show_otaurl == true then
        log.info("fota.url", ota_url)
    end
    local code, headers, body = http.request("GET", ota_url, nil, nil, opts, server_cert, client_cert, client_key, client_password).wait()
    log.info("http fota", code, headers, body)
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
        ret = 4
    end
    cbFnc(ret)
end

--[[
fota升级
@api libfota.request(cbFnc,ota_url,storge_location, len, param1,ota_port,libfota_timeout,server_cert, client_cert, client_key, client_password)
@function cbFnc 用户回调函数，回调函数的调用形式为：cbFnc(result) , 必须传
@string ota_url 升级URL, 若不填则自动使用合宙iot平台
@number/string storge_location 可选,fota数据存储的起始位置<br>如果是int，则是由芯片平台具体判断<br>如果是string，则存储在文件系统中<br>如果为nil，则由底层决定存储位置
@number len 可选,数据存储的最大空间
@userdata param1,可选,如果数据存储在spiflash时,为spi_device
@number ota_port 可选,请求端口,默认80
@number libfota_timeout 可选,请求超时时间,单位毫秒,默认30000毫秒
@string server_cert 可选,服务器ca证书数据
@string client_cert 可选,客户端ca证书数据
@string client_key 可选,客户端私钥加密数据
@string client_password 可选,客户端私钥口令数据
@boolean show_otaurl 可选,是否从日志中输出打印OTA升级包的URL路径，默认会打印
@return nil 无返回值
]]
function libfota.request(cbFnc,ota_url,storge_location, len, param1,ota_port,libfota_timeout,server_cert, client_cert, client_key, client_password, show_otaurl)
    sys.taskInit(fota_task, cbFnc,storge_location, len, param1,ota_url, ota_port,libfota_timeout or 30000,server_cert, client_cert, client_key, client_password, show_otaurl)
end

return libfota

