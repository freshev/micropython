
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "w5500_network"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")

log.style(1)

--w5500.init(spi.SPI_2, 24000000, pin.PB03, pin.PC00, pin.PC03)
w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
--w5500.init(spi.SPI_0, 24000000, pin.PB13, pin.PC09, pin.PC08)

w5500.config()	--The default is DHCP mode
w5500.bind(socket.ETH0)
--When testing server mode, it is recommended to use static IP and static DNS, but of course it is not mandatory.
--w5500.config("10.0.0.80","255.255.255.0","10.0.0.1")    
--w5500.bind(socket.ETH0)
--socket.setDNS(socket.ETH0, 1, "114.114.114.114")

--The following demonstrates using blocking mode to make serial port transparent transmission to a remote server. A simple serial port DTU, using serial port 3, IP in the LAN, the IP can be replaced with the domain name, and the port can be replaced with your own
--require "dtu_demo"
--dtuDemo(3, "10.0.0.3", 12000)

--The following demonstrates how to implement the NTP calibration time function using the callback method.
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

--require "ota_demo"
-- otaDemo()
--require "server_demo"
--SerDemo(14000) --14000 is the local port
--UDPSerDemo(14000) --UDP's server demo

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
