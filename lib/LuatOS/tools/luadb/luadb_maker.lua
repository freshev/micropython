--[[
luadb maker, for script.bin

--Instructions for use
1. 需要打包的文件存放到  disk 目录
2. 用luatos.exe luadb_maker.lua 执行本脚本
3. 然后就会生成script.bin

--Workflow
1. 遍历disk目录,得到列表
2. 对.lua文件进行luac编译
3. 按luadb格式合成文件

运行所需要的程序:
1. luatos.exe, 可通过bsp/win32编译, 也可以从github action获取现成的
2. luac536.exe 与原版luac.exe的区别是启用了 `#define LUA_32BITS`
]]

--The directory where scripts and other resource files are stored. Subfolders are not allowed.
script_dir = "disk"
--Whether to keep all
debug_all = false

--Traverse files, io.lsdir should also work
local function lsdir(path, files, shortname)
    local exe = io.popen("dir /b " .. (shortname and " " or " /s ") .. path)
    if exe then
        for line in exe:lines() do
            table.insert(files, line)
        end
        exe:close()
    end
end

--Encapsulate the logic of calling local programs
local function oscall(cmd, quite, cwd)
    if cwd and Base_CWD then
        lfs.chdir(cwd)
    end
    if tool_debug then
        log.info("cmd", cmd)
    end
    local exe = io.popen(cmd)
    if exe then
        for line in exe:lines() do
            if not quite then
                log.info("cmd", line)
            end
        end
        exe:close()
    end
    if cwd and Base_CWD then
        lfs.chdir(Base_CWD)
    end
end

--TLD format packaging, Tag - Len - data
function TLD(buff, T, D)
    buff:pack("bb", T, D:len())
    buff:write(D)
end

-----------------------
--- Start formal logic
-----------------------

--Get a list of all files in the disk directory
local files = {}
lsdir(script_dir, files, true)
oscall("mkdir tmp")

--Create the required buffer
local buff = zbuff.create(1024*1024)
local magic = string.char(0x5A, 0xA5, 0X5A, 0xA5)

--Write magic first
--buff:pack("bbbbbb", 0x01, 0x5A, 0xA5, 0X5A, 0xA5)
TLD(buff, 0x01, magic)

--Then there is the version number, currently it is 2
--buff:write(string.char(0x02, 0x02, 0x00, 0x02))
TLD(buff, 0x02, string.char(0x00, 0x02))

--head length, fixed length
buff:write(string.char(0x03, 0x04))
buff:pack("I", 0x12)

--Number of files, based on actual situation
buff:write(string.char(0x04, 0x02))
buff:pack("H", #files)

--CRC value, although it exists, it is not actually verified
buff:write(string.char(0xFE, 0x02))
buff:pack("H", 0xFFFF)

--If it is a lua file, convert it to luac and then add
--If it is another file, add it directly
for _, value in ipairs(files) do
    TLD(buff, 0x01, magic)
    if value:endsWith(".lua") then
        TLD(buff, 0x02, value .. "c")
        --The built-in dump can also be used. However, you have to consider whether size_t and 32bit are enabled.
        --local lf = loadfile(script_dir .. "\\" .. value)
        --local data = string.dump(lf, value == "main.lua" or debug_all)
        io.popen("luac536.exe -s -o tmp.luac " .. script_dir .. "\\" .. value):read("*a")
        local data = io.readFile("tmp.luac")
        TLD(buff, 0x03, pack.pack("I", #data))
        TLD(buff, 0xFE, string.char(0xFF, 0xFF))
        buff:write(data)
    else
        TLD(buff, 0x02, value)
        TLD(buff, 0x03, io.fileSize(script_dir .. "\\" .. value))
        TLD(buff, 0xFE, string.char(0xFF, 0xFF))
        buff:write(io.readFile( script_dir .. "\\" .. value))
    end
end

--Finally get all the data
local data = buff:toStr(0, buff:seek(0, zbuff.SEEK_CUR))
log.info("target size", #data)

--Write to target file
io.writeFile("script.bin", data)

--knock off
os.exit(0)
