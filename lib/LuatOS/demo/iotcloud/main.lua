
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "mqttdemo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the mqtt library requires the following statements]]
_G.sysplus = require("sysplus")

local iotcloud = require("iotcloud")

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Unified networking functions
sys.taskInit(function()
    local device_id = mcu.unique_id():toHex()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to automatic network distribution
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        device_id = wlan.getMac()
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2) -- Automatically switch SIM cards
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
    elseif w5500 then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
    elseif socket or mqtt then
        --The adapted socket library is also OK
        --No other operations, just give a comment
    else
        --For other bsps that I don’t know, let me give you some hints in a loop.
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本bsp可能未适配网络层, 请查证")
        end
    end
    --By default, it waits until the connection is successful
    sys.waitUntil("IP_READY")
    sys.publish("net_ready", device_id)
end)

sys.taskInit(function()
    --Waiting for Internet connection
    local ret, device_id = sys.waitUntil("net_ready")

    -------- Modify the following access methods according to your own needs, and modify the relevant parameters to your own ----------

    --Tencent Cloud
    --Dynamic registration
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx" ,product_secret = "xxx"})

    --Key verification
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "123456789",key = "xxx=="})
    --Certificate verification
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "123456789"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})


    --Alibaba Cloud
    --Dynamic registration (no pre-registration required)
    --iotcloudc = iotcloud.new(iotcloud.ALIYUN,{produt_id = "xxx",product_secret = "xxx"})
    --Key verification (pre-registration)
    --iotcloudc = iotcloud.new(iotcloud.ALIYUN,{produt_id = "xxx",device_name = "xxx",key = "xxx"})


    -- onenet
    --Dynamic registration
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",userid = "xxx",userkey = "xxx"})
    --One type, one secret
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",product_secret = "xxx"})
    --One machine, one secret
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",device_name = "xxx",key = "xxx"})

    --Huawei Cloud
    --Dynamic registration (no pre-registration required)
    -- iotcloudc = iotcloud.new(iotcloud.HUAWEI,{produt_id = "xxx",project_id = "xxx",endpoint = "xxx",
    --                         iam_username="xxx",iam_password="xxx",iam_domain="xxx"})
    --Key verification (pre-registration)
    -- iotcloudc = iotcloud.new(iotcloud.HUAWEI,{produt_id = "xxx",endpoint = "xxx",device_name = "xxx",device_secret = "xxx"})




    if iotcloudc then
        iotcloudc:connect()
    end



end)

sys.subscribe("iotcloud", function(cloudc,event,data,payload)
    if event == iotcloud.CONNECT then --The cloud platform is connected
            --iotcloud:subscribe("test") -- Subscribe to the topic
    elseif event == iotcloud.RECEIVE then
            print("iotcloud","topic", data, "payload", payload)
            --User handling code
    elseif event ==  iotcloud.OTA then
        if data then
            rtos.reboot()
        end
    elseif event == iotcloud.DISCONNECT then --The cloud platform is disconnected
            --User handling code
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
