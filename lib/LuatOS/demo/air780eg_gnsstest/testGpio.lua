

LED_VBAT = 26
gpio.setup(LED_VBAT, 0) --Low voltage warning light

adc.open(adc.CH_CPU)

--GPIO adapted to GNSS test equipment
sys.taskInit(function()
    while 1 do
        local vbat = adc.get(adc.CH_VBAT)
        log.info("vbat", vbat)
        if vbat < 3400 then
            gpio.set(LED_VBAT, 1)
            sys.wait(100)
            gpio.set(LED_VBAT, 0)
            sys.wait(900)
        else
            sys.wait(1000)
        end
    end
end)

