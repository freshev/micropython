
target("ymodem")
local LIB_DIR = "$(buildir)/cJymodemSON/"
set_kind("static")
set_targetdir(LIB_DIR)

--Add code and header files
    add_includedirs(SDK_TOP .. "/thirdparty/sfud",{public = true})
	add_includedirs(SDK_TOP .. "/thirdparty/vfs",{public = true})
    add_files(SDK_TOP.."/thirdparty/sfud/*.c",{public = true})
    add_includedirs(USER_PROJECT_DIR .. "/inc",{public = true})

    add_includedirs("./",{public = true})
    add_files("./luat_ymodem.c",{public = true})
    add_includedirs(SDK_TOP .. "/thirdparty/ymodem",{public = true})
--The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
-- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
-- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

--You can continue to add add_includedirs and add_files
--automatic link
LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libymodem.a "
target_end()
