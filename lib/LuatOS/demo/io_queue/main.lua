
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ioqueue"
VERSION = "1.0.0"
--Can only be used on platforms that support ioqueue!!
--sys library is standard
_G.sys = require("sys")

local rtos_bsp = rtos.bsp()
function pinx() --According to different development boards, assign different gpio pin numbers to the LED
	if rtos_bsp == "AIR105" then
		mcu.setXTAL(true)   --In order to test more accurately, adjust to the external clock
        return 1, pin.PD06, pin.PD07
    elseif rtos_bsp == "EC718P" then --Air780E development board pins
		gpio.setup(12,nil,nil)
		gpio.setup(13,nil,nil)
        return 1, 12,13
    else
        log.info("main", "define led pin in main.lua")
        return 255,255
    end
end

sys.taskInit(function()

    local _,tick_us = mcu.tick64()
    local hw_timer_id, capture_pin,out_pin = pinx()
    local buff1 = zbuff.create(100)
    local buff2 = zbuff.create(100)
    local cnt1,cnt2,i,lastTick,bit1Tick,nowTick,j,bit
	mcu.hardfault(0)
	sys.wait(2000)
    bit1Tick = 100 * tick_us
    while 1 do
		log.info('start dht11')
        --Test single bus DHT11
        ioqueue.stop(hw_timer_id)   --Make sure hardware timer 1 is idle
        ioqueue.init(hw_timer_id,100,1) --Set 100 commands in the io queue and repeat it once. In fact, you don’t need that many commands.
        ioqueue.setgpio(hw_timer_id, capture_pin, true, gpio.PULLUP)   --The data line is pulled high, pull-up input
        ioqueue.setdelay(hw_timer_id, 10000, 0, false)  --Single delay 10ms
        ioqueue.setgpio(hw_timer_id, capture_pin, false, 0, 0)   --Data line pulled low
        ioqueue.setdelay(hw_timer_id, 18000, 0, false)  --Single delay 18ms
        ioqueue.set_cap(hw_timer_id, capture_pin, gpio.PULLUP, gpio.FALLING, 100000 * tick_us)   --Set to falling edge interrupt capture, maximum timing 100000us, pull-up input, here it has replaced the output of 20~40us high level
        for i = 1,42,1 do
            ioqueue.capture(hw_timer_id)    --Capture the tick value when 42 external interrupts occur, 2 start signals + 40bit data
        end
        ioqueue.cap_done(hw_timer_id, capture_pin)  --Stop capturing
        ioqueue.setgpio(hw_timer_id, capture_pin, true, gpio.PULLUP)   --The data line is pulled high, pull-up input
        ioqueue.start(hw_timer_id)
        sys.waitUntil("IO_QUEUE_DONE_"..hw_timer_id)
        ioqueue.stop(hw_timer_id)
        --Start parsing the captured data
        cnt1,cnt2 = ioqueue.get(hw_timer_id, buff1, buff2)
        if cnt2 ~= 42 then
            log.info('test fail')
            goto TEST_OUT
        end
        lastTick = buff2:query(6 + 2, 4, false) --The second interrupted tick is used as the starting tick. The first tick is useless.
        j = 0
        bit = 8
        buff1[0] = 0
        for i = 2,41,1 do
			
            --Check whether it is the falling edge interrupt of the corresponding pin, but it is not necessary.
            if buff2[i * 6 + 0] ~= capture_pin or buff2[i * 6 + 1] ~= 0 then
                log.error("capture", i, buff2[i * 6 + 0], buff2[i * 6 + 1])
            end
            --Determine whether it is bit1 or bit0 by calculating the tick difference
            nowTick = buff2:query(i * 6 + 2, 4, false)
            buff1[j] = buff1[j] << 1 
            if (nowTick - lastTick) > bit1Tick then
               buff1[j] = buff1[j] + 1
            end
            bit = bit - 1
            if bit == 0 then
                j = j + 1
                bit = 8
            end
            lastTick = nowTick
        end
        buff1[5] = buff1[0] + buff1[1] + buff1[2] + buff1[3]
        if buff1[4] ~= buff1[5] then
            log.info('check fail', buff1[4], buff1[5])
        else
            log.info("湿度", buff1[0] .. '.' .. buff1[1], "温度", buff1[2] .. '.' ..  buff1[3])
        end
        ::TEST_OUT::
        ioqueue.release(hw_timer_id)

		log.info('output 1 start')
		 --Test high-precision fixed interval timing output, 1us interval flip level
        ioqueue.init(hw_timer_id, 100, 100)
        ioqueue.setgpio(hw_timer_id, out_pin, false,0,1)   --Set as output port, level 1
        ioqueue.setdelay(hw_timer_id, 0, tick_us - 3, true)  --Set to continuous delay, 1 us each time. If it is not accurate, fine-tune time_tick and the delay starts.
        for i = 0,40,1 do
            ioqueue.output(hw_timer_id, out_pin, 0)
            ioqueue.delay(hw_timer_id)     --Delay 1 time in a row
            ioqueue.output(hw_timer_id, out_pin, 1)
            ioqueue.delay(hw_timer_id)     --Delay 1 time in a row
        end
        ioqueue.start(hw_timer_id)
        sys.waitUntil("IO_QUEUE_DONE_"..hw_timer_id)
        log.info('output 1 done')
        ioqueue.stop(hw_timer_id)
        ioqueue.release(hw_timer_id)
        sys.wait(500)

		log.info('output 2 start')
        --Test high-precision variable interval timing output
        ioqueue.init(hw_timer_id, 100, 100)
        ioqueue.setgpio(hw_timer_id, out_pin, false,0,1)   --Set as output port, level 1
        ioqueue.setdelay(hw_timer_id, 0, tick_us - 3)  --Single delay is 1us. If it is not accurate, fine-tune time_tick.
        ioqueue.output(hw_timer_id, out_pin, 0) --low level
        ioqueue.setdelay(hw_timer_id, 1, tick_us - 3)  --Single delay 2us
        ioqueue.output(hw_timer_id, out_pin, 1) --high level
        ioqueue.setdelay(hw_timer_id, 2, tick_us - 3)  --Single delay 3us
        ioqueue.output(hw_timer_id, out_pin, 0) --low level
        ioqueue.setdelay(hw_timer_id, 3, tick_us - 3)  --Single delay 4us
        ioqueue.output(hw_timer_id, out_pin, 1) --high level
        ioqueue.setdelay(hw_timer_id, 4, tick_us - 3)  --Single delay 5us
        ioqueue.output(hw_timer_id, out_pin, 0) --low level
        ioqueue.setdelay(hw_timer_id, 5, tick_us - 3)  --Single delay 6us
        ioqueue.output(hw_timer_id, out_pin, 1) --high level
        ioqueue.start(hw_timer_id)
        sys.waitUntil("IO_QUEUE_DONE_"..hw_timer_id)
		log.info('output 2 done')
        ioqueue.stop(hw_timer_id)
        ioqueue.release(hw_timer_id)
        sys.wait(500)
		

		

		

    end
end)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
