
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "camera_raw_mode"
VERSION = "1.1"
PRODUCT_KEY = "s1uUnY6KA06ifIjcutm5oNbG3MZf5aUv" --Replace it with your own
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
log.style(1)
w5500.init(spi.SPI_2, 24000000, pin.PB03, pin.PC00, pin.PE10)
w5500.config()
w5500.bind(socket.ETH0)
require "camera_raw"
--The raw camera data is collected and sent to the UDP server in the LAN. Because the W5500 is not fast enough, the effect is very poor.
camDemo("10.0.0.3", 12000)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
