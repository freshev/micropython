--PROJECT and VERSION variables must be defined at this location
--PROJECT: ascii string type, you can define it casually, as long as it is not used, it will be fine
--VERSION: ascii string type. If you use the Luat IoT Cloud Platform firmware upgrade function, it must be defined according to "X.X.X", where X represents a 1-digit number; otherwise, it can be defined casually
PROJECT = "101_UPDATE"
VERSION = "1.0.0"

--Load the log function Modules and set the log output level
--If you turn off the log output by calling the log Modules interface, set the level to log.LOG_SILENT.
require "log"
LOG_LEVEL = log.LOGLEVEL_TRACE
--[[
如果使用UART输出日志，打开这行注释的代码"--log.openTrace(true,1,115200)"即可，根据自己的需求修改此接口的参数
如果要彻底关闭脚本中的输出日志（包括调用log模块接口和Lua标准print接口输出的日志），执行log.openTrace(false,第二个参数跟调用openTrace接口打开日志的第二个参数相同)，例如：
1、没有调用过sys.opntrace配置日志输出端口或者最后一次是调用log.openTrace(true,nil,921600)配置日志输出端口，此时要关闭输出日志，直接调用log.openTrace(false)即可
2、最后一次是调用log.openTrace(true,1,115200)配置日志输出端口，此时要关闭输出日志，直接调用log.openTrace(false,1)即可
]]
--log.openTrace(true,1,115200)

require "sys"

require "net"
--Query GSM signal strength every 1 minute
--Query base station information every 1 minute
net.startQueryAll(60000, 60000)

--Turn off the RNDIS network card function here
--Otherwise, after the Modules is connected to the computer via USB, an RNDIS network card will be enumerated in the computer's network adapter. The computer will use this network card to access the Internet by default, resulting in a loss of traffic on the SIM card used by the Modules.
--If you need to turn on this function in your project, just change ril.request("AT+RNDISCALL=0,1") to ril.request("AT+RNDISCALL=1,1")
--Note: core firmware: V0030 and later versions, V3028 and later versions, can stably support this function.
ril.request("AT+RNDISCALL=0,1")

--Load the console debugging function Modules (the code here configures uart2, baud rate 115200)
--This function Modules is not necessary. Whether to load it depends on the project requirements.
--Note when using: The uart used by the console should not conflict with the uart used by other functions.
--For usage instructions, please refer to the "console function usage instructions.docx" under demo/console.
--require "console"
--console.setup(2, 115200)

--Load network indicator and LTE indicator function Moduless
--Decide based on your own project needs and hardware configuration: 1. Whether to load this functional Modules; 2. Configure the indicator pins
--The network indicator pin on the Air720U development board officially sold by Hezhou is pio.P0_1, and the LTE indicator pin is pio.P0_4
require "netLed"
pmd.ldoset(2,pmd.LDO_VLCD)
netLed.setup(true,pio.P0_1,pio.P0_4)
--In the network indicator function Modules, the blinking patterns of the indicator lights under various working conditions are configured by default. Please refer to the default value of ledBlinkTime configuration in netLed.lua.
--If the default value cannot meet the needs, call netLed.updateBlinkTime here to configure the blinking duration.
--In the LTE indicator function Modules, if it is configured to register with the 4G network, the light will be always on, and any other status lights will be off.

--Load the error log management function Modules [strongly recommended to turn on this function]
--The following 2 lines of code simply demonstrate how to use the errDump function. For details, please refer to the errDump API.
require "errDump"
errDump.request("udp://dev_msg1.openluat.com:12425", nil, true)

--Load the remote upgrade function Modules [It is strongly recommended to turn on this function. If you use Alibaba Cloud's OTA function, you do not need to turn on this function]
--The following 3 lines of code simply demonstrate how to use the update function. For details, please refer to the update api and demo/update.
--PRODUCT_KEY = "v32xEAKsGTIEQxtqgwCldp5aPlcnPs3K"
--require "update"
--update.request()

--Load the serial port function test Modules
require "uartSentFile"

--Start system framework
sys.init(0, 0)
sys.run()
