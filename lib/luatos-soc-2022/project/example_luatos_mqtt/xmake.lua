local TARGET_NAME = "example_luatos_mqtt"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

includes(SDK_TOP.."/luatos_lwip_socket")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)

    includes(SDK_TOP .. "/thirdparty/libemqtt")
    add_deps("libemqtt")

    add_includedirs("./inc",{public = true})
    --add_includedirs(SDK_TOP .. "/interface/private_include",
    --                 {public = true})
    add_files("./src/*.c",{public = true})
    
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()