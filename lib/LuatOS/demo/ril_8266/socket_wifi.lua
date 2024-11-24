--- Module functions: data link activation, SOCKET management (creation, connection, data sending and receiving, status maintenance)
--@Modules socket
--@author openLuat
--@license MIT
--@copyright openLuat
--@release 2017.9.25
local socket_wifi = {}

link_wifi = require "link_wifi"

local ril = ril_wifi
local req = ril.request

local valid = {"3", "2", "1", "0"}
local validSsl = {"3", "2", "1", "0"}
local sockets = {}
local socketsSsl = {}
--Maximum value of data sent in a single time
local SENDSIZE = 1460
--The maximum subscript of the buffer
local INDEX_MAX = 256

--User-defined DNS resolver
local dnsParser
local dnsParserToken = 0

--- Is SOCKET available?
--@return true is available, false is not available
socket_wifi.isReady = link_wifi.isReady

local function isSocketActive(ssl)
    for _, c in pairs(ssl and socketsSsl or sockets) do
        if c.connected then
            return true
        end
    end
end

local function socketStatusNtfy()
    sys.publish("SOCKET_ACTIVE", isSocketActive() or isSocketActive(true))
end

local function stopConnectTimer(tSocket, id)
    if id and tSocket[id] and tSocket[id].co and coroutine.status(tSocket[id].co) == "suspended" and (tSocket[id].wait == "+SSLCONNECT" or tSocket[id].wait == "+CIPSTART") then
        --and (tSocket[id].wait == "+SSLCONNECT" or (tSocket[id].protocol == "UDP" and tSocket[id].wait == "+CIPSTART")) then
        sys.timerStop(coroutine.resume, tSocket[id].co, false, "TIMEOUT")
    end
end

local function errorInd(error)
    local coSuspended = {}

    for k, v in pairs({sockets, socketsSsl}) do
        --if #v ~= 0 then
        for _, c in pairs(v) do --When an IP status error occurs, all connected sockets are notified
            --if c.connected or c.created then
            if error == 'CLOSED' and not c.ssl then
                c.connected = false
                socketStatusNtfy()
            end
            c.error = error
            if c.co and coroutine.status(c.co) == "suspended" then
                stopConnectTimer(v, c.id)
                --coroutine.resume(c.co, false)
                table.insert(coSuspended, c.co)
            end
            -- end
        end
        -- end
    end

    for k, v in pairs(coSuspended) do
        if v and coroutine.status(v) == "suspended" then
            coroutine.resume(v, false)
        end
    end
end

sys.subscribe("IP_ERROR_IND", function()
    errorInd('IP_ERROR_IND')
end)
sys.subscribe('IP_SHUT_IND', function()
    errorInd('CLOSED')
end)

--Subscribe to the message processing function returned by rsp
local function onSocketURC(data, prefix)
    local tag, id, result = string.match(data, "([SSL]*)[&]*(%d), *([%u :%d]+)")
    tSocket = (tag == "SSL" and socketsSsl or sockets)
    if not id or not tSocket[id] then
        log.error('socket: urc on nil socket', data, id, tSocket[id], socketsSsl[id])
        return
    end
    if result == "CONNECT" or result:match("CONNECT ERROR") or result:match("CONNECT FAIL") then
        if tSocket[id].wait == "+CIPSTART" or tSocket[id].wait == "+SSLCONNECT" then
            stopConnectTimer(tSocket, id)
            coroutine.resume(tSocket[id].co, result == "CONNECT")
        else
            log.error("socket: error urc", tSocket[id].wait)
        end
        return
    end

    if tag == "SSL" and string.find(result, "ERROR:") == 1 then
        return
    end

    if string.find(result, "ERROR") or result == "CLOSED" then
        if result == 'CLOSED' and not tSocket[id].ssl then
            tSocket[id].connected = false
            socketStatusNtfy()
        end
        tSocket[id].error = result
        stopConnectTimer(tSocket, id)
        coroutine.resume(tSocket[id].co, false)
    end
end
--Create socket function
local mt = {}
mt.__index = mt
local function socket(protocol, cert)
    local ssl = nil-- protocol:match("SSL")
    local id = table.remove(ssl and validSsl or valid)
    if not id then
        log.warn("socket.socket: too many sockets")
        return nil
    end

    local co = coroutine.running()
    if not co then
        log.warn("socket.socket: socket must be called in coroutine")
        return nil
    end
    --Instance attribute parameter table
    local o = {
        id = id,
        protocol = protocol,
        ssl = ssl,
        cert = cert,
        co = co,
        input = {},
        output = {},
        wait = "",
        connected = false,
        iSubscribe = false,
        subMessage = nil
    }

    tSocket = (ssl and socketsSsl or sockets)
    tSocket[id] = o
        if cert then
            local tmpPath, result, caConf, certConf, keyConf
            if cert.caCert then
                result = true
                tmpPath = (cert.caCert:sub(1, 1) == "/") and cert.caCert or ("/ldata/" .. cert.caCert)
                if not io.exists("/at_ca.bin") then
                    result = false
                else
                    if crypto.md5("/at_ca.bin", "file") ~= crypto.md5(tmpPath, "file") then
                        result = false
                    end
                end
                if not result then
                    io.writeFile("/at_ca.bin", io.readFile(tmpPath))
                    req("AT+SYSFLASH=0,\"" .. "client_ca" .. "\",0,8192", nil, nil, nil, {
                        id = id,
                        path32 = "client_ca",
                        path8955 = tmpPath
                    })
                    coroutine.yield()
                end
                caConf = true
            end
            if cert.clientCert then
                result = true
                tmpPath = (cert.clientCert:sub(1, 1) == "/") and cert.clientCert or ("/ldata/" .. cert.clientCert)
                if not io.exists("/at_cert.bin") then
                    result = false
                else
                    if crypto.md5("/at_cert.bin", "file") ~= crypto.md5(tmpPath, "file") then
                        result = false
                    end
                end
                if not result then
                    io.writeFile("/at_cert.bin", io.readFile(tmpPath))
                    req("AT+SYSFLASH=0,\"" .. "client_cert" .. "\",0,8192", nil, nil, nil, {
                        id = id,
                        path32 = "client_cert",
                        path8955 = tmpPath
                    })
                    coroutine.yield()
                end
                certConf = true
            end
            if cert.clientKey then
                result = true
                tmpPath = (cert.clientKey:sub(1, 1) == "/") and cert.clientKey or ("/ldata/" .. cert.clientKey)
                if not io.exists("/at_key.bin") then
                    result = false
                else
                    if crypto.md5("/at_key.bin", "file") ~= crypto.md5(tmpPath, "file") then
                        result = false
                    end
                end
                if not result then
                    io.writeFile("/at_key.bin", io.readFile(tmpPath))
                    req("AT+SYSFLASH=0,\"" .. "client_key" .. "\",0,8192", nil, nil, nil, {
                        id = id,
                        path32 = "client_key",
                        path8955 = tmpPath
                    })
                    coroutine.yield()
                end
                keyConf = true
            end
            if caConf and not certConf and not keyConf then
                req(string.format("AT+CIPSSLCCONF=%d,%d,%d,%d", id, 2, 0, 0))
            elseif not caConf and certConf and not keyConf then
                req(string.format("AT+CIPSSLCCONF=%d,%d,%d,%d", id, 1, 0, 0))
            elseif caConf and certConf and keyConf then
                req(string.format("AT+CIPSSLCCONF=%d,%d,%d,%d", id, 3, 0, 0))
            end
        else
            req(string.format("AT+CIPSSLCCONF=%d,%d", id, 0))
        end
 
    return setmetatable(o, mt)
end
--- Create a TCP-based socket object
--@bool[opt=nil] ssl, whether it is an SSL connection, true means yes, the rest means no
--@table[opt=nil] cert, certificate configuration required for SSL connection. The parameters are meaningful only when the SSL parameter is true. The cert format is as follows:
-- {
--caCert = "ca.crt", --CA certificate file (Base64 encoded X.509 format). If this parameter exists, it means that the client will verify the server's certificate; if it does not exist, it will not be verified.
--clientCert = "client.crt", --Client certificate file (Base64 encoded X.509 format), this parameter will be used when the server verifies the client's certificate
--clientKey = "client.key", --Client private key file (Base64 encoded X.509 format)
--clientPassword = "123456", --Client certificate file password [optional]
-- }
--@return client, the socket client object is returned if the creation is successful; nil is returned if the creation fails.
-- @usage
--c = socket.tcp()
--c = socket.tcp(true)
--c = socket.tcp(true, {caCert="ca.crt"})
--c = socket.tcp(true, {caCert="ca.crt", clientCert="client.crt", clientKey="client.key"})
--c = socket.tcp(true, {caCert="ca.crt", clientCert="client.crt", clientKey="client.key", clientPassword="123456"})
function socket_wifi.tcp(ssl, cert)
    return socket((ssl == true and "SSL" or "TCP"), (ssl == true) and cert or nil)
end
--- Create UDP-based socket object
--@return client, the socket client object is returned if the creation is successful; nil is returned if the creation fails.
--@usage c = socket.udp()
function socket_wifi.udp()
    return socket("UDP")
end

local sslInited
local tSslInputCert, sSslInputCert = {}, ""

local function sslInit()
    if not sslInited then
        sslInited = true
        req("AT+SSLINIT")
    end

    local i, item
    for i = 1, #tSslInputCert do
        item = table.remove(tSslInputCert, 1)
        req(item.cmd, item.arg)
    end
    tSslInputCert = {}
end

local function sslTerm()
    if sslInited then
        if not isSocketActive(true) then
            sSslInputCert, sslInited = ""
            req("AT+SSLTERM")
        end
    end
end

local function sslInputCert(t, f)
    if sSslInputCert:match(t .. f .. "&") then
        return
    end
    if not tSslInputCert then
        tSslInputCert = {}
    end
    local s = io.readFile((f:sub(1, 1) == "/") and f or ("/ldata/" .. f))
    if not s then
        log.error("inputcrt err open", path)
        return
    end
    --table.insert(tSslInputCert, {cmd = "AT+SSLCERT=0,\"" .. t .. "\",\"" .. f .. "\",1," .. s:len(), arg = s or ""})
    table.insert(tSslInputCert, {
        cmd = "AT+SYSFLASH=0,\"" .. t .. "\",\"" .. f .. "\",1," .. s:len(),
        arg = s or ""
    })
    sSslInputCert = sSslInputCert .. t .. f .. "&"
end

local path32, path8955
--- Connect to server
--@string address server address, supports ip and domain name
--@param port string or number type, server port
--@return bool result true - success, false - failure
--@number timeout, the maximum timeout of the linked server
--@usage  c = socket.tcp(); c:connect("www.baidu.com",80,5);
function mt:connect(address, port, timeout)
    assert(self.co == coroutine.running(), "socket:connect: coroutine mismatch")

    if not link.isReady() then
        log.info("socket.connect: ip not ready")
        return false
    end

    if cc and cc.anyCallExist() then
        log.info("socket:connect: call exist, cannot connect")
        return false
    end
    self.address = address
    self.port = port
    if self.cert then
        local tConfigCert, i = {}
        --if self.cert then
        --if self.cert.caCert then
        --sslInputCert("cacrt", self.cert.caCert)
        --table.insert(tConfigCert, "AT+SSLCERT=1," .. self.id .. ",\"cacrt\",\"" .. self.cert.caCert .. "\"")
        --     end
        --if self.cert.clientCert then
        --sslInputCert("localcrt", self.cert.clientCert)
        --table.insert(tConfigCert, "AT+SSLCERT=1," .. self.id .. ",\"localcrt\",\"" .. self.cert.clientCert .. "\",\"" .. (self.cert.clientPassword or "") .. "\"")
        --     end
        --if self.cert.clientKey then
        --sslInputCert("localprivatekey", self.cert.clientKey)
        --table.insert(tConfigCert, "AT+SSLCERT=1," .. self.id .. ",\"localprivatekey\",\"" .. self.cert.clientKey .. "\"")
        --     end
        -- end

        -- sslInit()
        --req(string.format("AT+SSLCREATE=%d,\"%s\",%d", self.id, address .. ":" .. port, (self.cert and self.cert.caCert) and 0 or 1))
        --self.created = true
        --for i = 1, #tConfigCert do
        --     req(tConfigCert[i])
        -- end
        --req("AT+SSLCONNECT=" .. self.id)
        --

        req(string.format("AT+CIPSTART=%d,\"%s\",\"%s\",%s", self.id, "SSL", address, port))
    else
        req(string.format("AT+CIPSTART=%d,\"%s\",\"%s\",%s", self.id, self.protocol, address, port))
    end
    --if self.ssl or self.protocol == "UDP" then sys.timerStart(coroutine.resume, 120000, self.co, false, "TIMEOUT") end
    sys.timerStart(coroutine.resume, (timeout or 120) * 1000, self.co, false, "TIMEOUT")
	
    ril.regUrc((self.ssl and "SSL&" or "") .. self.id, onSocketURC)
    self.wait = self.ssl and "+SSLCONNECT" or "+CIPSTART"

    local r, s = coroutine.yield()

    if r == false and s == "DNS" then
        if self.ssl then
            self:sslDestroy()
            self.error = nil
        end

        --require "http"
        --Request Tencent Cloud free HTTPDns analysis
        
        local statusCode, head, body = http.request("GET", "119.29.29.29/d?dn=" .. address).wait()

        --DNS resolution successful
        if result and statusCode == 200 and body and body:match("^[%d%.]+") then
            return self:connect(body:match("^([%d%.]+)"), port)
            --DNS resolution failed
        else
            if dnsParser then
                dnsParserToken = dnsParserToken + 1
                dnsParser(address, dnsParserToken)
                local result, ip = sys.waitUntil("USER_DNS_PARSE_RESULT_" .. dnsParserToken, 40000)
                if result and ip and ip:match("^[%d%.]+") then
                    return self:connect(ip:match("^[%d%.]+"), port)
                end
            end
        end
    end

    if r == false then
        if self.ssl then
            self:sslDestroy()
        end
        sys.publish("LIB_SOCKET_CONNECT_FAIL_IND", self.ssl, self.protocol, address, port)
        return false
    end
    self.connected = true
    socketStatusNtfy()
    return true
end

--- Asynchronous receive and receive selector
--@number keepAlive, the maximum communication interval between the server and the client, also called the maximum heartbeat packet time, in seconds
--@string pingreq, string of heartbeat packet
--@return boole,false indicates failure, true indicates success
function mt:asyncSelect(keepAlive, pingreq)
    assert(self.co == coroutine.running(), "socket:asyncSelect: coroutine mismatch")
    if self.error then
        log.warn('socket.client:asyncSelect', 'error', self.error)
        return false
    end

    self.wait = "SOCKET_SEND"
    while #self.output ~= 0 do
        local data = table.concat(self.output)
        self.output = {}
        for i = 1, string.len(data), SENDSIZE do
            --Packet data according to the maximum MTU unit
            local stepData = string.sub(data, i, i + SENDSIZE - 1)
            --Send AT command to execute data sending
            req(string.format("AT+" .. (self.ssl and "SSL" or "CIP") .. "SEND=%d,%d", self.id, string.len(stepData)), stepData)
            self.wait = self.ssl and "+SSLSEND" or "+CIPSEND"
            if not coroutine.yield() then
                if self.ssl then
                    self:sslDestroy()
                end
                sys.publish("LIB_SOCKET_SEND_FAIL_IND", self.ssl, self.protocol, self.address, self.port)
                return false
            end
        end
    end
    self.wait = "SOCKET_WAIT"
    sys.publish("SOCKET_SEND", self.id)
    if keepAlive and keepAlive ~= 0 then
        if type(pingreq) == "function" then
            sys.timerStart(pingreq, keepAlive * 1000)
        else
            sys.timerStart(self.asyncSend, keepAlive * 1000, self, pingreq or "\0")
        end
    end
    return coroutine.yield()
end
--- Send data asynchronously
--@string data data
--@return result true - success, false - failure
--@usage  c = socket.tcp(); c:connect(); c:asyncSend("12345678");
function mt:asyncSend(data)
    if self.error then
        log.warn('socket.client:asyncSend', 'error', self.error)
        return false
    end
    table.insert(self.output, data or "")
    if self.wait == "SOCKET_WAIT" then
        coroutine.resume(self.co, true)
    end
    return true
end
--- Receive data asynchronously
--@return nil, means no data was received
--@return data If it is UDP protocol, return a new data packet. If it is TCP, return all received data. If there is no data, return an empty string with length 0.
--@usage c = socket.tcp(); c:connect()
--@usage data = c:asyncRecv()
function mt:asyncRecv()
    if #self.input == 0 then
        return ""
    end
    if self.protocol == "UDP" then
        return table.remove(self.input)
    else
        local s = table.concat(self.input)
        self.input = {}
        return s
    end
end

--- Send data
--@string data data
--@return result true - success, false - failure
--@usage  c = socket.tcp(); c:connect(); c:send("12345678");
function mt:send(data)
    assert(self.co == coroutine.running(), "socket:send: coroutine mismatch")
    if self.error then
        log.warn('socket.client:send', 'error', self.error)
        return false
    end
    if self.id == nil then
        log.warn('socket.client:send', 'closed')
        return false
    end

    for i = 1, string.len(data or ""), SENDSIZE do
        --Packet data according to the maximum MTU unit
        local stepData = string.sub(data, i, i + SENDSIZE - 1)
        --Send AT command to execute data sending
        req(string.format("AT+" .. (self.ssl and "SSL" or "CIP") .. "SEND=%d,%d", self.id, string.len(stepData)), stepData)
        self.wait = self.ssl and "+SSLSEND" or "+CIPSEND"
        if not coroutine.yield() then
            if self.ssl then
                self:sslDestroy()
            end
            sys.publish("LIB_SOCKET_SEND_FAIL_IND", self.ssl, self.protocol, self.address, self.port)
            return false
        end
    end
    return true
end
--- receive data
--@number[opt=0] timeout optional parameter, receiving timeout time, in milliseconds
--@string[opt=nil] msg optional parameter, controls the thread where the socket is located to exit the recv blocking state
--@bool[opt=nil] msgNoResume optional parameter, controls the thread where the socket is located to exit the recv blocking state. False or nil means "in the recv blocking state, after receiving the msg message, you can exit the blocking state", true means not to exit.
--@return result Data reception result, true means success, false means failure
--@return data If successful, return the received data; the error returned is "timeout" when timeout occurs; msg returns the string of msg when the control exits
--@return param If msg returns false, the value of data is msg, and the value of param is the parameter of msg.
--@usage c = socket.tcp(); c:connect()
--@usage result, data = c:recv()
--@usage false,msg,param = c:recv(60000,"publish_msg")
function mt:recv(timeout, msg, msgNoResume)
    assert(self.co == coroutine.running(), "socket:recv: coroutine mismatch")
    if self.error then
        log.warn('socket.client:recv', 'error', self.error)
        return false
    end
    self.msgNoResume = msgNoResume
    if msg and not self.iSubscribe then
        self.iSubscribe = msg
        self.subMessage = function(data)
            --if data then table.insert(self.output, data) end
            if (self.wait == "+RECEIVE" or self.wait == "+SSL RECEIVE") and not self.msgNoResume then
                if data then
                    table.insert(self.output, data)
                end
                coroutine.resume(self.co, 0xAA)
            end
        end
        sys.subscribe(msg, self.subMessage)
    end
    if msg and #self.output ~= 0 then
        sys.publish(msg, false)
    end
    if #self.input == 0 then
        self.wait = self.ssl and "+SSL RECEIVE" or "+RECEIVE"
        if timeout and timeout > 0 then
            local r, s = sys.wait(timeout)
            --if not r then
            --return false, "timeout"
            --elseif r and r == msg then
            --return false, r, s
            -- else
            --if self.ssl and not r then self:sslDestroy() end
            --return r, s
            -- end
            if r == nil then
                return false, "timeout"
            elseif r == 0xAA then
                local dat = table.concat(self.output)
                self.output = {}
                return false, msg, dat
            else
                if self.ssl and not r then
                    self:sslDestroy()
                end
                return r, s
            end
        else
            local r, s = coroutine.yield()
            if r == 0xAA then
                local dat = table.concat(self.output)
                self.output = {}
                return false, msg, dat
            else
                return r, s
            end
        end
    end

    if self.protocol == "UDP" then
        return true, table.remove(self.input)
    else
        local s = table.concat(self.input)
        self.input = {}
        return true, s
    end
end

function mt:sslDestroy()
    assert(self.co == coroutine.running(), "socket:sslDestroy: coroutine mismatch")
    if self.ssl and (self.connected or self.created) then
        self.connected = false
        self.created = false
        req("AT+SSLDESTROY=" .. self.id)
        self.wait = "+SSLDESTROY"
        coroutine.yield()
        socketStatusNtfy()
    end
end
--- Destroy a socket
--@return nil
--@usage  c = socket.tcp(); c:connect(); c:send("123"); c:close()
function mt:close(slow)
    assert(self.co == coroutine.running(), "socket:close: coroutine mismatch")
    if self.iSubscribe then
        sys.unsubscribe(self.iSubscribe, self.subMessage)
        self.iSubscribe = false
    end
    if self.connected or self.created then
        self.connected = false
        self.created = false
        req(self.ssl and ("AT+SSLDESTROY=" .. self.id) or ("AT+CIPCLOSE=" .. self.id .. (slow and ",0" or "")))
        self.wait = self.ssl and "+SSLDESTROY" or "+CIPCLOSE"
        coroutine.yield()
        socketStatusNtfy()
    end
    if self.id ~= nil then
        ril.deRegUrc((self.ssl and "SSL&" or "") .. self.id, onSocketURC)
        table.insert((self.ssl and validSsl or valid), 1, self.id)
        if self.ssl then
            socketsSsl[self.id] = nil
        else
        sockets[self.id] = nil
        end
        self.id = nil
    end
end
local function onResponse(cmd, success, response, intermediate)
    local prefix = string.match(cmd, "AT(%+%u+)")
    local id = string.match(cmd, "AT%+%u+=(%d)")
    if response == '+PDP: DEACT' then
        sys.publish('PDP_DEACT_IND')
    end --cipsend will return +PDP: DEACT in response if it happens to be pdp deact
    local tSocket = prefix:match("SSL") and socketsSsl or sockets
    if not tSocket[id] then
        log.warn('socket: response on nil socket', cmd, response)
        return
    end

    if cmd:match("^AT%+SSLCREATE") then
        tSocket[id].createResp = response
    end
    if tSocket[id].wait == prefix then
        if (prefix == "+CIPSTART" or prefix == "+SSLCONNECT") and success then
            --CIPSTART,SSLCONNECT returns OK only to indicate acceptance
            return
        end

        if prefix == '+CIPSEND' then
            if response ~= 'SEND OK' and response ~= 'OK' then
                local acceptLen = response:match("Recv (%d) bytes")
                if acceptLen then
                    if acceptLen ~= cmd:match("AT%+%u+=%d,(%d+)") then
                        success = false
                    end
                else
                    success = false
                end
            end
        elseif prefix == "+SSLSEND" then
            if response:match("%d, *([%u%d :]+)") ~= 'SEND OK' then
                success = false
            end
        end

        local reason, address
        if not success then
            if prefix == "+CIPSTART" then
                address = cmd:match("AT%+CIPSTART=%d,\"%a+\",\"(.+)\",%d+")
            elseif prefix == "+SSLCONNECT" and (tSocket[id].createResp or ""):match("SSL&%d+,CREATE ERROR: 4") then
                address = tSocket[id].address or ""
            end
            if address and not address:match("^[%d%.]+$") then
                reason = "DNS"
            end
        end

        if not reason and not success then
            tSocket[id].error = response
        end
        stopConnectTimer(tSocket, id)
        coroutine.resume(tSocket[id].co, success, reason)
    end
end

local function onSocketReceiveUrc(urc)
    local len, datatest = string.match(urc, "+CIPRECVDATA:(%d+),(.+)")
    local id = link_wifi.getRecvId()
    tSocket = (tag == "SSL" and socketsSsl or sockets)
    len = tonumber(len)
    if len == 0 then
        return urc
    end

    if string.len(datatest) == len then
        sys.publish("SOCKET_RECV", id)
        if tSocket[id].wait == "+RECEIVE" then
            coroutine.resume(tSocket[id].co, true, datatest)
        else --Data enters the buffer, and buffer overflow uses overwrite mode.
            if #tSocket[id].input > INDEX_MAX then
                tSocket[id].input = {}
            end
            table.insert(tSocket[id].input, datatest)
        end
    elseif string.len(datatest) < len then
        log.info("不等的情况")
        local cache = {}
        table.insert(cache, datatest)
        len = len - string.len(datatest)
        local function filter(data)
            --Remaining unreceived data length
            if string.len(data) >= len then --The content of the at channel is more than the remaining unreceived data
                --Intercept data sent from the network
                table.insert(cache, string.sub(data, 1, len))
                --The remaining data is thrown to AT for subsequent processing.
                data = string.sub(data, len + 1, -1)
                if not tSocket[id] then
                    log.warn('socket: receive on nil socket', id)
                else
                    sys.publish("SOCKET_RECV", id)
                    local s = table.concat(cache)
                if tSocket[id].wait == "+RECEIVE" or tSocket[id].wait == "+SSL RECEIVE" then
                        coroutine.resume(tSocket[id].co, true, s)
                    else --Data enters the buffer, and buffer overflow uses overwrite mode.
                        if #tSocket[id].input > INDEX_MAX then
                            tSocket[id].input = {}
                        end
                        table.insert(tSocket[id].input, s)
                    end
                end
                return data
            else
                table.insert(cache, data)
                len = len - string.len(data)
                return "", filter
            end
        end
        return filter
    end
end

ril.regRsp("+CIPCLOSE", onResponse)
ril.regRsp("+CIPSEND", onResponse)
ril.regRsp("+CIPSTART", onResponse)
ril.regRsp("+SSLDESTROY", onResponse)
ril.regRsp("+SSLCREATE", onResponse)
ril.regRsp("+SSLSEND", onResponse)
ril.regRsp("+SSLCONNECT", onResponse)
ril.regUrc("+CIPRECVDATA", onSocketReceiveUrc)
ril.regUrc("+SSL RECEIVE", onSocketReceiveUrc)

ril.regRsp("+SYSFLASH", function(cmd, result, response, intermediate, param)
    if cmd:find("AT%+SYSFLASH=0") then
        req("AT+SYSFLASH=1,\"" .. param.path32 .. "\",0," .. io.fileSize(param.path8955), io.readFile(param.path8955), nil, nil, param.id)
    elseif cmd:find("AT%+SYSFLASH=1") then
        local tSocket = sockets
        coroutine.resume(tSocket[param].co)
    end
end)


function socket_wifi.printStatus()
    log.info('socket.printStatus', 'valid id', table.concat(valid), table.concat(validSsl))

    for m, n in pairs({sockets, socketsSsl}) do
        for _, client in pairs(n) do
            for k, v in pairs(client) do
                log.info('socket.printStatus', 'client', client.id, k, v)
            end
        end
    end
end

--- Set parameters for automatic retransmission of TCP layer
--@number[opt=4] retryCnt, the number of retransmissions; the value range is 0 to 12
--@number[opt=16] retryMaxTimeout, limits the maximum timeout allowed for each retransmission (in seconds), the value range is 1 to 16
--@return nil
-- @usage
-- setTcpResendPara(3,8)
-- setTcpResendPara(4,16)
function socket_wifi.setTcpResendPara(retryCnt, retryMaxTimeout)
    req("AT+TCPUSERPARAM=6," .. (retryCnt or 4) .. ",7200," .. (retryMaxTimeout or 16))
    ril.setDataTimeout(((retryCnt or 4) * (retryMaxTimeout or 16) + 60) * 1000)
end

--- Set up user-defined DNS resolver.
--When connecting to the server through a domain name, the DNS resolution process is as follows:
--1. Use the method provided in core to connect to the operator's DNS server for resolution. If the resolution is successful, it will end; if the resolution fails, go to step 2.
--2. Use the free Tencent Cloud HttpDns provided in the script lib to parse. If the parsing is successful, it will end; if the parsing fails, go to step 3.
--3. If there is a user-defined DNS resolver, use the user-defined DNS resolver here to resolve it.
--@function[opt=nil] parserFnc, user-defined DNS parser function, the function calling form is:
--parserFnc(domainName,token), after calling the interface, it will wait for the message notification of the parsing result or the 40 second timeout fails.
--domainName: string type, representing domain name, such as "www.baidu.com"
--token: string type, the token of this DNS resolution request, such as "1"
--After the parsing is completed, publish a message to notify the parsing results. At most one IP address in the message parameter is returned, sys.publish("USER_DNS_PARSE_RESULT_"..token,ip), for example:
--          sys.publish("USER_DNS_PARSE_RESULT_1","115.239.211.112")
--Indicates that the parsing is successful and an IP address 115.239.211.112 is parsed.
--          sys.publish("USER_DNS_PARSE_RESULT_1")
--Indicates parsing failure
--@return nil
--@usage socket.setDnsParser(parserFnc)
function socket_wifi.setDnsParser(parserFnc)
    dnsParser = parserFnc
end

--- Set the data sending mode (call this interface setting before the network is ready).
--If set to fast mode, pay attention to the following two points:
--1. If the data sent through the send interface is successfully sent to the server, the device cannot obtain this success status.
--2. If the data sent through the send interface fails, the device can obtain the failure status.
--Slow send mode can obtain the success or failure of sending through the send interface.
--
-- ****************************************************************************************************************************************************************
--When the TCP protocol sends data, after the data is sent, it must wait until the server returns a TCP ACK packet before the data is considered to be sent successfully. In the case of poor network conditions, this ACK confirmation will cause the sending process to be very slow.
--As a result, the subsequent AT processing logic of the user program is always in a waiting state. For example, after executing the AT+CIPSEND action to send a packet of data, the next step is to execute AT+QTTS to play TTS, but CIPSEND waits for 1 minute before returning SEND OK.
--At this time, AT+QTTS will wait for 1 minute, which may not be what the program wants to see.
--At this time, it can be set to fast mode. AT+CIPSEND can immediately return a result. This result indicates "whether the data is saved in the buffer", thus not affecting the timely execution of other subsequent AT instructions.
-- 
--The AT version can use the AT+CIPQSEND command, and the Luat version can set the sending mode to fast or slow through the socket.setSendMode interface.
-- 
--In fast-send mode, there is a 1460*7=10220-byte buffer in the core. The data to be sent is first stored in this buffer, and then automatically sent cyclically in the core.
--If this buffer is full, AT+CIPSEND will directly return ERROR, and the socket:send interface will also directly return failure.
-- 
--If the following conditions are met at the same time, it is suitable to use the fast mode:
--1. The amount of data sent is small and the frequency of sending is low. The data sending speed will not exceed the 10220 bytes in the core;
--There is no precise judgment standard. You can simply judge it by not exceeding 10222 bytes in 3 minutes. There was an example that was not suitable for the fast mode:
--The user uses the Luat version of http to upload a file of tens of kilobytes, and sets the fast mode, causing the sending to fail all the time because data is inserted into the buffer in the core in a loop.
--The speed of inserting data is much faster than the speed of sending data to the server, so the buffer will soon become slow. When data is inserted again, a failure will be returned directly.
--2. For each data sent, there is no need to confirm the sending result.
--3. The data sending function cannot affect the timely response of other functions.
-- ****************************************************************************************************************************************************************
--
--@number[opt=0] mode, data sending mode, 0 means slow sending, 1 means fast sending
--@return nil
--@usage socket.setSendMode(1)
function socket_wifi.setSendMode(mode)
    linkTest.setSendMode(mode)
end

--setTcpResendPara(4, 16)
return socket_wifi
