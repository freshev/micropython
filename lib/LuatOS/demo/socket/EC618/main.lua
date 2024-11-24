
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "my_test"
VERSION = "1.2"
PRODUCT_KEY = "s1uUnY6KA06ifIjcutm5oNbG3MZf5aUv" --Replace it with your own
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
log.style(1)

----------------------------------------
--Error information is automatically reported to the platform, the default is iot.openluat.com
--Supports customization, please refer to the API manual for detailed configuration
--After being turned on, the boot reason will be reported. This requires data consumption, please pay attention.
if errDump then
    errDump.config(true, 600)
end
----------------------------------------


--If the operator's own DNS is not easy to use, you can use the following public DNS
-- socket.setDNS(nil,1,"223.5.5.5")	
-- socket.setDNS(nil,2,"114.114.114.114")

socket.sntp()
--socket.sntp("ntp.aliyun.com") --Customize sntp server address
--socket.sntp({"ntp.aliyun.com","ntp1.aliyun.com","ntp2.aliyun.com"}) --sntp custom server address
sys.subscribe("NTP_UPDATE", function()
    log.info("sntp", "time", os.date())
end)
sys.subscribe("NTP_ERROR", function()
    log.info("socket", "sntp error")
    socket.sntp()
end)

--require "async_socket_demo"
require "socket_demo"
--require "server_demo"
dtuDemo(1, "112.125.89.8", 43488)	--Write the IP and PORT of your own server according to the actual situation, uart id
-- SerDemo(15000)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
