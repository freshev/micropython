
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "gmssldemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--The default main frequency of Air101/Air103 is 80M. Running the national secret algorithm will be very slow. Adjust it to 240M.
if mcu and (rtos.bsp() == "AIR101" or rtos.bsp() == "AIR103" or rtos.bsp() == "AIR601" ) then
    mcu.setClk(240)
end

sys.taskInit(function()

    sys.wait(1000)
    log.info("gmssl", "start")
    --unencrypted string
    local originStr = "!!from LuatOS!!"

    --SM2, asymmetric encryption, similar to RSA, but an elliptical algorithm
    --The current implementation is still relatively slow.
    if gmssl.sm2encrypt then --Some BSPs do not support
        local pkx = "ABE87C924B7ECFDEA1748A06E89003C9F7F4DC5C3563873CE2CAE46F66DE8141"
        local pky = "9514733D38CC026F2452A6A3A3A4DA0C28F864AFA5FE2C45E0EB6B761FBB5286"
        local private = "129EDC282CD2E9C1144C2E7315F926D772BC96600D2771E8BE02060313FE00D5"

        --GMSSL default format
        log.info("==== SM2 默认GMSSL模式")
        local encodeStr = gmssl.sm2encrypt(pkx,pky,originStr)
        log.info("sm2默认模式", "加密后", encodeStr and  string.toHex(encodeStr))
        if encodeStr then
            log.info("sm2默认模式", "解密后", gmssl.sm2decrypt(private,encodeStr))
        end
        

        --Website compatibility mode https://i.goto327.top/CryptTools/SM2.aspx
        --Cipher text format C1C3C2, the new national standard, usually this
        log.info("==== SM2 网站兼容模式")
        local encodeStr = gmssl.sm2encrypt(pkx,pky,originStr, true)
        log.info("sm2网站兼容模式 C1C3C2", "加密后", encodeStr and  string.toHex(encodeStr))
        if encodeStr then
            log.info("sm2网站兼容模式 C1C3C2", "解密后", gmssl.sm2decrypt(private,encodeStr, true))
        else
            log.info("解密失败")
        end
        --Cipher text format C1C2C3, old national standard, old Java libraries usually support this
        log.info("==== SM2 网站兼容模式, 但C1C2C3")
        local encodeStr = gmssl.sm2encrypt(pkx,pky,originStr, true, true)
        log.info("sm2网站兼容模式 C1C2C3", "加密后", encodeStr and  string.toHex(encodeStr))
        if encodeStr then
            log.info("sm2网站兼容模式 C1C2C3", "解密后", gmssl.sm2decrypt(private,encodeStr, true, true))
        else
            log.info("解密失败")
        end
    end

    --SM3 algorithm, hash class
    if gmssl.sm3update then
        log.info("=== SM3测试")
        encodeStr = gmssl.sm3update("lqlq666lqlq946")
        log.info("gmssl.sm3update",string.toHex(encodeStr))
    end

    if gmssl.sm4encrypt then
        log.info("=== SM4测试")
        local passwd = "1234567890123456"
        local iv = "1234567890666666"
        --SM4 algorithm, symmetric encryption
        originStr = ">>SM4 ECB ZeroPadding test<<"
        --Encryption mode: ECB; Padding method: ZeroPadding; Key: 1234567890123456; Key length: 128 bit
        encodeStr = gmssl.sm4encrypt("ECB", "ZERO", originStr, passwd)
        log.info("sm4.ecb.zero", "加密后", string.toHex(encodeStr))
        log.info("sm4.ecb.zero", "解密后", gmssl.sm4decrypt("ECB","ZERO",encodeStr,passwd))

        originStr = ">>SM4 ECB Pkcs5Padding test<<"
        --Encryption mode: ECB; Padding method: Pkcs5Padding; Key: 1234567890123456; Key length: 128 bit
        encodeStr = gmssl.sm4encrypt("ECB", "PKCS5", originStr, passwd)
        log.info("sm4.ecb.pks5", "加密后", string.toHex(encodeStr))
        log.info("sm4.ecb.pks5", "解密后", gmssl.sm4decrypt("ECB","PKCS5",encodeStr,passwd))

        originStr = ">>SM4 CBC Pkcs5Padding test<<"
        --Encryption mode: CBC; Padding method: Pkcs5Padding; Key: 1234567890123456; Key length: 128 bit; Offset: 1234567890666666
        encodeStr = gmssl.sm4encrypt("CBC","PKCS5", originStr, passwd, iv)
        log.info("sm4.cbc.pks5", "加密后", string.toHex(encodeStr))
        log.info("sm4.cbc.pks5", "解密后", gmssl.sm4decrypt("CBC","PKCS5",encodeStr,passwd, iv))

        --Fully aligned 16-byte comparison test
        originStr = "1234567890123456"
        encodeStr = gmssl.sm4encrypt("ECB","PKCS7",originStr,passwd)
        log.info("sm4.ecb.pkcs7", encodeStr:toHex())
        encodeStr = gmssl.sm4encrypt("ECB","PKCS5",originStr,passwd)
        log.info("sm4.ecb.pkcs5", encodeStr:toHex())
        encodeStr = gmssl.sm4encrypt("ECB","ZERO",originStr,passwd)
        log.info("sm4.ecb.zero", encodeStr:toHex())
        encodeStr = gmssl.sm4encrypt("ECB","NONE",originStr,passwd)
        log.info("sm4.ecb.none", encodeStr:toHex())
    end

    --SM2 signature and verification
    if gmssl.sm2sign then
        local originStr = string.fromHex("434477813974bf58f94bcf760833c2b40f77a5fc360485b0b9ed1bd9682edb45")
        local pkx = "ABE87C924B7ECFDEA1748A06E89003C9F7F4DC5C3563873CE2CAE46F66DE8141"
        local pky = "9514733D38CC026F2452A6A3A3A4DA0C28F864AFA5FE2C45E0EB6B761FBB5286"
        local private = "129EDC282CD2E9C1144C2E7315F926D772BC96600D2771E8BE02060313FE00D5"

        --Without id, the default id="1234567812345678"
        local sig = gmssl.sm2sign(private, originStr, nil)
        log.info("sm2sign", sig and sig:toHex())
        if sig then
            local ret = gmssl.sm2verify(pkx, pky, originStr, nil, sig)
            log.info("sm2verify", ret or "false")
        end

        --With ID
        local id = "1234"
        local sig = gmssl.sm2sign(private, originStr, id)
        log.info("sm2sign", sig and sig:toHex())
        if sig then
            local ret = gmssl.sm2verify(pkx, pky, originStr, id, sig)
            log.info("sm2verify", ret or "false")
        end
    end

    log.info("gmssl", "ALL Done")
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
