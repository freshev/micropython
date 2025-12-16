local TARGET_NAME = "cloud_speaker_pahomqtt"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
includes(SDK_TOP .. "/thirdparty/audio_decoder")
target(TARGET_NAME)
    local LIB_DIR = "$(buildir)/mqttclient/"
    set_kind("static")
    set_targetdir(LIB_DIR)
    add_deps("audio_decoder")
    add_defines("MQTT_TASK",{public = true})

    includes(SDK_TOP .. "/thirdparty/mqtt")
    add_deps("mqtt")
    includes(SDK_TOP .. "/thirdparty/cJSON")
    ----Add code and header files
    add_includedirs(SDK_TOP .. "/thirdparty/mqtt/MQTTClient-C/src",{public = true})
    add_files(SDK_TOP .. "/thirdparty/mqtt/MQTTClient-C/src/*.c",{public = true})
    add_includedirs(SDK_TOP .. "/PLAT/core/tts/include/16k_lite_ver",{public = true})
    --Add your own code and header files
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    -- add_includedirs("../../thirdparty/fal/inc",{public = true})
    -- add_includedirs("../../thirdparty/flashdb/inc",{public = true})
    -- add_includedirs("../../thirdparty/am_kv/inc",{public = true})
    -- add_files("../../thirdparty/fal/src/*.c",{public = true})
    -- add_files("../../thirdparty/flashdb/src/*.c",{public = true})
    -- add_files("../../thirdparty/am_kv/src/*.c",{public = true})
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_16K.a "
    --You can even add your own library
target_end()
