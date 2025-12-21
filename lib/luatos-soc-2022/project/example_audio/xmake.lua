local TARGET_NAME = "example_audio"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
includes(SDK_TOP .. "/thirdparty/audio_decoder")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    add_deps("audio_decoder")

    includes(SDK_TOP .. "/thirdparty/miniz")
    add_deps("miniz")
    
    --Add code and header files
    add_includedirs("./include",{public = true})
    add_files("./src/*.c",{public = true})

    --The path can be written casually, and the code for any path can be added. The following code is equivalent to the above code.
    -- add_includedirs(SDK_TOP .. "project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "project/" .. TARGET_NAME .. "/src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_16K.a "
    --Use the following library for the 8K version, comment out the 16K library
    --LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_8K.a "
    --For the 8K English version, use the following library and comment out the 16K library.
    --LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_8K_eng.a "
    --For the 16K English version, use the following library and comment out the 16K library.
    --LIB_USER = LIB_USER .. SDK_TOP .. "/PLAT/core/lib/libaisound50_16K_eng.a "
    --You can even add your own library
target_end()
