
target("liblcd")
    local LIB_DIR = "$(buildir)/liblcd/"
    set_kind("static")

    set_targetdir(LIB_DIR)

    includes("../u8g2")
    add_deps("libu8g2")

    --Add code and header files
    add_includedirs("./",
                    "../qrcode",
                    "../tjpgd",
    {public = true})

    add_files("./*.c",
                "../qrcode/*.c",
                "../tjpgd/*.c",
    {public = true})

    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libliblcd.a "
target_end()
