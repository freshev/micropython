target("libhttp")
    local LIB_DIR = "$(buildir)/libhttp/"
    set_kind("static")
    add_deps("luatos_lwip_socket")
    set_targetdir(LIB_DIR)

    --Add code and header files
    add_includedirs("./",
                    SDK_TOP.."/luatos_lwip_socket/include",
    {public = true})

    add_files("./*.c",{public = true})

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "liblibhttp.a "
target_end()