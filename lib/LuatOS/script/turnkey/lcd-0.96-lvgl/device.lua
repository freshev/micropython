local device
--Hardware compatibility layer

--Someone didn't adapt to the pin library, I won't say who it is
if not pin then pin = {} end

--LuatOS-SoC_V0006_air101 looks like this
local chips = {
    air101 = {
        useFont = true,
        spi = 0,
        spiCS = pin.PB04,
        spiSpeed = 20*1000*1000,
        lcdDC = pin.PB01,
        lcdRST = pin.PB03,
        lcdBL = pin.PB00,
        keyL = pin.PB11,
        keyR = pin.PA7,
        keyU = pin.PA1,
        keyD = pin.PA0,
        keyO = pin.PA4,
    },
    air105 = {
        useFont = true,
        spi = 5,
        spiCS = pin.PC14,
        spiSpeed = 48*1000*1000,
        lcdDC = pin.PE8,
        lcdRST = pin.PC12,
        lcdBL = pin.PE9,
        keyL = pin.PB4,
        keyR = pin.PC7,
        keyU = pin.PE6,
        keyD = pin.PA10,
        keyO = pin.PE7,
    },
    esp32c3 = {
        useFont = false,
        spi = 2,
        spiCS = 7,
        spiSpeed = 40*1000*1000,
        lcdDC = 6,
        lcdRST = 10,
        lcdBL = 11,
        keyL = 13,
        keyR = 8,
        keyU = 5,
        keyD = 9,
        keyO = 4,
    },
    ec618 = {
        useFont = false,
        spi = 0,
        spiCS = 8,
        spiSpeed = 20*1000*1000,
        lcdDC = 10,
        lcdRST = 1,
        lcdBL = 22,
        keyL = 0xff,
        keyR = 0xff,
        keyU = 12,
        keyD = 0xff,
        keyO = 11,
    },
    --Let's add the others as we go.
}
--These two are compatible
chips.air103 = chips.air101

--Get firmware version
local firmware = rtos.firmware and rtos.firmware() or rtos.get_version()
firmware = firmware:lower()--Convert all to lowercase
log.info("firmware",firmware)

--Return if matched
for name,info in pairs(chips) do
    if firmware:find(name) then
        device = info
        log.info("device","match",name)
        break
    end
end

if not device then log.error("警告！","无法为该模块匹配兼容文件") return end

chips = nil--Free up some space

return device
