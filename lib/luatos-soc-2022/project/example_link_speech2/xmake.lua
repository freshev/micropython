local TARGET_NAME = "example_link_speech2"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
includes(SDK_TOP .. "/thirdparty/audio_decoder")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    --Use third-party linksdk
    includes(SDK_TOP .. "/thirdparty/linksdk")
    add_deps("linksdk")
    add_deps("audio_decoder")
    --Add code and header files
    add_includedirs("./inc", SDK_TOP .. "/thirdparty/linksdk/core",{public = true})
    add_files("./src/link_speech_basic_demo.c",{public = true})

    --The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
    -- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link c

    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()
