
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "test"
VERSION = "1.0.0"

--[[
驱动下载, 与Air724驱动一致:
https://doc.openluat.com/wiki/21?wiki_page_id=2070
]]

--sys library is standard
_G.sys = require("sys")

sys.subscribe("USB_HID_INC", function(event)
    log.info("HID EVENT", event)
    if event == usbapp.NEW_DATA then
        sys.publish("HID_RX")
    end
end)
sys.taskInit(function()
    local rx_buff = zbuff.create(64)
    local tx_buff = zbuff.create(64)
    --The following demonstrates sending original packets in keyboard mode to implement keyboard functions.
    --In the underlying keyboard descriptor, 1 package contains 8 bytes, and byte0 is the control key value, where
    --bit0 LeftControl
    --bit1 LeftShift
    --bit2 LeftAlt
    --bit 3 left GUI
    --bit4 RightControl
    --bit5 RightShift
    --bit6 RightAlt
    --bit7 RightGUI win键
    --byte1 fixed 0
    --byte2~8 other key values, up to 6 keys can be pressed at the same time
    --When no button is pressed, all 8 bytes are 0
    --In order to be compatible with code scanner processing, multiple keystroke processing is allowed to be sent at one time.
    usbapp.start(0)
    while 1 do
        --Simulate pressing the number 0 and then lifting it
        tx_buff:copy(0, "\x00\x00\x27\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00")
        usbapp.hid_tx(0, tx_buff)
        tx_buff:del()
        sys.wait(5000)
         --Simulate pressing Ctrl+Alt+A and then lift it to take a QQ screenshot
        tx_buff:copy(0, "\x05\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00")
        usbapp.hid_tx(0, tx_buff)
        tx_buff:del()
        sys.wait(5000)
    end
    --The following demonstrates custom HID. Comment out the above keyboard mode demonstration code before use.
    usbapp.set_id(0, 0xaabb, 0xccdd)    --After changing the default VID and PID, the serial port cannot be recognized, but HID and MSD can still be used.
    usbapp.hid_mode(0, 1, 8)       --Custom HID mode, ep_size=8, each transmission requires a multiple of 8, suitable for applications with small data volume
    --usbapp.hid_mode(0, 1, 64) --Customized HID mode, ep_size=64, each transmission requires a multiple of 64, suitable for applications with large amounts of data
    usbapp.start(0)
    while 1 do  --Receive data and return hellworld+PC data
        local msg, data = sys.waitUntilExt("HID_RX")
        usbapp.hid_rx(0, rx_buff)
        tx_buff:copy(0, "helloworld" .. rx_buff:query())
        rx_buff:del()
        usbapp.hid_tx(0, tx_buff)
        tx_buff:del()
    end
    
end)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
