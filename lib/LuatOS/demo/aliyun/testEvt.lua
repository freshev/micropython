
local sys = require "sys"
local aliyun = require "aliyun"

--[[
函数名：pubqos1testackcb
功能  ：发布1条qos为1的消息后收到PUBACK的回调函数
参数  ：
		usertag：调用mqttclient:publish时传入的usertag
		result：任意数字表示发布成功，nil表示失败
返回值：无
]]
local publishCnt = 1
local function publishTestCb(result,para)
    log.info("aliyun", "发布后的反馈", result,para)
    sys.timerStart(publishTest,20000)
    publishCnt = publishCnt+1
end

--Publish a message with QOS 1
function publishTest()
    --Note: You can control the content encoding of the payload here. The aLiYun library will not perform any encoding conversion on the content of the payload.
    -- aliyun.publish(topic,qos,payload,cbFnc,cbPara)
    log.info("aliyun", "上行数据")
    aliyun.publish("/"..aliyun.opts.ProductKey.."/"..aliyun.opts.DeviceName.."/user/update",1,"LUATOS_CESHI",publishTestCb,"publishTest_"..publishCnt)
end

---Data reception processing function
--@string topic, UTF8 encoded message topic
--@string payload, original encoded message payload
local function rcvCbFnc(topic,payload,qos,retain,dup)
    log.info("aliyun", "收到下行数据", topic,payload,qos,retain,dup)
end

--- Processing function for connection results
--@bool result, connection result, true indicates successful connection, false or nil indicates connection failure
local function connectCbFnc(result)
    log.info("aliyun","连接结果", result)
    if result then
        sys.publish("aliyun_ready")
        log.info("aliyun", "连接成功")
        --Subscribe to topic
        --Subscribe to topics according to your project needs
        -- aliyun.subscribe(topic,qos)
        --aliyun.subscribe("/".. aliyun.opts.ProductKey.."/".. aliyun.opts.DeviceName.."/user/ceshi",1)

        --PUBLISH message test
        publishTest()
    else
        log.warn("aliyun", "连接失败")
    end
end

--Connection status processing function
aliyun.on("connect",connectCbFnc)

--Data reception processing function
aliyun.on("receive",rcvCbFnc)

--One-type-one-password registration callback function, added on 2024.6.17
--aliyun.on("reg", function(result)
--     aliyun.store(result)
-- end)

--Data sending processing function, generally not needed
--aliyun.on("receive", sentCbFnc)

--OTA status processing function
-- aliyun.on("ota",function(result)
--if result == 0 then
--log.info("aliyun", "OTA successful")
--         rtos.reboot()
--     end
-- end)
