target("minmea")
local LIB_DIR = "$(buildir)/minmea/"
set_kind("static")
set_targetdir(LIB_DIR)

-- add_defines("FEATURE_MQTT_TLS_ENABLE",{public = true})

--Add code and header files
add_includedirs("./",{public = true})
add_files("./*.c",{public = true})

--automatic link
LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "libminmea.a "
target_end()
