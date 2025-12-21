local TARGET_NAME = "luatos"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

local LUATOS_ROOT = SDK_TOP .. "/../LuatOS/"
local LUAT_USE_TTS_8K

target("gmssl")
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_optimize("fastest")

    add_cxflags("-finline")
    
    --Add code and header files
    add_includedirs("./inc",{public = true})
    add_includedirs(LUATOS_ROOT .. "lua/include")
    add_includedirs(LUATOS_ROOT .. "luat/include")
    add_includedirs(LUATOS_ROOT .. "components/cmux")
    add_includedirs(LUATOS_ROOT .. "components/cjson")
    add_includedirs(LUATOS_ROOT .. "components/fatfs")
    add_includedirs(LUATOS_ROOT .. "components/shell")

    --National secret algorithm, by chenxudong1208, based on GMSSL
    add_includedirs(LUATOS_ROOT.."components/gmssl/include")
    add_files(LUATOS_ROOT.."components/gmssl/**.c")

    LIB_USER = LIB_USER .. SDK_TOP .. "/" .. LIB_DIR .. "libgmssl.a "
target_end()

target("nes")
    set_kind("static")
    set_plat("cross")
    set_optimize("fastest")
    set_targetdir(LIB_DIR)
    add_includedirs("./inc",{public = true})
    add_includedirs(LUATOS_ROOT.."lua/include")
    add_includedirs(LUATOS_ROOT.."luat/include")
    add_includedirs(LUATOS_ROOT.."components/lcd",{public = true})
    add_includedirs(LUATOS_ROOT.."components/u8g2",{public = true})
    add_includedirs(LUATOS_ROOT.."components/nes/inc")
    add_includedirs(LUATOS_ROOT.."components/nes/port")
    add_files(LUATOS_ROOT.."components/nes/**.c")
    LIB_USER = LIB_USER .. SDK_TOP .. "/" .. LIB_DIR .. "libnes.a "
target_end()


target("fastlz")
    set_kind("static")
    set_plat("cross")
    set_optimize("fastest")
    set_targetdir(LIB_DIR)
    add_includedirs("./inc",{public = true})
    add_includedirs(LUATOS_ROOT.."lua/include")
    add_includedirs(LUATOS_ROOT.."luat/include")
    add_includedirs(LUATOS_ROOT.."components/lcd",{public = true})
    add_includedirs(LUATOS_ROOT.."components/u8g2",{public = true})
    add_includedirs(LUATOS_ROOT.."components/fastlz")
    add_files(LUATOS_ROOT.."components/fastlz/**.c")
    LIB_USER = LIB_USER .. SDK_TOP .. "/" .. LIB_DIR .. "libfastlz.a "
target_end()

add_includedirs(LUATOS_ROOT.."components/printf",{public = true})

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)

    on_load(function (target)
        local conf_data = io.readfile("$(projectdir)/project/luatos/inc/luat_conf_bsp.h")
        LUAT_USE_TTS_8K = conf_data:find("\r#define LUAT_USE_TTS_8K") or conf_data:find("\n#define LUAT_USE_TTS_8K")
        if LUAT_USE_TTS_8K then
            target:add("includedirs","$(projectdir)/PLAT/core/tts/include/8k_lite_ver")
            target:add("files","$(projectdir)/PLAT/core/lib/libaisound50_8K.a")
        else 
            target:add("includedirs","$(projectdir)/PLAT/core/tts/include/16k_lite_ver")
            target:add("files","$(projectdir)/PLAT/core/lib/libaisound50_16K.a")
        end

        local conf_data = io.readfile("$(projectdir)/project/luatos/inc/luat_conf_bsp.h")
        LUAT_USE_TTS_8K = conf_data:find("\r#define LUAT_USE_ANTBOT") or conf_data:find("\n#define LUAT_USE_ANTBOT")
        target:add("files","$(projectdir)/lib/libbot.a")
    end)

    --Add code and header files
    add_includedirs("./inc",{public = true})
    --add_includedirs(SDK_TOP .. "/interface/private_include",
    --                 {public = true})
    add_files("./src/*.c",{public = true})
    add_files(SDK_TOP .. "/interface/src/*.c",{public = true})
	add_files(SDK_TOP .. "/thirdparty/littlefs/**.c",{public = true})
	
    add_includedirs(LUATOS_ROOT .. "lua/include")
    add_includedirs(LUATOS_ROOT .. "luat/include")
    add_includedirs(LUATOS_ROOT .. "components/cmux")
    add_includedirs(LUATOS_ROOT .. "components/cjson")
    add_includedirs(LUATOS_ROOT .. "components/fatfs")
    add_includedirs(LUATOS_ROOT .. "components/shell")
	add_includedirs(LUATOS_ROOT .. "components/camera")

    add_files(LUATOS_ROOT .. "lua/src/*.c")
    add_files(LUATOS_ROOT .. "luat/modules/*.c")
    add_files(LUATOS_ROOT .. "luat/vfs/*.c")
    add_files(LUATOS_ROOT .. "components/cmux/*.c")
    add_files(LUATOS_ROOT .. "components/cjson/*.c")
    add_files(LUATOS_ROOT .. "components/crypto/*.c")
    
    -- weak
    add_files(LUATOS_ROOT.."/luat/weak/luat_spi_device.c",
            LUATOS_ROOT.."/luat/weak/luat_malloc_weak.c",
            LUATOS_ROOT.."/luat/weak/luat_mem_weak.c")
    -- printf
    add_includedirs(LUATOS_ROOT.."components/printf",{public = true})
    add_files(LUATOS_ROOT.."components/printf/*.c")

    -- gtfont
    add_includedirs(LUATOS_ROOT.."components/gtfont")
    add_files(LUATOS_ROOT.."components/gtfont/*.c")
    LIB_USER = LIB_USER .. SDK_TOP .. "/lib/libgt.a "

    -- coremark
    add_files(LUATOS_ROOT .. "components/coremark/*.c")
    add_includedirs(LUATOS_ROOT .. "components/coremark")

    -- lua-cjson
    add_files(LUATOS_ROOT .. "components/lua-cjson/*.c")
    add_includedirs(LUATOS_ROOT .. "components/lua-cjson")

    -- miniz
    add_files(LUATOS_ROOT .. "components/miniz/*.c")
    add_includedirs(LUATOS_ROOT .. "components/miniz")

    -- protobuf
    add_includedirs(LUATOS_ROOT.."components/serialization/protobuf")
    add_files(LUATOS_ROOT.."components/serialization/protobuf/*.c")

    -- fdb
    add_includedirs(SDK_TOP.."/thirdparty/fal/inc")
    add_includedirs(SDK_TOP.."/thirdparty/flashdb/inc")
    add_files(SDK_TOP.."/thirdparty/fal/src/*.c")
    add_files(SDK_TOP.."/thirdparty/flashdb/src/*.c")
    add_files(LUATOS_ROOT.."components/flashdb/src/luat_lib_fdb.c")

    -- rsa
    add_files(LUATOS_ROOT.."components/rsa/**.c")

    -- iotauth
    -- add_includedirs(LUATOS_ROOT.."components/iotauth")
    add_files(LUATOS_ROOT.."components/iotauth/**.c")

    -- sfud
    add_includedirs(LUATOS_ROOT.."components/sfud")
    add_files(LUATOS_ROOT.."components/sfud/**.c")

    -- fatfs
    add_includedirs(LUATOS_ROOT.."components/fatfs")
    add_files(LUATOS_ROOT.."components/fatfs/**.c")

    -- libgnss
    add_includedirs(LUATOS_ROOT .. "components/minmea")
    add_files(LUATOS_ROOT.."components/minmea/**.c")

    -- mlx90640
    add_files(LUATOS_ROOT.."components/mlx90640-library/*.c")
    add_includedirs(LUATOS_ROOT.."components/mlx90640-library")

    
    --Antbot, the library itself is closed source, but the integration into the Lua library is open source
    add_files(LUATOS_ROOT.."components/antbot/binding/*.c")
    add_includedirs(LUATOS_ROOT.."components/antbot/include")

    --xxtea library
    add_files(LUATOS_ROOT.."components/xxtea/src/*.c")
    add_files(LUATOS_ROOT.."components/xxtea/binding/*.c")
    add_includedirs(LUATOS_ROOT.."components/xxtea/include")

    --------------------------------------------------------------
    
    add_includedirs(LUATOS_ROOT.."lua/include")
    add_includedirs(LUATOS_ROOT.."luat/include")
    add_includedirs(LUATOS_ROOT.."components/lcd")
    add_includedirs(LUATOS_ROOT.."components/tjpgd")
    add_includedirs(LUATOS_ROOT.."components/eink")
    add_includedirs(LUATOS_ROOT.."components/epaper")
    add_includedirs(LUATOS_ROOT.."components/u8g2")
    add_includedirs(LUATOS_ROOT.."components/gtfont")
    add_includedirs(LUATOS_ROOT.."components/qrcode")

    add_includedirs(LUATOS_ROOT.."components/lvgl")
    add_includedirs(LUATOS_ROOT.."components/lvgl/binding")
    add_includedirs(LUATOS_ROOT.."components/lvgl/gen")
    add_includedirs(LUATOS_ROOT.."components/lvgl/src")
    add_includedirs(LUATOS_ROOT.."components/lvgl/font")
    add_includedirs(LUATOS_ROOT.."components/lvgl/src/lv_font")

    -- lvgl
    add_files(LUATOS_ROOT.."components/lvgl/**.c")
    --LV demos are not compiled by default, saving a lot of compilation time.
    remove_files(LUATOS_ROOT.."components/lvgl/lv_demos/**.c")

    -- eink
    add_files(LUATOS_ROOT.."components/eink/*.c")
    add_files(LUATOS_ROOT.."components/epaper/*.c")
    remove_files(LUATOS_ROOT.."components/epaper/GUI_Paint.c")

    -- u8g2
    add_files(LUATOS_ROOT.."components/u8g2/*.c")

    -- lcd
    add_files(LUATOS_ROOT.."components/lcd/*.c")
    add_files(LUATOS_ROOT.."components/tjpgd/*.c")
    add_files(LUATOS_ROOT.."components/qrcode/*.c")
    

    -- network
    add_includedirs(LUATOS_ROOT .. "components/ethernet/common", {public = true})
    add_includedirs(LUATOS_ROOT .. "components/ethernet/w5500", {public = true})
    add_includedirs(LUATOS_ROOT .. "components/common", {public = true})
    add_includedirs(LUATOS_ROOT .. "components/network/adapter", {public = true})
    add_includedirs(LUATOS_ROOT .. "components/mbedtls/include", {public = true})
    add_includedirs(LUATOS_ROOT .. "components/mbedtls/library", {public = true})
    add_files(LUATOS_ROOT.."components/ethernet/**.c")
    add_files(LUATOS_ROOT .. "components/ethernet/w5500/*.c")
    add_files(LUATOS_ROOT .. "components/network/adapter/*.c")
    add_files(LUATOS_ROOT .. "components/mbedtls/library/*.c")

    -- mqtt
    add_includedirs(LUATOS_ROOT.."components/network/libemqtt")
    add_files(LUATOS_ROOT.."components/network/libemqtt/*.c")

    -- http
    add_includedirs(LUATOS_ROOT.."components/network/libhttp")
    -- add_files(LUATOS_ROOT.."components/network/libhttp/luat_lib_http.c")
    add_files(LUATOS_ROOT.."components/network/libhttp/*.c")

    -- http_parser
    add_includedirs(LUATOS_ROOT.."components/network/http_parser")
    add_files(LUATOS_ROOT.."components/network/http_parser/*.c")

    -- websocket
    add_includedirs(LUATOS_ROOT.."components/network/websocket")
    add_files(LUATOS_ROOT.."components/network/websocket/*.c")

    -- errdump
    add_includedirs(LUATOS_ROOT.."components/network/errdump")
    add_files(LUATOS_ROOT.."components/network/errdump/*.c")

    -- httpsrv
    add_includedirs(LUATOS_ROOT.."components/network/httpsrv/inc")
    add_files(LUATOS_ROOT.."components/network/httpsrv/src/*.c")

    -- iotauth
    add_includedirs(LUATOS_ROOT.."components/iotauth", {public = true})
    add_files(LUATOS_ROOT.."components/iotauth/*.c")

    -- mobile
    add_includedirs(LUATOS_ROOT.."components/mobile")
    add_files(LUATOS_ROOT.."components/mobile/*.c")

    
    -- shell
    add_files(LUATOS_ROOT.."components/shell/*.c")

    -- i2c-tools
    add_includedirs(LUATOS_ROOT.."components/i2c-tools")
    add_files(LUATOS_ROOT.."components/i2c-tools/*.c")

    -- lora
    add_includedirs(LUATOS_ROOT.."components/lora/sx126x")
    add_files(LUATOS_ROOT.."components/lora/**.c")

    -- lora2
    add_includedirs(LUATOS_ROOT.."components/lora2/sx126x")
    add_files(LUATOS_ROOT.."components/lora2/**.c")
    
    -- fonts
    add_includedirs(LUATOS_ROOT.."components/luatfonts")
    add_files(LUATOS_ROOT.."components/luatfonts/**.c")

    
    -- wlan
    add_includedirs(LUATOS_ROOT.."components/wlan")
    add_files(LUATOS_ROOT.."components/wlan/**.c")
    -- audio
    add_includedirs(LUATOS_ROOT.."components/multimedia/")
    add_includedirs(LUATOS_ROOT.."components/multimedia/mp3_decode")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/amr_common/dec/include")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/amr_nb/common/include")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/amr_nb/dec/include")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/amr_wb/dec/include")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/opencore-amrnb")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/opencore-amrwb")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/oscl")
    add_includedirs(LUATOS_ROOT.."components/multimedia/amr_decode/amr_nb/enc/src")
    add_files(LUATOS_ROOT.."components/multimedia/**.c")
    ------------------------------------------------------------
    ------------------------------------------------------------
    -- sms
    add_includedirs(LUATOS_ROOT.."components/sms")
    add_files(LUATOS_ROOT.."components/sms/*.c")

    -- hmeta
    add_includedirs(LUATOS_ROOT.."components/hmeta")
    add_files(LUATOS_ROOT.."components/hmeta/*.c")

    -- profiler
    add_includedirs(LUATOS_ROOT.."components/mempool/profiler/include")
    add_files(LUATOS_ROOT.."components/mempool/profiler/**.c")

    -- fskv
    add_includedirs(LUATOS_ROOT.."components/fskv")
    add_files(LUATOS_ROOT.."components/fskv/*.c")

    -- sntp
    add_includedirs(LUATOS_ROOT.."components/network/libsntp")
    add_files(LUATOS_ROOT.."components/network/libsntp/*.c")

    -- libftp
    add_includedirs(LUATOS_ROOT.."components/network/libftp")
    add_files(LUATOS_ROOT.."components/network/libftp/*.c")

    -- sfd
    add_includedirs(LUATOS_ROOT.."components/sfd")
    add_files(LUATOS_ROOT.."components/sfd/*.c")

    -- fatfs
    add_includedirs(LUATOS_ROOT.."components/fatfs")
    add_files(LUATOS_ROOT.."components/fatfs/*.c")

    -- iconv
    add_includedirs(LUATOS_ROOT.."components/iconv")
    add_files(LUATOS_ROOT.."components/iconv/*.c")
    remove_files(LUATOS_ROOT.."components/iconv/luat_iconv.c")

    add_files(LUATOS_ROOT.."components/max30102/*.c")
    add_includedirs(LUATOS_ROOT.."components/max30102")


    add_files(LUATOS_ROOT.."components/ymodem/*.c")
    add_includedirs(LUATOS_ROOT.."components/ymodem")

    add_files(LUATOS_ROOT.."components/repl/*.c")
    add_includedirs(LUATOS_ROOT.."components/repl")

    add_files(LUATOS_ROOT.."components/statem/*.c")
    add_includedirs(LUATOS_ROOT.."components/statem")

    -- ercoap
    add_includedirs(LUATOS_ROOT.."components/network/ercoap/include",{public = true})
    add_files(LUATOS_ROOT.."components/network/ercoap/src/*.c")
    add_files(LUATOS_ROOT.."components/network/ercoap/binding/*.c")
    
    -- sqlite3
    add_includedirs(LUATOS_ROOT.."components/sqlite3/include",{public = true})
    add_files(LUATOS_ROOT.."components/sqlite3/src/*.c")
    add_files(LUATOS_ROOT.."components/sqlite3/binding/*.c")
    
    --ws2812 separate library
    add_includedirs(LUATOS_ROOT.."components/ws2812/include",{public = true})
    add_files(LUATOS_ROOT.."components/ws2812/src/*.c")
    add_files(LUATOS_ROOT.."components/ws2812/binding/*.c")

    -- ulwip
    add_includedirs(LUATOS_ROOT.."components/network/ulwip/include",{public = true})
    add_files(LUATOS_ROOT.."components/network/ulwip/src/*.c")
    add_files(LUATOS_ROOT.."components/network/ulwip/binding/*.c")
    add_includedirs(LUATOS_ROOT.."components/network/adapter_lwip2",{public = true})
    add_files(LUATOS_ROOT.."components/network/adapter_lwip2/*.c")

    --ht1621 LCD driver
    add_includedirs(LUATOS_ROOT.."components/ht1621/include",{public = true})
    add_files(LUATOS_ROOT.."components/ht1621/src/*.c")
    add_files(LUATOS_ROOT.."components/ht1621/binding/*.c")

    
    --As a final supplement, otherwise it will always report header files that the main library does not have.
    add_includedirs(SDK_TOP .. "/interface/include")

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. "/" .. LIB_DIR .. LIB_NAME .. " "
    --LIB_USER = LIB_USER .. SDK_TOP .. "/" .. LIB_DIR .. "libgmssl.a" .. " "
    --You can even add your own library
	LIB_USER = LIB_USER .. SDK_TOP .. "/lib/libmp3.a "
target_end()
