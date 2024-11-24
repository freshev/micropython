
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "xxtea"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")


sys.taskInit(function()
    if not xxtea then
        while true do
            sys.wait(1000)
            --Print a message every 1 second
            log.info("testCrypto.xxteaTest","xxtea库不存在,请云编译一份最新版固件,并选上xxtea库")
        end
    end
    while true do
        sys.wait(1000)
        local text = "Hello World!"
        local key = "07946"
        local encrypt_data = xxtea.encrypt(text, key)
        log.info("testCrypto.xxteaTest","xxtea_encrypt:", encrypt_data:toHex())
        local decrypt_data = xxtea.decrypt(encrypt_data, key)
        log.info("testCrypto.xxteaTest","decrypt_data:", decrypt_data:toHex())
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
