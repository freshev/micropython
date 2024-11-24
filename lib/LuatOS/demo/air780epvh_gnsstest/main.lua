--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "gnsstest"
VERSION = "1.0.1"
PRODUCT_KEY = "" --Needed for base station positioning

--[[
本demo需要很多流量!!!
注意: 室内无信号!! 无法定位!!!
]]

--sys library is standard
_G.sys = require("sys")
require("sysplus")

_G.gps_uart_id = 2

--Demonstrate GNSS positioning, including AGPS
require "testGnss"

--The corresponding web page for the demonstration reporting to the MQTT server is https://iot.openluat.com/iot/device-gnss
--require "testMqtt"

--Switch the GPIO high and low levels after the demonstration positioning is successful.
--require "testGpio"

--This TCP demonstration is connected to gps.nutz.cn port 19002, irtu's custom packet format
--The webpage is https://gps.nutz.cn/. Enter the IMEI number to refer to the current location.
--The WeChat applet is irtu. Click on the IMEI number and scan the QR code of the Modules to view the current location and historical track.
--Server source code address: https://gitee.com/wendal/irtu-gps
require "testTcp"

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
