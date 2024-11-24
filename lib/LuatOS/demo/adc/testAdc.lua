
--[[
1. Air101，Air103 模块上的ADC0脚-PA1, 0~2.4v,不要超过范围使用!!!
2. Air101，Air103模块上的ADC1脚-PA4, 0~2.4v,不要超过范围使用!!!
3. Air103 模块上的ADC2脚-PA2, 0~2.4v,不要超过范围使用!!! 
4. Air103 模块上的ADC3脚-PA3, 0~2.4v,不要超过范围使用!!! 
5. Air101,Air103 adc.CH_CPU 为内部温度 ，adc.CH_VBAT为VBAT
6. Air105 adc参考电压是1.88V，所有通道一致，
7. Air105内部分压没有隔离措施，在开启内部分压后，量程有所变化，具体看寄存器手册，1~5分压后能测到3.6，6通道能接近5V，但是不能直接测5V，可以测4.2V 0通道是始终开启无法关闭分压。
8. Air780E内部ADC接口为12bits 外部直流分压为0-3.4V
9. Air780E内部具有2个ADC接口，ADC0 --AIO3 ADC1 -- AIO4
10. 特殊通道, CPU内部温度Temp --adc.CH_CPU main power supply pin voltage VBAT -- adc.CH_VBAT
11. 设置分压(adc.setRange)要在adc.open之前设置，否则无效!!
]]

local testAdc = {}

local rtos_bsp = rtos.bsp()
function adc_pin() --Set the ADC number according to different development boards
    if rtos_bsp == "AIR101" then --Air101 development board ADC number
        return 0,1,255,255,adc.CH_CPU ,adc.CH_VBAT 
    elseif rtos_bsp == "AIR103" then --Air103 development board ADC number
        return 0,1,2,3,adc.CH_CPU ,adc.CH_VBAT 
    elseif rtos_bsp == "AIR105" then --Air105 development board ADC number
        --The voltage divider is not enabled by default, and the range is 0-1.8v with high accuracy.
        --Setting the partial pressure must be set before adc.open, otherwise it will be invalid!!
        -- adc.setRange(adc.ADC_RANGE_3_6)
        return 0,5,6,255,255,255
    elseif rtos_bsp == "ESP32C3" then --ESP32C3 development board ADC number
        return 0,1,2,3,adc.CH_CPU , 255
    elseif rtos_bsp == "ESP32C2" then --ESP32C2 development board ADC number
        return 0,1,2,3,adc.CH_CPU , 255
    elseif rtos_bsp == "ESP32S3" then --ESP32S3 development board ADC number
        return 0,1,2,3,adc.CH_CPU , 255
    elseif rtos_bsp == "EC618" then --Air780E development board ADC number
        --The voltage divider is not enabled by default, and the range is 0-1.2v with high accuracy.
        --Setting the partial pressure must be set before adc.open, otherwise it will be invalid!!
        -- adc.setRange(adc.ADC_RANGE_3_8)
        return 0,1,255,255,adc.CH_CPU ,adc.CH_VBAT 
    elseif rtos_bsp == "EC718P" then --Air780EP development board ADC number
        --The voltage divider is not enabled by default, and the range is 0-1.6v with high accuracy.
        --After turning on the voltage divider, the maximum external input cannot exceed 3.3V.
        --Setting the partial pressure must be set before adc.open, otherwise it will be invalid!!
        -- adc.setRange(adc.ADC_RANGE_MAX)
        return 0,1,255,255,adc.CH_CPU ,adc.CH_VBAT 
    elseif rtos_bsp == "UIS8850BM" then 
        return 0,255,255,255, adc.CH_CPU ,adc.CH_VBAT 
    else
        log.info("main", "define ADC pin in main.lua")
        return 255,255,255,255, adc.CH_CPU ,adc.CH_VBAT 
    end
end
local adc_pin_0,adc_pin_1,adc_pin_2,adc_pin_3,adc_pin_temp,adc_pin_vbat=adc_pin()


function testAdc.dotest()
    if rtos_bsp == "AIR105" then
        adc.setRange(adc.ADC_RANGE_3_6) --When the internal pressure divider is turned on, the measuring range can be expanded.
    end
    if adc_pin_0 and adc_pin_0 ~= 255 then adc.open(adc_pin_0) end
    if adc_pin_1 and adc_pin_1 ~= 255 then adc.open(adc_pin_1) end
    if adc_pin_2 and adc_pin_2 ~= 255 then adc.open(adc_pin_2) end
    if adc_pin_3 and adc_pin_3 ~= 255 then adc.open(adc_pin_3) end
    if adc_pin_temp and adc_pin_temp ~= 255 then adc.open(adc_pin_temp) end
    if adc_pin_vbat and adc_pin_vbat ~= 255 then adc.open(adc_pin_vbat) end

    if adc_pin_0 and adc_pin_0 ~= 255 and mcu and mcu.ticks then
        sys.wait(1000)
        log.info("开始读取ADC")
        local ms_start = mcu.ticks()
        for i = 1, 100, 1 do
            adc.get(adc_pin_0)
        end
        local ms_end = mcu.ticks()
        log.info("结束读取ADC")
        log.info("adc", "读取耗时", "100次", ms_end - ms_start, "ms", "单次", (ms_end - ms_start) // 100, "ms")
    end

    --The following is a loop printing. It is normal that 0 is not printed when grounded.
    --The accuracy of ADC is not too high. If you need a high-precision ADC, it is recommended to add an additional ADC chip.
    while true do
        if adc_pin_0 and adc_pin_0 ~= 255 then
            log.debug("adc", "adc" .. tostring(adc_pin_0), adc.get(adc_pin_0)) --If adc.get reports nil, change it to adc.read
        end
        if adc_pin_1 and adc_pin_1 ~= 255 then
            log.debug("adc", "adc" .. tostring(adc_pin_1), adc.get(adc_pin_1))
        end
        if adc_pin_2 and adc_pin_2 ~= 255 then
            log.debug("adc", "adc" .. tostring(adc_pin_2), adc.get(adc_pin_2))
        end
        if adc_pin_3 and adc_pin_3 ~= 255 then
            log.debug("adc", "adc" .. tostring(adc_pin_3), adc.get(adc_pin_3))
        end
        if adc_pin_temp and adc_pin_temp ~= 255 then
            log.debug("adc", "CPU TEMP", adc.get(adc_pin_temp))
        end
        if adc_pin_vbat and adc_pin_vbat ~= 255 then
            log.debug("adc", "VBAT", adc.get(adc_pin_vbat))
        end
        sys.wait(1000)
    end

    --If you no longer want to read, you can turn off the adc to reduce power consumption. It is not necessary.
    if adc_pin_0 and adc_pin_0 ~= 255 then adc.close(adc_pin_0) end
    if adc_pin_1 and adc_pin_1 ~= 255 then adc.close(adc_pin_1) end
    if adc_pin_2 and adc_pin_2 ~= 255 then adc.close(adc_pin_2) end
    if adc_pin_3 and adc_pin_3 ~= 255 then adc.close(adc_pin_3) end
    if adc_pin_temp and adc_pin_temp ~= 255 then adc.close(adc_pin_temp) end
    if adc_pin_vbat and adc_pin_vbat ~= 255 then adc.close(adc_pin_vbat) end

end

return testAdc
