
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "httpdemo"
VERSION = "1.0.0"

--[[
本demo需要http库, 大部分能联网的设备都具有这个库
http也是内置库, 无需require

1. 如需上传大文件,请使用 httpplus 库, 对应demo/httpplus
2. 
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the http library requires the following statements]]
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to esptouch distribution network
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        wlan.setMode(wlan.STATION)
        wlan.connect(ssid, password, 1)
        local result, data = sys.waitUntil("IP_READY", 30000)
        log.info("wlan", "IP_READY", result, data)
        device_id = wlan.getMac()
    elseif rtos.bsp() == "AIR105" then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
        sys.wait(1000)
        --TODO Get mac address as device_id
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2)
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
        log.info("ipv6", mobile.ipv6(true))
        sys.waitUntil("IP_READY", 30000)
    elseif http then
        sys.waitUntil("IP_READY")
    else
        while 1 do
            sys.wait(1000)
            log.info("http", "当前固件未包含http库")
        end
    end
    log.info("已联网")
    sys.publish("net_ready")
end)

function demo_http_get()
    --The most common HTTP GET request
    local code, headers, body = http.request("GET", "https://www.air32.cn/").wait()
    log.info("http.get", code, headers, body)
    local code, headers, body = http.request("GET", "https://mirrors6.tuna.tsinghua.edu.cn/", nil, nil, {ipv6=true}).wait()
    log.info("http.get", code, headers, body)
    sys.wait(100)
    local code, headers, body = http.request("GET", "https://www.luatos.com/").wait()
    log.info("http.get", code, headers, body)

    --Print on demand
    --code response value. If it is greater than or equal to 100, it is a server response. If it is less than 100, it is an error code.
    --Headers is a table, generally exists as debugging data
    --The body is a string. Note that Lua's string is byte[]/char* with length, which can contain invisible characters.
    --log.info("http", code, json.encode(headers or {}), #body > 512 and #body or body)
end

function demo_http_post_json()
    --POST request demo
    local req_headers = {}
    req_headers["Content-Type"] = "application/json"
    local body = json.encode({name="LuatOS"})
    local code, headers, body = http.request("POST","http://site0.cn/api/httptest/simple/date", 
            req_headers,
            body --The body, string, zbuff, and file required for the POST request can be
    ).wait()
    log.info("http.post", code, headers, body)
end

function demo_http_post_form()
    --POST request demo
    local req_headers = {}
    req_headers["Content-Type"] = "application/x-www-form-urlencoded"
    local params = {
        ABC = "123",
        DEF = 345
    }
    local body = ""
    for k, v in pairs(params) do
        body = body .. tostring(k) .. "=" .. tostring(v):urlEncode() .. "&"
    end
    local code, headers, body = http.request("POST","http://echohttp.wendal.cn/post", 
            req_headers,
            body --The body, string, zbuff, and file required for the POST request can be
    ).wait()
    log.info("http.post.form", code, headers, body)
end

--local function http_download_callback(content_len,body_len,userdata)
--     print("http_download_callback",content_len,body_len,userdata)
-- end

--local http_userdata = "123456789"

function demo_http_download()

    --POST and download, synchronization operations within tasks
    local opts = {}                 --Additional configuration items
    opts["dst"] = "/data.bin"       --Download path, optional
    opts["timeout"] = 30000         --Timeout length, unit ms, optional
    --opts["adapter"] = socket.ETH0 -- which network card to use, optional
    --opts["callback"] = http_download_callback
    --opts["userdata"] = http_userdata

    for k, v in pairs(opts) do
        print("opts",k,v)
    end
    
    local code, headers, body = http.request("POST","http://site0.cn/api/httptest/simple/date",
            {}, --Headers added to the request, can be nil
            "", 
            opts
    ).wait()
    log.info("http.post", code, headers, body) --Only return code and headers

    --local f = io.open("/data.bin", "rb")
    --if f then
    --local data = f:read("*a")
    --log.info("fs", "data", data, data:toHex())
    -- end
    
    --GET request, just open a task and let it execute itself, regardless of the execution result.
    sys.taskInit(http.request("GET","http://site0.cn/api/httptest/simple/time").wait)
end

function demo_http_post_file()
        ---- POST multipart/form-data mode upload files --- manual splicing
        local boundary = "----WebKitFormBoundary"..os.time()
        local req_headers = {
            ["Content-Type"] = "multipart/form-data; boundary="..boundary,
        }
        local body = "--"..boundary.."\r\n"..
                     "Content-Disposition: form-data; name=\"uploadFile\"; filename=\"luatos_uploadFile_TEST01.txt\""..
                     "\r\nContent-Type: text/plain\r\n\r\n"..
                     "1111http_测试一二三四654zacc\r\n"..
                     "--"..boundary

        log.info("headers: ", "\r\n"..json.encode(req_headers))
        log.info("body: ", "\r\n"..body)
        local code, headers, body = http.request("POST","http://airtest.openluat.com:2900/uploadFileToStatic",
                req_headers,
                body --The body, string, zbuff, and file required for the POST request can be
        ).wait()
        log.info("http.post", code, headers, body)

        --You can also use postMultipartFormData(url, params) to upload files
        postMultipartFormData(
            "http://airtest.openluat.com:2900/uploadFileToStatic",
            {
                --texts =
                -- {
                --["imei"] = "862991234567890",
                --["time"] = "20180802180345"
                -- },
                
                files =
                {
                    ["uploadFile"] = "/luadb/luatos_uploadFile.txt",
                }
            }
        )
end


local function demo_http_get_gzip()
    --Here we use the API of Zephyr Weather for demonstration.
    --The response of this API is always gzip compressed and needs to be decompressed with the miniz library.
    local code, headers, body = http.request("GET", "https://devapi.qweather.com/v7/weather/now?location=101010100&key=0e8c72015e2b4a1dbff1688ad54053de").wait()
    log.info("http.gzip", code)
    if code == 200 then
        local re = miniz.uncompress(body:sub(11), 0)
        log.info("和风天气", re)
        if re then
            local jdata = json.decode(re)
            log.info("jdata", jdata)
            if jdata then
                log.info("和风天气", jdata.code)
                if jdata.now then
                    log.info("和风天气", "天气", jdata.now.text)
                    log.info("和风天气", "温度", jdata.now.temp)
                end
            end
        end
    end
end

sys.taskInit(function()
    sys.wait(100)
    --Print the supported cipher suites. Generally speaking, the firmware already contains 99% of the common cipher suites.
    --if crypto.cipher_suites then
    --log.info("cipher", "suites", json.encode(crypto.cipher_suites()))
    -- end

    -------------------------------------
    -------- HTTP demo code ---------------
    -------------------------------------
    sys.waitUntil("net_ready") --Waiting for Internet connection

    while 1 do
        --Demo GET request
        demo_http_get()
        --form submission
        -- demo_http_post_form()
        --POST a json string
        -- demo_http_post_json()
        --Upload files, mulitform form
        -- demo_http_post_file()
        --File download
        -- demo_http_download()
        --Response to gzip compression, taking Zefeng Weather as an example
        -- demo_http_get_gzip()

        sys.wait(1000)
        --Print memory status
        log.info("sys", rtos.meminfo("sys"))
        log.info("lua", rtos.meminfo("lua"))
        sys.wait(600000)
    end
end)

---- MultipartForm upload files
--url string request URL address
--req_headers table request headers
--params table data parameters that need to be transmitted
function postMultipartFormData(url, params)
    local boundary = "----WebKitFormBoundary"..os.time()
    local req_headers = {
        ["Content-Type"] = "multipart/form-data; boundary="..boundary,
    }
    local body = {}

    --Parse splicing body
    for k,v in pairs(params) do
        if k=="texts" then
            local bodyText = ""
            for kk,vv in pairs(v) do
                print(kk,vv)
                bodyText = bodyText.."--"..boundary.."\r\nContent-Disposition: form-data; name=\""..kk.."\"\r\n\r\n"..vv.."\r\n"
            end
            table.insert(body, bodyText)
        elseif k=="files" then
            local contentType =
            {
                txt = "text/plain",             --text
                jpg = "image/jpeg",             --JPG format pictures
                jpeg = "image/jpeg",            --JPEG format pictures
                png = "image/png",              --PNG format pictures
                gif = "image/gif",              --GIF format pictures
                html = "image/html",            -- HTML
                json = "application/json"       -- JSON
            }
            
            for kk,vv in pairs(v) do
                if type(vv) == "table" then
                    for i=1, #vv do
                        print(kk,vv[i])
                        table.insert(body, "--"..boundary.."\r\nContent-Disposition: form-data; name=\""..kk.."\"; filename=\""..vv[i]:match("[^%/]+%w$").."\"\r\nContent-Type: "..contentType[vv[i]:match("%.(%w+)$")].."\r\n\r\n")
                        table.insert(body, io.readFile(vv[i]))
                        table.insert(body, "\r\n")
                    end
                else
                    print(kk,vv)
                    table.insert(body, "--"..boundary.."\r\nContent-Disposition: form-data; name=\""..kk.."\"; filename=\""..vv:match("[^%/]+%w$").."\"\r\nContent-Type: "..contentType[vv:match("%.(%w+)$")].."\r\n\r\n")
                    table.insert(body, io.readFile(vv))
                    table.insert(body, "\r\n")
                end
            end
        end
    end 
    table.insert(body, "--"..boundary.."--\r\n")
    body = table.concat(body)
    log.info("headers: ", "\r\n" .. json.encode(req_headers), type(body))
    log.info("body: " .. body:len() .. "\r\n" .. body)
    local code, headers, body = http.request("POST",url,
            req_headers,
            body
    ).wait()   
    log.info("http.post", code, headers, body)
end


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
