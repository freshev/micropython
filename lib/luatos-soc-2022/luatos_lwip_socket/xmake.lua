target("luatos_lwip_socket")
    local LIB_DIR = "$(buildir)/luatos_lwip_socket/"
    set_kind("static")
    set_targetdir(LIB_DIR)
    
    --Add code and header files
    add_includedirs("./include",{public = true})
    add_files("./**.c",{public = true})
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libluatos_lwip_socket.a "
target_end()
