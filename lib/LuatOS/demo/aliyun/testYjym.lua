
local sys = require "sys"
local aliyun = require "aliyun"

--Modify the following parameters according to your own server
--Alibaba Cloud information: https://help.aliyun.com/document_detail/147356.htm?spm=a2c4g.73742.0.0.4782214ch6jkXb#section-rtu-6kn-kru
local tPara = {
    --One machine, one secret
    --Device name, must be unique
    DeviceName = "azNhIbNNTdsVwY2mhZno",
    --Product key, on the product details page
    ProductKey = "a1YFuY6OC1e",     --Product key
    --Device key, one type and one password is not required, one machine and one password (pre-registration) must be filled in
    DeviceSecret = "5iRxTePbEMguOuZqltZrJBR0JjWJSdA7", --Device secret
    --Fill in the instance id. On the instance details page, if it is an old public instance, please fill in the RegionId parameter.
    InstanceId = "",
    RegionId = "cn-shanghai",
    --Whether to use ssl encrypted connection
    mqtt_isssl = false,
}

--Wait for networking, and then initialize the aliyun library
sys.taskInit(function()
    -- sys.waitUntil("IP_READY")
    sys.waitUntil("net_ready")

    log.info("已联网", "开始初始化aliyun库")
    aliyun.setup(tPara)
end)
