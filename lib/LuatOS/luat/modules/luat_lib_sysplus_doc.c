/*@Modulessysplus
@summary A powerful addition to the sys library
@version 1.0
@date 2022.11.23
@tag LUAT_CONF_BSP
@usage
-- This library is a supplement to the sys library, adding the following content:
-- 1. cwait mechanism
-- 2. Task message mechanism, an enhanced version of sub/pub

-- It is needed in socket, libnet, http library and other scenarios, so it also needs to require*/

/*Waiting to receive a target message
@api sysplus.waitMsg(taskName, target, timeout)
@string task name, id used to wake up the task
@string target message, if nil, it means it will exit after receiving any message
@int timeout time, if it is nil, it means no timeout and wait forever
@return table successfully returns table-type msg, returns false after timeout
@usage
-- Waiting for tasks
sysplus.waitMsg('a', 'b', 1000)
-- Note that this function will automatically be registered as the global function sys_wait*/
void doc_sysplus_wait(void){};

/*Send a message to the target task
@api sysplus.sendMsg(taskName, target, arg2, arg3, arg4)
@string task name, id used to wake up the task
@any Parameter 1 in the message is also the target in waitMsg
@any Parameter 2 in the message
@any Parameter 3 in the message
@any Parameter 4 in the message
@return bool returns true if successful, otherwise returns false
@usage
--Send a message to task a and target b
sysplus.sendMsg('a', 'b')
-- Note that this function will automatically be registered as the global function sys_send*/
void doc_sysplus_send(void){};

/*Create a task thread, call the function at the last line of the Modules and register the task function in the Modules, then import the Modules into main.lua
@api sysplus.taskInitEx(fun, taskName, cbFun, ...)
@function Task function name, used to call when resume wakes up
@string task name, id used to wake up the task
@function callback function when receiving non-target message
@any...variable parameters of task function fun
@return number Returns the thread number of the task
@usage
sysplus.taskInitEx(task1,'a',callback)*/
void doc_sysplus_taskInitEx(void){};

/*Delete the task thread created by taskInitEx
@api sysplus.taskDel(taskName)
@string task name, id used to wake up the task
@usage
sysplus.taskDel('a')*/
void doc_sysplus_taskDel(void){};

/*Clear the message queue of the specified task
@api sysplus.cleanMsg(taskName)
@string task name
@usage
sysplus.cleanMsg('a')*/
void doc_sysplus_cleanMsg(void){};
