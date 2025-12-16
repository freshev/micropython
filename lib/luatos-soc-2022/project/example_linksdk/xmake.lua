local TARGET_NAME = "example_linksdk"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    --Use third-party linksdk
    includes(SDK_TOP .. "/thirdparty/linksdk")
    add_deps("linksdk")
    --Add code and header files
    add_includedirs("./inc",
                    SDK_TOP .. "/thirdparty/linksdk/core",{public = true})
    --add_files("./src/example_sysdep_api_test.c",{public = true})
    add_files("./src/example_mqtt_basic.c",{public = true})

    --The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
    -- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link c

    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()
