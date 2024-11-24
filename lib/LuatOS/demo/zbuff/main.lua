--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "zbuffdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

sys.taskInit(function()
    sys.wait(3000)
    --zbuff can be understood as char[], char*, uint8_t*
    --In order to better integrate with Lua, zbuff has length and pointer position, and can be dynamically expanded.
    local buff = zbuff.create(1024)
    --Can be used as an array to directly assign and obtain values
    buff[0] = 0xAE
    log.info("zbuff", "buff[0]", buff[0])

    --Operate in io form
    
    --Write data write, the pointer will move after the operation, just like the file handle
    buff:write("123") --string
    buff:write(0x12, 0x13, 0x13, 0x33) --You can also write the numerical value directly
    
    --Set the pointer position, seek
    buff:seek(5, zbuff.SEEK_CUR) --Pointer position +5
    buff:seek(0)                 --absolute address

    --When reading data, the pointer will also move.
    local data = buff:read(3)
    log.info("zbuff", "data", data)

    --Clear all data, but the pointer position remains unchanged
    buff:clear() --Fill in 0 by default
    buff:clear(0xA5) --You can also specify the content of the fill

    --Support writing or reading data in the form of pack library
    buff:seek(0)
    buff:pack(">IIHA", 0x1234, 0x4567, 0x12,"abcdefg")
    buff:seek(0)
    local cnt,a,b,c,s = buff:unpack(">IIHA10")

    --You can also read and write data directly by type
    local len = buff:writeI8(10)
    local len = buff:writeU32(1024)
    local i8data = buff:readI8()
    local u32data = buff:readU32()

    --Get the data in the specified interval
    local fz = buff:toStr(0,5)

    --Get its length
    log.info("zbuff", "len", buff:len())
    --Get its pointer position
    log.info("zbuff", "len", buff:used())

    --Test writeF32. Note that for EC618 series (Air780E, etc.), V1106 will crash. It will be fixed in V1107.
    buff:seek(0, zbuff.SEEK_SET)
    buff:writeF32(1.2)
    buff:seek(0, zbuff.SEEK_SET)
    log.info("buff", "rw", "f32", buff:readF32())

    --For more usage, please refer to the api documentation

    log.info("zbuff", "demo done")
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
