
target("mqtt")
    local LIB_DIR = "$(buildir)/mqtt/"
    set_kind("static")
    set_targetdir(LIB_DIR)

    add_defines("FEATURE_MQTT_TLS_ENABLE",{public = true})
    
    --Add code and header files
    add_includedirs("./",
                    SDK_TOP .. "thirdparty/mbedtls/include/mbedtls",
                    SDK_TOP .. "thirdparty/cjson",
                    "./MQTTPacket/src",
                    "./MQTTClient-C/src/FreeRTOS",
    {public = true})

    add_files("./MQTTPacket/src/*.c",
                "./MQTTClient-C/src/FreeRTOS/*.c",
    {public = true})

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libmqtt.a "
target_end()
