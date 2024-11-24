
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fdbdemo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    sys.wait(1000) --This prevents the logs from being flushed and is not needed in the production environment.

    --Check if the current firmware supports fdb
    if not fdb then
        while true do
            log.info("fdb", "this demo need fdb")
            sys.wait(1000)
        end
    end

    --Initialize kv database
    fdb.kvdb_init("onchip_flash")
    log.info("fdb", "init complete")
    --Put a bunch of values   first
    local bootime = fdb.kv_get("boottime")
    if bootime == nil or type(bootime) ~= "number" then
        bootime = 0
    else
        bootime = bootime + 1
    end
    fdb.kv_set("boottime", bootime)

    fdb.kv_set("my_bool", true)
    fdb.kv_set("my_int", 123)
    fdb.kv_set("my_number", 1.23)
    fdb.kv_set("my_str", "luatos")
    fdb.kv_set("my_table", {name="wendal",age=18})
    
    fdb.kv_set("my_str_int", "123")
    fdb.kv_set("1", "123") --Single byte key
    --fdb.kv_set("my_nil", nil) -- will prompt failure and does not support null values.


    log.info("fdb", "boottime",      type(fdb.kv_get("boottime")),    fdb.kv_get("boottime"))
    log.info("fdb", "my_bool",      type(fdb.kv_get("my_bool")),    fdb.kv_get("my_bool"))
    log.info("fdb", "my_int",       type(fdb.kv_get("my_int")),     fdb.kv_get("my_int"))
    log.info("fdb", "my_number",    type(fdb.kv_get("my_number")),  fdb.kv_get("my_number"))
    log.info("fdb", "my_str",       type(fdb.kv_get("my_str")),     fdb.kv_get("my_str"))
    log.info("fdb", "my_table",     type(fdb.kv_get("my_table")),   json.encode(fdb.kv_get("my_table")))
    log.info("fdb", "my_str_int",     type(fdb.kv_get("my_str_int")),   fdb.kv_get("my_str_int"))
    log.info("fdb", "1 byte key",     type(fdb.kv_get("1")),   json.encode(fdb.kv_get("1")))

    if fdb.sett then
        local ret = fdb.sett("mytable", "wendal", 123)
        log.info("ret", ret, json.encode(ret))
        ret = fdb.sett("mytable", "age", 18)
        log.info("ret", ret, json.encode(ret))
        ret = fdb.sett("mytable", "city", "guangzhou")
        log.info("ret", ret, json.encode(ret))
        ret = fdb.sett("mytable", "nickname", "wendal")
        log.info("ret", ret, json.encode(ret))
        log.info("fdb", "mytable",     type(fdb.kv_get("mytable")),   json.encode(fdb.kv_get("mytable")))
    end

    --Delete test
    fdb.kv_del("my_bool")
    local t = fdb.kv_get("my_bool")
    log.info("fdb", "my_bool",      type(t),    t)

    if fdb.kv_iter then
        local iter = fdb.kv_iter()
        if iter then
            while 1 do
                local k = fdb.kv_next(iter)
                if not k then
                    log.info("fdb", "iter exit")
                    break
                end
                log.info("fdb", k, "value", fdb.kv_get(k))
            end
        else
            log.info("fdb", "iter is null")
        end
    else
        log.info("fdb", "without iter")
    end

    --stress test
    local start = mcu.ticks()
    local count = 1000
    while count > 0 do
        -- sys.wait(10)
        count = count - 1
        --fdb.kv_set("BENT1", "--" .. os.date() .. "--")
        --fdb.kv_set("BENT2", "--" .. os.date() .. "--")
        --fdb.kv_set("BENT3", "--" .. os.date() .. "--")
        --fdb.kv_set("BENT4", "--" .. os.date() .. "--")
        fdb.kv_get("my_bool")
    end
    log.info("fdb", (mcu.ticks() - start) / 1000)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
