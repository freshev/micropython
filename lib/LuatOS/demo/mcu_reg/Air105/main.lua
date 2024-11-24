
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "test"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

log.info('read 0x20090000', mcu.x32(mcu.reg32(0x20090000))) --Read the value of the address 0x20090000
log.info('write 0x20090000', mcu.x32(mcu.reg32(0x20090000, 0xabcdef12))) --Write 0xabcdef12
log.info('mod bit31 0x20090000', mcu.x32(mcu.reg32(0x20090000, 0x00000000, 0x80000000))) --Modify bit31 to 0
log.info('mod bit30 0x20090000', mcu.x32(mcu.reg32(0x20090000, 0x40000000, 0x40000000))) --Modify bit30 to 1

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
