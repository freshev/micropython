local TARGET_NAME = "example_https"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
includes(SDK_TOP.."/thirdparty/libhttp")
includes(SDK_TOP.."/luatos_lwip_socket")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    add_deps("libhttp") --Added HTTP client support and automatically loaded socket dependencies
    --Add code and header files
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
    -- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()
