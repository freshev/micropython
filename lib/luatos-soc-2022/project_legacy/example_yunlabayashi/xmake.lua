local TARGET_NAME = "example_yunlabayashi"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
includes(SDK_TOP .. "/thirdparty/audio_decoder")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    add_defines("MQTT_TASK",{public = true})

    includes(SDK_TOP .. "/thirdparty/mqtt")
    add_deps("mqtt")
    
    ----Add code and header files
    add_includedirs(SDK_TOP .. "/thirdparty/mqtt/MQTTClient-C/src",{public = true})
    add_files(SDK_TOP .. "/thirdparty/mqtt/MQTTClient-C/src/*.c",{public = true})
    --Add code and header files
     includes(SDK_TOP .. "/thirdparty/httpclient")
    add_deps("httpclient")
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})
    add_deps("audio_decoder")
    --Add code and header files
    add_includedirs("/include",{public = true})
    add_includedirs(SDK_TOP .. "/PLAT/core/tts/include/16k_lite_ver",{public = true})
    --The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
    -- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_16K_lite_beta.a "
    --You can even add your own library
target_end()
