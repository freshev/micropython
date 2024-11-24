
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "spi_no_blcok_test"
VERSION = "1.0"
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
log.style(1)
require "no_block_test"
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
