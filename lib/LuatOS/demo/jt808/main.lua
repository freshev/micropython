--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "my_test"
VERSION = "1.2"
PRODUCT_KEY = " " --PRODUCT_KEY under your own iot platform
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")

--[[This demo implements the basic framework of the jt808 protocol, which can report location information and heartbeat packets through tcp. Subsequent functions can be added according to this framework;
    使用前需修改下tcp的ip地址和端口；
    如果是780eg模块，可以直接烧录，如果是780e外挂定位模块，需要注意串口号!]]--
----------------------------------------
--Error information is automatically reported to the platform, the default is iot.openluat.com
--Supports customization, please refer to the API manual for detailed configuration
--After being turned on, the boot reason will be reported. This requires data consumption, please pay attention.
if errDump then
    errDump.config(true, 600)
end
----------------------------------------


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


--If the operator's own DNS is not easy to use, you can use the following public DNS
-- socket.setDNS(nil,1,"223.5.5.5")
-- socket.setDNS(nil,2,"114.114.114.114")

-- socket.sntp()
--socket.sntp("ntp.aliyun.com") --Customize sntp server address
--socket.sntp({"ntp.aliyun.com","ntp1.aliyun.com","ntp2.aliyun.com"}) --sntp custom server address
--sys.subscribe("NTP_UPDATE", function()
--log.info("sntp", "time", os.date())
-- end)
--sys.subscribe("NTP_ERROR", function()
--log.info("socket", "sntp error")
--     socket.sntp()
-- end)

-----------------------------------------------------------------------------------------------------------------
sys.taskInit(function()
    --Check if the current firmware supports fskv
    if not fskv then
        while true do
            log.info("fskv", "this demo need fskv")
            sys.wait(1000)
        end
    end

    --Initialize kv database
    fskv.init()
    fskv.set("authCode", " ")  --Authentication code after successful registration
    fskv.set("heartFreq",60)  --Heartbeat reporting interval, in seconds
    fskv.set("tcpSndTimeout",10)  --TCP wait response timeout, in seconds
    fskv.set("tcpResendMaxCnt", 3)  --Number of TCP retransmissions
    fskv.set("locRptStrategy", 0)   --Location reporting strategy, 0: regular reporting; 1: regular interval reporting; 2: regular and regular interval reporting
    fskv.set("locRptMode",0)      --Location reporting scheme, 0: based on ACC status; 1: based on login status and ACC status, first determine login status, if logged in then based on ACC status
    fskv.set("sleepLocRptFreq", 60)   --The time interval for position reporting during sleep, in seconds
    fskv.set("alarmLocRptFreq",5)  --Location reporting time interval during emergency alarm, unit is seconds
    fskv.set("wakeLocRptFreq", 20)   --Default location reporting interval, in seconds
    fskv.set("sleepLocRptDistance", 500)    --Report the distance interval when sleeping, in meters
    fskv.set("alarmLocRptDistance", 5)     --Location reporting time interval during emergency alarm, unit is meters
    fskv.set("wakeLocRptDistance", 50)    --Default location reporting interval, unit is meters
    fskv.set("fenceRadis", 100)     --Electronic fence radius in meters
    fskv.set("alarmFilter",0)    --The alarm mask word corresponds to the alarm flag in the position report message. If the corresponding bit is 1, the corresponding alarm is masked.
    fskv.set("keyFlag", 0)   --The key flag corresponds to the alarm flag in the location information report message. If the corresponding bit is 1, the corresponding alarm is a key alarm.
    fskv.set("speedLimit", 100)  --Maximum speed in kilometers per hour (km/h)
    fskv.set("speedExceedTime", 60)   --Overspeed duration, unit is seconds (s)
end)

-------------------------------------------------------------------------------------------------------------

local gpsMng = require "gpsMng"
require "JT808Prot"
require "socket_demo"
-- dtuDemo("112.125.89.8",35960)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
