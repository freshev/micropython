local JT808Prot = {}
local paraCtrl = require "paraCtrl"

--Terminal message ID
JT808Prot.T_COMMON_RSP = 0x0001
JT808Prot.T_LOC_RPT = 0x0200
JT808Prot.T_HEART_RPT = 0x0002

JT808Prot.PARA_HEART_FREQ = 0x0001
JT808Prot.PARA_TCP_RSP_TIMEOUT = 0x0002
JT808Prot.PARA_TCP_RESEND_CNT = 0x0003
JT808Prot.PARA_LOC_RPT_STRATEGY = 0x0020
JT808Prot.PARA_LOC_RPT_MODE = 0x0021
JT808Prot.PARA_SLEEP_LOC_RPT_FREQ = 0x0027
JT808Prot.PARA_ALARM_LOC_RPT_FREQ = 0x0028
JT808Prot.PARA_WAKE_LOC_RPT_FREQ = 0x0029
JT808Prot.PARA_WAKE_LOC_RPT_DISTANCE = 0x002C
JT808Prot.PARA_SLEEP_LOC_RPT_DISTANCE = 0x002E
JT808Prot.PARA_ALARM_LOC_RPT_DISTANCE = 0x002F
JT808Prot.PARA_FENCE_RADIS = 0x0031
JT808Prot.PARA_ALARM_FILTER = 0x0050
JT808Prot.PARA_KEY_FLAG = 0x0054
JT808Prot.PARA_SPEED_LIMIT = 0x0055
JT808Prot.PARA_SPEED_EXCEED_TIME = 0x0056

JT808Prot.CONTROL_RESET = 4

--Platform message ID
JT808Prot.S_COMMON_RSP = 0x8001
JT808Prot.S_REGISTER_RSP = 0x8100
JT808Prot.S_SET_PARA = 0x8103
JT808Prot.S_CONTROL = 0x8105

local tSendSeq = 0

local function encodeBcdNum(d,n)
    if d:len()<n then
        return (string.rep("0",n-d:len())..d):fromHex()
    else
        return (d:sub(1,n)):fromHex()
    end
end

local function calCrc(d)
    local sum = 0
    for i=1,d:len() do
        sum = sum~d:byte(i)
    end
    return sum
end

--terminal message packet
--Return value 1: packed string
--Return value 2: serial number
function JT808Prot.encode(msgId,...)

    local function cmnRsp(svrSeq,svrId,result)
        return pack.pack(">HHb",svrSeq,svrId,result)
    end

    local function locRpt(alarm,status,lat,lng,alt,spd,course,tm,extInfo)
        --return pack.pack(">ddddHHHAA",alarm,status,lat,lng,alt,spd,course,tm,extInfo)
        return pack.pack(">ddddHHHAA",alarm,status,lat,lng,math.modf(alt),math.modf(spd),math.modf(course),tm,extInfo)
    end

    local procer =
    {
        [JT808Prot.T_COMMON_RSP] = cmnRsp,
        [JT808Prot.T_LOC_RPT] = locRpt,
        [JT808Prot.T_HEART_RPT] = function() return "" end,
    }
    local msgBody = procer[msgId](...)    --message body



    local msgHead = pack.pack(">HHAH",  --message header
                    msgId, --Message ID
                    msgBody:len(), --Message body attributes, subcontracting and data encryption have not yet been implemented
                    encodeBcdNum(paraCtrl.getTerminalNum(),12), --Terminal mobile phone number
                    tSendSeq --Message serial number
                    )
    local curSeq = tSendSeq
    tSendSeq = (tSendSeq==0xFFFF) and 0 or (tSendSeq+1)
    --Check code
    local crc = calCrc(msgHead..msgBody)

    --escape
    local s = msgHead..msgBody..string.char(crc)
    s = s:gsub("\125","\125\1") --Limit -> Limit 01
    s = s:gsub("\126","\125\2") --H -> limit 02
    return string.char(0x7E)..s..string.char(0x7E),curSeq
end

--Return value 1: unprocessed data
--Return value 2: The parsed packet. If the parsing fails, it will be nil. If the parsing is successful, it will be of table type. There is a result variable of boolean type in the table to further represent the result.
function JT808Prot.decode(s)
    local _,tail,msg = s:find("\126(.-)\126")
    local decPacket,msgAttr = {}
    if not msg then
        log.warn("prot.decode","wait packet complete")
        return s,nil
    end

    --Anti-escaping
    msg = msg:gsub("\125\2","\126") --Limit 02 -> h
    msg = msg:gsub("\125\1","\125") --Limit 01 -> Limit

    if msg:len()<13 then
        log.error("prot.decode","packet len is too short")
        return s:sub(tail+1,-1),nil
    end

    --Verify check code
    if calCrc(msg:sub(1,-2))~=msg:byte(-1) then
        log.error("prot.decode","packet len is too short")
        return s:sub(tail+1,-1),nil
    end


    --Message header and message body
    msg = msg:sub(1,-2)

    --Message ID
    _,decPacket.msgId = pack.unpack(msg,">H")

    --Message serial number
    decPacket.msgSeq = pack.unpack(msg,">H")

    --Message body properties
    _,msgAttr = pack.unpack(msg:sub(3,4),">H")
    --subcontracting sign
    local msgMultiPacket = bit.isset(msgAttr,13)
    --Data encryption flag
    local msgEncrypt = bit.band(bit.rshift(msgAttr,10),0x07)
    --Message body length
    local msgLen = bit.band(msgAttr,0x03FF)

    local msgBodyHead = msgMultiPacket and 17 or 13
    local msgBody = msg:sub(msgBodyHead,-1)
    decPacket.result = msgBody:len()==msgLen
    if not decPacket.result then
        log.error("prot.decode","body len no match")
        return s:sub(tail+1,-1),decPacket
    end

    local function cmnRsp(body)
        if body:len()~=5 then
            log.error("prot.decode.cmnRsp","len~=5 error",body:len())
            return false
        end

        _,decPacket.tmnlSeq,decPacket.tmnlId,decPacket.rspResult = pack.unpack(body,">HHb")
    end

    local function setPara(body)
        if body:len()<5 then
            log.error("prot.decode.setPara","len<5 error",body:len())
            return false
        end

        local cnt,i = body.byte(1)
        local tmp,id = body:sub(2,-1)
        while tmp:len() > 0 do
            _,id = pack.unpack(tmp,">i")
            if not list[id] then
                log.error("prot.decode.setPara","unsupport para",id)
                return false
            end
            local paraLen = tmp:byte(5)
            if tmp:len() < paraLen+5 then
                log.error("prot.decode.setPara","para data incomplete",id)
                return false
            end
            if not paraCtrl.setPara(id,paraLen,tmp:sub(6,5+paraLen)) then
                log.error("prot.decode.setPara","para proc error",id)
                return false
            end
            tmp = tmp:sub(6+tmpLen,-1)
        end

        return true
    end

    local function control(body)
        if body:len()<1 then
            log.error("prot.decode.control","len<1 error",body:len())
            return false
        end

        decPacket.controlCmd = body:byte(1)
        return true
    end


    local procer =
    {
        [JT808Prot.S_COMMON_RSP] = cmnRsp,
        [JT808Prot.S_SET_PARA] = setPara,
        [JT808Prot.S_CONTROL] = control,
    }

    if not procer[decPacket.msgId] then
        log.error("prot.decode","invalid id",decPacket.msgId)
        decPacket.result = false
        return s:sub(tail+1,-1),decPacket
    end


    if procer[decPacket.msgId](msgBody)==false then
        log.error("prot.decode","procer fail")
        decPacket.result = false
    end

    return s:sub(tail+1,-1),decPacket
end

return JT808Prot
