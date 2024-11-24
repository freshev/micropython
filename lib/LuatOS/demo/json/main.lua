--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "jsondemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

log.info("main", PROJECT, VERSION)

--The json library supports converting table to string, or vice versa, string to table
--If the conversion fails, a nil value will be returned. It is strongly recommended to add additional judgment when using it.
sys.taskInit(function()
    while 1 do
        sys.wait(1000)
        --Convert table to string
        local t = {abc=123, def="123", ttt=true}
        local jdata = json.encode(t)
        log.info("json", jdata)

        --Convert string to table
        local str = "{\"abc\":1234545}" --Strings can come from anywhere, the network, text, user input, etc.
        local t = json.decode(str)
        if t then --If decoding fails, nil will be returned.
            log.info("json", "decode", t.abc)
        end

        --Table in Lua is a mixture of array and hashmap
        --This will cause some trouble for json, especially the empty table
        local t = {abc={}}
        --Assume that the business needs to output {"abc":[]}
        --It will actually output {"abc": {}}. An empty table will give priority to the hashmap format rather than the array format.
        log.info("json", "encode", json.encode(t))
        --Mixed scenarios, json scenarios should be avoided
        t.abc.def = "123"
        t.abc[1] = 345
        --The output content is {"abc":{"1":345,"def":"123"}}
        log.info("json", "encode2", json.encode(t))

        --Floating point demonstration
        --Default%.7g
        log.info("json", json.encode({abc=1234.300}))
        --Limit decimal point to 1 place
        log.info("json", json.encode({abc=1234.300}, "1f"))

        --2023.6.8 Deal with the problem that \n becomes \b after encoding
        local tmp = "ABC\r\nDEF\r\n"
        local tmp2 = json.encode({str=tmp})
        log.info("json", tmp2)
        local tmp3 = json.decode(tmp2)
        log.info("json", "tmp3", tmp3.str, tmp3.str == tmp)
        -- break

        log.info("json.null", json.encode({name=json.null}))
        log.info("json.null", json.decode("{\"abc\":null}").abc == json.null)
        log.info("json.null", json.decode("{\"abc\":null}").abc == nil)

        --The following code only runs correctly with 64bit firmware
        local tmp = [[{ "timestamp": 1691998307973}]]
        local abc, err = json.decode(tmp)
        log.info("json", abc, err)
        log.info("json", abc and abc.timestamp)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
