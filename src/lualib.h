/*
** $Id: lualib.h,v 1.43.1.1 2013/04/12 18:48:47 roberto Exp $
** Lua standard libraries
** See Copyright Notice in lua.h
*/


#ifndef lualib_h
#define lualib_h

#include "lua.h"


#define LUA_POLICYLIBNAME	"policies"
LUAMOD_API int (luaopen_base) (lua_State *L);

#define LUA_CONVLIBNAME	"convert"
LUAMOD_API int (luaopen_conv) (lua_State *L);

#define LUA_COLIBNAME	"coroutine"
LUAMOD_API int (luaopen_coroutine) (lua_State *L);

#define LUA_TABLIBNAME	"list"
LUAMOD_API int (luaopen_table) (lua_State *L);

#define LUA_IOLIBNAME	"iofile"
LUAMOD_API int (luaopen_io) (lua_State *L);

#define LUA_OSLIBNAME	"os"
LUAMOD_API int (luaopen_os) (lua_State *L);

#define LUA_STRLIBNAME	"string"
LUAMOD_API int (luaopen_string) (lua_State *L);

#define LUA_BITLIBNAME	"bit32"
LUAMOD_API int (luaopen_bit32) (lua_State *L);

#define LUA_MATHLIBNAME	"math"
LUAMOD_API int (luaopen_math) (lua_State *L);

#define LUA_DBLIBNAME	"sys"
LUAMOD_API int (luaopen_debug) (lua_State *L);

#define LUA_LOADLIBNAME	"lib"
LUAMOD_API int (luaopen_package) (lua_State *L);


/* open all previous libraries */
LUALIB_API void (luaL_openlibs) (lua_State *L);



#if !defined(lua_assert)
#define lua_assert(x)	((void)0)
#endif


#endif
