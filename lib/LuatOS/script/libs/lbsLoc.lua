--[[
@module lbsLoc
@summary lbsLoc 发送基站定位请求
@version 1.0
@date    2022.12.16
@author  luatos
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
--Note: The PRODUCT_KEY here is for demonstration purposes only and cannot be used in a production environment
--In mass production projects, you must use the project productKey you created in iot.openluat.com, which can be viewed in the project details.
--The coordinate system for base station positioning is WSG84
PRODUCT_KEY = "v32xEAKsGTIEQxtqgwCldp5aPlcnPs3K"
local lbsLoc = require("lbsLoc")
--Function: Callback function after obtaining the latitude and longitude corresponding to the base station
--Parameters: -- result: number type, 0 means success, 1 means the network environment is not ready yet, 2 means failure to connect to the server, 3 means failure to send data, 4 means the receiving server response timed out, 5 means the server failed to return the query; when it is 0, Only the next 5 parameters are meaningful
		--lat: string type, latitude, 3 digits for the integer part and 7 digits for the decimal part, for example, 031.2425864
		--lng: string type, longitude, 3 digits for the integer part and 7 digits for the decimal part, for example, 121.4736522
        --addr: currently meaningless
        --time: string type or nil, the time returned by the server, 6 bytes, year, month, day, hour, minute and second, need to be converted to hexadecimal for reading
            --The first byte: year minus 2000, for example, 2017, it is 0x11
            --The second byte: month, for example, July is 0x07, December is 0x0C
            --The third byte: day, for example, the 11th is 0x0B
            --The fourth byte: hour, for example, when 18, it is 0x12
            --The fifth byte: points, for example, 59 points is 0x3B
            --The sixth byte: seconds, for example, 48 seconds is 0x30
        --locType: numble type or nil, positioning type, 0 indicates successful base station positioning, 255 indicates successful WIFI positioning
function getLocCb(result, lat, lng, addr, time, locType)
    log.info("testLbsLoc.getLocCb", result, lat, lng)
    --Successfully obtained latitude and longitude, coordinate system WGS84
    if result == 0 then
        log.info("服务器返回的时间", time:toHex())
        log.info("定位类型,基站定位成功返回0", locType)
    end
end

sys.taskInit(function()
    sys.waitUntil("IP_READY", 30000)
    while 1 do
        mobile.reqCellInfo(15)
        sys.waitUntil("CELL_INFO_UPDATE", 3000)
        lbsLoc.request(getLocCb)
        sys.wait(60000)
    end
end)
]]

local sys = require "sys"
local sysplus = require("sysplus")
local libnet = require("libnet")

local lbsLoc = {}
local d1Name = "lbsLoc"

--- Convert ASCII string to BCD encoding format string (only supports numbers)
--@string inStr String to be converted
--@number destLen The expected length of the converted string, if the actual length is insufficient, fill it with F
--@return string data, converted string
-- @usage
local function numToBcdNum(inStr,destLen)
    local l,t,num = string.len(inStr or ""),{}
    destLen = destLen or (inStr:len()+1)/2
    for i=1,l,2 do
        num = tonumber(inStr:sub(i,i+1),16)
        if i==l then
            num = 0xf0+num
        else
            num = (num%0x10)*0x10 + (num-(num%0x10))/0x10
        end
        table.insert(t,num)
    end

    local s = string.char(unpack(t))

    l = string.len(s)
    if l < destLen then
        s = s .. string.rep("\255",destLen-l)
    elseif l > destLen then
        s = string.sub(s,1,destLen)
    end

    return s
end

--- BCD encoding format string is converted into number ASCII string (only supports numbers)
--@string num string to be converted
--@return string data, converted string
-- @usage
local function bcdNumToNum(num)
	local byte,v1,v2
	local t = {}

	for i=1,num:len() do
		byte = num:byte(i)
		v1,v2 = bit.band(byte,0x0f),bit.band(bit.rshift(byte,4),0x0f)

		if v1 == 0x0f then break end
		table.insert(t,v1)

		if v2 == 0x0f then break end
		table.insert(t,v2)
	end

	return table.concat(t)
end


local function netCB(msg)
	--log.info("Unprocessed message", msg[1], msg[2], msg[3], msg[4])
end


local function enCellInfo(s)
    local ret,t,mcc,mnc,lac,ci,rssi,k,v,m,n,cntrssi = "",{}
        for k,v in pairs(s) do
            mcc,mnc,lac,ci,rssi = v.mcc,v.mnc,v.tac,v.cid,((v.rsrq + 144) >31) and 31 or (v.rsrq + 144)
            local handle = nil
            for k,v in pairs(t) do
                if v.lac == lac and v.mcc == mcc and v.mnc == mnc then
                    if #v.rssici < 8 then
                        table.insert(v.rssici,{rssi=rssi,ci=ci})
                    end
                    handle = true
                break
                end
            end
            if not handle then
                table.insert(t,{mcc=mcc,mnc=mnc,lac=lac,rssici={{rssi=rssi,ci=ci}}})
            end
            log.debug("rssi,mcc,mnc,lac,ci", rssi,mcc,mnc,lac,ci)
        end
        for k,v in pairs(t) do
            ret = ret .. pack.pack(">HHb",v.lac,v.mcc,v.mnc)
            for m,n in pairs(v.rssici) do
                cntrssi = bit.bor(bit.lshift(((m == 1) and (#v.rssici-1) or 0),5),n.rssi)
                ret = ret .. pack.pack(">bi",cntrssi,n.ci)
            end
        end
        return string.char(#t)..ret
end

local function enWifiInfo(tWifi)
    local ret,cnt = "", 0
    if tWifi then
        for k,v in pairs(tWifi) do
            -- log.info("lbsLoc.enWifiInfo",k,v)
            ret = ret..pack.pack("Ab",(k:gsub(":","")):fromHex(),(v<0) and (v+255) or v)
            cnt = cnt+1
        end
    end
    return string.char(cnt)..ret
end

local function enMuid()   --Get Modules MUID
    local muid = mobile.muid()
    return string.char(muid:len())..muid
end

local function trans(str)
    local s = str
    if str:len()<10 then
        s = str..string.rep("0",10-str:len())
    end

    return s:sub(1,3).."."..s:sub(4,10)
end


local function taskClient(cbFnc, reqAddr, timeout, productKey, host, port,reqTime, reqWifi)
    if mobile.status() == 0 then
        if not sys.waitUntil("IP_READY", timeout) then return cbFnc(1) end
        sys.wait(500)
    end
    if productKey == nil then
        productKey = ""
    end
    local retryCnt  = 0
    local reqStr = pack.pack("bAbAAAAA", productKey:len(), productKey,
                             (reqAddr and 2 or 0) + (reqTime and 4 or 0) + 8 +(reqWifi and 16 or 0) + 32, "",
                             numToBcdNum(mobile.imei()), enMuid(),
                             enCellInfo(mobile.getCellInfo()),
                             enWifiInfo(reqWifi))
    log.debug("reqStr", reqStr:toHex())
    local rx_buff = zbuff.create(17)
    -- sys.wait(5000)
    while true do
        local result,succ,param
        local netc = socket.create(nil, d1Name) --Create socket object
        if not netc then cbFnc(6) return end --Failed to create socket
        socket.debug(netc, false)
        socket.config(netc, nil, true, nil)
        --result = libnet.waitLink(d1Name, 0, netc)
        result = libnet.connect(d1Name, 5000, netc, host, port)
        if result then
            while true do
                --log.info(" lbsloc socket_service connect true")
                result = libnet.tx(d1Name, 0, netc, reqStr) ---Send data
                if result then
                    result, param = libnet.wait(d1Name, 15000 + retryCnt * 5, netc)
                    if not result then
                        socket.close(netc)
                        socket.release(netc)
                        retryCnt = retryCnt+1
                        if retryCnt>=3 then return cbFnc(4) end
                        break
                    end
                    succ, param = socket.rx(netc, rx_buff) --receive data
                    --log.info("Whether to receive and data length", succ, param)
                    if param ~= 0 then --If the reception is successful
                        socket.close(netc) --close connection
                        socket.release(netc)
                        local read_buff = rx_buff:toStr(0, param)
                        rx_buff:clear()
                        log.debug("lbsLoc receive", read_buff:toHex())
                        if read_buff:len() >= 11 and(read_buff:byte(1) == 0 or read_buff:byte(1) == 0xFF) then
                            local locType = read_buff:byte(1)
                            cbFnc(0, trans(bcdNumToNum(read_buff:sub(2, 6))),
                                trans(bcdNumToNum(read_buff:sub(7, 11))), reqAddr and
                                read_buff:sub(13, 12 + read_buff:byte(12)) or nil,
                                reqTime and read_buff:sub(reqAddr and (13 + read_buff:byte(12)) or 12, -1) or "",
                                locType)
                        else
                            log.warn("lbsLoc.query", "根据基站查询经纬度失败")
                            if read_buff:byte(1) == 2 then
                                log.warn("lbsLoc.query","main.lua中的PRODUCT_KEY和此设备在iot.openluat.com中所属项目的ProductKey必须一致，请去检查")
                            else
                                log.warn("lbsLoc.query","基站数据库查询不到所有小区的位置信息")
                                --log.warn("lbsLoc.query","Search encellinfo upwards in the trace, then open http://bs.openluat.com/ in the computer browser, and manually find all cell locations after encellinfo")
                                --log.warn("lbsLoc.query","If the location can be found manually, there is a BUG in the server, please report the problem directly to the technical staff")
                                --log.warn("lbsLoc.query", "If the location cannot be found manually, the base station database has not yet included the cell location information of the current device. Feedback to the technical staff and we will include it as soon as possible")
                            end
                            cbFnc(5)
                        end
                        return
                    else
                        socket.close(netc)
                        socket.release(netc)
                        retryCnt = retryCnt+1
                        if retryCnt>=3 then return cbFnc(4) end
                        break
                    end
                else
                    socket.close(netc)
                    socket.release(netc)
                    retryCnt = retryCnt+1
                    if retryCnt>=3 then return cbFnc(3) end
                    break
                end
            end
        else
            socket.close(netc)
            socket.release(netc)
            retryCnt = retryCnt + 1
            if retryCnt >= 3 then return cbFnc(2) end
        end
    end
end


--[[
发送基站定位请求
@api lbsLoc.request(cbFnc,reqAddr,timeout,productKey,host,port,reqTime,reqWifi)
@function cbFnc 用户回调函数，回调函数的调用形式为：cbFnc(result,lat,lng,addr,time,locType)
@bool reqAddr 是否请求服务器返回具体的位置字符串信息，已经不支持,填false或者nil
@number timeout 请求超时时间，单位毫秒，默认20000毫秒
@string productKey IOT网站上的产品KEY，如果在main.lua中定义了PRODUCT_KEY变量，则此参数可以传nil
@string host 服务器域名, 默认 "bs.openluat.com" ,可选备用服务器(不保证可用) "bs.air32.cn"
@string port 服务器端口，默认"12411",一般不需要设置
@return nil
@usage
--Reminder: The coordinate value returned is the WGS84 coordinate system
]]
function lbsLoc.request(cbFnc,reqAddr,timeout,productKey,host,port,reqTime,reqWifi)
    sysplus.taskInitEx(taskClient, d1Name, netCB, cbFnc, reqAddr,timeout or 20000,productKey or _G.PRODUCT_KEY,host or "bs.openluat.com",port or "12411", reqTime == nil and true or reqTime,reqWifi)
end

return lbsLoc
