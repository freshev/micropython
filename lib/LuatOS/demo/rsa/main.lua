
--- Module function: rsa example
--@Modules rsa
--@author wendal
--@release 2022.11.03

--[[
提醒: 本demo需要用到公钥和私钥, demo目录中的公钥私钥文件是演示用的, 实际使用请自行生成

生成公钥私钥, 可使用openssl命令, 或者找个网页生成. 2048 是RSA位数, 最高支持4096,但不推荐,因为很慢.

openssl genrsa -out privkey.pem 2048
openssl rsa -in privkey.pem -pubout -out public.pem


--When downloading scripts and resources to the device, be sure to add the two pem files in this directory, otherwise the demo will not run.
]]

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "rsademo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Because this is a general demo, air101/air103 runs at full speed so that it is not too slow -_-
if rtos.bsp() == "AIR101" or rtos.bsp() == "AIR103" or rtos.bsp() == "AIR601"  then
    if mcu then
        mcu.setClk(240)
    end
end

sys.taskInit(function()
    --In order for the log to be displayed normally, the delay is deliberately delayed by 2 seconds, which is not needed in actual use.
    sys.wait(2000)

    --Check if the rsa library is included. If not, please remind me.
    if not rsa then
        log.warn("main", "this demo need rsa lib!!!")
        return
    end

    --Read the public key and encrypt the data immediately
    local res = rsa.encrypt((io.readFile("/luadb/public.pem")), "abc")
    --Print results
    log.info("rsa", "encrypt", res and #res or 0, res and res:toHex() or "")

    --The following is decryption, which is usually not performed on the device side. It is mainly used to demonstrate usage and will be very slow.
    if res then
        --Read the private key, then decode the data
        local dst = rsa.decrypt((io.readFile("/luadb/privkey.pem")), res, "")
        log.info("rsa", "decrypt", dst and #dst or 0, dst and dst:toHex() or "")
    end

    --Demo signing and verification
    local hash = crypto.sha1("1234567890"):fromHex()
    --Signing is usually slow and is usually done by the server
    local sig = rsa.sign((io.readFile("/luadb/privkey.pem")), rsa.MD_SHA1, hash, "")
    log.info("rsa", "sign", sig and #sig or 0, sig and sig:toHex() or "")
    if sig then
        --Verification is very fast
        local ret = rsa.verify((io.readFile("/luadb/public.pem")), rsa.MD_SHA1, hash, sig)
        log.info("rsa", "verify", ret)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
