/*@Modules zbuff
@summary c memory data operation library
@version 0.1
@date 2021.03.31
@video https://www.bilibili.com/video/BV1gr4y1V7HN
@tagLUAT_USE_ZBUFF
@demo zbuff*/
#include "luat_base.h"
#include "luat_zbuff.h"

#define LUAT_LOG_TAG "zbuff"
#include "luat_log.h"

//Add data after the buff object and return the increased number of bytes
int add_bytes(luat_zbuff_t *buff, const char *source, size_t len)
{
    if (buff->len - buff->cursor < len)
        len = buff->len - buff->cursor;
    memcpy(buff->addr + buff->cursor, source, len);
    buff->cursor += len;
    return len;
}

#define SET_POINT_1(buff, point, color)                \
    if (color % 2)                                     \
        buff->addr[point / 8] |= 1 << (7 - point % 8); \
    else                                               \
        buff->addr[point / 8] &= ~(1 << (7 - point % 8))
#define SET_POINT_4(buff, point, color)                 \
    buff->addr[point / 2] &= (point % 2) ? 0xf0 : 0x0f; \
    buff->addr[point / 2] |= (point % 2) ? color : (color * 0x10)
#define SET_POINT_8(buff, point, color) buff->addr[point] = color
#define SET_POINT_16(buff, point, color) \
    buff->addr[point * 2] = color / 0x100;   \
    buff->addr[point * 2 + 1] = color % 0x100
#define SET_POINT_24(buff, point, color)                              \
    buff->addr[point * 3] = color / 0x10000;                          \
    buff->addr[point * 3 + 1] = color % 0x10000 / 0x100; \
    buff->addr[point * 3 + 2] = color % 0x100
#define SET_POINT_32(buff, point, color)                 \
    buff->addr[point] = color / 0x1000000;               \
    buff->addr[point + 1] = color % 0x1000000 / 0x10000; \
    buff->addr[point + 2] = color % 0x10000 / 0x100;     \
    buff->addr[point + 3] = color % 0x100

#define SET_POINT_CASE(n, point, color)    \
    case n:                                \
        SET_POINT_##n(buff, point, color); \
        break
//Change the color of a certain point
#define set_framebuffer_point(buff, point, color) \
    switch (buff->bit)                            \
    {                                             \
        SET_POINT_CASE(1, (point), (color));      \
        SET_POINT_CASE(4, (point), (color));      \
        SET_POINT_CASE(8, (point), (color));      \
        SET_POINT_CASE(16, (point), (color));     \
        SET_POINT_CASE(24, (point), (color));     \
        SET_POINT_CASE(32, (point), (color));     \
    default:                                      \
        break;                                    \
    }

#define GET_POINT_1(buff, point) (buff->addr[point / 8] >> (7 - point % 8)) % 2
#define GET_POINT_4(buff, point) (buff->addr[point / 2] >> ((point % 2) ? 0 : 4)) % 0x10
#define GET_POINT_8(buff, point) buff->addr[point]
#define GET_POINT_16(buff, point) buff->addr[point * 2] * 0x100 + buff->addr[point * 2 + 1]
#define GET_POINT_24(buff, point) \
    buff->addr[point * 3] * 0x10000 + buff->addr[point * 3 + 1] * 0x100 + buff->addr[point * 3 + 2]
#define GET_POINT_32(buff, point) \
    buff->addr[point] * 0x1000000 + buff->addr[point + 1] * 0x10000 + buff->addr[point + 2] * 0x100 + buff->addr[point + 3]
#define GET_POINT_CASE(n, point)           \
    case n:                                \
        return GET_POINT_##n(buff, point); \

//Get the color of a certain point
uint32_t get_framebuffer_point(luat_zbuff_t *buff,uint32_t point)
{
    switch (buff->bit)
    {
        GET_POINT_CASE(1, point);
        GET_POINT_CASE(4, point);
        GET_POINT_CASE(8, point);
        GET_POINT_CASE(16, point);
        GET_POINT_CASE(24, point);
        GET_POINT_CASE(32, point);
    default:
        break;
    }
    return 0;
}

/**
Create zbuff
@api zbuff.create(length,data,type)
@int number of bytes
@any Optional parameter, when number is the padding data, when string is the padding string
@number Optional parameter, memory type, optional: zbuff.HEAP_SRAM (internal sram, default) zbuff.HEAP_PSRAM (external psram) zbuff.HEAP_AUTO (automatic application, if psram exists, apply in psram, if it does not exist or fails Apply at sram) Note: This item is related to hardware support
@return object zbuff object, if creation fails, nil will be returned
@usage
--Create zbuff
local buff = zbuff.create(1024) -- blank
local buff = zbuff.create(1024, 0x33) --Create a memory area with initial values     all 0x33
local buff = zbuff.create(1024, "123321456654") -- Create and fill in the content of an existing string

-- Create zbuff for framebuff
-- zbuff.create({width,height,bit},data,type)
-- table width, height, color bit depth
@int optional parameter, fill in data
@number Optional parameter, memory type, optional: zbuff.HEAP_SRAM (internal sram, default) zbuff.HEAP_PSRAM (external psram) zbuff.HEAP_AUTO (automatic application, if psram exists, apply in psram, if it does not exist or fails Apply at sram) Note: This item is related to hardware support
@return object zbuff object, if creation fails, nil will be returned
@usage
--Create zbuff
local buff = zbuff.create({128,160,16})--Create a 128*160 framebuff
local buff = zbuff.create({128,160,16},0xf800)--Create a 128*160 framebuff, the initial state is red*/
static int l_zbuff_create(lua_State *L)
{
    size_t len;
    uint32_t width = 0,height = 0;
    uint8_t bit = 0;
    if (lua_istable(L, 1)){
        lua_rawgeti(L, 1, 3);
        lua_rawgeti(L, 1, 2);
        lua_rawgeti(L, 1, 1);
        width = luaL_checkinteger(L, -1);
        height = luaL_checkinteger(L, -2);
        bit = luaL_checkinteger(L, -3);
        if (bit != 1 && bit != 4 && bit != 8 && bit != 16 && bit != 24 && bit != 32) return 0;
        len = (width * height * bit - 1) / 8 + 1;
        lua_pop(L, 3);
    } else {
        len = luaL_checkinteger(L, 1);
    }
    if (len <= 0) return 0;

    luat_zbuff_t *buff = (luat_zbuff_t *)lua_newuserdata(L, sizeof(luat_zbuff_t));
    if (buff == NULL) return 0;

    if (lua_isinteger(L, 3)){
    	buff->type = luaL_optinteger(L, 3, LUAT_HEAP_SRAM);
    } else {
        buff->type = LUAT_HEAP_SRAM;
    }
    buff->addr = (uint8_t *)luat_heap_opt_malloc(buff->type,len);
    if (buff->addr == NULL){
        lua_pushnil(L);
        lua_pushstring(L, "memory not enough");
        return 2;
    }

    buff->len = len;
    buff->cursor = 0;

    if (lua_istable(L, 1)){
        buff->width = width;
        buff->height = height;
        buff->bit = bit;
        if (lua_isinteger(L, 2)){
            LUA_INTEGER initial = luaL_checkinteger(L, 2);
            uint32_t i;
            for (i = 0; i < buff->width * buff->height; i++){
                set_framebuffer_point(buff, i, initial);
            }
        }
    }else{
        buff->width = buff->height = buff->bit = 0;
        if (lua_isinteger(L, 2)){
            memset(buff->addr, luaL_checkinteger(L, 2) % 0x100, len);
        }
        else if (lua_isstring(L, 2)){
            const char *data = luaL_optlstring(L, 2, "", &len);
            if (len > buff->len) len = buff->len; //prevent crossing the boundary
            memcpy(buff->addr, data, len);
            buff->cursor = len;
        }else{
            memset(buff->addr, 0, len);
        }
    }

    luaL_setmetatable(L, LUAT_ZBUFF_TYPE);
    return 1;
}

/**
zbuff writes data (starting from the current pointer position; the pointer will move backward after execution)
@api buff:write(para,...)
@any writes the buff data, which is one parameter for string and multiple parameters for number.
@return number The length of data successfully written
@usage
-- File-like read and write operations
local len = buff:write("123") -- Write data, the pointer moves backward accordingly, and returns the written data length
local len = buff:write(0x1a,0x30,0x31,0x32,0x00,0x01) -- Write multiple bytes of data numerically*/
static int l_zbuff_write(lua_State *L)
{
    if (lua_isinteger(L, 2))
    {
        int len = 0;
        int data = 0;
        luat_zbuff_t *buff = tozbuff(L);
        while (lua_isinteger(L, 2 + len) && buff->cursor < buff->len)
        {
            data = luaL_checkinteger(L, 2 + len);
            *(uint8_t *)(buff->addr + buff->cursor) = data % 0x100;
            buff->cursor++;
            len++;
        }
        lua_pushinteger(L, len);
        return 1;
    }
    else
    {
        size_t len;
        const char *data = luaL_checklstring(L, 2, &len);
        luat_zbuff_t *buff = tozbuff(L);
        if (len + buff->cursor > buff->len) //prevent crossing the boundary
        {
            len = buff->len - buff->cursor;
        }
        memcpy(buff->addr + buff->cursor, data, len);
        buff->cursor = buff->cursor + len;
        lua_pushinteger(L, len);
        return 1;
    }
}

/**
zbuff reads data (starting from the current pointer position; the pointer will move backward after execution)
@api buff:read(length)
@int Read the number of bytes in buff
@return string read result
@usage
-- File-like read and write operations
local str = buff:read(3)*/
static int l_zbuff_read(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int read_num = luaL_optinteger(L, 2, 1);
    if (read_num > buff->len - buff->cursor) //prevent crossing the boundary
    {
        read_num = buff->len - buff->cursor;
    }
    if (read_num <= 0)
    {
        lua_pushlstring(L, NULL, 0);
        return 1;
    }
    char *return_str = (char *)luat_heap_opt_malloc(buff->type,read_num);
    if (return_str == NULL)
    {
        return 0;
    }
    memcpy(return_str, buff->addr + buff->cursor, read_num);
    lua_pushlstring(L, return_str, read_num);
    buff->cursor += read_num;
    luat_heap_opt_free(buff->type, return_str);
    return 1;
}

/**
zbuff clears data (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:clear(num)
@int Optional, default is 0. The value to be set to will not change the buff pointer position
@usage
-- All initialized to 0
buff:clear(0)*/
static int l_zbuff_clear(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int num = luaL_optinteger(L, 2, 0);
    memset(buff->addr, num % 0x100, buff->len);
    return 0;
}

/**
zbuff sets the cursor position (may be related to the current pointer position; the pointer will be set to the specified position after execution)
@api buff:seek(base,offset)
@int offset length
@int where, base point, default zbuff.SEEK_SET. zbuff.SEEK_SET: The base point is 0 (the beginning of the file), zbuff.SEEK_CUR: The base point is the current position, zbuff.SEEK_END:   The base point is the end of the file
@return int The position of the cursor calculated from the beginning of the buff after setting the cursor
@usage
buff:seek(0) -- Set the cursor to the specified position
buff:seek(5,zbuff.SEEK_CUR)
buff:seek(-3,zbuff.SEEK_END)*/
static int l_zbuff_seek(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);

    int offset = luaL_checkinteger(L, 2);
    int whence = luaL_optinteger(L, 3, ZBUFF_SEEK_SET);
    switch (whence)
    {
    case ZBUFF_SEEK_SET:
        break;
    case ZBUFF_SEEK_CUR:
        offset = buff->cursor + offset;
        break;
    case ZBUFF_SEEK_END:
        offset = buff->len + offset;
        break;
    default:
        return 0;
    }
    if (offset <= 0)
        offset = 0;
    if (offset > buff->len)
        offset = buff->len;
    buff->cursor = offset;
    lua_pushinteger(L, buff->cursor);
    return 1;
}

//code from https://github.com/LuaDist/lpack/blob/master/lpack.c
#define	OP_STRING	'A'
#define	OP_FLOAT	'f'
#define	OP_DOUBLE	'd'
#define	OP_NUMBER	'n'
#define	OP_CHAR		'c'
#define	OP_BYTE		'b'
#define	OP_SHORT	'h'
#define	OP_USHORT	'H'
#define	OP_INT		'i'
#define	OP_UINT		'I'
#define	OP_LONG		'l'
#define	OP_ULONG	'L'
#define	OP_LITTLEENDIAN	'<'
#define	OP_BIGENDIAN	'>'
#define	OP_NATIVE	'='

#define isdigit(c) ((c) >= '0' && (c) <= '9')
static void badcode(lua_State *L, int c)
{
    char s[]="bad code `?'";
    s[sizeof(s)-3]=c;
    luaL_argerror(L,1,s);
}
static int doendian(int c)
{
    int x=1;
    int e=*(char*)&x;
    if (c==OP_LITTLEENDIAN) return !e;
    if (c==OP_BIGENDIAN) return e;
    if (c==OP_NATIVE) return 0;
    return 0;
}
static void doswap(int swap, void *p, size_t n)
{
    if (swap)
    {
        char *a = p;
        int i, j;
        for (i = 0, j = n - 1, n = n / 2; n--; i++, j--)
        {
            char t = a[i];
            a[i] = a[j];
            a[j] = t;
        }
    }
}

/**
Convert a series of data according to format characters and write (starting from the current pointer position; the pointer will move backward after execution)
@api buff:pack(format,val1, val2,...)
@string The format of the following data (see the example below for symbol meaning)
@val The data passed in can be multiple data
@return int length of successfully written data
@usage
buff:pack(">IIHA", 0x1234, 0x4567, 0x12,"abcdefg") -- Write several data according to the format
--A string
-- f float
--ddouble
-- n Lua number
--cchar
-- b byte / unsigned char
-- h short
-- H unsigned short
-- i int
--I unsigned int
-- l long
-- L unsigned long
-- < little endian
-- > big endian
-- = default endianness*/
#define PACKNUMBER(OP, T)                                    \
    case OP:                                                 \
    {                                                        \
        T a = (T)luaL_checknumber(L, i++);                   \
        doswap(swap, &a, sizeof(a));                         \
        write_len += add_bytes(buff, (void *)&a, sizeof(a)); \
        break;                                               \
    }

#define PACKINT(OP, T)                                    \
    case OP:                                                 \
    {                                                        \
        T a = (T)luaL_checkinteger(L, i++);                   \
        doswap(swap, &a, sizeof(a));                         \
        write_len += add_bytes(buff, (void *)&a, sizeof(a)); \
        break;                                               \
    }

static int l_zbuff_pack(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int i = 3;
    char *f = (char *)luaL_checkstring(L, 2);
    int swap = 0;
    int write_len = 0; //The length has been written
    while (*f)
    {
        if (buff->cursor == buff->len) //It's over
            break;
        int c = *f++;
        int N = 1;
        if (isdigit(*f))
        {
            N = 0;
            while (isdigit(*f))
                N = 10 * N + (*f++) - '0';
        }
        while (N--)
        {
            if (buff->cursor == buff->len) //It's over
                break;
            switch (c)
            {
            case OP_LITTLEENDIAN:
            case OP_BIGENDIAN:
            case OP_NATIVE:
            {
                swap = doendian(c);
                N = 0;
                break;
            }
            case OP_STRING:
            {
                size_t l;
                const char *a = luaL_checklstring(L, i++, &l);
                write_len += add_bytes(buff, a, l);
                break;
            }
            PACKNUMBER(OP_NUMBER, lua_Number)
            PACKNUMBER(OP_DOUBLE, double)
            PACKNUMBER(OP_FLOAT, float)
            PACKINT(OP_CHAR, char)
            PACKINT(OP_BYTE, unsigned char)
            PACKINT(OP_SHORT, short)
            PACKINT(OP_USHORT, unsigned short)
            PACKINT(OP_INT, int)
            PACKINT(OP_UINT, unsigned int)
            PACKINT(OP_LONG, long)
            PACKINT(OP_ULONG, unsigned long)
            case ' ':
            case ',':
                break;
            default:
                badcode(L, c);
                break;
            }
        }
    }
    lua_pushinteger(L, write_len);
    return 1;
}

#define UNPACKINT(OP, T)                    \
    case OP:                                \
    {                                       \
        T a;                                \
        int m = sizeof(a);                  \
        if (i + m > len)                    \
            goto done;                      \
        memcpy(&a, s + i, m);               \
        i += m;                             \
        doswap(swap, &a, m);                \
        lua_pushinteger(L, (lua_Integer)a); \
        ++n;                                \
        break;                              \
    }

#define UNPACKINT8(OP,T)		\
	case OP:				\
	{					\
		T a;				\
		int m=sizeof(a);			\
		if (i+m>len) goto done;		\
		memcpy(&a,s+i,m);			\
		i+=m;				\
		doswap(swap,&a,m);			\
		int t = (a & 0x80)?(0xffffff00+a):a;\
		lua_pushinteger(L,(lua_Integer)t);	\
		++n;				\
		break;				\
	}

#define UNPACKNUMBER(OP, T)               \
    case OP:                              \
    {                                     \
        T a;                              \
        int m = sizeof(a);                \
        if (i + m > len)                  \
            goto done;                    \
        memcpy(&a, s + i, m);             \
        i += m;                           \
        doswap(swap, &a, m);              \
        lua_pushnumber(L, (lua_Number)a); \
        ++n;                              \
        break;                            \
    }
/**
Read a series of data according to the format characters (starting from the current pointer position; the pointer will move backward after execution)
@api buff:unpack(format)
@string data format (see the example of the pack interface above for symbol meaning)
@return int length of successfully read data bytes
@return any data read according to format
@usage
local cnt,a,b,c,s = buff:unpack(">IIHA10") -- read several data according to the format
--If all are read successfully, cnt is 4+4+2+10=20*/
static int l_zbuff_unpack(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    char *f = (char *)luaL_checkstring(L, 2);
    size_t len = buff->len - buff->cursor;
    const char *s = (const char*)(buff->addr + buff->cursor);
    int i = 0;
    int n = 0;
    int swap = 0;
    lua_pushnil(L); // Used to occupy the number
    while (*f)
    {
        int c = *f++;
        int N = 1;
        if (isdigit(*f))
        {
            N = 0;
            while (isdigit(*f))
                N = 10 * N + (*f++) - '0';
            if (N == 0 && c == OP_STRING)
            {
                lua_pushliteral(L, "");
                ++n;
            }
        }
        while (N--){
            if (!lua_checkstack(L, n))
                return luaL_error(L, "too many results to unpack");
            switch (c)
            {
            case OP_LITTLEENDIAN:
            case OP_BIGENDIAN:
            case OP_NATIVE:
            {
                swap = doendian(c);
                N = 0;
                break;
            }
            case OP_STRING:
            {
                ++N;
                if (i + N > len)
                    goto done;
                lua_pushlstring(L, s + i, N);
                i += N;
                ++n;
                N = 0;
                break;
            }
            UNPACKNUMBER(OP_NUMBER, lua_Number)
            UNPACKNUMBER(OP_DOUBLE, double)
            UNPACKNUMBER(OP_FLOAT, float)
			UNPACKINT8(OP_CHAR, char)
            UNPACKINT(OP_BYTE, unsigned char)
            UNPACKINT(OP_SHORT, short)
            UNPACKINT(OP_USHORT, unsigned short)
            UNPACKINT(OP_INT, int)
            UNPACKINT(OP_UINT, unsigned int)
            UNPACKINT(OP_LONG, long)
            UNPACKINT(OP_ULONG, unsigned long)
            case ' ':
            case ',':
                break;
            default:
                badcode(L, c);
                break;
            }
        }
    }
done:
    buff->cursor += i;
    lua_pushinteger(L, i);
    lua_replace(L, -n - 2);
    return n + 1;
}

/**
Read data of a specified type (starting from the current pointer position; the pointer will move backward after execution)
@api buff:read type()
@Note The reading types can be: I8, U8, I16, U16, I32, U32, I64, U64, F32, F64
@return number The data read, or nil if out of bounds
@usage
local data = buff:readI8()
local data = buff:readU32()*/
#define zread(n, t, f)                                       \
    static int l_zbuff_read_##n(lua_State *L)                \
    {                                                        \
        luat_zbuff_t *buff = tozbuff(L);                      \
        if (buff->len - buff->cursor < sizeof(t))            \
            return 0;                                        \
        t tmp;                                              \
        memcpy(&tmp, buff->addr + buff->cursor, sizeof(t));  \
        lua_push##f(L, tmp);                                 \
        buff->cursor += sizeof(t);                           \
        return 1;                                            \
    }
zread(i8, int8_t, integer);
zread(u8, uint8_t, integer);
zread(i16, int16_t, integer);
zread(u16, uint16_t, integer);
zread(i32, int32_t, integer);
zread(u32, uint32_t, integer);
zread(i64, int64_t, integer);
zread(u64, uint64_t, integer);
zread(f32, float, number);
zread(f64, double, number);

/**
Write a specified type of data (starting from the current pointer position; the pointer will move backward after execution)
@api buff:write type()
@number Data to be written
@Note The writing type can be: I8, U8, I16, U16, I32, U32, I64, U64, F32, F64
@return number The length of successful writing
@usage
local len = buff:writeI8(10)
local len = buff:writeU32(1024)*/
#define zwrite(n, t, f)                                               \
    static int l_zbuff_write_##n(lua_State *L)                        \
    {                                                                 \
        luat_zbuff_t *buff = tozbuff(L);                                \
        if (buff->len - buff->cursor < sizeof(t))                     \
        {                                                             \
            lua_pushinteger(L, 0);                                    \
            return 1;                                                 \
        }                                                             \
        t tmp =   (t)luaL_check##f(L, 2);                             \
        memcpy(buff->addr + buff->cursor, &(tmp), sizeof(t));            \
        buff->cursor += sizeof(t);                                    \
        lua_pushinteger(L, sizeof(t));                                \
        return 1;                                                     \
    }
zwrite(i8, int8_t, integer);
zwrite(u8, uint8_t, integer);
zwrite(i16, int16_t, integer);
zwrite(u16, uint16_t, integer);
zwrite(i32, int32_t, integer);
zwrite(u32, uint32_t, integer);
zwrite(i64, int64_t, integer);
zwrite(u64, uint64_t, integer);
zwrite(f32, float, number);
zwrite(f64, double, number);

/**
Get the data according to the starting position and length (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:toStr(offset,length)
@int The starting position of the data (the starting position is 0), the default value is also 0
@int The length of the data, the default is all data
@return string read data
@usage
local s = buff:toStr(0,5)--Read the first five bytes of data
local s = buff:toStr() -- take out the entire zbuff data
local s = buff:toStr(0, buff:used()) -- take out the used part, the same as buff:query()*/
static int l_zbuff_toStr(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int start = luaL_optinteger(L, 2, 0);
    if (start > buff->len)
        start = buff->len;
    int len = luaL_optinteger(L, 3, buff->len);
    if (start + len > buff->len)
        len = buff->len - start;
    lua_pushlstring(L, (const char*)(buff->addr + start), len);
    return 1;
}

/**
Get the length of the zbuff object (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:len()
@return int The length of the zbuff object
@usage
len = buff:len()
len = #buff*/
static int l_zbuff_len(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    lua_pushinteger(L, buff->len);
    return 1;
}

/**
Set the FrameBuffer property of the buff object (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:setFrameBuffer(width,height,bit,color)
@int FrameBuffer width
@int FrameBuffer height
@int FrameBuffer color bit depth
@int Initial color of FrameBuffer
@return bool If the setting is successful, it will return true.
@usage
result = buff:setFrameBuffer(320,240,16,0xffff)*/
static int l_zbuff_set_frame_buffer(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    //Check if there is enough space
    if((luaL_checkinteger(L, 2) * luaL_checkinteger(L, 3) * luaL_checkinteger(L, 4) - 1) / 8 + 1 > buff->len)
        return 0;
    buff->width = luaL_checkinteger(L,2);
    buff->height = luaL_checkinteger(L,3);
    buff->bit = luaL_checkinteger(L,4);
    if (lua_isinteger(L, 5))
    {
        LUA_INTEGER color = luaL_checkinteger(L, 5);
        uint32_t i;
        for (i = 0; i < buff->width * buff->height; i++)
            set_framebuffer_point(buff, i, color);
    }
    lua_pushboolean(L,1);
    return 1;
}

/**
Set or get the color of a certain pixel in the FrameBuffer (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:pixel(x,y,color)
@int The distance from the leftmost point, the range is 0~width-1
@int The distance from the top, the range is 0~height-1
@int color, if left blank, it means to get the color of the position
@return any When setting the color, true will be returned if the setting is successful; when reading the color, the value of the color will be returned, and nil will be returned if the reading fails.
@usage
rerult = buff:pixel(0,3,0)
color = buff:pixel(0,3)*/
static int l_zbuff_pixel(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    uint32_t x = luaL_checkinteger(L,2);
    uint32_t y = luaL_checkinteger(L,3);
    if(x>=buff->width||y>=buff->height)
        return 0;
    if (lua_isinteger(L, 4))
    {
        LUA_INTEGER color = luaL_checkinteger(L, 4);
        set_framebuffer_point(buff, x + y * buff->width, color);
        lua_pushboolean(L,1);
        return 1;
    }
    else
    {
        lua_pushinteger(L,get_framebuffer_point(buff,x + y * buff->width));
        return 1;
    }
}

/**
Draw a line (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:drawLine(x1,y1,x2,y2,color)
@int The distance between the starting coordinate point and the leftmost point, the range is 0~width-1
@int The distance between the starting coordinate point and the top edge, the range is 0~height-1
@int The distance between the end coordinate point and the leftmost point, the range is 0~width-1
@int The distance between the end coordinate point and the top edge, the range is 0~height-1
@int optional, color, default is 0
@return bool If the painting is successful, it will return true.
@usage
rerult = buff:drawLine(0,0,2,3,0xffff)*/
#define abs(n) (n>0?n:-n)
static int l_zbuff_draw_line(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    if(buff->width<=0) return 0;//Not framebuffer data
    uint32_t x0 = luaL_checkinteger(L,2);
    uint32_t y0 = luaL_checkinteger(L,3);
    uint32_t x1 = luaL_checkinteger(L,4);
    uint32_t y1 = luaL_checkinteger(L,5);
    uint32_t color = luaL_optinteger(L,6,0);

    //Code reference https://blog.csdn.net/qq_43405938/article/details/102700922
    int x = x0, y = y0, dx = x1 - x0, dy = y1 - y0;
    int max = (abs(dy) > abs(dx)) ? abs(dy) : abs(dx);
    int min = (abs(dy) > abs(dx)) ? abs(dx) : abs(dy);
    float e = 2 * min - max;
    for (int i = 0; i < max; i++)
    {
        if(x>=0&&y>=0&&x<buff->width&&y<buff->height)
            set_framebuffer_point(buff,x+y*buff->width,color);
        if (e >= 0)
        {
            e = e - 2 * max;
            (abs(dy) > abs(dx)) ? (dx >= 0 ? x++ : x--) : (dy >= 0 ? y++ : y--);
        }
        e += 2 * min;
        (abs(dy) > abs(dx)) ? (dy >= 0 ? y++ : y--) : (dx >= 0 ? x++ : x--);
    }

    lua_pushboolean(L,1);
    return 1;
}

/**
Draw a rectangle (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:drawRect(x1,y1,x2,y2,color,fill)
@int The distance between the starting coordinate point and the leftmost point, the range is 0~width-1
@int The distance between the starting coordinate point and the top edge, the range is 0~height-1
@int The distance between the end coordinate point and the leftmost point, the range is 0~width-1
@int The distance between the end coordinate point and the top edge, the range is 0~height-1
@int optional, color, default is 0
@bool optional, whether to fill in internally, default nil
@return bool If the painting is successful, it will return true.
@usage
rerult = buff:drawRect(0,0,2,3,0xffff)*/
#define CHECK0(n,max) if(n<0)n=0;if(n>=max)n=max-1
static int l_zbuff_draw_rectangle(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    if(buff->width<=0) return 0;//Not framebuffer data
    int32_t x1 = (int32_t)luaL_checkinteger(L,2);  CHECK0(x1,buff->width);
    int32_t y1 = (int32_t)luaL_checkinteger(L,3);  CHECK0(y1,buff->height);
    int32_t x2 = (int32_t)luaL_checkinteger(L,4);  CHECK0(x2,buff->width);
    int32_t y2 = (int32_t)luaL_checkinteger(L,5);  CHECK0(y2,buff->height);
    int32_t color = (int32_t)luaL_optinteger(L,6,0);
    uint8_t fill = lua_toboolean(L,7);
    int x,y;
    int32_t xmax=x1>x2?x1:x2,xmin=x1>x2?x2:x1,ymax=y1>y2?y1:y2,ymin=y1>y2?y2:y1;
    if(fill){
        for(x=xmin;x<=xmax;x++)
            for(y=ymin;y<=ymax;y++)
                set_framebuffer_point(buff,x+y*buff->width,color);
    }else{
        for(x=xmin;x<=xmax;x++){
            set_framebuffer_point(buff,x+ymin*buff->width,color);
            set_framebuffer_point(buff,x+ymax*buff->width,color);
        }
        for(y=ymin;y<=ymax;y++){
            set_framebuffer_point(buff,xmin+y*buff->width,color);
            set_framebuffer_point(buff,xmax+y*buff->width,color);
        }
    }
    lua_pushboolean(L,1);
    return 1;
}

/**
Draw a circle (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff:drawCircle(x,y,r,color,fill)
@int The distance between **circle center** and the leftmost side, the range is 0~width-1
@int **The distance between the center of the circle** and the top edge, the range is 0~height-1
@int radius of circle
@int optional, circle color, default is 0
@bool optional, whether to fill in internally, default nil
@return bool If the painting is successful, it will return true.
@usage
rerult = buff:drawCircle(15,5,3,0xC)
rerult = buff:drawCircle(15,5,3,0xC,true)*/
#define DRAW_CIRCLE_ALL(buff, xc, yc, x, y, c)                                \
    {                                                                          \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc + x) + (yc + y) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc - x) + (yc + y) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc + x) + (yc - y) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc - x) + (yc - y) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc + y) + (yc + x) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc - y) + (yc + x) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc + y) + (yc - x) * buff->width, c); \
        if (x >= 0 && y >= 0 && x < buff->width && y < buff->height)           \
            set_framebuffer_point(buff, (xc - y) + (yc - x) * buff->width, c); \
    }
static int l_zbuff_draw_circle(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    if(buff->width<=0) return 0;//Not framebuffer data
    int32_t xc = luaL_checkinteger(L,2);
    int32_t yc = luaL_checkinteger(L,3);
    int32_t r = luaL_checkinteger(L,4);
    int32_t color = luaL_optinteger(L,5,0);
    uint8_t fill = lua_toboolean(L,6);

    //Code reference https://www.cnblogs.com/wlzy/p/8695226.html
    //The circle is not in the visible area
    if (xc + r < 0 || xc - r >= buff->width || yc + r < 0 || yc - r >= buff->height)
        return 0;

    int x = 0, y = r, yi, d;
    d = 3 - 2 * r;

    while (x <= y)
    {
        if (fill)
        {
            for (yi = x; yi <= y; yi++)
                DRAW_CIRCLE_ALL(buff, xc, yc, x, yi, color);
        }
        else
        {
            DRAW_CIRCLE_ALL(buff, xc, yc, x, y, color);
        }
        if (d < 0)
        {
            d = d + 4 * x + 6;
        }
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
    lua_pushboolean(L,1);
    return 1;
}

/**
Read and write data in the form of subscripts (independent of the current pointer position; the pointer position remains unchanged after execution)
@api buff[n]
@int Number of data, subscript starting from 0 (C standard)
@return number The data at this location
@usage
buff[0] = 0xc8
local data = buff[0]*/
static int l_zbuff_index(lua_State *L)
{
    //luat_zbuff_t **pp = luaL_checkudata(L, 1, LUAT_ZBUFF_TYPE);
    // int i;

    luaL_getmetatable(L, LUAT_ZBUFF_TYPE);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);

    if (lua_isnil(L, -1))
    {
        /* found no method, so get value from userdata. */
        luat_zbuff_t *buff = tozbuff(L);
        int o = luaL_checkinteger(L, 2);
        if (o >= buff->len)
            return 0;
        lua_pushinteger(L, buff->addr[o]);
        return 1;
    };
    return 1;
}

static int l_zbuff_newindex(lua_State *L)
{
    if (lua_isinteger(L, 2))
    {
        luat_zbuff_t *buff = tozbuff(L);
        if (lua_isinteger(L, 2))
        {
            int o = luaL_checkinteger(L, 2);
            int n = luaL_checkinteger(L, 3) % 256;
            if (o > buff->len)
                return 0;
            buff->addr[o] = n;
        }
    }
    return 0;
}

// __gc l_zbuff_gc is the default gc function for zbuff. When gc is called, it will release the requested memory and gc out the zbuff. The following is a comment for the user to manually call

/**
Release the memory applied by zbuff Note: zbuff and the memory applied by zbuff will be automatically released during gc, so there is usually no need to call this function. Please make sure you understand the purpose of this function before calling it! Calling this function will not release the zbuff, it will only release the memory requested by the zbuff. The zbuff needs to be automatically released after the GC! ! !
@api buff:free()
@usage
buff:free()*/
static int l_zbuff_gc(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    if (buff->addr){
        luat_heap_opt_free(buff->type,buff->addr);
        buff->addr = NULL;
        buff->len = 0;
        buff->used = 0;
    }
    return 0;
}

int __zbuff_resize(luat_zbuff_t *buff, uint32_t new_size)
{
	void *p = luat_heap_opt_realloc(buff->type, buff->addr, new_size?new_size:1);
	if (p)
	{
		buff->addr = p;
		buff->len = new_size;
		buff->used = (buff->len > buff->used)?buff->used:buff->len;
		return 0;
	}
	else
	{
        LLOGE("zbuff realloc failed %d -> %d", buff->len, new_size);
		return -1;
	}
}

/**
Adjust the size of the actual allocated space of zbuff, similar to the effect of realloc, new = realloc(old, n), which can be expanded or reduced (if len is smaller than used after reduction, then used=new len)
@api buff:resize(n)
@int new space size
@usage
buff:resize(20)*/
static int l_zbuff_resize(lua_State *L)
{
	luat_zbuff_t *buff = tozbuff(L);
	if (lua_isinteger(L, 2))
	{
		uint32_t n = luaL_checkinteger(L, 2);
		__zbuff_resize(buff, n);
	}
	return 0;
}

/**
zbuff dynamically writes data, similar to the memcpy effect, and dynamically expands the space when the original space is insufficient.
@api buff:copy(start, para,...)
@int Write the starting position of the buff. If it is not a number, it is the used of the buff. If it is less than 0, it counts forward from used, -1 = used - 1
@any writes the buff data, which is one parameter for string or zbuff, and multiple parameters for number.
@return number The length of data successfully written
@usage
local len = buff:copy(nil, "123") -- similar to memcpy(&buff[used], "123", 3) used+= 3 starts writing data from buff, and the pointer moves backward accordingly
local len = buff:copy(0, "123") -- similar to memcpy(&buff[0], "123", 3) if (used < 3) used = 3 writes data from position 0, the pointer may move
local len = buff:copy(2, 0x1a,0x30,0x31,0x32,0x00,0x01) -- similar to memcpy(&buff[2], [0x1a,0x30,0x31,0x32,0x00,0x01], 6) if ( used < (2+6)) used = (2+6) Starting from position 2, write multiple bytes of data numerically
local len = buff:copy(9, buff2) -- similar to memcpy(&buff[9], &buff2[0], buff2's used) if (used < (9+buff2's used)) used = (9+buff2's used) Starting from position 9, merge into the content 0~used in buff2
local len = buff:copy(5, buff2, 10, 1024) -- similar to memcpy(&buff[5], &buff2[10], 1024) if (used < (5+1024)) used = (5+1024)*/
static int l_zbuff_copy(lua_State *L)
{
	luat_zbuff_t *buff = tozbuff(L);
	int temp_cursor = luaL_optinteger(L, 2, buff->used);
	if (temp_cursor < 0)
	{
		temp_cursor = buff->used + temp_cursor;
		if (temp_cursor < 0)
		{
			lua_pushinteger(L, 0);
			return 1;
		}
	}
    if (lua_isinteger(L, 3))
    {
        int len = 0;
        int data = 0;
        while (lua_isinteger(L, 3 + len))
        {
        	if (temp_cursor > buff->len)
        	{
        		if (__zbuff_resize(buff, temp_cursor * 2))
        		{
        	        lua_pushinteger(L, len);
        	        return 1;
        		}
        	}
            data = luaL_checkinteger(L, 3 + len);
            *(uint8_t *)(buff->addr + temp_cursor) = data % 0x100;
            temp_cursor++;
            len++;
        }
        buff->used = (temp_cursor > buff->used)?temp_cursor:buff->used;
        lua_pushinteger(L, len);
        return 1;
    }
    else if (lua_isstring(L, 3))
    {
        size_t len;
        const char *data = luaL_checklstring(L, 3, &len);
        if (len + temp_cursor > buff->len) //prevent crossing the boundary
        {
        	if (__zbuff_resize(buff, buff->len + len + temp_cursor))
        	{
    	        lua_pushinteger(L, 0);
    	        return 1;
        	}
        }
        memcpy(buff->addr + temp_cursor, data, len);
        temp_cursor = temp_cursor + len;
        buff->used = (temp_cursor > buff->used)?temp_cursor:buff->used;
        lua_pushinteger(L, len);
        return 1;
    }
    else if (lua_isuserdata(L, 3))
    {
        luat_zbuff_t *copy_buff = ((luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE));
        uint32_t start =  luaL_optinteger(L, 4, 0);
        uint32_t len =  luaL_optinteger(L, 5, copy_buff->used);
        if (len + temp_cursor > buff->len) //prevent crossing the boundary
        {
        	if (__zbuff_resize(buff, buff->len + len + temp_cursor))
        	{
    	        lua_pushinteger(L, 0);
    	        return 1;
        	}
        }
        memcpy(buff->addr + temp_cursor, copy_buff->addr + start, len);
        temp_cursor += len;
        buff->used = (temp_cursor > buff->used)?temp_cursor:buff->used;
        lua_pushinteger(L, len);
        return 1;
    }
    lua_pushinteger(L, 0);
    return 1;
}

/**
Get the offset from the last data location pointer in zbuff to the first address to indicate the amount of valid data in zbuff. Note that this is different from the allocated space size. Since seek() will change the last data location pointer, it also Will affect the return value of used().
@api buff:used()
@return int effective data size
@usage
buff:used()*/
static int l_zbuff_used(lua_State *L)
{
	luat_zbuff_t *buff = tozbuff(L);
    lua_pushinteger(L, buff->used);
    return 1;
}

/**
Delete a piece of data in the zbuff 0~used range. Note that it only changes the used value, and does not actually clear the data in the ram.
@api buff:del(offset,length)
@int starting position start, default 0, if <0, count forward from used, for example -1 then start= used - 1
@int length del_len, the default is used, if the value of start + del_len is greater than used, del_len = used - start will be forcibly adjusted
@usage
buff:del(1,4) --Delete 4 bytes of data starting from position 1
buff:del(-1,4) --Delete 4 bytes of data starting from position used-1, but this will definitely exceed used, so del_len will be adjusted to 1, which actually deletes the last byte*/
static int l_zbuff_del(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int start = luaL_optinteger(L, 2, 0);
    if (start < 0)
    {
    	start += buff->used;
    	if (start < 0)
    	{
    		return 0;
    	}
    }

    if (start >= (int)buff->used)
        return 0;

    uint32_t len = luaL_optinteger(L, 3, buff->used);
    if (start + len > buff->used)
        len = buff->used - start;
    if (!len)
    {
    	return 0;
    }
    if ((start + len) == buff->used)
    {
    	buff->used = start;
    }
    else
    {
		uint32_t rest = buff->used - len;
		memmove(buff->addr + start, buff->addr + start + len, rest);
		buff->used = rest;
    }

    return 0;
}

static uint32_t BytesGetBe32(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static uint32_t BytesGetLe32(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/**
Take out the data according to the starting position and length from 0 to used. If it is 1, 2, 4, or 8 bytes, convert it into floating point or integer according to the subsequent parameters.
@api buff:query(offset,length,isbigend,issigned,isfloat)
@int The starting position of the data (the starting position is 0)
@int length of data
@boolean Whether it is big-endian format. If it is nil, it will not be converted and the byte stream will be output directly.
@boolean Whether it is signed, the default is false
@boolean Whether it is a floating point type, the default is false
@return string read data
@usage
local s = buff:query(0,5)--Read the first five bytes of data*/
static int l_zbuff_query(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int start = luaL_optinteger(L, 2, 0);
    if (start < 0)
    {
    	start += buff->used;
    	if (start < 0)
    	{
    		lua_pushnil(L);
    		return 1;
    	}
    }
    if (start > buff->used)
        start = buff->used;
    uint32_t len = luaL_optinteger(L, 3, buff->used);
    if (start + len > buff->used)
        len = buff->used - start;
    if (!len)
    {
		lua_pushnil(L);
		return 1;
    }
    if (lua_isboolean(L, 4) && (len <= 8))
    {
    	int is_bigend = lua_toboolean(L, 4);
    	int is_float = 0;
    	int is_signed = 0;
    	if (lua_isboolean(L, 5))
    	{
    		is_signed = lua_toboolean(L, 5);
    	}
    	if (lua_isboolean(L, 6))
    	{
    		is_float = lua_toboolean(L, 6);
    	}
    	uint8_t *p = buff->addr + start;
    	uint8_t uc;
    	int16_t s;
    	uint16_t us;
    	int32_t i;
    	// uint32_t ui;
    	int64_t l;
    	float f;
    	double d;
    	switch(len)
    	{
    	case 1:
    		if (is_signed)
    		{
    			i = (p[0] & 0x80)?(p[0] + 0xffffff00):p[0];
    			lua_pushinteger(L, i);
    		}
    		else
    		{
    			uc = p[0];
    			lua_pushinteger(L, uc);
    		}
    		break;
    	case 2:
    		if (is_bigend)
    		{
    			us = (p[0] << 8) | p[1];
    		}
    		else
    		{
    			us = (p[1] << 8) | p[0];
    		}
    		if (is_signed)
    		{
    			s = us;
    			lua_pushinteger(L, s);
    		}
    		else
    		{
    			lua_pushinteger(L, us);
    		}
    		break;
    	case 4:
    		if (is_float)
    		{
    			memcpy(&f, p, len);
    			lua_pushnumber(L, f);
    		}
    		else
    		{
        		if (is_bigend)
        		{
        			i = BytesGetBe32(p);
        		}
        		else
        		{
        			i = BytesGetLe32(p);
        		}
        		lua_pushinteger(L, i);
    		}

    		break;
    	case 8:
    		if (is_float)
    		{
    			memcpy(&d, p, len);
    			lua_pushnumber(L, d);
    		}
    		else
    		{
        		if (is_bigend)
        		{
        			l = BytesGetBe32(p + 4) | ((int64_t)BytesGetBe32(p) << 32);
        		}
        		else
        		{
        			l = BytesGetLe32(p) | ((int64_t)BytesGetLe32(p + 4) << 32);
        		}
        		lua_pushinteger(L, l);
    		}
    		break;
    	default:
    		lua_pushnil(L);
    	}
    	return 1;
    }
    lua_pushlstring(L, (const char*)(buff->addr + start), len);
    return 1;
}

/**
The operation of zbuff is similar to memset, similar to memset(&buff[start], num, len). Of course, there is ram out-of-bounds protection, which will have certain restrictions on len.
@api buff:set(start, num, len)
@int optional, starting position, default is 0,
@int Optional, default is 0. the value to set to
@int Optional, length, default is the entire space, if it exceeds the range, it will be automatically truncated
@usage
-- All initialized to 0
buff:set() -- equivalent to memset(buff, 0, sizeof(buff))
buff:set(8) -- equivalent to memset(&buff[8], 0, sizeof(buff) - 8)
buff:set(0, 0x55) -- equivalent to memset(buff, 0x55, sizeof(buff))
buff:set(4, 0xaa, 12) -- etc. used for memset(&buff[4], 0xaa, 12)*/
static int l_zbuff_set(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    int num = luaL_optinteger(L, 3, 0);
    uint32_t start = luaL_optinteger(L, 2, 0);
    uint32_t len = luaL_optinteger(L, 4, buff->len);
    memset(buff->addr + start, num & 0x00ff, ((len + start) > buff->len)?(buff->len - start):len);
    return 0;
}

/**
The operation of zbuff is similar to memcmp, similar to memcmp(&buff[start], &buff2[start2], len)
@api buff:isEqual(start, buff2, start2, len)
@int optional, starting position, default is 0,
@zbuff comparison object
@int Optional, starting position of the compared object, default is 0
@int comparison length
@return boolean true is equal, false is not equal
@return int If equal, return 0; if not equal, return the serial number of the first unequal position.
@usage
local result, offset = buff:isEqual(1, buff2, 2, 10) --Equal to memcmp(&buff[1], &buff2[2], 10)*/
static int l_zbuff_equal(lua_State *L)
{
    luat_zbuff_t *buff = tozbuff(L);
    uint32_t offset1 = luaL_optinteger(L, 2, 0);
    luat_zbuff_t *buff2 = ((luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE));
    uint32_t offset2 = luaL_optinteger(L, 4, 0);
    uint32_t len = luaL_optinteger(L, 5, 1);
    uint32_t i;
    uint8_t *b1 = buff->addr + offset1;
    uint8_t *b2 = buff2->addr + offset2;
    for(i = 0; i < len; i++) {
    	if (b1[i] != b2[i]) {
    		lua_pushboolean(L, 0);
    		lua_pushinteger(L, i);
    		return 2;
    	}
    }
	lua_pushboolean(L, 1);
	lua_pushinteger(L, 0);
	return 2;
}

static const luaL_Reg lib_zbuff[] = {
    {"write", l_zbuff_write},
    {"read", l_zbuff_read},
    {"clear", l_zbuff_clear},
    {"seek", l_zbuff_seek},
    {"pack", l_zbuff_pack},
    {"unpack", l_zbuff_unpack},
    {"get", l_zbuff_index},
    {"readI8", l_zbuff_read_i8},
    {"readI16", l_zbuff_read_i16},
    {"readI32", l_zbuff_read_i32},
    {"readI64", l_zbuff_read_i64},
    {"readU8", l_zbuff_read_u8},
    {"readU16", l_zbuff_read_u16},
    {"readU32", l_zbuff_read_u32},
    {"readU64", l_zbuff_read_u64},
    {"readF32", l_zbuff_read_f32},
    {"readF64", l_zbuff_read_f64},
    {"writeI8", l_zbuff_write_i8},
    {"writeI16", l_zbuff_write_i16},
    {"writeI32", l_zbuff_write_i32},
    {"writeI64", l_zbuff_write_i64},
    {"writeU8", l_zbuff_write_u8},
    {"writeU16", l_zbuff_write_u16},
    {"writeU32", l_zbuff_write_u32},
    {"writeU64", l_zbuff_write_u64},
    {"writeF32", l_zbuff_write_f32},
    {"writeF64", l_zbuff_write_f64},
    {"toStr", l_zbuff_toStr},
    {"len", l_zbuff_len},
    {"setFrameBuffer", l_zbuff_set_frame_buffer},
    {"pixel", l_zbuff_pixel},
    {"drawLine", l_zbuff_draw_line},
    {"drawRect", l_zbuff_draw_rectangle},
    {"drawCircle", l_zbuff_draw_circle},
    //{"__index", l_zbuff_index},
    //{"__len", l_zbuff_len},
    //{"__newindex", l_zbuff_newindex},
    {"free", l_zbuff_gc},
	//The following is extended usage. The data increase and decrease operations should not be used together with the above read and write. The usage of numerical pointers is inconsistent.
	{"copy", l_zbuff_copy},
	{"set", l_zbuff_set},
	{"query",l_zbuff_query},
	{"del", l_zbuff_del},
	{"resize", l_zbuff_resize},
	{"reSize", l_zbuff_resize},
	{"used", l_zbuff_used},
	{"isEqual", l_zbuff_equal},
    {NULL, NULL}};

static int luat_zbuff_meta_index(lua_State *L) {
    if (lua_isinteger(L, 2)) {
        return l_zbuff_index(L);
    }
    if (lua_isstring(L, 2)) {
        const char* keyname = luaL_checkstring(L, 2);
        //printf("zbuff keyname = %s\n", keyname);
        int i = 0;
        while (1) {
            if (lib_zbuff[i].name == NULL) break;
            if (!strcmp(keyname, lib_zbuff[i].name)) {
                lua_pushcfunction(L, lib_zbuff[i].func);
                return 1;
            }
            i++;
        }
    }
    return 0;
}

static void createmeta(lua_State *L)
{
    luaL_newmetatable(L, LUAT_ZBUFF_TYPE); /* create metatable for file handles */
    // lua_pushvalue(L, -1);                  /* push metatable */
    // lua_setfield(L, -2, "__index");        /* metatable.__index = metatable */
    // luaL_setfuncs(L, lib_zbuff, 0);        /* add file methods to new metatable */
    //luaL_setfuncs(L, lib_zbuff_metamethods, 0);
    //luaL_setfuncs(L, lib_zbuff, 0);

    lua_pushcfunction(L, l_zbuff_len);
    lua_setfield(L, -2, "__len");
    lua_pushcfunction(L, l_zbuff_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, luat_zbuff_meta_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_zbuff_newindex);
    lua_setfield(L, -2, "__newindex");

    lua_pop(L, 1); /* pop new metatable */
    //luaL_newlib(L, lib_zbuff);
}

#include "rotable2.h"
static const rotable_Reg_t reg_zbuff[] =
    {
        {"create",  ROREG_FUNC(l_zbuff_create)},
        //@const SEEK_SET number based on head
        {"SEEK_SET", ROREG_INT(ZBUFF_SEEK_SET)},
        //@const SEEK_CUR number based on current position
        {"SEEK_CUR", ROREG_INT(ZBUFF_SEEK_CUR)},
        //@const SEEK_END number is based on the end
        {"SEEK_END", ROREG_INT(ZBUFF_SEEK_END)},
        //@const HEAP_AUTO number automatically applies (if psram exists, apply in psram, if it does not exist or fails, apply in sram)
        {"HEAP_AUTO",   ROREG_INT(LUAT_HEAP_AUTO)},
        //@const HEAP_SRAM number applies in sram
        {"HEAP_SRAM",   ROREG_INT(LUAT_HEAP_SRAM)},
        //@const HEAP_PSRAM number Apply in psram
        {"HEAP_PSRAM",  ROREG_INT(LUAT_HEAP_PSRAM)},
        {NULL,       ROREG_INT(0)
    }
};

LUAMOD_API int luaopen_zbuff(lua_State *L)
{
    luat_newlib2(L, reg_zbuff);
    createmeta(L);
    return 1;
}
