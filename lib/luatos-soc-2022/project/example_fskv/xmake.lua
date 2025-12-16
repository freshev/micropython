local TARGET_NAME = "example_fskv"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)

    
    --Add code and header files
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})
	
	add_includedirs(SDK_TOP .. "/thirdparty/fskv",{public = true})
    add_files(SDK_TOP .. "/thirdparty/fskv/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()
