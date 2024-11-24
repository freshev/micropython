
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fskvdemo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    sys.wait(1000) --This prevents the logs from being wiped out and is not needed in the production environment.

    --Check if the current firmware supports fskv
    if not fskv then
        while true do
            log.info("fskv", "this demo need fskv")
            sys.wait(1000)
        end
    end

    --Initialize kv database
    fskv.init()
    log.info("fskv", "init complete")
    --Put a bunch of values   first
    local bootime = fskv.get("boottime")
    if bootime == nil or type(bootime) ~= "number" then
        bootime = 0
    else
        bootime = bootime + 1
    end
    fskv.set("boottime", bootime)

    fskv.set("my_bool", true)
    fskv.set("my_int", 123)
    fskv.set("my_number", 1.23)
    fskv.set("my_str", "luatos")
    fskv.set("my_table", {name="wendal",age=18})
    
    fskv.set("my_str_int", "123")
    fskv.set("1", "123") --Single byte key
    --fskv.set("my_nil", nil) -- will prompt failure and does not support null values.


    log.info("fskv", "boottime",      type(fskv.get("boottime")),    fskv.get("boottime"))
    log.info("fskv", "my_bool",      type(fskv.get("my_bool")),    fskv.get("my_bool"))
    log.info("fskv", "my_int",       type(fskv.get("my_int")),     fskv.get("my_int"))
    log.info("fskv", "my_number",    type(fskv.get("my_number")),  fskv.get("my_number"))
    log.info("fskv", "my_str",       type(fskv.get("my_str")),     fskv.get("my_str"))
    log.info("fskv", "my_table",     type(fskv.get("my_table")),   json.encode(fskv.get("my_table")))
    log.info("fskv", "my_str_int",     type(fskv.get("my_str_int")),   fskv.get("my_str_int"))
    log.info("fskv", "1 byte key",     type(fskv.get("1")),   json.encode(fskv.get("1")))

    --Delete test
    fskv.del("my_bool")
    local t = fskv.get("my_bool")
    log.info("fskv", "my_bool",      type(t),    t)

    --Query kv database status
    --local used, total,kv_count = fskv.stat()
    --log.info("fdb", "kv", used,total,kv_count)

    -- fskv.clr()
    --local used, total,kv_count = fskv.stat()
    --log.info("fdb", "kv", used,total,kv_count)
    

    --stress test
    --local start = mcu.ticks()
    --local count = 1000
    --for i=1,count do
    ---- sys.wait(10)
    ---- count = count - 1
    ---- fskv.set("BENT1", "--" .. os.date() .. "--")
    ---- fskv.set("BENT2", "--" .. os.date() .. "--")
    ---- fskv.set("BENT3", "--" .. os.date() .. "--")
    ---- fskv.set("BENT4", "--" .. os.date() .. "--")
    --     fskv.get("my_bool")
    -- end
    --log.info("fskv", mcu.ticks() - start)

    if fskv.sett then
        --Set data, string, numerical value, table, Boolean value, any
        --But it cannot be nil, function, userdata, task
        log.info("fdb", fskv.sett("mytable", "wendal", "goodgoodstudy"))
        log.info("fdb", fskv.sett("mytable", "upgrade", true))
        log.info("fdb", fskv.sett("mytable", "timer", 1))
        log.info("fdb", fskv.sett("mytable", "bigd", {name="wendal",age=123}))
        
        --The following statement will print out a table of 4 elements
        log.info("fdb", fskv.get("mytable"), json.encode(fskv.get("mytable")))
        --Note: If the key does not exist, or the original value is not of table type, it will be completely overwritten.
        --For example, if you write the following method, you will get the table instead of the string in the first line.
        log.info("fdb", fskv.set("mykv", "123"))
        log.info("fdb", fskv.sett("mykv", "age", "123")) --What is saved will be {age:"123"}

        --Delete test
        log.info("fdb", fskv.set("mytable", {age=18, name="wendal"}))
        log.info("fdb", fskv.sett("mytable", "name", nil))
        log.info("fdb", fskv.get("mytable"), json.encode(fskv.get("mytable")))
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
