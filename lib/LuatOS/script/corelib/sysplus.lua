
--[[
@module sysplus
@summary LuaTask核心增强逻辑
@version 1.0
@date    2022.04.27
@author  晨旭/刘清宇/李思琦
@usage

]]

local sys = require "sys"
local sysplus = {}

----------------------------------------------
--Provided for asynchronous C interface use, by Chen Xu
sysplus.cwaitMt = {
    wait = function(t,r)
        return function()
            if r and type(r) == "table" then--Create new waiting for failed return
                return table.unpack(r)
            end
            return sys.waitUntilMsg(t)
        end
    end,
    cb = function(t,r)
        return function(f)
            if type(f) ~= "function" then return end
            sys.taskInit(function ()
                if r and type(r) == "table" then
                    --sys.wait(1)--If sys.publish is called in the callback, calling the callback directly will not trigger the next line. . .
                    f(table.unpack(r))
                    return
                end
                f(sys.waitUntilMsg(t))
            end)
        end
    end,
}
sysplus.cwaitMt.__index = function(t,i)
    if sysplus.cwaitMt[i] then
        return sysplus.cwaitMt[i](rawget(t,"w"),rawget(t,"r"))
    else
        rawget(t,i)
    end
end
_G.sys_cw = function (w,...)
    local r = {...}
    local t = {w=w,r=(#r > 0 and r or nil)}
    setmetatable(t,sysplus.cwaitMt)
    return t
end

-------------------------------------------------------------------
------------Task-based task extension by Li Siqi-----------------------------

--task list
local taskList = {}

--- Create a task thread, call the function at the last line of the Modules and register the task function in the Modules, then import the Modules into main.lua
--@param fun task function name, used to be called when resume wakes up
--@param taskName task name, the id used to wake up the task
--@param cbFun callback function when receiving non-target message
--@param ... variable parameters of task function fun
--@return co returns the thread number of the task
--@usage sysplus.taskInitEx(task1,'a',callback)
function sysplus.taskInitEx(fun, taskName, cbFun, ...)
    taskList[taskName]={msgQueue={}, To=false, cb=cbFun}
    return sys.taskInit(fun, ...)
end

--- Delete the task thread created by taskInitEx
--@param taskName task name, the id used to wake up the task
--@return None
--@usage sysplus.taskDel('a')
function sysplus.taskDel(taskName)
    taskList[taskName]=nil
end

local function waitTo(taskName)
    taskList[taskName].To = true
    sys.publish(taskName)
end

--- Waiting to receive a target message
--@param taskName task name, the id used to wake up the task
--@param target target message, if it is nil, it means it will exit after receiving any message.
--@param ms timeout, if nil, it means no timeout, wait forever
--@return msg or false Successfully returns table-type msg, returns false after timeout
--@usage sysplus.waitMsg('a', 'b', 1000)
function sysplus.waitMsg(taskName, target, ms)
    if taskList[taskName] == nil then
        log.error("sysplus", "sys.taskInitEx启动的task才能使用waitMsg")
        return false
    end
    local msg = false
    local message = nil
    if #taskList[taskName].msgQueue > 0 then
        msg = table.remove(taskList[taskName].msgQueue, 1)
        if target == nil then
            return msg
        end
        if (msg[1] == target) then
            return msg
        elseif type(taskList[taskName].cb) == "function" then
            taskList[taskName].cb(msg)
        end
    end
    sys.subscribe(taskName, coroutine.running())
    sys.timerStop(waitTo, taskName)
    if ms and ms ~= 0 then
        sys.timerStart(waitTo, ms, taskName)
    end
    taskList[taskName].To = false
    local finish=false
    while not finish do
        message = coroutine.yield()
        if #taskList[taskName].msgQueue > 0 then
            msg = table.remove(taskList[taskName].msgQueue, 1)
            --sys.info("check target", msg[1], target)
            if target == nil then
                finish = true
            else
                if (msg[1] == target) then
                    finish = true
                elseif type(taskList[taskName].cb) == "function" then
                    taskList[taskName].cb(msg)
                end
            end
        elseif taskList[taskName].To then
            --sys.info(taskName, "wait message timeout")
            finish = true
        end
    end
    if taskList[taskName].To then
        msg = nil
    end
    taskList[taskName].To = false
    sys.timerStop(waitTo, taskName)
    sys.unsubscribe(taskName, coroutine.running())
    return msg
end

--- Send a message to the target task
--@param taskName task name, the id used to wake up the task
--@param param1 Parameter 1 in the message is also the target in waitMsg
--@param param2 Parameter 2 in the message
--@param param3 Parameter 3 in the message
--@param param4 Parameter 4 in the message
--@return true or false returns true successfully
--@usage sysplus.sendMsg('a', 'b')
function sysplus.sendMsg(taskName, param1, param2, param3, param4)
    if taskList[taskName]~=nil then
        table.insert(taskList[taskName].msgQueue, {param1, param2, param3, param4})
        sys.publish(taskName)
        return true
    end
    return false
end

function sysplus.cleanMsg(taskName)
    if taskList[taskName]~=nil then
        taskList[taskName].msgQueue = {}
        return true
    end
    return false
end

function sysplus.taskCB(taskName, msg)
    if taskList[taskName]~=nil then
        if type(taskList[taskName].cb) == "function" then
            taskList[taskName].cb(msg)
            return
        end
    end
    log.error(taskName, "no cb fun")
end

_G.sys_send = sysplus.sendMsg
_G.sys_wait = sysplus.waitMsg

return sysplus
----------------------------
