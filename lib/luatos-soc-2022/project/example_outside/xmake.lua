--[[
本example演示在SDK目录之外存放项目代码

目录结构演示如下:

其中: 

SDK所在目录 D:\github\luatos-soc-ec618
项目代码所在目录 D:\github\ec618-webabc

D:\github
    - luatos-soc-ec618
        - build.bat
        - PLAT
        - ...
    - ec618-webabc
        - code
            - xmake.lua
            - src
                app_main.c
                task_mqtt.c
            - inc
                app_main.h
                RTE_Device.h
        - doc
            - README.md

]]

--Both relative paths and absolute paths are available
local PROJECT_DIR = "D:/github/ec618-webabc"
--local PROJECT_DIR = "../../ec618-webabc"

--Just give the path to the project code xmake.lua
includes(PROJECT_DIR .. "/code/xmake.lua")

--xmake.lua in the project code requires TARGET_NAME to match
--For example, the path of this example in the SDK is project/example_outside, then the TARGET_NAME in the project is example_outside

--[[
local TARGET_NAME = "example_outside"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    
    --Add code and header files
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --add_files("../../thirdparty/fal/src/*.c",{public = true})
    --add_files("../../thirdparty/flashdb/src/*.c",{public = true})

    --You can continue to add add_includedirs and add_files
    --automatic link
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --You can even add your own library
target_end()
]]

--[[
为了防止误提交到主库, 可在当前目录添加.gitignore, 忽略全部文件, 然后强制添加xmake.lua
git add -f project/example_outside/xmake.lua

.gitignore文件的内容, 本目录有参考, 就一个字符 "*"
]]
