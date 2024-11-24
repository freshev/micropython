
target("libu8g2")
    local LIB_DIR = "$(buildir)/libu8g2/"
    set_kind("static")

    set_targetdir(LIB_DIR)

    --Add code and header files
    add_includedirs("./",{public = true})
    add_files("./*.c",{public = true})

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "liblibu8g2.a "
target_end()
