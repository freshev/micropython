PROJECT = "aliyundemo"
VERSION = "1.0.0"
local sys = require "sys"

--One type, one encryption is preferred to use fskv to store the key.
if fskv then
    fskv.init()
end

--Networking function, optional
--To use the one-machine-one-secret demonstration, you need to open netready
--require "netready"

--Alibaba Cloud event processing function
require "testEvt"

--Demonstration of one machine and one secret
require "testYjym"

--Demonstration of one type and one secret must be modified to your own information to run through!!!
--require "testYxym"

--For aliyun+ low power consumption demonstration, you need to enable testYjym or testYxym
--require "testPm"

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
