--- Module function: Luat coroutine scheduling framework
--[[
@module sys
@summary LuaTask核心逻辑
@version 1.0
@date    2018.01.01
@author  稀饭/wendal/晨旭
@usage
--sys is usually embedded in the firmware and does not need to be added to the script list manually unless you are modifying sys.lua
--After this file is modified, you need to call update_lib_inline to update the C file in vfs
_G.sys = require("sys")

sys.taskInit(function()
    sys.wait(1000)
    log.info("sys", "say hi")
end)

sys.run()
]]

local sys = {}

local table = _G.table
local unpack = table.unpack
local rtos = _G.rtos
local coroutine = _G.coroutine
local log = _G.log

--Lib script version number. As long as any script in the lib is modified, this version number needs to be updated.
SCRIPT_LIB_VER = "2.3.2"

--TaskID maximum value
local TASK_TIMER_ID_MAX = 0x1FFFFF
--The maximum value of msgId (do not modify it otherwise there will be a risk of msgId collision)
local MSG_TIMER_ID_MAX = 0x7FFFFF

--Task timer id
local taskTimerId = 0
--message timer id
local msgId = TASK_TIMER_ID_MAX
--timer id table
local timerPool = {}
local taskTimerPool = {}
--Message timer parameter table
local para = {}
--Whether the timer loops in the table
--local loop = {}
--When an error occurs when the Lua script runs, whether to fall back to the locally programmed version
--local sRollBack = true

_G.COROUTINE_ERROR_ROLL_BACK = true
_G.COROUTINE_ERROR_RESTART = true

--Add a decorator to coroutine.resume to capture coroutine errors
--local rawcoresume = coroutine.resume
local function wrapper(co,...)
    local arg = {...}
    if not arg[1] then
        local traceBack = debug.traceback(co)
        traceBack = (traceBack and traceBack~="") and (arg[2].."\r\n"..traceBack) or arg[2]
        log.error("coroutine.resume",traceBack)
        --if errDump and type(errDump.appendErr)=="function" then
        --    errDump.appendErr(traceBack)
        --end
        if _G.COROUTINE_ERROR_ROLL_BACK then
            sys.timerStart(assert,500,false,traceBack)
        elseif _G.COROUTINE_ERROR_RESTART then
            rtos.reboot()
        end
    end
    return ...
end
sys.coresume = function(...)
    local arg = {...}
    return wrapper(arg[1], coroutine.resume(...))
end

function sys.check_task()
    local co, ismain = coroutine.running()
    if ismain then
        error(debug.traceback("attempt to yield from outside a coroutine"))
    end
    return co
end

--- Task delay function can only be used in task functions
--@number ms integer, the maximum wait is 126322567 milliseconds
--@return Returns nil when the timer ends. When called by other threads, it returns the parameters passed by the calling thread.
--@usage sys.wait(30)
function sys.wait(ms)
    --Parameter detection, parameters cannot be negative values
    --assert(ms > 0, "The wait time cannot be negative!")
    --Select an unused timer ID for this task thread
    local co = sys.check_task()
    while true do
        if taskTimerId >= TASK_TIMER_ID_MAX - 1 then
            taskTimerId = 0
        else
            taskTimerId = taskTimerId + 1
        end
        if taskTimerPool[taskTimerId] == nil then
            break
        end
    end
    local timerid = taskTimerId
    taskTimerPool[co] = timerid
    timerPool[timerid] = co
    --Call core's rtos timer
    if 1 ~= rtos.timer_start(timerid, ms) then log.debug("rtos.timer_start error") return end
    --The task thread that suspends the call
    local message = {coroutine.yield()}
    if #message ~= 0 then
        rtos.timer_stop(timerid)
        taskTimerPool[co] = nil
        timerPool[timerid] = nil
        return unpack(message)
    end
end

--- The conditional waiting function of the Task (including conditions such as event messages and timer messages) can only be used in task functions.
--@param id message ID
--@number ms waiting timeout, unit ms, maximum waiting 126322567 milliseconds
--@return result Returns true when receiving a message, returns false when timeout
--@return data Receive message and return message parameters
--@usage result, data = sys.waitUntil("SIM_IND", 120000)
function sys.waitUntil(id, ms)
    local co = sys.check_task()
    sys.subscribe(id, co)
    local message = ms and {sys.wait(ms)} or {coroutine.yield()}
    sys.unsubscribe(id, co)
    return message[1] ~= nil, unpack(message, 2, #message)
end

--- Same as above, but does not return the waiting result
function sys.waitUntilMsg(id)
    local co = sys.check_task()
    sys.subscribe(id, co)
    local message = {coroutine.yield()}
    sys.unsubscribe(id, co)
    return unpack(message, 2, #message)
end

--- The conditional waiting function extension of the Task task (including conditions such as event messages and timer messages) can only be used in task functions.
--@param id message ID
--@number ms waiting timeout, unit ms, maximum waiting 126322567 milliseconds
--@return message Returns message when receiving a message, returns false when timeout
--@return data Receive message and return message parameters
--@usage result, data = sys.waitUntilExt("SIM_IND", 120000)
function sys.waitUntilExt(id, ms)
    local co = sys.check_task()
    sys.subscribe(id, co)
    local message = ms and {sys.wait(ms)} or {coroutine.yield()}
    sys.unsubscribe(id, co)
    if message[1] ~= nil then return unpack(message) end
    return false
end

--- Create a task thread, call the function at the last line of the Modules and register the task function in the Modules, then import the Modules into main.lua
--@param fun task function name, used to be called when resume wakes up
--@param ... variable parameters of task function fun
--@return co returns the thread number of the task
--@usage sys.taskInit(task1,'a','b')
function sys.taskInit(fun, ...)
    local co = coroutine.create(fun)
    sys.coresume(co, ...)
    return co
end

--------------------------------------- rtos message callback processing part----- ----------------------------------------
--[[
函数名：cmpTable
功能  ：比较两个table的内容是否相同，注意：table中不能再包含table
参数  ：
t1：第一个table
t2：第二个table
返回值：相同返回true，否则false
]]
local function cmpTable(t1, t2)
    if not t2 then return #t1 == 0 end
    if #t1 == #t2 then
        for i = 1, #t1 do
            if unpack(t1, i, i) ~= unpack(t2, i, i) then
                return false
            end
        end
        return true
    end
    return false
end

--- turn off timer
--@param val When the value is number, it is recognized as the timer ID. When the value is a callback function, parameters need to be passed.
--@param... When the val value is a function, the variable parameter of the function
--@return None
--@usage timerStop(1)
function sys.timerStop(val, ...)
    --val is the timer ID
    if type(val) == 'number' then
        timerPool[val], para[val] = nil, nil
        rtos.timer_stop(val)
    else
        for k, v in pairs(timerPool) do
            --The callback function is the same
            if type(v) == 'table' and v.cb == val or v == val then
                --Variable parameters are the same
                if cmpTable({...}, para[k]) then
                    rtos.timer_stop(k)
                    timerPool[k], para[k] = nil, nil
                    break
                end
            end
        end
    end
end

--- Close all timers in the same callback function
--@param fnc timer callback function
--@return None
--@usage timerStopAll(cbFnc)
function sys.timerStopAll(fnc)
    for k, v in pairs(timerPool) do
        if type(v) == "table" and v.cb == fnc or v == fnc then
            rtos.timer_stop(k)
            timerPool[k], para[k] = nil, nil
        end
    end
end

function sys.timerAdvStart(fnc, ms, _repeat, ...)
    --Callback function and duration detection
    --assert(fnc ~= nil, "sys.timerStart(first param) is nil !")
    --assert(ms > 0, "sys.timerStart(Second parameter) is <= zero !")
    --Turn off the exact same timer
    local arg = {...}
    if #arg == 0 then
        sys.timerStop(fnc)
    else
        sys.timerStop(fnc, ...)
    end
    --Apply for an ID for the timer. ID values   1-20 are reserved for tasks, and 20-30 are reserved for message-specific timers.
    while true do
        if msgId >= MSG_TIMER_ID_MAX then msgId = TASK_TIMER_ID_MAX end
        msgId = msgId + 1
        if timerPool[msgId] == nil then
            timerPool[msgId] = fnc
            break
        end
    end
    --Call the underlying interface to start the timer
    if rtos.timer_start(msgId, ms, _repeat) ~= 1 then return end
    --If variable parameters exist, save the parameters in the timer parameter table
    if #arg ~= 0 then
        para[msgId] = arg
    end
    --Return timer id
    return msgId
end

--- Start a timer
--@param fnc timer callback function
--@number ms integer, maximum timing 126322567 milliseconds
--@param ... variable parameters fnc parameters
--@return number timer ID, if failed, return nil
function sys.timerStart(fnc, ms, ...)
    return sys.timerAdvStart(fnc, ms, 0, ...)
end

--- Start a loop timer
--@param fnc timer callback function
--@number ms integer, maximum timing 126322567 milliseconds
--@param ... variable parameters fnc parameters
--@return number timer ID, if failed, return nil
function sys.timerLoopStart(fnc, ms, ...)
    return sys.timerAdvStart(fnc, ms, -1, ...)
end

--- Determine whether a timer is on
--@param val has two forms
--One is the timer id returned when the timer is turned on. In this form, there is no need to pass in variable parameters...it can uniquely mark a timer.
--The other is the callback function when the timer is turned on. In this form, variable parameters must be passed in... to uniquely mark a timer.
--@param ... variable parameters
--@return number returns true when enabled, otherwise nil
function sys.timerIsActive(val, ...)
    if type(val) == "number" then
        return timerPool[val]
    else
        for k, v in pairs(timerPool) do
            if v == val then
                if cmpTable({...}, para[k]) then return true end
            end
        end
    end
end


--------------------------------------- LUA application message subscription/publishing interface--- ---------------------------------------
--Subscriber list
local subscribers = {}
--internal message queue
local messageQueue = {}

--- Subscribe to messages
--@param id message id
--@param callback message callback processing
--@usage subscribe("NET_STATUS_IND", callback)
function sys.subscribe(id, callback)
    --if not id or type(id) == "boolean" or (type(callback) ~= "function" and type(callback) ~= "thread") then
    --log.warn("warning: sys.subscribe invalid parameter", id, callback)
    --    return
    --end
    --log.debug("sys", "subscribe", id, callback)
    if type(id) == "table" then
        --Support multiple topic subscriptions
        for _, v in pairs(id) do sys.subscribe(v, callback) end
        return
    end
    if not subscribers[id] then subscribers[id] = {} end
    subscribers[id][callback] = true
end
--- Unsubscribe from messages
--@param id message id
--@param callback message callback processing
--@usage unsubscribe("NET_STATUS_IND", callback)
function sys.unsubscribe(id, callback)
    --if not id or type(id) == "boolean" or (type(callback) ~= "function" and type(callback) ~= "thread") then
    --log.warn("warning: sys.unsubscribe invalid parameter", id, callback)
    --    return
    --end
    --log.debug("sys", "unsubscribe", id, callback)
    if type(id) == "table" then
        --Support multiple topic subscriptions
        for _, v in pairs(id) do sys.unsubscribe(v, callback) end
        return
    end
    if subscribers[id] then subscribers[id][callback] = nil end
    --Determine whether the message has no other subscriptions
    for k, _ in pairs(subscribers[id]) do
        return
    end
    subscribers[id] = nil
end

--- Publish internal messages and store them in the internal message queue
--@param ... variable parameters, user-defined
--@return None
--@usage publish("NET_STATUS_IND")
function sys.publish(...)
    table.insert(messageQueue, {...})
end

--Distribute messages
local function dispatch()
    while true do
        if #messageQueue == 0 then
            break
        end
        local message = table.remove(messageQueue, 1)
        if subscribers[message[1]] then
            local tmpt = {}
            for callback, _ in pairs(subscribers[message[1]]) do
                table.insert(tmpt, callback)
            end
            for _, callback in ipairs(tmpt) do
                if type(callback) == "function" then
                    callback(unpack(message, 2, #message))
                elseif type(callback) == "thread" then
                    sys.coresume(callback, unpack(message))
                end
            end
        end
    end
end

--rtos message callback
--local handlers = {}
--setmetatable(handlers, {__index = function() return function() end end, })

--- Register rtos message callback processing function
--@number id message type id
--@param handler message processing function
--@return None
--@usage rtos.on(rtos.MSG_KEYPAD, function(param) handle keypad message end)
--function sys.on(id, handler)
--handlers[id] = handler
--end

--------------------------------------- Luat main scheduling framework ------ ----------------------------------
function sys.safeRun()
    --Distribute inside information
    dispatch()
    --Block reading external messages
    local msg, param, exparam = rtos.receive(rtos.INF_TIMEOUT)
    --log.info("sys", msg, param, exparam, tableNSize(timerPool), tableNSize(para), tableNSize(taskTimerPool), tableNSize(subscribers))
    --Empty message?
    if not msg or msg == 0 then
        --No action
    --Determine whether it is a timer message and whether the message is registered
    elseif msg == rtos.MSG_TIMER and timerPool[param] then
        if param < TASK_TIMER_ID_MAX then
            local taskId = timerPool[param]
            timerPool[param] = nil
            if taskTimerPool[taskId] == param then
                taskTimerPool[taskId] = nil
                sys.coresume(taskId)
            end
        else
            local cb = timerPool[param]
            --If it is not a loop timer, delete this timer from the timer id table
            if exparam == 0 then timerPool[param] = nil end
            if para[param] ~= nil then
                cb(unpack(para[param]))
                if exparam == 0 then para[param] = nil end
            else
                cb()
            end
            --If it is a loop timer, continue to start this timer
            --if loop[param] then rtos.timer_start(param, loop[param]) end
        end
    --Other messages (audio messages, charging management messages, key messages, etc.)
    --elseif type(msg) == "number" then
    --handlers[msg](param, exparam)
    --else
    --    handlers[msg.id](msg)
    end
end

--- run() obtains core messages from the bottom layer and processes related messages in a timely manner, queries the timer and schedules each successfully registered task thread to run and suspend
--@return None
--@usage sys.run()
if _G.SYSP then
    function sys.run() end
else
    function sys.run()
        while true do
            sys.safeRun()
        end
    end
end

_G.sys_pub = sys.publish

return sys
----------------------------
