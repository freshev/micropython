--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "wifidemo"
VERSION = "1.0.0"

--[[
本demo需要 V100x系列固件, 不兼容V000x系列
https://gitee.com/openLuat/LuatOS/releases
]]

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require("sysplus")

sys.subscribe("IP_READY", function(ip)
    log.info("wlan", "ip ready", ip)
    --The connection is successful and you can initiate http, mqtt, and other requests.
end)

fdb.kvdb_init()

--Use the BOOT key, i.e. GPIO9, as a button to clear the distribution network information
BTN_BOOT = 9
gpio.debounce(BTN_BOOT, 1000)
gpio.setup(BTN_BOOT, function()
    log.info("gpio", "boot button pressed")
    sys.publish("BTN_BOOT")
end)

sys.taskInit(function()
    sys.wait(500) --Waiting for 500ms here is just for the convenience of reading the log, it is not necessary.
    wlan.init() --Initialize wifi protocol stack

    --Get the last saved distribution network information. If it exists, connect directly to the Internet. No network configuration is required.
    --Note that the data saved by fdb is stored when the power is turned off, and will not be cleared by refreshing the script/firmware.
    --If you need to completely clear the configuration information, you can call fdb.clear() to clear it completely.
    if fdb.kv_get("wlan_ssid") then
       wlan.connect(fdb.kv_get("wlan_ssid"), fdb.kv_get("wlan_passwd"))
       return --Just wait until you get online
    end

    --The following is the esptouch distribution network of smartconfig
    --For network distribution APP, please search esptouch, the latest version is 2.3.0
    --Use esptouch when distributing the network. Although esptouch V2 is also supported, esptouch has better compatibility.
    --ESP32C3 only supports 2.4G wifi, 5G wifi is not supported
    --When distributing the network, the mobile phone should be close to the Modules to facilitate faster network distribution.
    while 1 do
        --Start the distribution network, the default is esptouch mode
        wlan.smartconfig()
        local ret, ssid, passwd = sys.waitUntil("SC_RESULT", 180*1000) --Wait 3 minutes
        if ret == false then
            log.info("smartconfig", "timeout")
            wlan.smartconfig(wlan.STOP)
            sys.wait(3000) --Wait another 3 seconds to reconfigure the network, or reboot directly.
        else
            --After obtaining the network configuration, ssid and passwd will have values.
            log.info("smartconfig", ssid, passwd)
            log.info("fdb", "save ssid and passwd")
            --Fdb.gafsa("and lensd", master)
            --fdb.kv_set("wlan_passwd", passwd)

            --Wait 3 seconds before restarting, because esptouch will broadcast after connecting to the Internet, informing the APP that the network configuration is successful.
            --log.info("wifi", "wait 3s to reboot")
            -- sys.wait(3000)
            --It is recommended to restart here, but of course this is not mandatory.
            --After restarting, there is network configuration information, so it automatically connects.
            -- rtos.reboot()
            break
        end
    end
end)


--The following task demonstrates clearing distribution network information by pressing buttons.
--The effect achieved is: after powering on for 500ms, press and hold the BOOT button for more than 3 seconds to clear the network configuration information, then restart or flash the light quickly.
sys.taskInit(function()
    --After powering on, wait 500ms
    sys.wait(500)
    --Then start monitoring the BTN button
    while true do
        local flag = true
        while true do
            --Wait for the boot button to be pressed
            local ret = sys.waitUntil("BTN_BOOT", 3000)
            --log.info("gpio", "BTN_BOOT", "wait", ret)
            if ret then
                break
            end
        end
        log.info("wifi", "Got BOOT button pressed")
        for i=1, 30 do
            --It is required to maintain a low level for 3 seconds. If it is released midway, it will be invalid.
            if gpio.get(BTN_BOOT) ~= 0 then
                log.info("wifi", "BOOT button released, wait next press")
                flag = false
                break
            end
            sys.wait(100)
        end

        if flag then
            --If the user really wants to request network configuration information, then clear it.
            log.info("gpio", "boot pressed 3s, remove ssid/passwd")
            fdb.kv_del("wlan_ssid")
            fdb.kv_del("wlan_passwd")
            --fdb.clear() -- Another solution here is to clear all data in fdb, which is equivalent to restoring factory configuration from a business perspective
            log.info("gpio", "removed, wait for reboot")

            --Option 1, restart directly. After restarting, because there is no network configuration data, network configuration will automatically start.
            -- rtos.reboot()

            --Option 2, flash the light for 100ms, allowing the user to reset and restart by themselves
            gpio.setup(12, 0)
            while 1 do
                gpio.toggle(12)
                sys.wait(100)
            end
        end
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
