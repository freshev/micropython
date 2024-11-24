/*@Modules pack
@summary Packing and unpacking format string
@version 1.0
@date 2021.12.20
@video https://www.bilibili.com/video/BV1Sr4y1n7bP
@tag LUAT_USE_PACK
@usage
--[[
 '<' Set to little endian encoding
 '>' Set to big endian encoding
 '=' endianness follows local settings
 'z' empty string, 0 bytes
 'a' size_t string, the first 4 bytes express the length, followed by N bytes of data
 'A' specifies the length of the string, such as A8, which represents 8 bytes of data
 'f' float, 4 bytes
 'd' double, 8 bytes
 'n' Lua number, 4 bytes for 32bit firmware, 8 bytes for 64bit firmware
 'c' char , 1 byte
 'b' byte = unsigned char, 1 byte
 'h' short, 2 bytes
 'H' unsigned short, 2 bytes
 'i' int, 4 bytes
 'I' unsigned int, 4 bytes
 'l' long, 8 bytes, only 64bit firmware can obtain it correctly
 'L' unsigned long, 8 bytes, only 64bit firmware can obtain it correctly
]]

-- Please check the demo for detailed usage.*/

#define	OP_ZSTRING	      'z'		/* zero-terminated string */
#define	OP_BSTRING	      'p'		/* string preceded by length byte */
#define	OP_WSTRING	      'P'		/* string preceded by length word */
#define	OP_SSTRING	      'a'		/* string preceded by length size_t */
#define	OP_STRING	      'A'		/* string */
#define	OP_FLOAT	         'f'		/* float */
#define	OP_DOUBLE	      'd'		/* double */
#define	OP_NUMBER	      'n'		/* Lua number */
#define	OP_CHAR		      'c'		/* char */
#define	OP_BYTE		      'b'		/* byte = unsigned char */
#define	OP_SHORT	         'h'		/* short */
#define	OP_USHORT	      'H'		/* unsigned short */
#define	OP_INT		      'i'		/* int */
#define	OP_UINT		      'I'		/* unsigned int */
#define	OP_LONG		      'l'		/* long */
#define	OP_ULONG	         'L'		/* unsigned long */
#define	OP_LITTLEENDIAN	'<'		/* little endian */
#define	OP_BIGENDIAN	   '>'		/* big endian */
#define	OP_NATIVE	      '='		/* native endian */

#include <ctype.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define LUAT_LOG_TAG "pack"
#include "luat_log.h"

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
  char *a=p;
  int i,j;
  for (i=0, j=n-1, n=n/2; n--; i++, j--)
  {
   char t=a[i]; a[i]=a[j]; a[j]=t;
  }
 }
}

#define UNPACKNUMBER(OP,T)		\
   case OP:				\
   {					\
    T a;				\
    int m=sizeof(a);			\
    if (i+m>len) goto done;		\
    memcpy(&a,s+i,m);			\
    i+=m;				\
    doswap(swap,&a,m);			\
    lua_pushnumber(L,(lua_Number)a);	\
    ++n;				\
    break;				\
   }

#define UNPACKINT(OP,T)		\
   case OP:				\
   {					\
    T a;				\
    int m=sizeof(a);			\
    if (i+m>len) goto done;		\
    memcpy(&a,s+i,m);			\
    i+=m;				\
    doswap(swap,&a,m);			\
    lua_pushinteger(L,(lua_Integer)a);	\
    ++n;				\
    break;				\
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

#define UNPACKSTRING(OP,T)		\
   case OP:				\
   {					\
    T l;				\
    int m=sizeof(l);			\
    if (i+m>len) goto done;		\
    memcpy(&l,s+i,m);			\
    doswap(swap,&l,m);			\
    if (i+m+l>len) goto done;		\
    i+=m;				\
    lua_pushlstring(L,s+i,l);		\
    i+=l;				\
    ++n;				\
    break;				\
   }

/*Unpack string
@api pack.unpack(string, format, init)
@string the string to be unpacked
@string formatting symbol
@int The default value is 1, marking the beginning of unpacking
@return int The position of the string marker
@return any The first unpacked value, depending on the format value, there may be N return values
@usage
local _,a = pack.unpack(x,">h") --unpack into short (2 bytes)*/
static int l_unpack(lua_State *L) 
{
 size_t len;
 const char *s=luaL_checklstring(L,1,&len);
 const unsigned char *f= (const unsigned char*)luaL_checkstring(L,2);
 int i=luaL_optnumber(L,3,1)-1;
 int n=0;
 int swap=0;
 lua_pushnil(L);
 while (*f)
 {
  int c=*f++;
  int N=1;
  if (isdigit(*f)) 
  {
   N=0;
   while (isdigit(*f)) N=10*N+(*f++)-'0';
   if (N==0 && c==OP_STRING) { lua_pushliteral(L,""); ++n; }
  }
  while (N--) {
   if (!lua_checkstack(L, n))
    return luaL_error(L, "too many results to unpack");
   switch (c)
   {
      
      case OP_LITTLEENDIAN:
      case OP_BIGENDIAN:
      case OP_NATIVE:
      {
      swap=doendian(c);
      N=0;
      break;
      }
      case OP_STRING:
      {
      ++N;
      if (i+N>len) goto done;
      lua_pushlstring(L,s+i,N);
      i+=N;
      ++n;
      N=0;
      break;
      }
      case OP_ZSTRING:
      {
      size_t l;
      if (i>=len) goto done;
      l=strlen(s+i);
      lua_pushlstring(L,s+i,l);
      i+=l+1;
      ++n;
      break;
      }
      UNPACKSTRING(OP_BSTRING, unsigned char)
      UNPACKSTRING(OP_WSTRING, unsigned short)
      UNPACKSTRING(OP_SSTRING, size_t)
      UNPACKNUMBER(OP_NUMBER, lua_Number)
   #ifndef LUA_NUMBER_INTEGRAL   
      UNPACKNUMBER(OP_DOUBLE, double)
      UNPACKNUMBER(OP_FLOAT, float)
   #endif   
      UNPACKINT8(OP_CHAR, char)
      UNPACKINT(OP_BYTE, unsigned char)
      UNPACKINT(OP_SHORT, short)
      UNPACKINT(OP_USHORT, unsigned short)
      UNPACKINT(OP_INT, int)
      UNPACKINT(OP_UINT, unsigned int)
      UNPACKINT(OP_LONG, long)
      UNPACKINT(OP_ULONG, unsigned long)
      case ' ': case ',':
      break;
      default:
      badcode(L,c);
      break;
   }
  }
 }
done:
 lua_pushnumber(L,i+1);
 lua_replace(L,-n-2);
 return n+1;
}

#define PACKNUMBER(OP,T)			\
   case OP:					\
   {						\
    T a=(T)luaL_checknumber(L,i++);		\
    doswap(swap,&a,sizeof(a));			\
    luaL_addlstring(&b,(void*)&a,sizeof(a));	\
    break;					\
   }

#define PACKINT(OP,T)			\
   case OP:					\
   {						\
    T a=(T)luaL_checkinteger(L,i++);		\
    doswap(swap,&a,sizeof(a));			\
    luaL_addlstring(&b,(void*)&a,sizeof(a));	\
    break;					\
   }

#define PACKSTRING(OP,T)			\
   case OP:					\
   {						\
    size_t l;					\
    const char *a=luaL_checklstring(L,i++,&l);	\
    T ll=(T)l;					\
    doswap(swap,&ll,sizeof(ll));		\
    luaL_addlstring(&b,(void*)&ll,sizeof(ll));	\
    luaL_addlstring(&b,a,l);			\
    break;					\
   }

/*packed string value
@api pack.pack(format, val1, val2, val3, valn)
@string format formatting symbol
@any The first value to be packed
@any The second value to be packed
@any The second value to be packed
@any The nth value to be packed
@return string A string containing all formatting variables
@usage
local data = pack.pack('<h', crypto.crc16("MODBUS", val))
log.info("data", data, data:toHex())*/
static int l_pack(lua_State *L)
{
 int i=2;
 const unsigned char *f=(const unsigned char*)luaL_checkstring(L,1);
 int swap=0;
 luaL_Buffer b;
 luaL_buffinit(L,&b);
 while (*f)
 {
  int c=*f++;
  int N=1;
  if (isdigit(*f)) 
  {
   N=0;
   while (isdigit(*f)) N=10*N+(*f++)-'0';
  }
  while (N--) switch (c)
  {
   case OP_LITTLEENDIAN:
   case OP_BIGENDIAN:
   case OP_NATIVE:
   {
    swap=doendian(c);
    N=0;
    break;
   }
   case OP_STRING:
   case OP_ZSTRING:
   {
    size_t l;
    const char *a=luaL_checklstring(L,i++,&l);
    luaL_addlstring(&b,a,l+(c==OP_ZSTRING));
    break;
   }
   PACKSTRING(OP_BSTRING, unsigned char)
   PACKSTRING(OP_WSTRING, unsigned short)
   PACKSTRING(OP_SSTRING, size_t)
   PACKNUMBER(OP_NUMBER, lua_Number)
#ifndef LUA_NUMBER_INTEGRAL   
   PACKNUMBER(OP_DOUBLE, double)
   PACKNUMBER(OP_FLOAT, float)
#endif
   PACKINT(OP_CHAR, char)
   PACKINT(OP_BYTE, unsigned char)
   PACKINT(OP_SHORT, short)
   PACKINT(OP_USHORT, unsigned short)
   PACKINT(OP_INT, int)
   PACKINT(OP_UINT, unsigned int)
   PACKINT(OP_LONG, long)
   PACKINT(OP_ULONG, unsigned long)
   case ' ': case ',':
    break;
   default:
    badcode(L,c);
    break;
  }
 }
 luaL_pushresult(&b);
 return 1;
}

int luat_pack(lua_State *L) {
   return l_pack(L);
}

int luat_unpack(lua_State *L) {
   return l_unpack(L);
}

#include "rotable2.h"
static const rotable_Reg_t reg_pack[] =
{
	{"pack",	   ROREG_FUNC(l_pack)},
	{"unpack",	ROREG_FUNC(l_unpack)},
	{NULL,	   ROREG_INT(0) }
};

LUAMOD_API int luaopen_pack( lua_State *L ) {
    luat_newlib2(L, reg_pack);
    return 1;
}
