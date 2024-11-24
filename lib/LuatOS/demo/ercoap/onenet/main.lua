

PROJECT = "onenetcoap"
VERSION = "1.0.0"

_G.sys = require("sys")
require "sysplus"

--onenet coap access address
udp_host = "183.230.102.122"
udp_port = 5683
--Device information
produt_id = "SJaLt5cVL2"
device_name = "luatospc"
device_key = "dUZVVWRIcjVsV2pSbTJsckd0TmgyRXNnMTJWMXhIMkk="

_, _, main_token = iotauth.onenet(produt_id,device_name,device_key,"sha1")

--UDP event handler
local rxbuff = zbuff.create(1500)
local running = false
local post_token = ""
function udpcb(sc, event)
    --log.info("udp", sc, string.format("%08X", event))
    if event == socket.EVENT then
        local ok, len, remote_ip, remote_port = socket.rx(sc, rxbuff)
        if ok then
            local data = rxbuff:query()
            rxbuff:del()
            log.info("udp", "读到数据", data:toHex())
            ercoap.print(data)
            local resp = ercoap.parse(data)
            if resp and resp.code == 201 then
                log.info("login success", resp.code, resp.payload:toHex())
                --It is very important here to obtain the token values   required for other requests.
                post_token = resp.payload
            end
        else
            log.info("udp", "服务器断开了连接")
            running = false
            sys.publish("UDP_OFFLINE")
        end
    elseif event == socket.TX_OK then
        log.info("udp", "上行完成")
    elseif event == socket.ON_LINE then
        log.info("udp", "UDP已准备就绪,可以上行")
        --Uplink login package
        local data = ercoap.onenet("login", produt_id, device_name, main_token)
        --log.info("Upstream login package", data:toHex())
        socket.tx(sc, data)
    else
        log.info("udp", "其他事件", event)
    end
end

sys.taskInit(function()
    sys.waitUntil("IP_READY")
    sys.wait(100)
    log.info("创建 udp netc")
    local netc = socket.create(nil, udpcb)
    --log.info("netc", netc)
    socket.config(netc, nil, true)
    --socket.debug(netc, true)
    log.info("执行连接")
    local ok = socket.connect(netc, udp_host, udp_port)
    log.info("socket connect", ok)
    if ok then
        running = true
        while running do
            local result = sys.waitUntil("UDP_OFFLINE", 30000)
            if result then
               break
            end
            --Uplink heartbeat packet
            --onenet_coap_auth(string.format("$sys/%s/%s/keep_alive", produt_id, device_name))
            local data = ercoap.onenet("keep_alive", produt_id, device_name, main_token)
            --log.info("uplink heartbeat packet", data:toHex())
            socket.tx(netc, data)

            sys.wait(1000)
            local post = {
                id = "123",
                --st = token,
                params = {
                    --WaterConsumption = {
                    --value = 1.5
                    -- },
                    WaterMeterState = {
                        value = 0
                    }
                }
            }
            local jdata = (json.encode(post, "2f"))
            log.info("uplink", jdata)
            --jdata = [[{"id":"3","version":"1.0","params":{"WaterMeterState":{"value":0}}}]]
            --log.info("uplink2", jdata)
            local data = ercoap.onenet("thing/property/post", produt_id, device_name, post_token, jdata)
            --log.info("onenet", "Upward object model data", data:toHex())
            socket.tx(netc, data)
        end
    else
        log.info("UDP初始化失败")
        sys.wait(100)
    end
   
    log.info("连接中断, 关闭资源")
    socket.close(netc)
    socket.release(netc)
    log.info("全部结束")
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
