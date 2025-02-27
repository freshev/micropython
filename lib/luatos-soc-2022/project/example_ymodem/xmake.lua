local TARGET_NAME = "example_ymodem"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)

    includes(SDK_TOP .. "/thirdparty/ymodem")
    add_deps("ymodem")

    --Add code and header files
    add_includedirs(SDK_TOP .. "/thirdparty/sfud",{public = true})
	add_includedirs(SDK_TOP .. "/thirdparty/vfs",{public = true})

	add_files(SDK_TOP.."/thirdparty/vfs/*.c|luat_fs_fatfs.c",{public = true})
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

-- add_includedirs(SDK_TOP .. "/thirdparty/sfud",{public = true})
-- add_files(SDK_TOP.."/thirdparty/sfud/*.c",{public = true})
