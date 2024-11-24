
target("audio_decoder")
    local LIB_DIR = "$(buildir)/audio_decoder/"
    set_kind("static")
    set_targetdir(LIB_DIR)
    
    --Add code and header files
    add_includedirs("./mp3/include",{public = true})
    add_includedirs("./amr/amr_common/dec/include",{public = true})
    add_includedirs("./amr/amr_nb/common/include",{public = true})
    add_includedirs("./amr/amr_nb/dec/include",{public = true})
    add_includedirs("./amr/amr_wb/dec/include",{public = true})
    add_includedirs("./amr/opencore-amrnb",{public = true})
    add_includedirs("./amr/opencore-amrwb",{public = true})
    add_includedirs("./amr/oscl",{public = true})
    add_includedirs("./amr/amr_nb/enc/src",{public = true})
    --**.c will recurse files in all subfolders
    add_files("./amr/**.c",{public = true})

    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libaudio_decoder.a "
	LIB_USER = LIB_USER .. SDK_TOP .. "/lib/libmp3.a "
target_end()
