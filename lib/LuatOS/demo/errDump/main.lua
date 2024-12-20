PROJECT = "errdump_test"
VERSION = "1.0"
PRODUCT_KEY = "s1uUnY6KA06ifIjcutm5oNbG3MZf5aUv" --Replace it with your own
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
log.style(1)

--The following demonstrates automatic sending
--errDump.config(true, 600, "user_id") -- The default is off. Use this to add additional user identification, such as user-defined IDs.

--local function test_user_log()
--while true do
-- 		sys.wait(15000)
--errDump.record("Test the user's recording function")
-- 	end
-- end

--local function test_error_log()
-- 	sys.wait(60000)
---- lllllllllog.record("Test the user's recording function") --Crash if wrong code is written by default
-- end



--The following demonstrates manual acquisition of information
errDump.config(true, 0)
local function test_user_log()
	local buff = zbuff.create(4096)
	local new_flag = errDump.dump(buff, errDump.TYPE_SYS)		--Manually read the exception log once after booting the computer
	if buff:used() > 0 then
		log.info(buff:toStr(0, buff:used()))	--Print out the exception log
	end
	new_flag = errDump.dump(buff, errDump.TYPE_SYS)
	if not new_flag then
		log.info("没有新数据了，删除系统错误日志")
		errDump.dump(nil, errDump.TYPE_SYS, true)
	end
	while true do
		sys.wait(15000)
		errDump.record("测试一下用户的记录功能")
		local new_flag = errDump.dump(buff, errDump.TYPE_USR)
		if new_flag then
			log.info("errBuff", buff:toStr(0, buff:used()))
		end
		new_flag = errDump.dump(buff, errDump.TYPE_USR)
		if not new_flag then
			log.info("没有新数据了，删除用户错误日志")
			errDump.dump(nil, errDump.TYPE_USR, true)
		end

	end
end

local function test_error_log()
	sys.wait(60000)
	lllllllllog.record("测试一下用户的记录功能") --Wrong code written by default causes crash
end

sys.taskInit(test_user_log)
sys.taskInit(test_error_log)
sys.run()
