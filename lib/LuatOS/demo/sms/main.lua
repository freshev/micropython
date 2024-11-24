--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "smsdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require "sysplus" --The http library requires this sysplus

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end
log.info("main", "sms demo")

--Assist in sending http requests, because the http library needs to run in the task
function http_post(url, headers, body)
    sys.taskInit(function()
        local code, headers, body = http.request("POST", url, headers, body).wait()
        log.info("resp", code)
    end)
end

function sms_handler(num, txt)
    --num mobile phone number
    --txt text content
    log.info("sms", num, txt, txt:toHex())

    --http demo 1, send json
    local body = json.encode({phone=num, txt=txt})
    local headers = {}
    headers["Content-Type"] = "application/json"
    log.info("json", body)
    http_post("http://www.luatos.com/api/sms/blackhole", headers, body)
    --http demo 2, issuing orders
    headers = {}
    headers["Content-Type"] = "application/x-www-form-urlencoded"
    local body = string.format("phone=%s&txt=%s", num:urlEncode(), txt:urlEncode())
    log.info("params", body)
    http_post("http://www.luatos.com/api/sms/blackhole", headers, body)
    --http demo 3, no headers required, send directly
    http_post("http://www.luatos.com/api/sms/blackhole", nil, num .. "," .. txt)
    --If you want to send to Dingding, refer to demo/dingding
    --If you want to send to Feishu, please refer to demo/feishu
end

--------------------------------------------------------------------
--There are multiple ways to receive text messages, just choose one.
--1. Set callback function
--sms.setNewSmsCb(sms_handler)
--2. Subscribe to system messages
--sys.subscribe("SMS_INC", sms_handler)
--3. Wait in the task
sys.taskInit(function()
    while 1 do
        local ret, num, txt = sys.waitUntil("SMS_INC", 300000)
        if num then
            --Option 1, leave it to a custom function
            sms_handler(num, txt)
            --Option 2, because this is within a task, http.request can be called directly
            --local body = json.encode({phone=num, txt=txt})
            --local headers = {}
            --headers["Content-Type"] = "application/json"
            --log.info("json", body)
            --local code, headers, body = http.request("POST", "http://www.luatos.com/api/sms/blackhole", headers, body).wait()
            --log.info("resp", code)
        end
    end
end)

-------------------------------------------------------------------
--To send a text message, just call sms.send. It doesn’t matter whether it is a task or not.
sys.taskInit(function()
    sys.wait(10000)
    --Check text messages using China Mobile card
    --sms.send("+8610086", "301")
    --China Unicom card phone bill check
    sms.send("10010", "101")
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
