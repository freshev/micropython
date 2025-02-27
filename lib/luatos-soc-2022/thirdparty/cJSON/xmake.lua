
target("cJSON")
local LIB_DIR = "$(buildir)/cJSON/"
set_kind("static")
set_targetdir(LIB_DIR)

--Add code and header files
add_includedirs("./",{public = true})
add_files("./*.c",{public = true})
add_includedirs(SDK_TOP .. "/thirdparty/cJSON",{public = true})
--The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
-- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
-- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

--You can continue to add add_includedirs and add_files
--automatic link
LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libcJSON.a "
target_end()
