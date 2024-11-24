PROJECT="LVGL"
VERSION="0.0.1"
sys=require"sys"

device = require"device"
key = require"key"
animation = require"animation"

--HSPI is SPI5, the maximum is 96M, some screens do not support it, so here we write 48M
spi_lcd = spi.deviceSetup(device.spi,device.spiCS,0,0,8,device.spiSpeed,spi.MSB,1,1)
lcd.init("st7735v",{port = "device",pin_dc=device.lcdDC,pin_rst=device.lcdRST,pin_pwr=device.lcdBL,direction = 2,w = 80,h = 160,xoffset = 24,yoffset = 0},spi_lcd)
lcd.invoff()--This screen may need to be displayed in reverse

log.info("lvgl", lvgl.init())

scr = lvgl.obj_create(nil, nil)

if device.useFont then
    local font_16 = lvgl.font_load("/luadb/16_test_fonts.bin")
    if not font_16 then
        log.info("font error","font not found")
    else
        lvgl.obj_set_style_local_text_font(scr, lvgl.LABEL_PART_MAIN, lvgl.STATE_DEFAULT, font_16)
    end
end

home = require"home"
about = require"about"

lvgl.scr_load(scr)


home.show()
key.setCb(home.keyCb)


sys.run()
