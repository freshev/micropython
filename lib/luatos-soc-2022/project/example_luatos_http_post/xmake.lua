local TARGET_NAME = "example_luatos_http_post"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
includes(SDK_TOP.."/luatos_lwip_socket")
includes(SDK_TOP.."/thirdparty/libhttp")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    --add_deps("luatos_lwip_socket") --socket dependency
	add_deps("libhttp") --Added HTTP client support and automatically loaded socket dependencies
    --Add code and header files
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})
	

    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
target_end()
