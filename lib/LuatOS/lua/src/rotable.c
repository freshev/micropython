
/**
 * rotable implements a read-only table under Lua, which is usually only used for library registration.</p>
 * In principle, it is to declare a userdata memory block, and then disguise it as a table through the metatable.<p/>
 * Because the metatable is shared and the userdata memory block is small (less than 30 bytes), and it is not related to the number of methods, it saves a lot of memory.
 **/
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "lua.h"
#include "rotable.h"


/* The lookup code uses binary search on sorted `rotable_Reg` arrays
 * to find functions/methods. For a small number of elements a linear
 * search might be faster. */
#ifndef ROTABLE_BINSEARCH_MIN
#  define ROTABLE_BINSEARCH_MIN  5
#endif


typedef struct {
#if LUA_VERSION_NUM < 503
  /* on Lua 5.3 we use a lightuserdata in the uservalue of the
   * userdata to hold the pointer to the luaL_Reg structure. Older
   * Lua versions have to store it in the userdata payload. */
  rotable_Reg const* p;
#endif
  /* number of elements in the luaL_Reg array *if* it is sorted by
   * name. We can use binary search in this case. Otherwise we need
   * linear searches anyway and we can scan for the final `{NULL,0}`
   * entry. `n` is 0 in this case. */
  int n;
} rotable;


static char const unique_address[ 1 ] = { 0 };


static int reg_compare(void const* a, void const* b) {
  return strcmp( (char const*)a, ((rotable_Reg const*)b)->name );
}


static rotable* check_rotable( lua_State* L, int idx, char const* func ) {
  rotable* t = (rotable*)lua_touserdata( L, idx );
  if( t ) {
    if( lua_getmetatable( L, idx ) ) {
      lua_pushlightuserdata( L, (void*)unique_address );
      lua_rawget( L, LUA_REGISTRYINDEX );
      if( !lua_rawequal( L, -1, -2 ) )
        t = 0;
      lua_pop( L, 2 );
    }
  }
  if( !t ) {
    char const* type = lua_typename( L, lua_type( L, idx ) );
    if( lua_type( L, idx ) == LUA_TLIGHTUSERDATA ) {
      type = "light userdata";
    } else if( lua_getmetatable( L, idx ) ) {
      lua_getfield( L, -1, "__name" );
      lua_replace( L, -2 ); /* we don't need the metatable anymore */
      if( lua_type( L, -1 ) == LUA_TSTRING )
        type = lua_tostring( L, -1 );
    }
    lua_pushfstring( L, "bad argument #%d to '%s' "
                     "(rotable expected, got %s)", idx, func, type );
    lua_error( L );
  }
  return t;
}


static rotable_Reg const* find_key( rotable_Reg const* p, int n,
                                    char const* s ) {
  if( s ) {
    //if( n >= ROTABLE_BINSEARCH_MIN ) { /* binary search */
    //  return (rotable_Reg const*)bsearch( s, p, n, sizeof( *p ), reg_compare );
    //} else { /* use linear scan */
      for( ; p->name != NULL; ++p ) {
        if( 0 == reg_compare( s, p ) )
          return p;
      }
    //}
  }
  return 0;
}


static int rotable_func_index( lua_State* L ) {
  char const* s = lua_tostring( L, 2 );
  rotable_Reg const* p = (rotable_Reg const*)lua_touserdata( L, lua_upvalueindex( 1 ) );
  rotable_Reg const* p2 = p;
  int n = lua_tointeger( L, lua_upvalueindex( 2 ) );
  p = find_key( p, n, s );
  if( p ) {
    if (p->func)
      lua_pushcfunction( L, p->func );
    else
      lua_pushinteger(L, p->value);
  }
  else {
    // Check if the first method is __index, if so, call it
    if (p2->name != NULL && !strcmp("__index", p2->name)) {
      lua_pushcfunction(L, p2->func);
      lua_pushvalue(L, 2);
      lua_call(L, 1, 1);
      return 1;
    }
    lua_pushnil( L );
  }
  return 1;
}


static int rotable_udata_index( lua_State* L ) {
  rotable* t = (rotable*)lua_touserdata( L, 1 );
  char const* s = lua_tostring( L, 2 );
#if LUA_VERSION_NUM < 503
  rotable_Reg const* p = t->p;
#else
  rotable_Reg const* p = 0;
  lua_getuservalue( L, 1 );
  p = (rotable_Reg const*)lua_touserdata( L, -1 );
  rotable_Reg const* p2 = p;
#endif
  p = find_key( p, t->n, s );
  if( p ) {
    if (p->func)
      lua_pushcfunction( L, p->func );
    else
      lua_pushinteger(L, p->value);
  }
  else {
    // Check if the first method is __index, if so, call it
    if (p2->name != NULL && !strcmp("__index", p2->name)) {
      lua_pushcfunction(L, p2->func);
      lua_pushvalue(L, 2);
      lua_call(L, 1, 1);
      return 1;
    }
    lua_pushnil( L );
  }
  return 1;
}


static int rotable_udata_len( lua_State* L ) {
  lua_pushinteger( L, 0 );
  return 1;
}


static int rotable_iter( lua_State* L ) {
  check_rotable( L, 1, "__pairs iterator" );
  char const* s = lua_tostring( L, 2 );
  rotable_Reg const* q = 0;
#if LUA_VERSION_NUM < 503
  rotable_Reg const* p = t->p;
#else
  rotable_Reg const* p = 0;
  lua_getuservalue( L, 1 );
  p = (rotable_Reg const*)lua_touserdata( L, -1 );
#endif
  if( s ) {
    // if( t->n >= ROTABLE_BINSEARCH_MIN ) { /* binary search */
    //   q = (rotable_Reg const*)bsearch( s, p, t->n, sizeof( *p ), reg_compare );
    //   if( q )
    //     ++q;
    //   else
    //     q = p + t->n;
    // } else { /* use linear scan */
      for( q = p; q->name != NULL; ++q ) {
        if( 0 == reg_compare( s, q ) ) {
          ++q;
          break;
        }
      }
    // }
  } else
    q = p;
  if (q == NULL || q->name == NULL) {

  }
  else if( q->func ) {
    lua_pushstring( L, q->name );
    lua_pushcfunction( L, q->func );
    return 2;
  }
  else {
    lua_pushstring( L, q->name );
    lua_pushinteger( L, q->value );
    return 2;
  }
  return 0;
}


static int rotable_udata_pairs( lua_State* L ) {
  lua_pushcfunction( L, rotable_iter );
  lua_pushvalue( L, 1 );
  lua_pushnil( L );
  return 3;
}

/**
 * The function corresponding to lua_newlib is used to generate a library table. The difference is that lua_newlib generates a normal table, while this function generates a rotable.*/

ROTABLE_EXPORT void rotable_newlib( lua_State* L, void const* v ) {
  rotable_Reg const* reg = (rotable_Reg const*)v;
  rotable* t = (rotable*)lua_newuserdata( L, sizeof( *t ) );
  lua_pushlightuserdata( L, (void*)unique_address );
  lua_rawget( L, LUA_REGISTRYINDEX );
  if( !lua_istable( L, -1 ) ) {
    lua_pop( L, 1 );
    lua_createtable( L, 0, 5 );
    lua_pushcfunction( L, rotable_udata_index );
    lua_setfield( L, -2, "__index" );
    lua_pushcfunction( L, rotable_udata_len );
    lua_setfield( L, -2, "__len" );
    lua_pushcfunction( L, rotable_udata_pairs );
    lua_setfield( L, -2, "__pairs" );
    lua_pushboolean( L, 0 );
    lua_setfield( L, -2, "__metatable" );
    lua_pushliteral( L, "rotable" );
    lua_setfield( L, -2, "__name" );
    lua_pushlightuserdata( L, (void*)unique_address );
    lua_pushvalue( L, -2 );
    lua_rawset( L, LUA_REGISTRYINDEX );
  }
  lua_setmetatable( L, -2 );
#if LUA_VERSION_NUM < 503
  t->p = reg;
#else
  lua_pushlightuserdata( L, (void*)reg );
  lua_setuservalue( L, -2 );
#endif
}

/**
 * Also generate metatables in rotable form for custom objects. This form requires more memory than rotable_newlib, but at least it is a solution.*/
ROTABLE_EXPORT void rotable_newidx( lua_State* L, void const* v ) {
  lua_pushlightuserdata( L, (void*)v);
  lua_pushcclosure( L, rotable_func_index, 1 );
}

