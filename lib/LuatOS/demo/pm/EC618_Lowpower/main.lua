--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "socket_low_power"
VERSION = "1.0"
PRODUCT_KEY = "123" --Replace it with your own
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
log.style(1)


require "functionTest"
--require "psm_plus"
--require "remote_net_wakeup"

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
