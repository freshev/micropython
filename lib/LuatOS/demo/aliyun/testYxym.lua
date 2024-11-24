
local sys = require "sys"
local aliyun = require "aliyun"

--Modify the following parameters according to your own server
--Alibaba Cloud information: https://help.aliyun.com/document_detail/147356.htm?spm=a2c4g.73742.0.0.4782214ch6jkXb#section-rtu-6kn-kru
tPara = {
    --One type, one secret - ProductSecret requires filling in the product secret
    --One type, one password, divided into 2 types: pre-registration and pre-registration-free
    --Public instances only support pre-registration, fill in False for Registration
    --Enterprise instances support pre-registration and pre-registration-free. If you want to use pre-registration-free, fill in True for Registration, otherwise fill in False.
    Registration = false,
    --Device name, must be unique
    DeviceName = "abcd123",
    --Product key, on the product details page
    ProductKey = "a1DtzomWBme",     --Product key
    --Product secret, one type and one password need to be filled in
    ProductSecret = "dxV3o2IekLLsOMFn",
    --Fill in the instance id. On the instance details page, if it is an old public instance, please fill in the host parameter.
    InstanceId = "",
    RegionId = "cn-shanghai",
    --Whether to use ssl encrypted connection
    mqtt_isssl = true,
}

--Based on the deviceSecret, clientid, and deviceToken stored in the kv file area that does not disappear after power failure, it is judged whether to perform a normal connection or reconnect after power failure.
sys.taskInit(function()
    sys.waitUntil("IP_READY")
    log.info("已联网", "开始初始化aliyun库")

    local store = aliyun.store()

    --Determine whether they are the same triplet, if not, reconnect
    if store.deviceName ~= tPara.DeviceName or store.productKey ~= tPara.ProductKey then
        --Clear the registration information of fskv area
        if fskv then
            fskv.del("DeviceName")
            fskv.del("ProductKey")
            fskv.del("deviceToken")
            fskv.del("deviceSecret")
            fskv.del("clientid")
        else
            os.remove("/alireg.json")
        end
        store = {}
    end

    if store.clientid and store.deviceToken and #store.clientid > 0 and #store.deviceToken > 0 then
        tPara.clientId = store.clientid
        tPara.deviceToken = store.deviceToken
        tPara.reginfo = true
    elseif store.deviceSecret and #store.deviceSecret > 0 then
        tPara.deviceSecret = store.deviceSecret
        tPara.reginfo = true
    end
    aliyun.setup(tPara)
end)

