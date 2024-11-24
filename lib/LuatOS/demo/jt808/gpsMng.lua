local gpsMng = {}

local gps_uart_id = 2

libgnss.clear() --Clear data and initialize

uart.setup(gps_uart_id, 115200)

function exec_agnss()
    if http then
        --AGNSS has been tuned
        while 1 do
            local code, headers, body = http.request("GET", "http://download.openluat.com/9501-xingli/HXXT_GPS_BDS_AGNSS_DATA.dat").wait()
            --local code, headers, body = http.request("GET", "http://nutzam.com/6228.bin").wait()
            log.info("gnss", "AGNSS", code, body and #body or 0)
            if code == 200 and body and #body > 1024 then
                --uart.write(gps_uart_id, "$reset,0,h01\r\n")
                -- sys.wait(200)
                --uart.write(gps_uart_id, body)
                for offset=1,#body,512 do
                    log.info("gnss", "AGNSS", "write >>>", #body:sub(offset, offset + 511))
                    uart.write(gps_uart_id, body:sub(offset, offset + 511))
                    sys.wait(100) --Waiting for 100ms will be more successful
                end
                io.writeFile("/6228.bin", body)
                break
            end
            sys.wait(60*1000)
        end
    end
    sys.wait(20)
    --Read previous location information
    local gnssloc = io.readFile("/gnssloc")
    if gnssloc then
        str = "$AIDPOS," .. gnssloc
        log.info("POS", str)
        uart.write(gps_uart_id, str .. "\r\n")
        str = nil
        gnssloc = nil
    else
        --TODO initiate base station positioning
        uart.write(gps_uart_id, "$AIDPOS,3432.70,N,10885.25,E,1.0\r\n")
    end
end
sys.taskInit(function()
    --The default baud rate of the GPS of the Air780EG engineering sample is 9600, and the mass production version is 115200. The following is the temporary code
    log.info("GPS", "start")
    pm.power(pm.GPS, true)
    --Bind uart, the bottom layer automatically processes GNSS data
    --The second parameter is forwarded to the virtual UART to facilitate analysis by the host computer.
    libgnss.bind(gps_uart_id, uart.VUART_0)
    --libgnss.on("raw", function(data)
    ---- Not reported by default, open it yourself if necessary
    --data = data:split("\r\n")
    --if data == nil then
    --         return
    --     end
    -- end)
    sys.wait(200) --It takes time for the GPNSS chip to start up
    --Debug log, optional
    -- libgnss.debug(true)
    --Show serial port configuration
    --uart.write(gps_uart_id, "$CFGPRT,1\r\n")
    -- sys.wait(20)
    --Add displayed sentences
    --uart.write(gps_uart_id, "$CFGMSG,0,1,1\r\n") -- GLL
    -- sys.wait(20)
    --uart.write(gps_uart_id, "$CFGMSG,0,5,1\r\n") -- VTG
    -- sys.wait(20)
    --uart.write(gps_uart_id, "$CFGMSG,0,6,1\r\n") -- ZDA
    -- sys.wait(20)
    exec_agnss()
end)

sys.taskInit(function ()
    while true do
        log.info("nmea", "isFix", libgnss.isFix())
        sys.wait(10000)
    end

end)

--Get time
function gpsMng.getTime()
    local tTime = libgnss.getZda()
    return (string.format("%02d%02d%02d%02d%02d%02d",tTime.year-2000,tTime.month,tTime.day,tTime.hour+8,tTime.min,tTime.sec)):fromHex()
end

return gpsMng
