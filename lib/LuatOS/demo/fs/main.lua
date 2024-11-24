
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fsdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

local function fs_test()
    --root/is writable
    local f = io.open("/boot_time", "rb")
    local c = 0
    if f then
        local data = f:read("*a")
        log.info("fs", "data", data, data:toHex())
        c = tonumber(data)
        f:close()
    end
    log.info("fs", "boot count", c)
    c = c + 1
    f = io.open("/boot_time", "wb")
    --if f ~= nil then
    log.info("fs", "write c to file", c, tostring(c))
    f:write(tostring(c))
    f:close()
    --end

    log.info("io.writeFile", io.writeFile("/abc.txt", "ABCDEFG"))

    log.info("io.readFile", io.readFile("/abc.txt"))
    local f = io.open("/abc.txt", "rb")
    local c = 0
    if f then
        local data = f:read("*a")
        log.info("fs", "data2", data, data:toHex())
        f:close()
    end

    --seek and tell test
    local f = io.open("/abc.txt", "rb")
    local c = 0
    if f then
        f:seek("end", 0)
        f:seek("set", 0)
        local data = f:read("*a")
        log.info("fs", "data3", data, data:toHex())
        f:close()
    end

    if fs then
        --The root directory is readable and writable
        log.info("fsstat", fs.fsstat("/"))
        --/luadb/ is read-only
        log.info("fsstat", fs.fsstat("/luadb/"))
    end

    local ret, files = io.lsdir("/")
    log.info("fs", "lsdir", json.encode(files))

    ret, files = io.lsdir("/luadb/")
    log.info("fs", "lsdir", json.encode(files))

    --Read the files added during flashing and demonstrate line-by-line reading
    --The non-lua files selected when flashing are all stored in the /luadb/ directory, with no subfolders in a single layer.
    f = io.open("/luadb/abc.txt", "rb")
    if f then
        while true do
            local line = f:read("l")
            if not line or #line == 0 then
                break
            end
            log.info("fs", "read line", line)
        end
        f:close()
        log.info("fs", "close f")
    else
        log.info("fs", "pls add abc.txt!!")
    end

    --Folder operations
    sys.wait(3000)
    io.mkdir("/iot/")
    f = io.open("/iot/1.txt", "w+")
    if f then
        f:write("hi, LuatOS " .. os.date())
        f:close()
    else
        log.info("fs", "open file for write failed")
    end
    f = io.open("/iot/1.txt", "r")
    if f then
        local data = f:read("*a")
        f:close()
        log.info("fs", "writed data", data)
    else
        log.info("fs", "open file for read failed")
    end

    --2023.6.6 Added io.readFile to support configuring the starting position and length
    io.writeFile("/test.txt", "0123456789")
    log.info("stream", io.readFile("/test.txt", "rb", 3, 5))
end



sys.taskInit(function()
    --In order to display the log, there is a delay of one second here.
    --No delay required for normal use
    sys.wait(1000)
    fs_test()
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
