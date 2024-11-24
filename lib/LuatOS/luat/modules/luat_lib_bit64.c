/*@Modules bit64
@summary Basic arithmetic and logical operations on 64-bit data on 32-bit systems
@version 0.1
@date 2023.03.11
@tag LUAT_USE_BIT64
@demo bit64
@note 64-bit data is stored in 9-byte string, byte7~byte0 stores data, byte8=0 means integer, others represent floating point*/
#include "luat_base.h"
#include "luat_mem.h"
#define LUAT_LOG_TAG "bit64"
#include "luat_log.h"
#include <stdlib.h>

#define D64_FLAG 0x01
#ifdef LUAT_USE_BIT64
/**
Convert 64bit data to 32bit output
@api bit64.to32(data64bit)
@string 9 bytes data
@return any Output int or number based on 64bit data*/
static int l_bit64_to32(lua_State *L)
{
	double d64;
	int64_t i64;
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);

    if (len != 9)
    {
    	lua_pushnil(L);
    }
    if (data[8])
    {
        memcpy(&d64, data, 8);
        lua_pushnumber(L, (lua_Number)d64);
    }
    else
    {
    	memcpy(&i64, data, 8);
    	lua_pushinteger(L, (lua_Integer)i64);
    }
    return 1;
}

/**
Convert 32bit data to 64bit data
@api bit64.to64(data32bit)
@int/number 32bit data
@return string 9 bytes data*/
static int l_bit64_to64(lua_State *L)
{
	double d64;
	uint64_t u64;
	uint8_t data[9] = {0};
	if (lua_isinteger(L, 1))
	{
		u64 = (lua_Unsigned)lua_tointeger(L, 1);
		memcpy(data, &u64, 8);
	}
	else if (lua_isnumber(L, 1))
	{
		d64 = lua_tonumber(L, 1);
		data[8] = D64_FLAG;
		memcpy(data, &d64, 8);

	}
	lua_pushlstring(L, (const char*)data, 9);
	return 1;
}

/**
64bit data is formatted and printed into a string for displaying values.
@api bit64.show(a,type,flag)
@string 64bit data to be printed
@int base, 10=decimal, 16=16, default 10, only supports 10 or 16
@boolean Whether the integer is printed in unsigned mode, true is, false is not, the default is false, floating point is ignored
@return string printable value*/
static int l_bit64_show(lua_State *L)
{
	int64_t i64 = 0;
	double d64 = 0;
    size_t len = 0;
    uint8_t data[64] = {0};
    uint8_t flag = 0;
    const char *string = luaL_checklstring(L, 1, &len);
    if (len != 9)
    {
    	lua_pushnil(L);
    	return 1;
    }
    if (string[8])
    {
    	memcpy(&d64, string, 8);
    }
    else
    {
    	memcpy(&i64, string, 8);
    }
    uint8_t type = luaL_optinteger(L, 2, 10);
	if (lua_isboolean(L, 3))
	{
		flag = lua_toboolean(L, 3);
	}
	if (type != 16)
	{
		if (string[8])
		{
			len = snprintf_((char*)data, 63, "%f", d64);
		}
		else
		{
			if (flag)
			{
				len = snprintf_((char*)data, 63, "%llu", i64);
			}
			else
			{
				len = snprintf_((char*)data, 63, "%lld", (uint64_t)i64);
			}
		}
	}
	else
	{
		if (string[8])
		{
			len = snprintf_((char*)data, 63, "0x%llx", d64);
		}
		else
		{
			len = snprintf_((char*)data, 63, "0x%llx", i64);
		}
	}
	lua_pushlstring(L, (const char*)data, len);
	return 1;
}

static int l_bit64_calculate(lua_State *L, uint8_t op)
{
	double d64_a = 0,d64_b = 0;
	int64_t i64_a = 0, i64_b = 0;
	uint64_t u64 = 0;
    size_t len = 0;
    uint8_t data[9] = {0};
    uint8_t flag1 = 0;
    uint8_t flag2 = 0;
    uint8_t fa= 0,fb =0;
    const char *string = luaL_checklstring(L, 1, &len);
    if (len != 9)
    {
    	goto DONE;
    }
    fa = string[8];
    if (fa)
    {
    	memcpy(&d64_a, string, 8);
    }
    else
    {
    	memcpy(&i64_a, string, 8);
    }
	if (lua_isinteger(L, 2))
	{
		i64_b = lua_tointeger(L, 2);
		fb = 0;
	}
	else if (lua_isnumber(L, 2))
	{
		d64_b = lua_tonumber(L, 2);
		fb = 1;
	}
	else
	{
		string = luaL_checklstring(L, 2, &len);
	    if (len != 9)
	    {
	    	goto DONE;
	    }
	    fb = string[8];
	    if (fb)
	    {
	    	memcpy(&d64_b, string, 8);
	    }
	    else
	    {
	    	memcpy(&i64_b, string, 8);
	    }
	}

	if (lua_isboolean(L, 3))
	{
		flag1 = lua_toboolean(L, 3);
	}
	if (lua_isboolean(L, 4))
	{
		flag2 = lua_toboolean(L, 4);
	}
	switch(op)
	{
	case 0:
		if (fa && fb)
		{
			d64_a = d64_a + d64_b;
			goto FLOAT_OP;
		}
		if (fa && !fb)
		{
			d64_a = d64_a + i64_b;
			goto FLOAT_OP;
		}
		if (!fa && fb)
		{
			d64_a = i64_a + d64_b;
			goto FLOAT_OP;
		}
		if (!fa && !fb)
		{
			if (flag1)
			{
				u64 = (uint64_t)i64_a + (uint64_t)i64_b;
				memcpy(data, &u64, 8);
			}
			else
			{
				i64_a = i64_a + i64_b;
				memcpy(data, &i64_a, 8);
			}
			goto DONE;
		}
		break;
	case 1:
		if (fa && fb)
		{
			d64_a = d64_a - d64_b;
			goto FLOAT_OP;
		}

		if (fa && !fb)
		{
			d64_a = d64_a - i64_b;
			goto FLOAT_OP;
		}

		if (!fa && fb)
		{
			d64_a = i64_a - d64_b;
			goto FLOAT_OP;
		}

		if (!fa && !fb)
		{
			if (flag1)
			{
				u64 = (uint64_t)i64_a - (uint64_t)i64_b;
				memcpy(data, &u64, 8);
			}
			else
			{
				i64_a = i64_a - i64_b;
				memcpy(data, &i64_a, 8);
			}
			goto DONE;
		}
		break;
	case 2:
		if (fa && fb)
		{
			d64_a = d64_a * d64_b;
			goto FLOAT_OP;
		}
		if (fa && !fb)
		{
			d64_a = d64_a * i64_b;
			goto FLOAT_OP;
		}
		if (!fa && fb)
		{
			d64_a = i64_a * d64_b;
			goto FLOAT_OP;
		}
		if (!fa && !fb)
		{
			if (flag1)
			{
				u64 = (uint64_t)i64_a * (uint64_t)i64_b;
				memcpy(data, &u64, 8);
			}
			else
			{
				i64_a = i64_a * i64_b;
				memcpy(data, &i64_a, 8);
			}
			goto DONE;
		}
		break;
	case 3:
		if (fa && fb)
		{
			d64_a = d64_a / d64_b;
			goto FLOAT_OP;
		}
		if (fa && !fb)
		{
			d64_a = d64_a / i64_b;
			goto FLOAT_OP;
		}
		if (!fa && fb)
		{
			d64_a = i64_a / d64_b;
			goto FLOAT_OP;
		}
		if (!fa && !fb)
		{
			if (flag1)
			{
				u64 = (uint64_t)i64_a / (uint64_t)i64_b;
				memcpy(data, &u64, 8);
			}
			else
			{
				i64_a = i64_a / i64_b;
				memcpy(data, &i64_a, 8);
			}
			goto DONE;
		}
		break;
	}
FLOAT_OP:
	if (flag2)
	{
		i64_a = d64_a;
		memcpy(data, &i64_a, 8);
	}
	else
	{
		data[8] = D64_FLAG;
		memcpy(data, &d64_a, 8);
	}
	goto DONE;
DONE:
	lua_pushlstring(L, (const char*)data, 9);
	return 1;
}

/**
64bit data is added, a+b, if one of a and b is floating point, the floating point operation is performed.
@api bit64.plus(a,b,flag1,flag2)
@string a
@string/int/number b
@boolean Whether to use unsigned mode when performing integer operations, true yes, false no, the default is false, floating point operations are ignored
@boolean Whether the floating point operation result should be forced to be converted into an integer. True is yes, false is not. The default is false. Integer operations are ignored.
@return string 9 bytes data*/
static int l_bit64_plus(lua_State *L)
{
	return l_bit64_calculate(L, 0);
}


/**
When subtracting 64-bit data, a-b, if one of a and b is a floating point, the floating point operation is performed.
@api bit64.minus(a,b,flag1,flag2)
@string a
@string/int/number b
@boolean Whether to use unsigned mode when performing integer operations, true yes, false no, the default is false, floating point operations are ignored
@boolean Whether the floating point operation result should be forced to be converted into an integer. True is yes, false is not. The default is false. Integer operations are ignored.
@return string 9 bytes data*/
static int l_bit64_minus(lua_State *L)
{
	return l_bit64_calculate(L, 1);
}

/**
64-bit data multiplication, a*b, if one of a and b is floating point, the floating point operation is performed
@api bit64.multi(a,b,flag1,flag2)
@string a
@string/int/number b
@boolean Whether to use unsigned mode when performing integer operations, true yes, false no, the default is false, floating point operations are ignored
@boolean Whether the floating point operation result should be forced to be converted into an integer. True is yes, false is not. The default is false. Integer operations are ignored.
@return string 9 bytes data*/
static int l_bit64_multiply(lua_State *L)
{
	return l_bit64_calculate(L, 2);
}

/**
64bit data is divided into a/b. If one of a and b is a floating point, the floating point operation is performed.
@api bit64.pide(a,b,flag1,flag2)
@string a
@string/int/number b
@boolean Whether to use unsigned mode when performing integer operations, true yes, false no, the default is false, floating point operations are ignored
@boolean Whether the floating point operation result should be forced to be converted into an integer. True is yes, false is not. The default is false. Integer operations are ignored.
@return string 9 bytes data*/
static int l_bit64_pide(lua_State *L)
{
	return l_bit64_calculate(L, 3);
}

/**
64bit data displacement a>>b or a<<b
@api bit64.shift(a,b,flag)
@string a
@int b
@boolean displacement direction, true for left shift<<, false for right shift>>, default false
@return string 9 bytes data*/
static int l_bit64_shift(lua_State *L)
{
	uint64_t u64;
	uint32_t pos = 0;
    size_t len;
    uint8_t data[9] = {0};
    uint8_t flag = 0;
    const char *string = luaL_checklstring(L, 1, &len);
    if (len != 9)
    {
    	goto DONE;
    }
    data[8] = string[8];
    memcpy(&u64, string, 8);

	if (lua_isinteger(L, 2))
	{
		pos = lua_tointeger(L, 2);
		if (!pos)
		{
			goto DONE;
		}
	}
	else
	{
		goto DONE;
	}
	if (lua_isboolean(L, 3))
	{
		flag = lua_toboolean(L, 3);
	}
	if (flag)
	{
		u64 = u64 << pos;
	}
	else
	{
		u64 = u64 >> pos;
	}
    data[8] = string[8];
    memcpy(data, &u64, 8);
	lua_pushlstring(L, (const char*)data, 9);
	return 1;
DONE:
	lua_pushlstring(L, (const char*)string, len);
	return 1;
}

/*Convert string to LongLong data
@api bit64.strtoll(data, base)
@string The data to be converted must exist
@int conversion base, default 10, optional 16 or 8
@return string 9 bytes data
@usage
-- This API was added on 2023.10.27
-- Reminder, if the conversion fails, 9 bytes of 0x00 will be returned.
local data = bit64.strtoll("864040064024194", 10)
log.info("data", data:toHex())
log.info("data", bit64.show(data))*/
static int l_bit64_strtoll(lua_State *L) {
	size_t len = 0;
	int64_t value = 0;
	int base = 0;
	char* stopstring;
	uint8_t re[9] = {0};
	const char* data = luaL_checklstring(L, 1, &len);
	base = luaL_optinteger(L, 2, 10);
	if (len == 0) {
		return 0;
	}
	value = strtoll(data, &stopstring, base);
	re[8] = 0;
	memcpy(re, (const char*)&value, 8);
	lua_pushlstring(L, (const char*)re, 9);
	return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_bit64[] = {
	{"to32", ROREG_FUNC(l_bit64_to32)},
	{"to64", ROREG_FUNC(l_bit64_to64)},
	{"plus", ROREG_FUNC(l_bit64_plus)},
	{"minus", ROREG_FUNC(l_bit64_minus)},
	{"multi", ROREG_FUNC(l_bit64_multiply)},
	{"pide", ROREG_FUNC(l_bit64_pide)},
	{"shift", ROREG_FUNC(l_bit64_shift)},
	{"show", ROREG_FUNC(l_bit64_show)},
	{"strtoll", ROREG_FUNC(l_bit64_strtoll)},
	{NULL,       ROREG_INT(0)}
};

LUAMOD_API int luaopen_bit64(lua_State *L)
{
    luat_newlib2(L, reg_bit64);
    return 1;
}
#endif
