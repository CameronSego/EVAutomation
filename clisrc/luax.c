
#include "luax.h"
#include <stdio.h>

int luax_savecallback(lua_State * L, int idx, const char * name)
{
  lua_getfield(L, idx, name);
  if(lua_isfunction(L, -1)) {
    printf("'%s' set\n", name);
    return luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    printf("'%s' not a function\n", name);
    lua_pop(L, 1);
    return LUA_REFNIL;
  }
}

int debug_traceback_ref = LUA_NOREF;
int require_ref = LUA_NOREF;

lua_State * luax_L = 0;

int luax_call(int regidx, int narg, int nres)
{
  // put the function below the arguments
  lua_rawgeti(luax_L, LUA_REGISTRYINDEX, regidx);
  lua_insert(luax_L, -1-narg);
  // put the stack trace below the function and arguments
  lua_rawgeti(luax_L, LUA_REGISTRYINDEX, debug_traceback_ref);
  lua_insert(luax_L, -2-narg);
  if(lua_pcall(luax_L, narg, nres, -2-narg)) {
    size_t errlen;
    const char * errmsg = lua_tolstring(luax_L, -1, &errlen);
    printf("UNHANDLED LUA ERROR: %.*s\n", errlen, errmsg);
    lua_pop(luax_L, 2);
    return 0;
  }
  lua_remove(luax_L, -1-nres);
  return 1;
}

int luax_require(const char * name, int nres)
{
  lua_pushstring(luax_L, name);
  return luax_call(require_ref, 1, nres);
}

void luax_init()
{
  luax_L = luaL_newstate();

  luaL_openlibs(luax_L);

  lua_getglobal(luax_L, "debug");
  lua_getfield(luax_L, -1, "traceback");
  debug_traceback_ref = luaL_ref(luax_L, LUA_REGISTRYINDEX);
  lua_pop(luax_L, 1); // pop debug table

  lua_getglobal(luax_L, "require");
  require_ref = luaL_ref(luax_L, LUA_REGISTRYINDEX);
}
void luax_deinit()
{
  printf("lua_gettop(luax_L) at exit: %d\n", lua_gettop(luax_L));
  lua_close(luax_L);
  luax_L = 0;
}

