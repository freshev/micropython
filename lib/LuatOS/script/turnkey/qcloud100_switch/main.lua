--[[
这个代码对应公众号文档 https://mp.weixin.qq.com/s/ST_8Uej8R7qLUsikfh3Xtw

针对的硬件是 ESP32系列
]]

PROJECT = "qcloud100"
VERSION = "1.0.0"

--Test support hardware: ESP32C3/Air105/Air780E

local sys = require "sys"
require("sysplus")

--[[
使用指南
1. 登录腾讯云 https://cloud.tencent.com/ 扫码登录
2. 访问物联网平台 https://console.cloud.tencent.com/iotexplorer
3. 进入公共实例, 如果没有就开通
4. 如果没有项目, 点"新建项目", 名字随意, 例如 luatos
5. 这时,新项目就好出现在列表里, 点击进去
6. 接下来, 点"新建产品", 产品名称随意(例如abc), 品类选自定义品类, 通信方式选4G或者Wifi,认证方式选择密钥认证,数据协议选物模型
7. 左侧菜单, 选"设备管理", "添加设备", 产品选刚刚建好的, 输入一个英文的设备名称, 例如 abcdf, 确定后新设备出现在列表里
8. 点击该设备的"查看", 就能看到详情的信息, 逐个填好
9. 如果是wifi设备, 在当前文件搜索ssid,填好wifi名称和密码
10. 接下来刷机, 不懂刷机就到 wiki.luatos.com 看视频

补充说明, 物模型的设置:
1. 左侧菜单, "产品开发", 然后点之前建好的项目
2. 页面往下拉到底, 选择"新建自定义功能"
3. 为了跑通这个本例子, 需要新建3个属性, 对着界面逐个添加
   |功能类型|功能名称|标识符       |数据类型|取值范围    |
   |属性    |LED灯   |power_switch|布尔型  |不用选     |
   |属性    |电压    |vbat        |整型    | -1, 100000|
   |属性    |温度    |temp        |整型    | -1, 100000|

不要输入"|",那是分割符
]]
product_key = "K7NURKF5J3" --Product ID, a string of English letters, fill in the preceding double quotes
device_id = "abcdf" --The device name must be changed to your own data.
device_secret = "ChhVXLtSPiTKFzGatB80Jw==" --device key


sub_topic = "$thing/down/property/" .. product_key .. "/".. device_id
pub_topic = "$thing/up/property/" .. product_key .. "/".. device_id

adc.open(adc.CH_VBAT)
adc.open(adc.CH_CPU)

function on_btn()
    log.info("btn", "按键触发")
    --Press once to report the voltage value once
    --Format reference https://cloud.tencent.com/document/product/1081/34916
    if mqttc and mqttc:ready() then
        
        local data = {
            method = "report",
            clientToken = "123",
            params = {
                vbat = adc.get(adc.CH_VBAT), --Read voltage value
                temp = adc.get(adc.CH_CPU)
                --If you create more attributes of the object model, you can continue to fill them in here. Pay attention to the format, especially the commas.
            }
        }
        local jdata = (json.encode(data))
        log.info("准备上传数据", jdata)
        --You can directly call the mqttc object for reporting
        --mqttc:publish(pub_topic, (json.encode(data)), 1)
        --It can also be passed to the sys.waitUntil("mqtt_pub", 30000) statement for indirect reporting through the message mechanism.
        sys.publish("mqtt_pub", pub_topic, jdata, 1)
    end
end

sys.taskInit(function()
    --Unified networking functions
    if wlan and wlan.connect then --ESP32 series, C3/S3 are both available
        if rtos.bsp():startsWith("ESP32") then
            LED = gpio.setup(12, 0, gpio.PULLUP) --The GPIO number corresponding to the controlled light
            gpio.debounce(9, 100, 1)
            BTN = gpio.setup(9, on_btn, gpio.PULLUP, gpio.FALLING) --BOOT button is used as a button
        else
            --air101/air103, for internal testing
            LED = gpio.setup(pin.PB09, 0, gpio.PULLUP) --The GPIO number corresponding to the controlled light
            gpio.debounce(pin.PA0, 100, 1)
            BTN = gpio.setup(pin.PA0, on_btn, gpio.PULLUP, gpio.FALLING) --BOOT button is used as a button
        end
        wlan.init()
        --
        --For ESP32 series, you need to fill in the name and password of the wifi here. Only supports the 2.4G frequency band.
        --
        local ssid, password = "luatos1234", "12341234"
        wlan.setMode(wlan.STATION)
        wlan.connect(ssid, password, 1)
    elseif rtos.bsp() == "AIR105" then --Air105 network card, W5500
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
        gpio.debounce(pin.PA10, 100, 1)
        BTN = gpio.setup(pin.PA10, on_btn, gpio.PULLUP, gpio.FALLING) --BOOT button is used as a button
        sys.wait(1000)
    elseif mobile then --Air780E, use 4G network
        --mobile.simid(2) -- Automatically select the card. If you don't know which card slot it is, uncomment it.
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        gpio.debounce(0, 100, 1)
        BTN = gpio.setup(0, on_btn, gpio.PULLUP, gpio.FALLING) --BOOT button is used as a button
    else
        while 1 do 
            sys.wait(1000)
            log.info("bsp", "未支持的模块", rtos.bsp())
        end
    end
    log.info("等待联网")
    local result, data = sys.waitUntil("IP_READY")
    log.info("wlan", "IP_READY", result, data)
    log.info("联网成功")
    sys.publish("net_ready")
end)

sys.taskInit(function()
    sys.waitUntil("net_ready")
    --The next step is to connect to Tencent Cloud.

    local client_id, user_name, password = iotauth.qcloud(product_key, device_id, device_secret, "sha1")
    log.info("mqtt参数", client_id, user_name, password)

    --MQTT parameters are ready, start connecting, and monitor data delivery
    mqttc = mqtt.create(nil, product_key .. ".iotcloud.tencentdevices.com", 1883)
    mqttc:auth(client_id, user_name, password)
    mqttc:keepalive(240) --Default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism
    mqttc:on(
        function(mqtt_client, event, data, payload)
            if event == "conack"then
                --It's connected and the authentication is OK.
                sys.publish("mqtt_conack")
                log.info("mqtt", "mqtt已连接")
                mqtt_client:subscribe(sub_topic)
            elseif event == "recv" then
                log.info("mqtt", "收到消息", data, payload)
                local json = json.decode(payload)
                if json.method == "control" then
                    if json.params.power_switch == 1 then
                        LED(1)
                    elseif json.params.power_switch == 0 then
                        LED(0)
                    end
                end
            elseif event == "sent"then
                log.info("mqtt", "sent", "pkgid", data)
            end
        end
    )
    mqttc:connect()
    --sys.wait(1000)
    sys.waitUntil("mqtt_conack")
    while true do
        local ret, topic, data, qos = sys.waitUntil("mqtt_pub", 30000)    
        if ret then
            if topic == "close" then
                break
            end
            mqttc:publish(topic, data, qos)
        end
    end
    mqttc:close()
    mqttc = nil
end)



--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
