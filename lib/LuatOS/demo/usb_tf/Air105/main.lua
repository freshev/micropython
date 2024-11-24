
--LuaTools requires two pieces of information, PROJECT and VERSION
--You must first mount the TF card and then start the USB
--Due to the speed of USB and SPI, the initialization process of U disk on the computer is very slow. It takes 20~30 seconds for a 16G card. The computer file browser will get stuck. This is normal.
--You can see the same problem when you plug the card reader into a USB-HUB with a full-speed interface.
--prvUSB_MSCTimeout 96:bot timeout!, reboot usb is normal and don’t worry about it.
--The reading speed is about 600KB~700KB, and the writing speed is 350KB. Note that 105 reads and writes TF cards faster than this one.
PROJECT = "usb_tf"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    sys.wait(500) --Start delay
    local spiId = 0
    local result = spi.setup(
        spiId,--serial port id
        255, --Do not use the default CS pin
        0,--CPHA
        0,--CPOL
        8,--data width
        400*1000  --Use lower frequency when initializing
    )
    local TF_CS = pin.PB13
    gpio.setup(TF_CS, 1)
    --fatfs.debug(1) -- If the mount fails, you can try to turn on debugging information to find out the reason.
    fatfs.mount(fatfs.SPI,"/sd", spiId, TF_CS, 24000000)
    local data, err = fatfs.getfree("SD")
    usbapp.udisk_attach_sdhc(0) --udisk is mapped to the TF card
    usbapp.start(0)
    sys.wait(600000)
end)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
