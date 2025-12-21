local TARGET_NAME = "web_led"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    local LIB_DIR = "$(buildir)/mqttclient/"
    set_kind("static")
    set_targetdir(LIB_DIR)
    
    add_defines("MQTT_TASK",{public = true})

    includes(SDK_TOP .. "/thirdparty/mqtt")
    add_deps("mqtt")
    
    ----Add code and header files
    add_includedirs(SDK_TOP .. "/thirdparty/mqtt/MQTTClient-C/src",{public = true})
    add_files(SDK_TOP .. "/thirdparty/mqtt/MQTTClient-C/src/*.c",{public = true})
    add_includedirs(SDK_TOP .. "thirdparty/cjson")
    add_files(SDK_TOP .. "/thirdparty/cJSON/*.c",{public = true})

    --Add your own code and header files
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    
    --You can even add your own library
target_end()
