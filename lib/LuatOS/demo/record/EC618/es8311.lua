audio.on(0, function(id, event)
    --When using play to play a file, there is only a playback completion callback.
    local succ,stop,file_cnt = audio.getError(0)
    if not succ then
        if stop then
            log.info("用户停止播放")
        else
            log.info("第", file_cnt, "个文件解码失败")
        end
    end
    --log.info("Complete playing an audio")
    sys.publish("AUDIO_PLAY_DONE")
end)

local i2c_id = 0
local i2s_id = 0

--es8311 device address
local es8311_address = 0x18    --Pay attention to hardware differences, own actual address
local record_cnt = 0
--es8311 initialization register configuration
local es8311_reg = {
	{0x45,0x00},
	{0x01,0x30},
	{0x02,0x10},
	{0x02,0x00},
	{0x03,0x10},
	{0x16,0x24},
	{0x04,0x20},
	{0x05,0x00},
	{0x06,3},
	{0x07,0x00},
	{0x08,0xFF},
	{0x09,0x0C},
	{0x0A,0x0C},
	{0x0B,0x00},
	{0x0C,0x00},
	{0x10,0x03},
	{0x11,0x7F},
	{0x00,0x80},
	{0x0D,0x01},
	{0x01,0x3F},
	{0x14,0x1a},
	{0x12,0x28},
	{0x13,0x00},
	{0x0E,0x02},
	{0x0F,0x44},
	{0x15,0x00},
	{0x1B,0x0A},
	{0x1C,0x6A},
	{0x37,0x48},
	{0x44,(0 <<7)},
	{0x17,210},
	{0x32,200},
}

--i2s data receiving buffer
local rx_buff = zbuff.create(3200)

--The AMR data storage buffer should be as large as possible. For MR475 encoding level, the file size per second is about 0.6k. Theoretically, the space required for five seconds of recording is 5 * 0.6 = 3k. Here, the buffer should be as large as possible.
local amr_buff = zbuff.create(5 * 1024)

--Create an amr encoder
local encoder = codec.create(codec.AMR, false)


--Recording file path
local recordPath = "/record.amr"

--i2s data receiving callback
local function record_cb(id, buff)
    if buff then
        log.info("I2S", id, "接收了", rx_buff:used())
        log.info("编码结果", codec.encode(encoder, rx_buff, amr_buff))		--Perform AMR encoding on the recording data. If successful, this interface will return true. The default encoding level is MR475.
		record_cnt = record_cnt + 1
		if record_cnt >= 25 then	--Stop after more than 5 seconds
			log.info("I2S", "stop") 
			i2s.stop(i2s_id)	
		end
    end
end

---- MultipartForm upload files
--url string request URL address
--filename string file name uploaded to the server
--filePath string The path of the file to be uploaded
local function postMultipartFormData(url, filename, filePath)
    local boundary = "----WebKitFormBoundary"..os.time()
    local req_headers = {
        ["Content-Type"] = "multipart/form-data; boundary=" .. boundary,
    }
    local body = {}
    table.insert(body, "--"..boundary.."\r\nContent-Disposition: form-data; name=\"file\"; filename=\"".. filename .."\"\r\n\r\n")
    table.insert(body, io.readFile(filePath))
    table.insert(body, "\r\n")
    table.insert(body, "--"..boundary.."--\r\n")
    body = table.concat(body)
    log.info("headers: ", "\r\n" .. json.encode(req_headers), type(body))
    log.info("body: " .. body:len() .. "\r\n" .. body)
    local code, headers, body = http.request("POST",url,
            req_headers,
            body
    ).wait()   
    log.info("http.post", code, headers, body)
end


local function record_task()
	os.remove(recordPath)
	gpio.setup(26, 1)									--Turn on the mic power supply of the recording development board
	audio.config(0, 25, 1, 6, 200)						
	pm.power(pm.DAC_EN, true)							--Turn on the power supply of es8311 chip
    log.info("i2c initial", i2c.setup(i2c_id, i2c.FAST))		--Turn on i2c
    for i, v in pairs(es8311_reg) do					--Initialize es8311
        i2c.send(i2c_id, es8311_address, v, 1)
    end
	i2s.setup(i2s_id, 0, 8000, 16, 1, i2s.MODE_I2S)			--Turn on i2s
    i2s.on(i2s_id, record_cb) 								--Register i2s to receive callbacks
    i2s.recv(i2s_id, rx_buff, 3200)
	i2c.send(i2c_id, es8311_address, {0x00, 0xc0}, 1)
    sys.wait(6000)
    i2c.send(i2c_id, es8311_address, {0x00, 0x80}, 1)			--ES8311 stops recording
    log.info("录音5秒结束")
	io.writeFile(recordPath, "#!AMR\n")					--Write amr file identification data to the file
	
	if rx_buff:len()  then
		io.writeFile(recordPath, amr_buff:query(), "a+b")	--Write encoded amr data to file
	else
		log.info("--------------> buff nil")
	end
	
	i2s.setup(i2s_id, 0, 0, 0, 0, i2s.MODE_MSB)
	local result = audio.play(0, {recordPath})			--Request audio playback
	log.info("音频播放结果", result)
	if result then
		sys.waitUntil("AUDIO_PLAY_DONE")				--Wait for the audio to finish playing
	else
														--Audio playback error
	end

	--The following demonstration is to send audio files to the server. If necessary, you can open the following code comments. The URL here is Hezhou's file upload test server. The uploaded files are to http://tools.openluat.com/tools /device-upload-testView
	--[[ 
		local timeTable = os.date("*t", os.time())
		local nowTime = string.format("%4d%02d%02d_%02d%02d%02d", timeTable.year, timeTable.month, timeTable.day, timeTable.hour, timeTable.min, timeTable.sec)
		local filename = mobile.imei() .. "_" .. nowTime .. ".amr"
		postMultipartFormData("http://tools.openluat.com/api/site/device_upload_file", filename, recordPath)
 	]]
	uart.setup(1, 115200)								--Open serial port 1
    uart.write(1, io.readFile(recordPath))				--Send recording files to serial port
end

sys.taskInit(record_task)
