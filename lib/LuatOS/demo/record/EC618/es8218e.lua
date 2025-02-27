--es8218e device address
local es8218e_address = 0x10

--es8218e initialization register configuration
local es8218e_reg = {
    {0x00, 0x00},
    {0x01, 0x2F + (0 << 7)},
    {0x02, 0x01},
    {0x03, 0x20},
    {0x04, 0x01},
    {0x05, 0x00},
    {0x06, 4 + (0 << 5)},
    {0x10, 0x18 + 0},
    {0x07, 0 + (3 << 2)},
    {0x09, 0x00},
    {0x0A, 0x22 + (0xCC * 0)},
    {0x0B, 0x02 - 0},
    {0x14, 0xA0},
    {0x0D, 0x30},
    {0x0E, 0x20},
    {0x23, 0x00},
    {0x24, 0x00},
    {0x18, 0x04},
    {0x19, 0x04},
    {0x0F, (0 << 5) + (1 << 4) + 7},
    {0x08, 0x00},
    {0x00, 0x80},
    {0x12, 0x1C},
    {0x11, 0},
    {0x01, (0x2f) + (1 << 7)},
}

--i2s data receiving buffer
local rx_buff = zbuff.create(3200)

--The amr data storage buffer should be as large as possible.
local amr_buff = zbuff.create(10240)

--Create an amr encoder
local encoder = codec.create(codec.AMR, false)

--i2s data receiving callback
local function record_cb(id, buff)
    if buff then
        log.info("I2S", id, "接收了", rx_buff:used())
        codec.encode(encoder, rx_buff, amr_buff)                --Encode the recording data with amr. If successful, this interface will return true.
    end
end



local function record_task()
    sys.wait(5000)
    uart.setup(1, 115200)                                       --Open serial port 1
    log.info("i2c initial",i2c.setup(0, i2c.FAST))              --Turn on i2c
    i2s.setup(0, 0, 8000, 16, 1, i2s.MODE_I2S)                  --Turn on i2s
    i2s.on(0, record_cb)                                        --Register i2s to receive callbacks
    i2s.recv(0, rx_buff, 3200)
    sys.wait(300)
    for i, v in pairs(es8218e_reg) do                           --Initialize es8218e
        i2c.send(0,es8218e_address,v,1)
    end
    sys.wait(5000)
    i2c.send(0, es8218e_address,{0x01, (0x2f) + (1 << 7)},1)    --Stop recording
    i2s.stop(0)                                                 --Stop receiving

    log.info("录音5秒结束")
    uart.write(1, "#!AMR\n")                                    --Send amr file identification data to the serial port
    sys.wait(5)                                                 --Wait 5ms to ensure successful sending
    uart.write(1, amr_buff:query())                             --Send encoded amr data to the serial port
end
pm.power(pm.DAC_EN, true)                                       --Turn on the power supply of es8218e chip
sys.taskInit(record_task)
