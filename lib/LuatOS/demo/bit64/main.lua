
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "bit64_test"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end


local data,b64,b32,a,b

if bit64 then
	log.style(1)
	log.info("bit64 演示")
	data = 12345678
	b64 = bit64.to64(data)
	b32 = bit64.to32(b64)
	log.info("i32", b32, mcu.x32(b32))
	data = -12345678
	b64 = bit64.to64(data)
	b32 = bit64.to32(b64)
	log.info("i32", b32, mcu.x32(b32))
	data = 12.34234
	b64 = bit64.to64(data)
	b32 = bit64.to32(b64)
	log.info("f32", data, b32)
	data = -12.34234
	b64 = bit64.to64(data)
	b32 = bit64.to32(b64)
	log.info("f32", data, b32)


	a = bit64.to64(87654321)
	b = bit64.to64(12345678)
	log.info("87654321+12345678=", bit64.show(bit64.plus(a,b)))
	log.info("87654321-12345678=", bit64.show(bit64.minus(a,b)))
	log.info("87654321*12345678=", bit64.show(bit64.multi(a,b)))
	log.info("87654321/12345678=", bit64.show(bit64.pide(a,b)))

	a = bit64.to64(87654321)
	b = 1234567
	log.info("87654321+1234567=", bit64.show(bit64.plus(a,b)))
	log.info("87654321-1234567=", bit64.show(bit64.minus(a,b)))
	log.info("87654321*1234567=", bit64.show(bit64.multi(a,b)))
	log.info("87654321/1234567=", bit64.show(bit64.pide(a,b)))


	a = bit64.to64(87654.326)
	b = bit64.to64(12345)
	log.info("87654.326+12345=", 87654.326 + 12345)
	log.info("87654.326+12345=", bit64.show(bit64.plus(a,b)))
	log.info("87654.326-12345=", bit64.show(bit64.minus(a,b)))
	log.info("87654.326*12345=", bit64.show(bit64.multi(a,b)))
	log.info("87654.326/12345=", bit64.show(bit64.pide(a,b)))

	a = bit64.to64(87654.32)
	b = bit64.to64(12345.67)
	log.info("float", "87654.32+12345.67=", 87654.32 + 12345.67)
	log.info("double","87654.32+12345.67=", bit64.show(bit64.plus(a,b)))
	log.info("double to float","87654.32+12345.67=", bit64.to32(bit64.plus(a,b)))
	log.info("87654.32-12345.67=", bit64.show(bit64.minus(a,b)))
	log.info("87654.32*12345.67=", bit64.show(bit64.multi(a,b)))
	log.info("87654.32/12345.67=", bit64.show(bit64.pide(a,b)))
	log.info("double to int64", "87654.32/12345.67=", bit64.show(bit64.pide(a,b,nil,true)))

	a = bit64.to64(0xc0000000)
	b = 2
	a = bit64.shift(a,8,true)
	log.info("0xc0000000 << 8 =", bit64.show(a, 16))
	log.info("0xc000000000+2=", bit64.show(bit64.plus(a,b), 16))
	log.info("0xc000000000-2=", bit64.show(bit64.minus(a,b), 16))
	log.info("0xc000000000*2=", bit64.show(bit64.multi(a,b), 16))
	log.info("0xc000000000/2=", bit64.show(bit64.pide(a,b), 16))
	log.style(0)

	if bit64.strtoll then
		local data = bit64.strtoll("864040064024194", 10)
		log.info("data", data:toHex())
		log.info("data", bit64.show(data))
	end
end

local function sys_run_time()
	local tick64, per = mcu.tick64(true)
	local per_cnt = per * 1000000
	while true do
		tick64, per = mcu.tick64(true)
		log.info("work time","当前时间", bit64.to32(bit64.pide(tick64,per_cnt)))
		sys.wait(1000)
	end
end

if mcu.tick64 then
	sys.taskInit(sys_run_time)
end

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
