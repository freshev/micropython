PROJECT = "poweron_tool"
VERSION = "1.0.0"

--Test support hardware: ESP32C3
--Test firmware version: LuatOS-SoC_V0003_ESP32C3[_USB].soc

sys = require "sys"

---------------------------------------------------------------------------
--Change these things to your own use
local wifiName,wifiPassword = "wifi", "password"--wifi account password
local pcIp =  "255.255.255.255"--Target pc IP, broadcast is 255.255.255.255
local pcMac = "112233445566" --Just write the hex value of mac
--For details on how to use the server, see https://api.luatos.org/#poweron
local mqttAddr = "mqtt://apicn.luatos.org:1883"--This is a public server that only allows subscription and push to two topics: poweron/request/+ and poweron/reply/+
local mqttUser,mqttPassword = "13xxxxxxxxx","888888"--Your ERP account and password are used to connect to the mqtt server. The default is eight 8 erp.openluat.com
local subscribeTopic,subscribePayload = "poweron/request/chenxuuu","poweron"
local replyTopic,replyPayload = "poweron/reply/chenxuuu","ok"
---------------------------------------------------------------------------

connected = false

--2 LEDs on the development board
local LED_D4 = gpio.setup(12, 0)
local LED_D5 = gpio.setup(13, 0)

sys.taskInit(function()
    while true do
        if connected then
            LED_D4(1)
            sys.wait(1000)
        else
            LED_D4(0)
            sys.wait(200)
            LED_D4(1)
            sys.wait(200)
        end
    end
end)


function wakeUp(mac)
    log.info("socket", "begin socket")
    local sock = socket.create(socket.UDP) -- udp
    log.info("socket.bind", socket.bind(sock, "0.0.0.0", 23333)) --UDP must be bound to the port
    local err = socket.connect(sock, pcIp, 7)--your computer ip
    if err ~= 0 then log.info("socket", err) return end

    mac = mac:fromHex()
    local msg = string.rep(string.char(0xff),6)..string.rep(mac,16)
    local len = socket.send(sock, msg)
    log.info("socket", "sendlen", len)
    socket.close(sock)
    return len == #msg, len
end

sys.taskInit(function()
    log.info("wlan", "wlan_init:",  wlan.init())
    wlan.setMode(wlan.STATION)
    wlan.connect(wifiName,wifiPassword)
    --When you successfully obtain the IP, it means you are connected to the LAN.
    local result, data = sys.waitUntil("IP_READY")
    log.info("wlan", "IP_READY", result, data)


    local mqttc = espmqtt.init({
        uri = mqttAddr,
        client_id = (esp32.getmac():toHex()),
        username = mqttUser,
        password = mqttPassword,
    })
    log.info("mqttc", mqttc)
    if mqttc then
        log.info("mqttc", "what happen")
        local ok, err = espmqtt.start(mqttc)
        log.info("mqttc", "start", ok, err)
        if ok then
            connected = true
            while 1 do
                log.info("mqttc", "wait ESPMQTT_EVT 30s")
                local result, c, ret, topic, data = sys.waitUntil("ESPMQTT_EVT", 30000)
                log.info("mqttc", result, c, ret)
                if result == false then
                    --No news, no movement
                    log.info("mqttc", "wait timeout")
                elseif ret == espmqtt.EVENT_DISCONNECTED then--Disconnected
                    log.info("mqttc", "disconnected!!!")
                    break
                elseif c == mqttc then
                    --It is the message of the current mqtt client, process it
                    if ret == espmqtt.EVENT_CONNECTED then
                        --The connection is successful, usually by defining some topics
                        espmqtt.subscribe(mqttc, subscribeTopic)
                    elseif ret == espmqtt.EVENT_DATA then
                        --A message comes from the server. If the data is very long (more than 4kb), it may be received in multiple times, causing the topic to be an empty string.
                        log.info("mqttc", topic, data)
                        if data == subscribePayload then--After receiving the payload information, it is booting.
                            LED_D5(1)
                            log.info("poweron","发送开机请求啦！")
                            wakeUp(pcMac)
                            espmqtt.publish(mqttc, replyTopic, replyPayload)--Reply
                            LED_D5(0)
                        end
                    else
                        --If there is a response to the subscription information of qos > 0, it will enter this branch.
                        --By default, mqtt automatically reconnects and does not require users to care.
                        log.info("mqttc", "event", ret)
                    end
                else
                    log.info("mqttc", "not this mqttc")
                end
            end
            connected = false
        else
            log.warn("mqttc", "bad start", err)
        end
        espmqtt.destroy(mqttc)
        log.warn("reboot", "device will reboot")
        rtos.reboot()
    end
end)


sys.run()
