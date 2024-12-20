
--- Module function: MQTT client
--@Modules mqtt
--@author openLuat
--@license MIT
--@copyright openLuat
--@release 2017.10.24

local mqtt = {}

--MQTT command id
local CONNECT, CONNACK, PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBSCRIBE, SUBACK, UNSUBSCRIBE, UNSUBACK, PINGREQ, PINGRESP, DISCONNECT = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
local CLIENT_COMMAND_TIMEOUT = 60000

sys = require("sys")
local pack = _G.pack
local string = _G.string
local encodeLen = mqttcore.encodeLen
--local encodeUTF8 = mqttcore.encodeUTF8
--local function encodeLen(len)
--local s = ""
--local digit
--     repeat
--digit = only % 128
--len = (len - digit) / 128
--if len > 0 then
----digit = bit.bor(digit, 0x80)
--digit = digit | 0x80
--         end
--s = s .. string.char(digit)
--until (len <= 0)
--return s
-- end

local encodeUTF8 = mqttcore.encodeUTF8
--local function encodeUTF8(s)
--if not s or #s == 0 then
--return ""
--     else
--return pack.pack(">P", s)
--     end
-- end

local packCONNECT = mqttcore.packCONNECT

--local function packCONNECT(clientId, keepAlive, username, password, cleanSession, will, version)
--local content = pack.pack(">PbbHPAAAA",
--version == "3.1" and "MQIsdp" or "MQTT",
--version == "3.1" and 3 or 4,
--(#username == 0 and 0 or 1) * 128 + (#password == 0 and 0 or 1) * 64 + will.retain * 32 + will.qos * 8 + will.flag * 4 + cleanSession * 2,
--         keepAlive,
--         clientId,
--         encodeUTF8(will.topic),
--         encodeUTF8(will.payload),
--         encodeUTF8(username),
--         encodeUTF8(password))
--local mydata = pack.pack(">bAA",
--CONNECT * 16,
--         encodeLen(string.len(content)),
--         content)
--local tdata = mqttcore.packCONNECT(clientId, keepAlive, username, password, cleanSession, will, version)
--log.info("mqtt", "true", mydata:toHex())
--log.info("mqtt", "false", tdata:toHex())
--return mydata
-- end

local packSUBSCRIBE = mqttcore.packSUBSCRIBE

--local function packSUBSCRIBE(dup, packetId, topics)
--local header = SUBSCRIBE * 16 + dup * 8 + 2
--local data = pack.pack(">H", packetId)
--for topic, qos in pairs(topics) do
--data = data .. pack.pack(">Pb", topic, qos)
--     end
--local mydata = pack.pack(">bAA", header, encodeLen(#data), data)
--log.info("mqtt", "true", mydata:toHex())
--local tdata = mqttcore.packSUBSCRIBE(dup, packetId, topics)
--log.info("mqtt", "false", tdata:toHex())
--return mydata
-- end

local packUNSUBSCRIBE = mqttcore.packUNSUBSCRIBE
--local function packUNSUBSCRIBE(dup, packetId, topics)
--local header = UNSUBSCRIBE * 16 + dup * 8 + 2
--local data = pack.pack(">H", packetId)
--for k, topic in pairs(topics) do
--data = data .. pack.pack(">P", topic)
--     end
--return pack.pack(">bAA", header, encodeLen(#data), data)
-- end

local packPUBLISH = mqttcore.packPUBLISH

--local function packPUBLISH(dup, qos, retain, packetId, topic, payload)
--local header = PUBLISH * 16 + dup * 8 + qos * 2 + retain
--local len = 2 + #topic + #payload
--local mydata = nil
--if qos > 0 then
--mydata = pack.pack(">bAPHA", header, encodeLen(len + 2), topic, packetId, payload)
--     else
--mydata = pack.pack(">bAPA", header, encodeLen(len), topic, payload)
--     end
--local tdata = mqttcore.packPUBLISH(dup, qos, retain, packetId, topic, payload)
--log.info("mqtt", "true", mydata:toHex())
--log.info("mqtt", "false", tdata:toHex())
--return mydata
-- end

local packACK = mqttcore.packACK

--local function packACK(id, dup, packetId)
--return pack.pack(">bbH", id * 16 + dup * 8 + (id == PUBREL and 1 or 0) * 2, 0x02, packetId)
-- end

local packZeroData = mqttcore.packZeroData

--local function packZeroData(id, dup, qos, retain)
--dup = dup or 0
--qos = qos or 0
--retain = retain or 0
--return pack.pack(">bb", id * 16 + dup * 8 + qos * 2 + retain, 0)
-- end

local function unpack(s)
    if #s < 2 then return end
    log.debug("mqtt.unpack", #s, string.toHex(string.sub(s, 1, 50)))

    --read remaining length
    local len = 0
    local multiplier = 1
    local pos = 2

    repeat
        if pos > #s then return end
        local digit = string.byte(s, pos)
        len = len + ((digit % 128) * multiplier)
        multiplier = multiplier * 128
        pos = pos + 1
    until digit < 128

    if #s < len + pos - 1 then return end

    local header = string.byte(s, 1)

    --local packet = {id = (header - (header % 16)) / 16, dup = ((header % 16) - ((header % 16) % 8)) / 8, qos = bit.band(header, 0x06) / 2, retain = bit.band(header, 0x01)}
    local packet = {id = (header - (header % 16)) >> 4, dup = ((header % 16) - ((header % 16) % 8)) >> 3, qos = (header & 0x06) >> 1, retain = (header & 0x01)}
    local nextpos

    if packet.id == CONNACK then
        nextpos, packet.ackFlag, packet.rc = pack.unpack(s, "bb", pos)
    elseif packet.id == PUBLISH then
        nextpos, packet.topic = pack.unpack(s, ">P", pos)
        if packet.qos > 0 then
            nextpos, packet.packetId = pack.unpack(s, ">H", nextpos)
        end
        packet.payload = string.sub(s, nextpos, pos + len - 1)
    elseif packet.id ~= PINGRESP then
        if len >= 2 then
            nextpos, packet.packetId = pack.unpack(s, ">H", pos)
        else
            packet.packetId = 0
        end
    end

    return packet, pos + len
end

local mqttc = {}
mqttc.__index = mqttc

--- Create an mqtt client instance
--@string clientId
--@number[opt=300] keepAlive heartbeat interval (unit: seconds), default 300 seconds
--@string[opt=""] username username, the username is empty and configured as "" or nil
--@string[opt=""] password password, if the password is empty, configure it to "" or nil
--@number[opt=1] cleanSession 1/0
--@table[opt=nil] will will parameter, the format is {qos=, retain=, topic=, payload=}
--@string[opt="3.1.1"] version MQTT version number
--@return table mqttc client instance
-- @usage
--mqttc = mqtt.client("clientid-123")
--mqttc = mqtt.client("clientid-123",200)
--mqttc = mqtt.client("clientid-123",nil,"user","password")
--mqttc = mqtt.client("clientid-123",nil,"user","password",nil,nil,"3.1")
function mqtt.client(clientId, keepAlive, username, password, cleanSession, will, version)
    local o = {}
    local packetId = 1

    if will then
        will.flag = 1
    else
        will = {flag = 0, qos = 0, retain = 0, topic = "", payload = ""}
    end

    o.clientId = clientId
    o.keepAlive = keepAlive or 300
    o.username = username or ""
    o.password = password or ""
    o.cleanSession = cleanSession or 1
    o.version = version or "3.1.1"
    o.will = will
    o.commandTimeout = CLIENT_COMMAND_TIMEOUT
    o.cache = {}--Received mqtt packet buffer
    o.inbuf = "" --Unfinished data buffering
    o.connected = false
    o.getNextPacketId = function()
        packetId = packetId == 65535 and 1 or (packetId + 1)
        return packetId
    end
    o.lastOTime = 0
    o.pkgs = {}

    setmetatable(o, mqttc)

    return o
end

--Check whether heartbeat packet needs to be sent
function mqttc:checkKeepAlive()
    if self.keepAlive == 0 then return true end
    if os.time() - self.lastOTime >= self.keepAlive then
        if not self:write(packZeroData(PINGREQ)) then
            log.info("mqtt.client:", "pingreq send fail")
            return false
        end
    end
    return true
end

--Send mqtt data
function mqttc:write(data)
    log.debug("mqtt.client:write", string.toHex(string.sub(data, 1, 50)))
    local r = self.io:send(data)
    if r then self.lastOTime = os.time() end
    return r
end

--Receive mqtt data packet
function mqttc:read(timeout, msg, msgNoResume)
    if not self:checkKeepAlive() then
        log.warn("mqtt.read checkKeepAlive fail")
        return false
    end

    local topic = "MQTTC_PKG_" .. tostring(self.io:id())
    local result, data, param = sys.waitUntil({topic, msg}, timeout)
    --log.debug("mqtt.read", result, data, param)
    if result then --Receive topic message
        local pkg = table.remove(self.pkgs, 1)
        if pkg ~= nil then
            --log.debug("mqtt", "get packet", pkg.id, pkg.packetId)
            return true, pkg
        end
        --log.debug("mqtt", "get sys.msg", msg, data)
        return false, msg, data
    else
        if self.io:closed() == 1 then
            return false
        else
            return false, "timeout"
        end
    end
end

local function update_resp(_self, data)
    if #data > 0 then
        if #_self.inbuf > 0 then
            _self.inbuf = _self.inbuf .. data
        else
            _self.inbuf = data
        end
    end
    --log.debug("mqttc", "data recv to unpack", _self.inbuf:toHex())
    local packet, nextpos = unpack(_self.inbuf)
    if packet then
        log.info("mqttc", "msg unpack ok", packet.id)
        _self.inbuf = string.sub(_self.inbuf, nextpos)
        table.insert(_self.pkgs, packet)
        sys.publish("MQTTC_PKG_" .. tostring(_self.io:id()))
        if #_self.inbuf > 0 then
            update_resp(_self, "")
        end
    else
        log.info("mqttc", "data not full")
    end

    return true
end

--Wait to receive the specified mqtt message
function mqttc:waitfor(id, timeout, msg, msgNoResume)
    for index, packet in ipairs(self.cache) do
        if packet.id == id then
            return true, table.remove(self.cache, index)
        end
    end

    while true do
        local insertCache = true
        local r, data, param = self:read(timeout, msg, msgNoResume)
        if r then
            if data.id == PUBLISH then
                if data.qos > 0 then
                    if not self:write(packACK(data.qos == 1 and PUBACK or PUBREC, 0, data.packetId)) then
                        log.info("mqtt.client:waitfor", "send publish ack failed", data.qos)
                        return false
                    end
                end
            elseif data.id == PUBREC or data.id == PUBREL then
                if not self:write(packACK(data.id == PUBREC and PUBREL or PUBCOMP, 0, data.packetId)) then
                    log.info("mqtt.client:waitfor", "send ack fail", data.id == PUBREC and "PUBREC" or "PUBCOMP")
                    return false
                end
                insertCache = false
            end

            if data.id == id then
                return true, data
            end
            if insertCache then table.insert(self.cache, data) end
        else
            return false, data, param
        end
    end
end

--- Connect to mqtt server
--@string host server address
--@param port string or number type, server port
--@string[opt="tcp"] transport "tcp"或者"tcp_ssl"
--@table[opt=nil] cert, table or nil type, ssl certificate, this parameter is only meaningful when the transport is "tcp_ssl". The cert format is as follows:
-- {
--caCert = "ca.crt", --CA certificate file (Base64 encoded X.509 format). If this parameter exists, it means that the client will verify the server's certificate; if it does not exist, it will not be verified.
--clientCert = "client.crt", --Client certificate file (Base64 encoded X.509 format), this parameter will be used when the server verifies the client's certificate
--clientKey = "client.key", --Client private key file (Base64 encoded X.509 format)
--clientPassword = "123456", --Client certificate file password [optional]
-- }
--@number timeout, the maximum timeout of the linked server
--@return result true indicates success, false or nil indicates failure
--@usage mqttc = mqtt.client("clientid-123", nil, nil, false); mqttc:connect("mqttserver.com", 1883, "tcp", 5)
function mqttc:connect(host, port, transport, cert, timeout)
    if self.connected then
        log.info("mqtt.client:connect", "has connected")
        return false
    end

    if self.io then
        self.io:close()
        self.io = nil
    end

    if transport and transport ~= "tcp" and transport ~= "tcp_ssl" then
        log.info("mqtt.client:connect", "invalid transport", transport)
        return false
    end

    self.io = socket.tcp(transport == "tcp_ssl" or type(cert) == "table", cert)

    if not self.io:connect(host, port, timeout) then
        log.info("mqtt.client:connect", "connect host fail")
        return false
    end

    if not self:write(packCONNECT(self.clientId, self.keepAlive, self.username, self.password, self.cleanSession, self.will, self.version)) then
        log.info("mqtt.client:connect", "send fail")
        return false
    end

    local r, packet = self:waitfor(CONNACK, self.commandTimeout, nil, true)
    --if not r or packet.rc ~= 0 then
    --log.info("mqtt.client:connect", "connack error", r and packet.rc or -1)
    --return false,packet.rc
    -- end
    if (not r) or (not packet) or packet.rc ~= 0 then
        log.info("mqtt.client:connect", "connack error", r and packet.rc or -1)
        return false, packet and packet.rc or -1
    end

    self.connected = true

    return true
end

--- Subscribe to topics
--@param topic, string or table type. When there is one topic, it is string type. When there are multiple topics, it is table type. The topic content is UTF8 encoded.
--@param[opt=0] qos, number or nil. When the topic is one topic, qos is number type (0/1/2, default 0); when the topic is multiple topics, qos is nil.
--@return bool true indicates success, false or nil indicates failure
-- @usage
--mqttc:subscribe("/abc", 0) -- subscribe topic "/abc" with qos = 0
--mqttc:subscribe({["/topic1"] = 0, ["/topic2"] = 1, ["/topic3"] = 2}) -- subscribe multi topic
function mqttc:subscribe(topic, qos)
    if not self.connected then
        log.info("mqtt.client:subscribe", "not connected")
        return false
    end

    local topics
    if type(topic) == "string" then
        topics = {[topic] = qos and qos or 0}
    else
        topics = topic
    end

    if not self:write(packSUBSCRIBE(0, self.getNextPacketId(), topics)) then
        log.info("mqtt.client:subscribe", "send failed")
        return false
    end

    local r, packet = self:waitfor(SUBACK, self.commandTimeout, nil, true)
    if not r then
        log.info("mqtt.client:subscribe", "wait ack failed")
        return false
    end

    if not (packet.grantedQos and packet.grantedQos~="" and not packet.grantedQos:match(string.char(0x80))) then
        log.info("mqtt.client:subscribe", "suback grant qos error", packet.grantedQos)
        return false
    end

    return true
end

--- Unsubscribe from topic
--@param topic, string or table type. When there is one topic, it is string type. When there are multiple topics, it is table type. The topic content is UTF8 encoded.
--@return bool true indicates success, false or nil indicates failure
-- @usage
--mqttc:unsubscribe("/abc") -- unsubscribe topic "/abc"
--mqttc:unsubscribe({"/topic1", "/topic2", "/topic3"}) -- unsubscribe multi topic
function mqttc:unsubscribe(topic)
    if not self.connected then
        log.info("mqtt.client:unsubscribe", "not connected")
        return false
    end

    local topics
    if type(topic) == "string" then
        topics = {topic}
    else
        topics = topic
    end

    if not self:write(packUNSUBSCRIBE(0, self.getNextPacketId(), topics)) then
        log.info("mqtt.client:unsubscribe", "send failed")
        return false
    end

    if not self:waitfor(UNSUBACK, self.commandTimeout, nil, true) then
        log.info("mqtt.client:unsubscribe", "wait ack failed")
        return false
    end

    return true
end

--- Post a message
--@string topic UTF8 encoded string
--@string payload The user controls the encoding of the payload. mqtt.lua will not perform any encoding conversion on the payload.
--@number[opt=0] qos 0/1/2, default 0
--@number[opt=0] retain 0 or 1
--@return bool Returns true if the release is successful, false if it fails.
-- @usage
--mqttc = mqtt.client("clientid-123", nil, nil, false)
--mqttc:connect("mqttserver.com", 1883, "tcp")
--mqttc:publish("/topic", "publish from luat mqtt client", 0)
function mqttc:publish(topic, payload, qos, retain)
    if not self.connected then
        log.info("mqtt.client:publish", "not connected")
        return false
    end

    qos = qos or 0
    retain = retain or 0

    if not self:write(packPUBLISH(0, qos, retain, qos > 0 and self.getNextPacketId() or 0, topic, payload)) then
        log.info("mqtt.client:publish", "socket send failed")
        return false
    end

    if qos == 0 then return true end

    if not self:waitfor(qos == 1 and PUBACK or PUBCOMP, self.commandTimeout, nil, true) then
        log.warn("mqtt.client:publish", "wait ack timeout")
        return false
    end

    return true
end

--- receive messages
--@number timeout receive timeout, in milliseconds
--@string[opt=nil] msg optional parameter, controls the thread where the socket is located to exit the recv blocking state
--@return result Data reception result, true means success, false means failure
--@return data
--If result is true, it means the mqtt package sent by the server
--
--If result is false, timeout fails and data is "timeout"
--If result is false, msg controls exit, and data is the string of msg.
--If result is false, the socket connection is passively disconnected and the control exits, and data is "CLOSED"
--If result is false, PDP disconnection control exits, data is "IP_ERROR_IND"
--
--If result is false, mqtt is not connected and data is nil
--If result is false, a PUBLISH message is received, sending a PUBACK or PUBREC message fails, and data is nil.
--If result is false, a PUBREC message is received, sending a PUBREL message fails, and data is nil.
--If result is false, a PUBREL message is received, sending a PUBCOMP message fails, and data is nil.
--If result is false, sending the PINGREQ message fails and data is nil.
--@return param If msg controls the exit, the value of param is the parameter of msg; in other cases, it is meaningless and is nil.
-- @usage
--true, packet = mqttc:receive(2000)
--false, error_message = mqttc:receive(2000)
--false, msg, para = mqttc:receive(2000,"APP_SEND_DATA")
function mqttc:receive(timeout, msg)
    if not self.connected then
        log.info("mqtt.client:receive", "not connected")
        return false
    end

    return self:waitfor(PUBLISH, timeout, msg)
end

--- Disconnect from the server
--@return nil
-- @usage
--mqttc = mqtt.client("clientid-123", nil, nil, false)
--mqttc:connect("mqttserver.com", 1883, "tcp")
--process data
-- mqttc:disconnect()
function mqttc:disconnect()
    if self.io then
        if self.connected then self:write(packZeroData(DISCONNECT)) end
        self.io:close()
        self.io = nil
    end
    self.cache = {}
    self.inbuf = ""
    self.connected = false
end


return mqtt
