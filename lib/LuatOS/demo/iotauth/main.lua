
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "iotauthdemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
local sys = require "sys"

log.info("main", PROJECT, VERSION)


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

sys.taskInit(function()
    sys.wait(2000)
    --The following demonstrations are all key calculations required by mqtt. They can be used together with the examples under demo/socket or demo/mqtt.

    --China Mobile OneNet
    local client_id,user_name,password = iotauth.onenet("qDPGh8t81z", "45463968338A185E", "MTIzNDU2")
    log.info("onenet",client_id,user_name,password)

    --Huawei Cloud
    local client_id,user_name,password = iotauth.iotda("6203cc94c7fb24029b110408_88888888","123456789")
    log.info("iotda",client_id,user_name,password)

    --Graffiti
    local client_id,user_name,password = iotauth.tuya(" 6c95875d0f5ba69607nzfl","fb803786602df760")
    log.info("tuya",client_id,user_name,password)

    --Baidu cloud service
    local client_id,user_name,password = iotauth.baidu("abcd123","mydevice","ImSeCrEt0I1M2jkl")
    log.info("baidu",client_id,user_name,password)

    --Tencent Cloud
    local client_id,user_name,password = iotauth.qcloud("LD8S5J1L07","test","acyv3QDJrRa0fW5UE58KnQ==")
    log.info("qcloud",client_id,user_name,password)

    --Alibaba Cloud
    local client_id,user_name,password = iotauth.aliyun("123456789","abcdefg","Y877Bgo8X5owd3lcB5wWDjryNPoB")
    log.info("aliyun",client_id,user_name,password)

end)



--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
