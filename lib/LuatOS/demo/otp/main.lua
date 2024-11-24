
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "otpdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--[[
提示: OTP是比较高级的API, 一般用于存储密钥等核心数据, 不建议用于普通数据的存放

OTP有个非常重要的特性, 就是一旦加锁就永久无法解锁, 这点非常非常重要

OTP通常由多个zone, 0,1,2, 每个zone通常由256字节, 但这个非常取决于具体模块.

OTP在没有加锁之前是可以抹除的, 每次都是整个zone一起抹除.
]]

sys.taskInit(function()
    sys.wait(3000)

    --Erase area
    -- otp.erase(2)

    --write otp area
    local ret = otp.write(2, "1234", 0)
    log.info("otp", "write", ret)

    --Read otp area
    for zone = 0, 2, 1 do
        local otpdata = otp.read(zone, 0, 64)
        if otpdata then
            log.info("otp", zone, otpdata:toHex())
            log.info("otp", zone, otpdata)
        end
    end

    --Lock the otp area, don’t try it easily, you can’t unlock it after locking it.
    -- otp.lock(2)
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
