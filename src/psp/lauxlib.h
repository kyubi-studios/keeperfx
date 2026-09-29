/* Lua 5.4 (pspdev) wrapper: adds back the Lua 5.1 / LuaJIT names KeeperFX uses. */
#ifndef KFX_PSP_LAUXLIB_H
#define KFX_PSP_LAUXLIB_H
#include "lua.h"
#include_next <lauxlib.h>
#ifndef luaL_checkint
#define luaL_checkint(L,n)  ((int)luaL_checkinteger(L, (n)))
#endif
#ifndef luaL_checklong
#define luaL_checklong(L,n) ((long)luaL_checkinteger(L, (n)))
#endif
#ifndef luaL_optint
#define luaL_optint(L,n,d)  ((int)luaL_optinteger(L, (n), (d)))
#endif
#endif
