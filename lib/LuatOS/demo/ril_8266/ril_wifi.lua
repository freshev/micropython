--- Module function: virtual serial port AT command interactive management
--@Modules ril
--@author openLuat
--@license MIT
--@copyright openLuat
--@release 2017.02.13

local ril_wifi = {}

local UART_ID = 1

--Load commonly used global functions to local
local vwrite = uart.write
local vread = uart.read

--Whether it is transparent transmission mode, true means transparent transmission mode, false or nil means non-transparent transmission mode
--Default non-transparent transmission mode
local transparentmode
--In transparent transmission mode, the processing function of virtual serial port data reception
local rcvfunc

local uartswitch
--Flag after execution of cipsend
local cipsendflag
--If there is no feedback for 1 minute after executing the AT command, it is determined that the execution of the AT command failed, and the software will be restarted.
local TIMEOUT,DATA_TIMEOUT = 120000,120000

--AT command response type
--NORESULT: The received response data is treated as a urc notification. If the AT command sent does not process the response or does not set the type, the default is this type.
--NUMBERIC: pure numeric type; for example, when sending the AT+CGSN command, the response content is: 862991527986589\r\nOK. This type refers to the 862991527986589 part as a pure numeric type.
--SLINE: A single-line string type with a prefix; for example, when sending an AT+CSQ command, the response content is: +CSQ: 23,99\r\nOK. This type refers to the +CSQ: 23,99 part as a single-line string. type
--MLINE: multi-line string type with prefix; for example, if you send the AT+CMGR=5 command, the response content is: +CMGR: 0,,84\r\n0891683108200105F76409A001560889F800087120315123842342050003590404590D003A59\r\nOK , this type refers to OK before the multi row string type
--STRING: String type without prefix, for example, when sending AT+ATWMFT=99 command, the response content is: SUCC\r\nOK, this type refers to SUCC
--SPECIAL: special type that requires special processing for AT commands, such as CIPSEND, CIPCLOSE, CIFSR
local NORESULT, NUMBERIC, SLINE, MLINE, STRING, SPECIAL = 0, 1, 2, 3, 4, 10

--The response type table of AT commands has the following items preset:
local RILCMD = {
    ["+CSQ"] = 2,
    ["+MUID"] = 2,
    ["+CGSN"] = 1,
    ["+WISN"] = 4,
    ["+CIMI"] = 1,
    ["+CCID"] = 1,
    ["+CGATT"] = 2,
    ["+CCLK"] = 2,
    ["+ATWMFT"] = 4,
    ["+CMGR"] = 3,
    ["+CMGS"] = 2,
    ["+CPBF"] = 3,
    ["+CPBR"] = 3,
    ['+CLCC'] = 3,
    ['+CNUM'] = 3,
    ["+CIPSEND"] = 10,
    ["+CIPCLOSE"] = 10,
    ["+SSLINIT"] = 10,
    ["+SSLCERT"] = 10,
    ["+SSLCREATE"] = 10,
    ["+SSLCONNECT"] = 10,
    ["+SSLSEND"] = 10,
    ["+SSLDESTROY"] = 10,
    ["+SSLTERM"] = 10,
    ["+CIFSR"] = 10,
    ["+CTFSGETID"] = 2,
    ["+CTFSDECRYPT"] = 2,
    ["+CTFSAUTH"] = 2,
    ["+ALIPAYOPEN"] = 2,
    ["+ALIPAYREP"] = 2,
    ["+ALIPAYPINFO"] = 2,
    ["+ALIPAYACT"] = 2,
    ["+ALIPAYDID"] = 2,
    ["+ALIPAYSIGN"] = 2
}

--radioready: whether the AT command channel is ready
--delaying: Before certain AT commands are executed, a period of delay is required before these AT commands are allowed to be executed; this flag indicates whether it is in the delayed state.
local radioready, delaying = false

--AT command queue
local cmdqueue = {
    "ATE0",
    -- "AT+SYSLOG=0",
    'AT+CIPSNTPCFG=1,8,"cn.ntp.org.cn","ntp.sjtu.edu.cn"',
}
--AT command currently being executed, parameters, feedback callback, delayed execution time, command header, type, feedback format
local currcmd, currarg, currsp, curdelay, cmdhead, cmdtype, rspformt, cmdRspParam
--Feedback results, intermediate information, result information
local result, interdata, respdata

local sslCreating

--There will be three situations in ril:
--Send AT command and receive response
--Send AT command, command timeout and no response
--The notification proactively reported by the underlying software, we will refer to it as urc below.
--[[
函数名：atimeout
功能  ：发送AT命令，命令超时没有应答的处理
参数  ：无
返回值：无
]]
local function atimeout()
    --Restart software
    sys.restart("ril.atimeout_" .. (currcmd or ""))
end

--[[
函数名：defrsp
功能  ：AT命令的默认应答处理。如果没有定义某个AT的应答处理函数，则会走到本函数
参数  ：
cmd：此应答对应的AT命令
success：AT命令执行结果，true或者false
response：AT命令的应答中的执行结果字符串
intermediate：AT命令的应答中的中间信息
返回值：无
]]
local function defrsp(cmd, success, response, intermediate)
    log.info("ril.defrsp", cmd, success, response, intermediate)
end

--AT command response processing table
local rsptable = {}
setmetatable(rsptable, {
    __index = function()
        return defrsp
    end
})

--Customized AT command response format table. When the AT command response is in STRING format, the user can further define the format in it.
local formtab = {}

---Register a processing function for a certain AT command response
--@param head The AT command header corresponding to this response has the first two AT characters removed.
--@param fnc AT command response processing function
--@param typ AT command response type, value range NORESULT, NUMBERIC, SLINE, MLINE, STRING, SPECIAL
--@param formt typ is STRING, further define the detailed format in STRING
--@return bool, returns true if successful, false if failed
--@usage ril.regRsp("+CSQ", rsp)
function ril_wifi.regRsp(head, fnc, typ, formt)
    --No response type defined
    if typ == nil then
        rsptable[head] = fnc
        return true
    end
    --Legal response types defined
    if typ == 0 or typ == 1 or typ == 2 or typ == 3 or typ == 4 or typ == 10 then
        --If the response type of the AT command already exists and is inconsistent with the newly set one
        if RILCMD[head] and RILCMD[head] ~= typ then
            return false
        end
        --keep
        RILCMD[head] = typ
        rsptable[head] = fnc
        formtab[head] = formt
        return true
    else
        return false
    end
end

--[[
函数名：rsp
功能  ：AT命令的应答处理
参数  ：无
返回值：无
]]
local function rsp()
    --Stop reply timeout timer
    sys.timerStopAll(atimeout)
    
    --If the response processing function has been specified synchronously when sending the AT command
    if currsp then
        currsp(currcmd, result, respdata, interdata)
    --The handler function is found in the user-registered response handler function table.
    else
        rsptable[cmdhead](currcmd, result, respdata, interdata, cmdRspParam)
    end
    --reset global variables
    --log.info("Was it cleared in rsp", currcmd, currarg, currsp, curdelay, cmdhead)
    if cmdhead == "+CIPSEND" and cipsendflag then
        return
    end
    currcmd, currarg, currsp, curdelay, cmdhead, cmdtype, rspformt = nil
    result, interdata, respdata = nil
end

--[[
函数名：defurc
功能  ：urc的默认处理。如果没有定义某个urc的应答处理函数，则会走到本函数
参数  ：
data：urc内容
返回值：无
]]
local function defurc(data)
    log.info("ril.defurc", data)
end

--urc processing table
local urctable = {}
setmetatable(urctable, {
    __index = function()
        return defurc
    end
})

--- Register a processing function for a certain urc
--@param prefix urc prefix, the first continuous string, including a combination of +, uppercase characters, and numbers
--@param handler urc processing function
--@return None
--@usage ril.regUrc("+CREG", neturc)
function ril_wifi.regUrc(prefix, handler)
    urctable[prefix] = handler
end

--- Unregister the processing function of a certain urc
--@param prefix urc prefix, the first continuous string, including a combination of +, uppercase characters, and numbers
--@return None
--@usage deRegUrc("+CREG")
function ril_wifi.deRegUrc(prefix)
    urctable[prefix] = nil
end

--"Data filter", when the virtual serial port receives data, you first need to call this function to filter it.
local urcfilter

--[[
函数名：urc
功能  ：urc处理
参数  ：
data：urc数据
返回值：无
]]
local function urc(data)
    
    --AT channel is ready

    point(#data, data)
    if  string.find(data, "ready") then
        radioready = true
        
    else
        local prefix = string.match(data, "([%+%*]*[%a%d& ]+)")
        --Execute the urc processing function of prefix and return the data filter
        urcfilter = urctable[prefix](data, prefix)
    end
end

--[[
函数名：procatc
功能  ：处理虚拟串口收到的数据
参数  ：
data：收到的数据
返回值：无
]]
local function procatc(data)
    log.info("ril.proatc", data)
    --If the command's response is in multi-line string format
    if interdata and cmdtype == MLINE then
        --If OK\r\n does not appear, it is considered that the response has not ended yet.
        if data ~= "OK\r\n" then
            --Remove the last \r\n
            if string.find(data, "\r\n", -2) then
                data = string.sub(data, 1, -3)
            end
            --Spliced   to intermediate data
            interdata = interdata .. "\r\n" .. data
            return
        end
    end
    --If there is a "data filter"
    if urcfilter then
        data, urcfilter = urcfilter(data)
    end
    --Remove the last \r\n
    if not data:find("+CIPRECVDATA") then
        if string.find(data, "\r\n", -2) then
            point(#data)
            data = string.sub(data, 1, -3)
        end
    else
        local len, tmp = data:match("%+CIPRECVDATA:(%d+),(.+)")
        if len + 2 == #tmp and string.find(data, "\r\n", -2) then
            data = string.sub(data, 1, -3)
        end
    end
    --Data is empty
    if data == "" then
        return
    end
    --If there is no command currently being executed, it is determined to be urc.
    if currcmd == nil then
        --log.info("It is judged as distribution network data", data, isurc, result)
        urc(data)
        return
    end
    
    local isurc = false
    
    --Some special error messages are converted into ERROR and processed uniformly.
    if string.find(data, "^%+CMS ERROR:") or string.find(data, "^%+CME ERROR:") or (data == "CONNECT FAIL" and currcmd and string.match(currcmd, "CIPSTART")) then
        data = "ERROR"
    end
    if sslCreating and data=="+PDP: DEACT" and tonumber(string.match(rtos.version(),"Luat_V(%d+)_"))<31 then
        sys.publish("SSL_DNS_PARSE_PDP_DEACT")
    end

    if cmdhead == "+CIPSEND" and data == "SEND OK" then
        result = true
        respdata = data
        cipsendflag = false
    elseif cmdhead == "+CIPSEND" and data == "OK" then
        result = true
        respdata = data
        cipsendflag = true
    --successful execution response
    elseif data == "OK" or data == "SHUT OK" then
        result = true
        respdata = data
    --Response to execution failure
    elseif data == "ERROR" or data == "NO ANSWER" or data == "NO DIALTONE" then
        result = false
        respdata = data
    --AT command response that requires continued input of parameters
    elseif data == ">" then
        --sending a text message
        if cmdhead == "+CMGS" then
            log.info("ril.procatc.send", currarg)
            vwrite(UART_ID, currarg, "\026")
        --Send data
        elseif cmdhead == "+CIPSEND" or cmdhead == "+SSLSEND" or cmdhead == "+SSLCERT" then
            log.info("ril.procatc.send", "first 200 bytes", currarg:sub(1,200))
            vwrite(UART_ID, currarg)
        elseif cmdhead == "+SYSFLASH" then
            log.info("ril.sysflash.send", "first 200 bytes", currarg:sub(1,200))
            vwrite(UART_ID, currarg)
        else
            log.error("error promot cmd:", currcmd)
        end
    else
        --No type
        if cmdtype == NORESULT then
            isurc = true
        --All numeric type
        elseif cmdtype == NUMBERIC then
            local numstr = string.match(data, "(%x+)")
            if numstr == data then
                interdata = data
            else
                isurc = true
            end
        --string type
        elseif cmdtype == STRING then
            --Check the format further
            if string.match(data, rspformt or "^.+$") then
                interdata = data
            else
                isurc = true
            end
        elseif cmdtype == SLINE or cmdtype == MLINE then
            if interdata == nil and string.find(data, cmdhead) == 1 then
                interdata = data
            else
                isurc = true
            end
        --special handling
        elseif cmdhead == "+CIFSR" then
            local s = string.match(data, "%d+%.%d+%.%d+%.%d+")
            if s ~= nil then
                interdata = s
                result = true
            else
                isurc = true
            end
        --special handling
        elseif cmdhead == "+CIPSEND" or cmdhead == "+CIPCLOSE" then
            local keystr = cmdhead == "+CIPSEND" and "SEND" or "CLOSE"
            local lid, res = string.match(data, "(%d), *([%u%d :]+)")
            
            if data:match("^%d, *CLOSED$") then
                isurc = true
            elseif lid and res then
                if (string.find(res, keystr) == 1 or string.find(res, "TCP ERROR") == 1 or string.find(res, "UDP ERROR") == 1 or string.find(data, "DATA ACCEPT")) and (lid == string.match(currcmd, "=(%d)")) then
                    result = data:match("ERROR") == nil
                    respdata = data
                else
                    isurc = true
                end
            elseif data == "+PDP: DEACT" then
                result = true
                respdata = data
            elseif data:match("^Recv %d+ bytes$") then
                result = true
                respdata = data
            else
                isurc = true
            end
        elseif cmdhead == "+SSLINIT" or cmdhead == "+SSLCERT" or cmdhead == "+SSLCREATE" or cmdhead == "+SSLCONNECT" or cmdhead == "+SSLSEND" or cmdhead == "+SSLDESTROY" or cmdhead == "+SSLTERM" then
            if string.match(data, "^SSL&%d, *CLOSED") or string.match(data, "^SSL&%d, *ERROR") or string.match(data, "SSL&%d,CONNECT ERROR") then
                isurc = true
            elseif string.match(data, "^SSL&%d,") then
                respdata = data
                if string.match(data, "ERROR") then
                    result = false
                else
                    result = true
                end
                if cmdhead == "+SSLCREATE" then
                    sslCreating = false
                end
            else
                isurc = true
            end
        else
            isurc = true
        end
    end
    --urc processing
    if isurc then
        urc(data)
    --response processing
    elseif result ~= nil then
        rsp()
    end
end

--Are you reading virtual serial port data?
local readat = false

--[[
函数名：getcmd
功能  ：解析一条AT命令
参数  ：
item：AT命令
返回值：当前AT命令的内容
]]
local function getcmd(item)
    local cmd, arg, rsp, delay, rspParam
    --The command is of type string
    if type(item) == "string" then
        --Command content
        cmd = item
    --The command is of table type
    elseif type(item) == "table" then
        --Command content
        cmd = item.cmd
        --Command parameters
        arg = item.arg
        --Command response processing function
        rsp = item.rsp
        --Command delay execution time
        delay = item.delay
        --Parameters carried by the command. This parameter is passed in when executing the callback.
        rspParam = item.rspParam
    else
        log.info("ril.getcmd", "getpack unknown item")
        return
    end
    --command prefix
    local head = string.match(cmd, "AT([%+%*]*%u+)")
    
    if head == nil then
        log.error("ril.getcmd", "request error cmd:", cmd)
        return
    end
    --These two commands must have parameters
    if head == "+CMGS" or head == "+CIPSEND" then --There must be parameters
        if arg == nil or arg == "" then
            log.error("ril.getcmd", "request error no arg", head)
            return
        end
    end
    --log.info("Have you reached the assignment of global variables?", cmd, arg, rsp, delay, head)
    --Assign global variables
    currcmd = cmd
    currarg = arg
    currsp = rsp
    curdelay = delay
    cmdhead = head
    cmdRspParam = rspParam
    cmdtype = RILCMD[head] or NORESULT
    rspformt = formtab[head]
    
    return currcmd
end

--[[
函数名：sendat
功能  ：发送AT命令
参数  ：无
返回值：无
]]
local function sendat()
    --The AT channel is not ready, the virtual serial port data is being read, there are AT commands being executed or there are no commands in the queue, and a certain AT message is being sent in a delayed manner.
    if not radioready or readat or currcmd ~= nil or delaying then
        return
    end
    --log.info("Ready")
    local item
    
    while true do
        --There is no AT command in the queue
        if #cmdqueue == 0 then
            return
        end
        --Read the first command
        item = table.remove(cmdqueue, 1)
        --parse command
        getcmd(item)
        --Need to delay sending
        if curdelay then
            --Start delay sending timer
            sys.timerStart(delayfunc, curdelay)
            --Clear global variables
            currcmd, currarg, currsp, curdelay, cmdhead, cmdtype, rspformt, cmdRspParam = nil
            item.delay = nil
            --Set delayed delivery flag
            delaying = true
            --Reinsert the command at the head of the command queue
            table.insert(cmdqueue, 1, item)
            return
        end
        
        if currcmd ~= nil then
            break
        end
    end
    --Start AT command response timeout timer
    if currcmd:match("^AT%+CIPSTART") or currcmd:match("^AT%+CIPSEND") or currcmd:match("^AT%+SSLCREATE") or currcmd:match("^AT%+SSLCONNECT") or currcmd:match("^AT%+SSLSEND") then
        sys.timerStart(atimeout,DATA_TIMEOUT)
    else
        sys.timerStart(atimeout, TIMEOUT)
    end
    
    if currcmd:match("^AT%+SSLCREATE") then
        sslCreating = true
    end
    
    log.info("ril.sendat", currcmd)
    --Send AT commands to the virtual serial port
    vwrite(UART_ID, currcmd .. "\r\n")
end

--Timer callback to delay execution of an AT command
--@return None
--@usage ril.delayfunc()
function ril_wifi.delayfunc()
    --clear delay flag
    delaying = nil
    --Execute AT command to send
    sendat()
end

--[[
函数名：atcreader
功能  ：“AT命令的虚拟串口数据接收消息”的处理函数，当虚拟串口收到数据时，会走到此函数中
参数  ：无
返回值：无
]]
local function atcreader(id,len)
    local alls = vread(UART_ID, len):split("\r\n")

    if not transparentmode then
        readat = true
    end
    --Loop to read the data received by the virtual serial port
    for i=1,#alls do
        local s = alls[i]
        --Read one line at a time
        --s = vread(UART_ID, len)
        log.debug("uart.log",s)
        if string.len(s) ~= 0 then
            if transparentmode then
                --Directly forward data in transparent transmission mode
                rcvfunc(s)
            else
                --Processing received data in non-transparent transmission mode
                procatc(s)
            end
        else
            break
        end
    end
    if not transparentmode then
        readat = false
        --After the data is processed, continue to execute the AT command to send
        sendat()
    end
end

--- Send AT commands to underlying software
--@param cmd AT command content
--@param arg AT command parameters, for example, after the AT+CMGS=12 command is executed, this parameter will be sent next; after the AT+CIPSEND=14 command is executed, this parameter will be sent next
--@param onrsp The processing function of AT command response. Only the currently sent AT command response is valid, and it will become invalid after processing.
--@param delay Delay milliseconds before sending this AT command
--@return None
--@usage ril.request("AT+CENG=1,1")
--@usage ril.request("AT+CRSM=214,28539,0,0,12,\"64f01064f03064f002fffff\"", nil, crsmResponse)
function ril_wifi.request(cmd, arg, onrsp, delay, param)
    if transparentmode then
        return
    end
    --Insert into buffer queue
    if arg or onrsp or delay or formt or param then
        table.insert(cmdqueue, {
		cmd = cmd, 
		arg = arg, 
		rsp = onrsp, 
		delay = delay, 
		rspParam = param
		})
    else
        table.insert(cmdqueue, cmd)
    end
    --Execute AT command to send
    point(cmd)
    sendat()
end

--[[
函数名：setransparentmode
功能  ：AT命令通道设置为透传模式
参数  ：
fnc：透传模式下，虚拟串口数据接收的处理函数
返回值：无
注意：透传模式和非透传模式，只支持开机的第一次设置，不支持中途切换
]]
function ril_wifi.setransparentmode(fnc)
    transparentmode, rcvfunc = true, fnc
end

--[[
函数名：sendtransparentdata
功能  ：透传模式下发送数据
参数  ：
data：数据
返回值：成功返回true，失败返回nil
]]
function ril_wifi.sendtransparentdata(data)
    if not transparentmode then
        return
    end
    vwrite(UART_ID, data)
    return true
end

function ril_wifi.setDataTimeout(tm)
    DATA_TIMEOUT = (tm<120000 and 120000 or tm)
end

uart.setup(UART_ID, 115200)
--Register the processing function of "AT command virtual serial port data reception message"
uart.on(UART_ID, "receive", atcreader)

uart.write(UART_ID,"AT+RST\r\n")

return ril_wifi
