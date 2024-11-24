
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "strtest"
VERSION = "2.0.0"

--[[
本demo演示 string字符串的基本操作
1. lua的字符串是带长度, 这意味着, 它不依赖0x00作为结束字符串, 可以包含任意数据
2. lua的字符串是不可变的, 就不能直接修改字符串的一个字符, 修改字符会返回一个新的字符串
]]

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function ()
    sys.wait(1000) --So as not to see the logs
    local tmp

    ----------------------------------------------
    --================================================
    --String declaration and generation
    --================================================

    --constant declaration
    local str = "123455" 
    log.info("str", str)

    --Synthetic formula
    str = string.char(0x31, 0x32, 0x33, 0x34)
    log.info("str", str)
    --Lua's string can contain any data, including 0x00
    str = string.char(0x12, 0x00, 0xF1, 0x3A)
    log.info("str", str:toHex()) --Note that toHex() is used here because it contains invisible characters.

    --Use escape characters
    str = "\x00\x12ABC"
    log.info("str", str:toHex()) --Note that toHex() is used here because it contains invisible characters.
    str = "ABC\r\n\t"
    log.info("str", str:toHex()) --Note that toHex() is used here because it contains invisible characters.


    --Analyze and generate
    str = string.fromHex("AABB00EE")
    log.info("str", str:toHex())
    str = string.fromHex("393837363433")
    log.info("str", #str, str)

    --Concatenation string, operator ".."
    str = "123" .. "," .. "ABC"
    log.info("str", #str, str)

    --Formatted generation
    str = string.format("%s,%d,%f", "123", 45678, 1.5)
    log.info("str", #str, str)


    --================================================
    --String parsing and processing
    --================================================
    --Get the length
    str = "1234567"
    log.info("str", #str)
    --Get the HEX string display of a string
    log.info("str", str:toHex())

    --Get the value at the specified position. Note that Lua's subscripts start with 1.
    str = "123ddss"
    log.info("str[1]", str:byte(1))
    log.info("str[4]", str:byte(4))
    log.info("str[1]", string.byte(str, 1))
    log.info("str[4]", string.byte(str, 4))

    --split by string
    str = "12,2,3,4,5"
    tmp = str:split(",")
    log.info("str.split", #tmp, tmp[1], tmp[3])
    tmp = string.split(str, ",") --Equivalent to the previous
    log.info("str.split", #tmp, tmp[1], tmp[3])
    str = "/tmp//def/1234/"
    tmp = str:split("/")
    log.info("str.split", #tmp, json.encode(tmp))
    --Newly added on 2023.04.11, empty split segments can be retained
    tmp = str:split("/", true) 
    log.info("str.split", #tmp, json.encode(tmp))

    --More information
    -- https://wiki.luatos.com/develop/hex_string.html
    -- https://wiki.luatos.com/_static/lua53doc/manual.html#3.4
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
