local key = {}

local cb = nil--Button callback function

--Set callback
function key.setCb(f)
    cb = f
end

--Anti-bounce processing
local dt = 200--Filtering time: 20ms
local isLock = nil--key lock

--Button callback
function key.cb(k,p)
    log.info("key",k,p,isLock,cb)
    if not cb or not p or isLock then return end
    cb(k)
    isLock = true
    sys.timerStart(function() isLock = false end,dt)
end

--Several buttons: Left L, Right R, Up U, Down D, OK O
gpio.setup(device.keyL, function(p) key.cb("L",p==0) end, gpio.PULLUP)
gpio.setup(device.keyR, function(p) key.cb("R",p==0) end, gpio.PULLUP)
gpio.setup(device.keyU, function(p) key.cb("U",p==0) end, gpio.PULLUP)
gpio.setup(device.keyD, function(p) key.cb("D",p==0) end, gpio.PULLUP)
gpio.setup(device.keyO, function(p) key.cb("O",p==0) end, gpio.PULLUP)


return key
