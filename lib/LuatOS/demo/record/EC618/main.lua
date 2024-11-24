PROJECT = "record_demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)
log.style(1)
--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
--es8311 recording demo
require "es8311"
--es8311 loop recording demo
--require "es8311_loop"
--es8218e recording demo
--require "es8218e"

--es7243e recording demo
--require "es7243e"

sys.run()
