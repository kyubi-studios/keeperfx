/* Lua 5.4 (pspdev) wrapper: adds back the Lua 5.1 / LuaJIT names KeeperFX uses. */
#ifndef KFX_PSP_LUA_H
#define KFX_PSP_LUA_H
#include_next <lua.h>
#ifndef lua_objlen
#define lua_objlen(L,i) lua_rawlen(L,(i))
#endif
#endif
