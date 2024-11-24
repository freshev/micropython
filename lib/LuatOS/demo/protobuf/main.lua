--- Module function: Google ProtoBuffs codec
--@Modules pb
--@author wendal
--@release 2022.9.8

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "pbdemo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    sys.wait(500)
    --If you don’t have this library, just compile it in the cloud: https://wiki.luatos.com/develop/compile/Cloud_compilation.html
    if not protobuf then
        log.info("protobuf", "this demo need protobuf lib")
        return
    end
    --Load the pb file, which is converted from pbtxt
    --There is no need to download pbtxt when downloading resources to Moduless
    --Conversion command: protoc.exe -operson.pb person.pbtxt
    --protoc.exe download address: https://github.com/protocolbuffers/protobuf/releases
    protobuf.load(io.readFile("/luadb/person.pb"))
    local tb = {
        name = "wendal",
        id = 123,
        email = "abc@qq.com"
    }
    while 1 do
        sys.wait(1000)
        --Encoding data with protobuf
        local pbdata = protobuf.encode("Person", tb)
        if pbdata then
            --Print the data length. The encoded data contains invisible characters, toHex is convenient for display
            log.info("protobuf", "encode",  #pbdata, (pbdata:toHex()))
        end
        --Use json to encode data for size comparison
        local jdata = json.encode(tb)
        if jdata then
            log.info("json", #jdata, jdata)
        end
        --It can be seen that protobuffs saves a lot of space than json

        --The follow-up is demonstration decoding
        local re = protobuf.decode("Person", pbdata)
        if re then
            --Print data, because the table cannot be displayed directly, it is converted to json for display.
            log.info("protobuf", "decode", json.encode(re))
        end
    end
end)

--Main loop, must be added
sys.run()
