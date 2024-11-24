
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "vsimdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")
mobile.config(mobile.CONF_USB_ETHERNET, 3)
mobile.vsimInit()
mobile.flymode(nil,true)
mobile.vsimOnOff(true)
mobile.flymode(nil,false)

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
    sys.wait(15000)
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
