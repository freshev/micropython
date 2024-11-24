local TARGET_NAME = "example_luatos_socket"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

local LUATOS_ROOT = SDK_TOP .. "/../LuatOS/"
includes(SDK_TOP.."/luatos_lwip_socket")
target(TARGET_NAME)
    set_kind("static")
    add_deps("luatos_lwip_socket")
    set_targetdir(LIB_DIR)
    --Use the following library for the 8K version, comment out the 16K library
    --LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_8K.a "
    --Add code and header files
    add_includedirs("./inc",{public = true})
    --add_includedirs(SDK_TOP .. "/interface/private_include",
    --                 {public = true})
    add_files("./src/*.c",{public = true})
    

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()
