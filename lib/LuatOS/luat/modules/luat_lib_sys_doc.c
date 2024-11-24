/*@Modules  sys
@summary sys库
@version 1.0
@date    2019.11.23
@video https://www.bilibili.com/video/BV1194y1o7q2
@tag LUAT_CONF_BSP*/

/*Task coroutine waits for the specified time
@api sys.wait(timeout)
@int Waiting time, in milliseconds, must be greater than 0, otherwise it will be invalid
@return any is usually nil, unless it is actively awakened (usually not)
@usage
sys.taskInit(function()
    while 1 do
        sys.wait(500)
    end
end)*/
void doc_sys_wait(void){};

/*Task coroutine waits for a specified length of time or a specific topic
@api sys.waitUntil(topic, timeout)
@string event topic
@int Waiting time, in milliseconds, must be greater than 0, otherwise it will be invalid
@return boolean If it is a timeout, return false, otherwise return true
@return any content corresponding to the topic
@usage
sys.taskInit(function()
    //do something
    local result, data = sys.waitUntil("NET_READY", 30000)
    // do something else
end)*/
void doc_sys_waitUntil(void){};

/*Create a Task coroutine
@api sys.taskInit(func, arg1, arg2, argN)
@function The function to be executed can be an anonymous function, a local or global function
@any Parameter 1 to be passed, optional
@any Parameter 2 to be passed, optional
@any Parameter N that needs to be passed, optional
@return task coroutine object
@usage
sys.taskInit(function(a, b, c)
    log.info("task", a, b, c) -- print task A B C
end, "A", "B", "N")*/
void doc_sys_taskInit(void){};

/*Create a timer. If it is not a Task, sys.waitXXX cannot be used directly in the function.
@api sys.timerStart(func, timeout, arg1, arg2, argN)
@function The function to be executed can be an anonymous function, a local or global function
@int Delay length, unit milliseconds
@any Parameter 1 to be passed, optional
@any Parameter 2 to be passed, optional
@any Parameter N that needs to be passed, optional
@return int timer id
@usage
sys.timerStart(function(a, b, c)
    log.info("task", a, b, c) -- Will be executed after 1000 milliseconds, print task A B C
end, 1000, "A", "B", "N")*/
void doc_sys_timerStart(void){};

/*Create a loop timer. Non-Task, sys.waitXXX cannot be directly used in the function
@api sys.timerLoopStart(func, timeout, arg1, arg2, argN)
@function The function to be executed can be an anonymous function, a local or global function
@int Delay length, unit milliseconds
@any Parameter 1 to be passed, optional
@any Parameter 2 to be passed, optional
@any Parameter N that needs to be passed, optional
@return int timer id
@usage
sys.timerLoopStart(function(a, b, c)
    log.info("task", a, b, c) -- Will be executed after 1000 milliseconds, print task A B C
end, 1000, "A", "B", "N")*/
void doc_sys_timerLoopStart(void){};

/*Turn off a timer.
@api sys.timerStop(id)
@int timer id
@return nil no return value
@usage
localtcount = 0
local tid
tid = sys.timerLoopStart(function(a, b, c)
    log.info("task", a, b, c) -- Will be executed after 1000 milliseconds, print task A B C
    if tcount > 10 then
        sys.timerStop(tid)
    end
    tcount = tcount + 1
end, 1000, "A", "B", "N")*/
void doc_sys_timerStop(void){};


/*Close all timers in the same callback function.
@api sys.timerStopAll(fnc)
@function fnc callback function
@return nil no return value
@usage
-- Close all timers whose callback function is publicTimerCbFnc
local function publicTimerCbFnc(tag)
log.info("publicTimerCbFnc",tag)
end
sys.timerStart(publicTimerCbFnc,8000,"first")
sys.timerStart(publicTimerCbFnc,8000,"second")
sys.timerStart(publicTimerCbFnc,8000,"third")
sys.timerStopAll(publicTimerCbFnc)*/
void doc_sys_timerStopAll(void){};


/*Publish a message to a specific topic channel
@api sys.publish(topic, arg1, agr2, argN)
@string topic value
Parameter 1 attached to @any
Parameter 2 attached to @any
@any comes with parameter N
@return nil no return value
@usage
sys.publish("BT_READY", false)*/
void doc_sys_publish(void){};

/*Subscribe to a topic channel
@api sys.subscribe(topic, func)
@string topic value
@function callback function, note, sys.waitXXX cannot be used directly
@return nil no return value
@usage
local function bt_cb(state)
    log.info("bt", state)
end
sys.subscribe("BT_READY", bt_cb)
sys.subscribe("BT_READY", function(state)
    log.info("sys", "Got BT_READY", state)
end)*/
void doc_sys_subscribe(void){};

/*Unsubscribe from topic channel
@api sys.unsubscribe(topic, func)
@string topic value
@function callback function, note, sys.waitXXX cannot be used directly
@return nil no return value
@usage
local function bt_cb(state)
    log.info("bt", state)
end
sys.unsubscribe("BT_READY", bt_cb)*/
void doc_sys_unsubscribe(void){};

/*The main loop method of the sys library is only allowed to be called at the end of main.lua
@api sys.run()
@return nil No return value. This method is almost impossible to return.
@usage
--Always the last sentence of main.lua, it may be simplified in the future
sys.run()
--The code after that will not be executed*/
void doc_sys_run(void){};
