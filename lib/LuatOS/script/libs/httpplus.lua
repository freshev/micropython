--[[
@module httpplus
@summary http库的补充
@version 1.0
@date    2023.11.23
@author  wendal
@demo   httpplus
@tag    LUAT_USE_NETWORK
@usage
--The functions supported by this library are:
--1. Problems with uploading large files, regardless of size
--2. Header setting of any length
--3. Body setting of any length
--4. Automatic identification of authentication URLs
--5. The body is returned using zbuff, which can be directly transferred to libraries such as uart.

--Differences from http library
--1. File downloading is not supported
--2. Does not support fota
]]


local httpplus = {}
local TAG = "httpplus"

local function http_opts_parse(opts)
    if not opts then
        log.error(TAG, "opts不能为nil")
        return -100, "opts不能为nil"
    end
    if not opts.url or #opts.url < 5 then
        log.error(TAG, "URL不存在或者太短了", url)
        return -100, "URL不存在或者太短了"
    end
    if not opts.headers then
        opts.headers = {}
    end

    if opts.debug or httpplus.debug then
        if not opts.log then
            opts.log = log.debug
        end
    else
        opts.log = function() 
            --log.info(TAG, "No log")
        end
    end

    --parse url
    --First determine whether the protocol is encrypted
    local is_ssl = false
    local tmp = ""
    if opts.url:startsWith("https://") then
        is_ssl = true
        tmp = opts.url:sub(9)
    elseif opts.url:startsWith("http://") then
        tmp = opts.url:sub(8)
    else
        tmp = opts.url
    end
    --log.info("http decomposition phase 1", is_ssl, tmp)
    --Then determine the host segment
    local uri = ""
    local host = ""
    local port = 0
    if tmp:find("/") then
        uri = tmp:sub((tmp:find("/"))) --Note that find will return multiple values
        tmp = tmp:sub(1, tmp:find("/") - 1)
    else
        uri = "/"
    end
    --log.info("http decomposition phase 2", is_ssl, tmp, uri)
    if tmp == nil or #tmp == 0 then
        log.error(TAG, "非法的URL", url)
        return -101, "非法的URL"
    end
    --Is there any authentication information?
    if tmp:find("@") then
        local auth = tmp:sub(1, tmp:find("@") - 1)
        if not opts.headers["Authorization"] then
            opts.headers["Authorization"] = "Basic " .. auth:toBase64()
        end
        --log.info("http authentication information", auth, opts.headers["Authorization"])
        tmp = tmp:sub(tmp:find("@") + 1)
    end
    --parse port
    if tmp:find(":") then
        host = tmp:sub(1, tmp:find(":") - 1)
        port = tmp:sub(tmp:find(":") + 1)
        port = tonumber(port)
    else
        host = tmp
    end
    if not port or port < 1 then
        if is_ssl then
            port = 443
        else
            port = 80
        end
    end
    --finishing touches
    if not opts.headers["Host"] then
        opts.headers["Host"] = string.format("%s:%d", host, port)
    end
    --Connection must be closed
    opts.headers["Connection"] = "Close"

    --Reset some variables to avoid judgment errors
    opts.is_closed = nil
    opts.body_len = 0

    --multipart requires boundary
    local boundary = "------------------------16ef6e68ef" .. tostring(os.time())
    opts.boundary = boundary
    opts.mp = {}

    if opts.files then
        --Forced to true
        opts.multipart = true
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
        for kk, vv in pairs(opts.files) do
            local ct = contentType[vv:match("%.(%w+)$")] or "application/octet-stream"
            local fname = vv:match("[^%/]+%w$")
            local tmp = string.format("--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n", boundary, kk, fname, ct)
            --log.info("File Transfer Header", tmp)
            table.insert(opts.mp, {vv, tmp, "file"})
            opts.body_len = opts.body_len + #tmp + io.fileSize(vv) + 2
            --log.info("Current body length", opts.body_len, "File length", io.fileSize(vv), fname, ct)
        end
    end

    --form data
    if opts.forms then
        if opts.multipart then
            for kk, vv in pairs(opts.forms) do
                local tmp = string.format("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n", boundary, kk)
                table.insert(opts.mp, {vv, tmp, "form"})
                opts.body_len = opts.body_len + #tmp + #vv + 2
                --log.info("Current body length", opts.body_len, "Data length", #vv)
            end
        else
            if not opts.headers["Content-Type"] then
                opts.headers["Content-Type"] = "application/x-www-form-urlencoded;charset=UTF-8"
            end
            local buff = zbuff.create(120)
            for kk, vv in pairs(opts.forms) do
                buff:copy(nil, kk)
                buff:copy(nil, "=")
                buff:copy(nil, string.urlEncode(tostring(vv)))
                buff:copy(nil, "&")
            end
            if buff:used() > 0 then
                buff:del(-1, 1)
                opts.body = buff
                opts.body_len = buff:used()
                opts.log(TAG, "普通表单", opts.body)
            end
        end
    end

    --If multipart mode
    if opts.multipart then
        --If the body is not set actively, then add the end
        if not opts.body then
            opts.body_len = opts.body_len + #boundary + 2 + 2 + 2
        end
        --Content-Type is not set? Then set it
        if not opts.headers["Content-Type"] then
            opts.headers["Content-Type"] = "multipart/form-data; boundary="..boundary
        end
    end

    --Set bodyfile directly
    if opts.bodyfile then
        local fd = io.open(opts.bodyfile, "rb")
        if not fd then
            log.error("httpplus", "bodyfile失败,文件不存在", opts.bodyfile)
            return -104, "bodyfile失败,文件不存在"
        end
        fd:close()
        opts.body_len = io.fileSize(opts.bodyfile)
    end

    --The body is set, but the length is not set.
    if opts.body and (not opts.body_len or opts.body_len == 0) then
        --When body is zbuff
        if type(opts.body) == "userdata" then
            opts.body_len = opts.body:used()
        --When body is json
        elseif type(opts.body) == "table" then
            opts.body = json.encode(opts.body, "7f")
            if opts.body then
                opts.body_len = #opts.body
                if not opts.headers["Content-Type"] then
                    opts.headers["Content-Type"] = "application/json;charset=UTF-8"
                    opts.log(TAG, "JSON", opts.body)
                end
            end
        --In other cases, it can only be used as text.
        else
            opts.body = tostring(opts.body)
            opts.body_len = #opts.body
        end
    end
    --Content-Length must be set, and the customer-defined value must be overridden.
    --opts.body_len = opts.body_len or 0
    opts.headers["Content-Length"] = tostring(opts.body_len or 0)

    --If method is not set, it will be completed automatically.
    if not opts.method or #opts.method == 0 then
        if opts.body_len > 0 then
            opts.method = "POST"
        else
            opts.method = "GET"
        end
    else
        --Make sure it is in capital letters
        opts.method = opts.method:upper()
    end

    if opts.debug then
        opts.log(TAG, is_ssl, host, port, uri, json.encode(opts.headers))
    end
    

    --Set the remaining properties
    opts.host = host
    opts.port = port
    opts.uri  = uri
    opts.is_ssl = is_ssl

    if not opts.timeout or opts.timeout == 0 then
        opts.timeout = 30
    end

    return --Completed successfully, no return value required
end



local function zbuff_find(buff, str)
    --log.info("zbuff search", buff:used(), #str)
    if buff:used() < #str then
        return
    end
    local maxoff = buff:used()
    maxoff = maxoff - #str
    local tmp = zbuff.create(#str)
    tmp:write(str)
    --log.info("tmp数据", tmp:query():toHex())
    for i = 0, maxoff, 1 do
        local flag = true
        for j = 0, #str - 1, 1 do
            --log.info("对比", i, j, string.char(buff[i+j]):toHex(), string.char(tmp[j]):toHex(), buff[i+j] ~= tmp[j])
            if buff[i+j] ~= tmp[j] then
                flag = false
                break
            end
        end
        if flag then
            return i
        end
    end
end

local function resp_parse(opts)
    --log.info("Here--------")
    local header_offset = zbuff_find(opts.rx_buff, "\r\n\r\n")
    --log.info("Header offset", header_offset)
    if not header_offset then
        log.warn(TAG, "没有检测到http响应头部,非法响应")
        opts.resp_code = -198
        return
    end
    local state_line_offset = zbuff_find(opts.rx_buff, "\r\n")
    local state_line = opts.rx_buff:query(0, state_line_offset)
    local tmp = state_line:split(" ")
    if not tmp or #tmp < 2 then
        log.warn(TAG, "非法的响应行", state_line)
        opts.resp_code = -197
        return
    end
    local code = tonumber(tmp[2])
    if not code then
        log.warn(TAG, "非法的响应码", tmp[2])
        opts.resp_code = -196
        return
    end
    opts.resp_code = code
    opts.resp = {
        headers = {}
    }
    opts.log(TAG, "state code", code)
    --TODO parse header and body

    opts.rx_buff:del(0, state_line_offset + 2)
    --opts.log(TAG, "remaining response body", opts.rx_buff:query())

    --Parse headers
    while 1 do
        local offset = zbuff_find(opts.rx_buff, "\r\n")
        if not offset then
            log.warn(TAG, "不合法的剩余headers", opts.rx_buff:query())
            break
        end
        if offset == 0 then
            --The last empty line of header
            opts.rx_buff:del(0, 2)
            break
        end
        local line = opts.rx_buff:query(0, offset)
        opts.rx_buff:del(0, offset + 2)
        local tmp2 = line:split(":")
        opts.log(TAG, tmp2[1]:trim(), tmp2[2]:trim())
        opts.resp.headers[tmp2[1]:trim()] = tmp2[2]:trim()
    end

    --if opts.resp_code < 299 then
        --parse body
        --It’s easy to handle with Content-Length
        if opts.resp.headers["Content-Length"] then
            opts.log(TAG, "有长度, 标准的咯")
            opts.resp.body = opts.rx_buff
        elseif opts.resp.headers["Transfer-Encoding"] == "chunked" then
            --log.info(TAG, "Data is chunked encoding", opts.rx_buff[0], opts.rx_buff[1])
            --log.info(TAG, "Data is chunked encoding", opts.rx_buff:query(0, 4):toHex())
            local coffset = 0
            local crun = true
            while crun and coffset < opts.rx_buff:used() do
                --Read the length from the current offset. The length will never exceed 8 bytes, right?
                local flag = true
                --local coffset = zbuff_find(opts.rx_buff, "\r\n")
                --if not coffset then
                    
                -- end
                for i = 1, 8, 1 do
                    if opts.rx_buff[coffset+i] == 0x0D and opts.rx_buff[coffset+i+1] == 0x0A then
                        local ctmp = opts.rx_buff:query(coffset, i)
                        --opts.log(TAG, "chunked fragment length", ctmp, ctmp:toHex())
                        local clen = tonumber(ctmp, 16)
                        --opts.log(TAG, "chunked fragment length 2", clen)
                        if clen == 0 then
                            --The end
                            opts.rx_buff:resize(coffset)
                            crun = false
                        else
                            --Delete chunked chunks first
                            opts.rx_buff:del(coffset, i+2)
                            coffset = coffset + clen
                        end
                        flag = false
                        break
                    end
                end
                --You can definitely search for chunked
                if flag then
                    log.error("非法的chunked块")
                    break
                end
            end
            opts.resp.body = opts.rx_buff
        end
    -- end

    --Clear rx_buff
    opts.rx_buff = nil

    --Finished scattering flowers
end

--socket callback function
local function http_socket_cb(opts, event)
    opts.log(TAG, "tcp.event", event)
    if event == socket.ON_LINE then
        --The TCP connection has been established, then you can go upstream.
        --opts.state = "ON_LINE"
        sys.publish(opts.topic)
    elseif event == socket.TX_OK then
        --The data transfer is completed. This message is needed if it is a file upload.
        --opts.state = "TX_OK"
        sys.publish(opts.topic)
    elseif event == socket.EVENT then
        --When data is received or the link is disconnected, you always need to read it once to know.
        local succ, data_len = socket.rx(opts.netc, opts.rx_buff)
        if succ and data_len > 0 then
            opts.log(TAG, "收到数据", data_len, "总长", #opts.rx_buff)
            --opts.log(TAG, "数据", opts.rx_buff:query())
        else
            if not opts.is_closed then
                opts.log(TAG, "服务器已经断开了连接或接收出错")
                opts.is_closed = true
                sys.publish(opts.topic)
            end
        end
    elseif event == socket.CLOSED then
        log.info(TAG, "连接已关闭")
        opts.is_closed = true
        sys.publish(opts.topic)
    end
end

local function http_exec(opts)
    local netc = socket.create(opts.adapter, function(sc, event)
        if opts.netc then
            return http_socket_cb(opts, event)
        end
    end)
    if not netc then
        log.error(TAG, "创建socket失败了!!")
        return -102
    end
    opts.netc = netc
    opts.rx_buff = zbuff.create(1024)
    opts.topic = tostring(netc)
    socket.config(netc, nil,nil, opts.is_ssl)
    if opts.debug or httpplus.debug then
        socket.debug(netc)
    end
    if not socket.connect(netc, opts.host, opts.port, opts.try_ipv6) then
        log.warn(TAG, "调用socket.connect返回错误了")
        return -103, "调用socket.connect返回错误了"
    end
    local ret = sys.waitUntil(opts.topic, 5000)
    if ret == false then
        log.warn(TAG, "建立连接超时了!!!")
        return -104, "建立连接超时了!!!"
    end
    
    --First is the head
    local line = string.format("%s %s HTTP/1.1\r\n", opts.method:upper(), opts.uri)
    --opts.log(TAG, line)
    socket.tx(netc, line)
    for k, v in pairs(opts.headers) do
        line = string.format("%s: %s\r\n", k, v)
        socket.tx(netc, line)
    end
    line = "\r\n"
    socket.tx(netc, line)

    --then body
    local rbody = ""
    local write_counter = 0
    if opts.mp and #opts.mp > 0 then
        opts.log(TAG, "执行mulitpart上传模式")
        for k, v in pairs(opts.mp) do
            socket.tx(netc, v[2])
            write_counter = write_counter + #v[2]
            if v[3] == "file" then
                --log.info("Write file header", v[2])
                local fd = io.open(v[1], "rb")
                --log.info("Write file data", v[1])
                if fd then
                    while not opts.is_closed do
                       local fdata = fd:read(1400)
                        if not fdata or #fdata == 0 then
                            break
                        end
                        --log.info("Write file data", "Length", #fdata)
                        socket.tx(netc, fdata)
                        write_counter = write_counter + #fdata
                        --Note that you need to wait for the TX_OK event here
                        sys.waitUntil(opts.topic, 3000)
                    end
                    fd:close()
                end
            else
                socket.tx(netc, v[1])
                write_counter = write_counter + #v[1]
            end
            socket.tx(netc, "\r\n")
            write_counter = write_counter + 2
        end
        --rbody = rbody .. "--" .. opts.boundary .. "--\r\n"
        socket.tx(netc, "--")
        socket.tx(netc, opts.boundary)
        socket.tx(netc, "--\r\n")
        write_counter = write_counter + #opts.boundary + 2 + 2 + 2
    elseif opts.bodyfile then
        local fd = io.open(opts.bodyfile, "rb")
        --log.info("Write file data", v[1])
        if fd then
            while not opts.is_closed do
                local fdata = fd:read(1400)
                if not fdata or #fdata == 0 then
                    break
                end
                --log.info("Write file data", "Length", #fdata)
                socket.tx(netc, fdata)
                write_counter = write_counter + #fdata
                --Note that you need to wait for the TX_OK event here
                sys.waitUntil(opts.topic, 300)
            end
            fd:close()
        end
    elseif opts.body then
        if type(opts.body) == "string" and #opts.body > 0 then
            socket.tx(netc, opts.body)
            write_counter = write_counter + #opts.body
        elseif type(opts.body) == "userdata" then
            write_counter = write_counter + opts.body:used()
            if opts.body:used() < 4*1024 then
                socket.tx(netc, opts.body)
            else
                local offset = 0
                local tmpbuff = opts.body
                local tsize = tmpbuff:used()
                while offset < tsize do
                    opts.log(TAG, "body(zbuff)分段写入", offset, tsize)
                    if tsize - offset > 4096 then
                        socket.tx(netc, tmpbuff:toStr(offset, 4096))
                        offset = offset + 4096
                        sys.waitUntil(opts.topic, 300)
                    else
                        socket.tx(netc, tmpbuff:toStr(offset, tsize - offset))
                        break
                    end
                end
            end
        end
    end
    --log.info("write length", "expected", opts.body_len, "actual", write_counter)
    --log.info("hex", rbody)

    --Process response information
    while not opts.is_closed and opts.timeout > 0 do
        log.info(TAG, "等待服务器完成响应")
        sys.waitUntil(opts.topic, 1000)
        opts.timeout = opts.timeout - 1
    end
    log.info(TAG, "服务器已完成响应,开始解析响应")
    resp_parse(opts)
    --log.info("Execution completed", "Return result")
end

--[[
执行HTTP请求
@api httpplus.request(opts)
@table 请求参数,是一个table,最起码得有url属性
@return int 响应码,服务器返回的状态码>=100, 若本地检测到错误,会返回<0的值
@return 服务器正常响应时返回结果, 否则是错误信息或者nil
@usage
--Introduction to request parameters
local opts = {
    url    = "https://httpbin.air32.cn/abc", --Required, target URL
    method = "POST", --Optional, default GET, if there are body, files, forms parameters, it will be set to POST
    headers = {}, --Optional, custom additional headers
    files = {},   --Optional, file upload. If this parameter exists, it will be forced to be uploaded in the form of multipart/form-data.
    forms = {},   --Optional, form parameters, if this parameter exists, if files do not exist, upload according to application/x-www-form-urlencoded
    body  = "abc=123",--Optional, custom body parameters, string/zbuff/table can be used, but cannot exist at the same time as files and forms.
    debug = false,    --Optional, turn on debugging log, default false
    try_ipv6 = false, --Optional, whether to give priority to trying ipv6 addresses, the default is false
    adapter = nil,    --Optional, network adapter number, automatically selected by default
    timeout = 30,     --Optional, timeout for reading server response, unit seconds, default 30
    bodyfile = "xxx"  --Optional, directly upload the file content as the body, which has higher priority than the body parameter.
}

local code, resp = httpplus.request({url="https://httpbin.air32.cn/get"})
log.info("http", code)
--Description of return value resp
--Case 1, when code >= 100, resp will be a table, containing 2 elements
if code >= 100 then
    --headers, is a table
    log.info("http", "headers", json.encode(resp.headers))
    --body, is a zbuff
    --Can be converted to Lua string through query function
    log.info("http", "headers", resp.body:query())
    --It can also be forwarded through uart.tx and other functions that support zbuff.
    --uart.tx(1, resp.body)
end
]]
function httpplus.request(opts)
    --Parameter analysis
    local ret = http_opts_parse(opts)
    if ret then
        return ret
    end

    --Execute request
    local ret, msg = pcall(http_exec, opts)
    if opts.netc then
        --clean connection
        if not opts.is_closed then
            socket.close(opts.netc)
        end
        socket.release(opts.netc)
        opts.netc = nil
    end
    --Handle responses or errors
    if not ret then
        log.error(TAG, msg)
        return -199, msg
    end
    return opts.resp_code, opts.resp
end

return httpplus
