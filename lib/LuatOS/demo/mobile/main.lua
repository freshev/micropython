
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "mobiledemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")



--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


--For dual-SIM devices, you can set it to automatically select the SIM card.
--However, in this way, the pin where SIM1 is located is forced to be reused as SIM function and cannot be reused as GPIO.
-- mobile.simid(2)
mobile.simid(2,true)--Prioritize SIM0


sys.taskInit(function()

	if rtos.bsp() == "UIS8850BM" then
		sys.wait(2000)
	end

	log.info("status", mobile.status())
    local band = zbuff.create(40)
    local band1 = zbuff.create(40)
    mobile.getBand(band)
    log.info("当前使用的band:")
    for i=0,band:used()-1 do
        log.info("band", band[i])
    end
    band1[0] = 38
    band1[1] = 39
    band1[2] = 40
    mobile.setBand(band1, 3)    --Change to use 38,39,40
    band1:clear()
    mobile.getBand(band1)
    log.info("修改后使用的band:")
    for i=0,band1:used()-1 do
        log.info("band", band1[i])
    end
    mobile.setBand(band, band:used())    --Change back to the original band, or you can choose to clear fs when downloading

    mobile.getBand(band1)
    log.info("修改回默认使用的band:")
    for i=0,band1:used()-1 do
        log.info("band", band1[i])
    end
	-- mobile.vsimInit()
	-- mobile.flymode(nil,true)
	-- mobile.vsimOnOff(true)
	-- mobile.flymode(nil,false)
    --mobile.apn(0,2,"") -- Activate CID2 using default APN
    --mobile.rtime(3) -- When there is no data interaction, RRC is automatically released after 3 seconds
    --The following is to configure the automatic search cell interval, which conflicts with the polling search. Just enable one.
    --mobile.setAuto(10000,30000, 5) -- SIM automatically recovers after being temporarily disconnected, and searches for surrounding community information once every 30 seconds.
	log.info("status", mobile.status())
    sys.wait(2000)
    while 1 do
        log.info("imei", mobile.imei())
        log.info("imsi", mobile.imsi())
        local sn = mobile.sn()
        if sn then
            log.info("sn",   sn:toHex())
        end
		log.info("status", mobile.status())
        

        log.info("iccid", mobile.iccid())
        log.info("csq", mobile.csq()) --The CSQ of 4G Modules does not fully represent the strength
        log.info("rssi", mobile.rssi()) --It needs to be judged based on rssi/rsrq/rsrp/snr.
        log.info("rsrq", mobile.rsrq())
        log.info("rsrp", mobile.rsrp())
        log.info("snr", mobile.snr())
        log.info("simid", mobile.simid()) --Here is how to get the current SIM card slot
        log.info("apn", mobile.apn(0,1))
        log.info("ip", socket.localIP())
		log.info("lua", rtos.meminfo())
        --sysmemory
        log.info("sys", rtos.meminfo("sys"))
        sys.wait(15000)
    end
end)

--Query of base station data

--Subscription type, the Modules itself will periodically query base station information, but usually does not include neighboring cells
sys.subscribe("CELL_INFO_UPDATE", function()
    log.info("cell", json.encode(mobile.getCellInfo()))
end)

--Polling type, including information about neighboring communities. This is a manual search, which conflicts with the automatic search above. Just enable one.
sys.taskInit(function()
    sys.wait(5000)
	mobile.config(mobile.CONF_SIM_WC_MODE, 2)
    while 1 do
        mobile.reqCellInfo(10)
        sys.wait(11000)
        log.info("cell", json.encode(mobile.getCellInfo()))
		mobile.config(mobile.CONF_SIM_WC_MODE, 2)
    end
end)

--Get the status of sim card

sys.subscribe("SIM_IND", function(status, value)
    log.info("sim status", status)
    if status == 'GET_NUMBER' then
        log.info("number", mobile.number(0))
    end
	if status == "SIM_WC" then
        log.info("sim", "write counter", value)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
