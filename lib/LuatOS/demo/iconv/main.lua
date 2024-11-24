--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "my_test"
VERSION = "1.2"
PRODUCT_KEY = "s1uUnY6KA06ifIjcutm5oNbG3MZf5aUv" --Replace it with your own
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")

--- Convert unicode little-endian encoding to gb2312 encoding
--@string ucs2s unicode little endian encoded data
--@return string data, gb2312 encoded data
--@usage local data = common.ucs2ToGb2312(ucs2s)
function ucs2ToGb2312(ucs2s)
    local cd = iconv.open("gb2312", "ucs2")
    return cd:iconv(ucs2s)
end

--- gb2312 encoding converted to unicode little endian encoding
--@string gb2312s gb2312 encoded data
--@return string data,unicode little endian encoded data
--@usage local data = common.gb2312ToUcs2(gb2312s)
function gb2312ToUcs2(gb2312s)
    local cd = iconv.open("ucs2", "gb2312")
    return cd:iconv(gb2312s)
end

--- Convert unicode big-endian encoding to gb2312 encoding
--@string ucs2s unicode big endian encoded data
--@return string data, gb2312 encoded data
--@usage data = common.ucs2beToGb2312(ucs2s)
function ucs2beToGb2312(ucs2s)
    local cd = iconv.open("gb2312", "ucs2be")
    return cd:iconv(ucs2s)
end

--- Convert gb2312 encoding to unicode big-endian encoding
--@string gb2312s gb2312 encoded data
--@return string data,unicode big endian encoded data
--@usage local data = common.gb2312ToUcs2be(gb2312s)
function gb2312ToUcs2be(gb2312s)
    local cd = iconv.open("ucs2be", "gb2312")
    return cd:iconv(gb2312s)
end

--- Convert unicode little-endian encoding to utf8 encoding
--@string ucs2s unicode little endian encoded data
--@return string data,utf8 encoded data
--@usage data = common.ucs2ToUtf8(ucs2s)
function ucs2ToUtf8(ucs2s)
    local cd = iconv.open("utf8", "ucs2")
    return cd:iconv(ucs2s)
end

--- Convert utf8 encoding to unicode little endian encoding
--@string utf8s utf8 encoded data
--@return string data,unicode little endian encoded data
--@usage local data = common.utf8ToUcs2(utf8s)
function utf8ToUcs2(utf8s)
    local cd = iconv.open("ucs2", "utf8")
    return cd:iconv(utf8s)
end

--- Convert unicode big-endian encoding to utf8 encoding
--@string ucs2s unicode big endian encoded data
--@return string data,utf8 encoded data
--@usage data = common.ucs2beToUtf8(ucs2s)
function ucs2beToUtf8(ucs2s)
    local cd = iconv.open("utf8", "ucs2be")
    return cd:iconv(ucs2s)
end

--- Convert utf8 encoding to unicode big-endian encoding
--@string utf8s utf8 encoded data
--@return string data,unicode big endian encoded data
--@usage local data = common.utf8ToUcs2be(utf8s)
function utf8ToUcs2be(utf8s)
    local cd = iconv.open("ucs2be", "utf8")
    return cd:iconv(utf8s)
end

--- Convert utf8 encoding to gb2312 encoding
--@string utf8s utf8 encoded data
--@return string data, gb2312 encoded data
--@usage local data = common.utf8ToGb2312(utf8s)
function utf8ToGb2312(utf8s)
    local cd = iconv.open("ucs2", "utf8")
    local ucs2s = cd:iconv(utf8s)
    cd = iconv.open("gb2312", "ucs2")
    return cd:iconv(ucs2s)
end

--- Convert gb2312 encoding to utf8 encoding
--@string gb2312s gb2312 encoded data
--@return string data,utf8 encoded data
--@usage local data = common.gb2312ToUtf8(gb2312s)
function gb2312ToUtf8(gb2312s)
    local cd = iconv.open("ucs2", "gb2312")
    local ucs2s = cd:iconv(gb2312s)
    cd = iconv.open("utf8", "ucs2")
    return cd:iconv(ucs2s)
end

--------------------------------------------------------------------------------------------------------
--[[
函数名：ucs2ToGb2312
功能  ：unicode小端编码 转化为 gb2312编码,并打印出gd2312编码数据
参数  ：
        ucs2s：unicode小端编码数据,注意输入参数的字节数
返回值：
]]
local function testucs2ToGb2312(ucs2s)
    print("ucs2ToGb2312")
    local gd2312num = ucs2ToGb2312(ucs2s)--The call is common.ucs2ToGb2312, and the string corresponding to the encoding is returned.
    --print("gb2312  code：",gd2312num)
    print("gb2312  code：",string.toHex(gd2312num))
end

--[[
函数名：gb2312ToUcs2
功能  ：gb2312编码 转化为 unicode十六进制小端编码数据并打印
参数  ：
        gb2312s：gb2312编码数据，注意输入参数的字节数
返回值：
]]
local function testgb2312ToUcs2(gd2312num)
    print("gb2312ToUcs2")
    local ucs2num = gb2312ToUcs2(gd2312num)
    print("unicode little-endian code:" .. string.toHex(ucs2num)) --To convert binary to hexadecimal, otherwise it cannot be output
end

--[[
函数名：ucs2beToGb2312
功能  ：unicode大端编码 转化为 gb2312编码，并打印出gb2312编码数据,
大端编码数据是与小端编码数据位置调换
参数  ：
        ucs2s：unicode大端编码数据，注意输入参数的字节数
返回值：
]]
local function testucs2beToGb2312(ucs2s)
    print("ucs2beToGb2312")
    local gd2312num = ucs2beToGb2312(ucs2s) --The converted data can be directly converted into characters and can be output directly.
    print("gd2312 code :" .. string.toHex(gd2312num))
end

--[[
函数名：gb2312ToUcs2be
功能  ：gb2312编码 转化为 unicode大端编码，并打印出unicode大端编码
参数  ：
        gb2312s：gb2312编码数据，注意输入参数的字节数
返回值：unicode大端编码数据
]]
function testgb2312ToUcs2be(gb2312s)
    print("gb2312ToUcs2be")
    local ucs2benum = gb2312ToUcs2be(gb2312s)
    print("unicode big-endian code :" .. string.toHex(ucs2benum))
end

--[[
函数名：ucs2ToUtf8
功能  ：unicode小端编码 转化为 utf8编码,并打印出utf8十六进制编码数据
参数  ：
        ucs2s：unicode小端编码数据，注意输入参数的字节数
返回值：
]]
local function testucs2ToUtf8(usc2)
    print("ucs2ToUtf8")
    local utf8num = ucs2ToUtf8(usc2)
    print("utf8  code:" .. string.toHex(utf8num))

end

--[[
函数名：utf8ToGb2312
功能  ：utf8编码 转化为 gb2312编码,并打印出gb2312编码数据
参数  ：
        utf8s：utf8编码数据，注意输入参数的字节数
返回值：
]]
local function testutf8ToGb2312(utf8s)
    print("utf8ToGb2312")
    local gb2312num = utf8ToGb2312(utf8s)
    print("gd2312 code:" .. string.toHex(gb2312num))

end

--[[
函数名：utf8ToGb2312
功能  ：utf8编码 转化为 gb2312编码,并打印出gb2312编码数据
参数  ：
        utf8s：utf8编码数据，注意输入参数的字节数
返回值：
]]
local function testgb2312ToUtf8(gb2312s)
    print("gb2312ToUtf8")
    local utf8s = gb2312ToUtf8(gb2312s)
    print("utf8s code:" .. utf8s)

end



sys.taskInit(function()
    while 1 do
        sys.wait(1000)
        testucs2ToGb2312(string.fromHex("1162")) --"1162" is the ucs2 encoding of the word "I". String.fromHex is called here to convert the parameter into binary, which is two bytes.
        testgb2312ToUcs2(string.fromHex("CED2")) --"CED2" is the gb22312 encoding of the word "I"
        testucs2beToGb2312(string.fromHex("6211")) --"6211" is the ucs2be encoding of the word "I"
        testgb2312ToUcs2be(string.fromHex("CED2"))
        testucs2ToUtf8(string.fromHex("1162"))
        testutf8ToGb2312(string.fromHex("E68891")) --"E68891" is the utf8 encoding of the word "I"
        testgb2312ToUtf8(string.fromHex("CED2"))
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
