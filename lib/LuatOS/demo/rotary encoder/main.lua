
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "testrotary"
VERSION = "1.0.0"

--Can only be used on platforms that support ioqueue!!

--sys library is standard
_G.sys = require("sys")
require "rotary"
rotary_start()
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
