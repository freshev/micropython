
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "minizdemo"
VERSION = "1.0.0"

sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    sys.wait(1000)
    --Compressed string, for convenience of demonstration, base64 encoding is used here.
    --The memory of most MCU devices is relatively small. miniz.compress is usually completed on the server side and will not be demonstrated here.
    --miniz can decompress standard zlib data streams
    local b64str = "eAEFQIGNwyAMXOUm+E2+OzjhCCiOjYyhyvbVR7K7IR0l+iau8G82eIW5jXVoPzF5pse/B8FaPXLiWTNxEMsKI+WmIR0l+iayEY2i2V4UbqqPh5bwimyEuY11aD8xeaYHxAquvom6VDFUXqQjG1Fek6efCFfCK0b0LUnQMjiCxhUT05GNL75dFUWCSMcjN3EE5c4Wvq42/36R41fa"
    local str = b64str:fromBase64()

    local dstr = miniz.uncompress(str)
    --Compressed data length 156
    --The length of the decompressed data, that is, the length of the original data 235
    log.info("miniz", "compressed", #str, "uncompressed", #dstr)
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
