local TARGET_NAME = "example_cjson"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    
    --Add code and header files
    add_includedirs("./inc",{public = true})
    add_includedirs(SDK_TOP .. "/thirdparty/cJSON")
    add_files(SDK_TOP .. "/thirdparty/cJSON/**.c")
    add_files("./src/**.c")
    --The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
    -- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})
    -- add_defines("MBEDTLS_TCPIP_LWIP",{public = true})
    --Link on demand mbedtls
    -- add_defines("MBEDTLS_CONFIG_FILE=\"config_ec_ssl_comm.h\"")
    --add_files(SDK_TOP .. "PLAT/middleware/thirdparty/mbedtls/library/*.c")

    --You can continue to add add_includedirs and add_files
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME
    --You can even add your own library
target_end()
