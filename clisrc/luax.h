#ifndef LUAX_H
#define LUAX_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define luax_absindex(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

extern lua_State * luax_L;

// Saves a function from the table at the given index (to the registry), and
// returns its registry index.  If the field is not a function, returns
// LUA_REFNIL.
// Unsaving a callback is done via luaL_unref(L, LUA_REGISTRYINDEX, ridx)
extern int luax_savecallback(lua_State * L, int idx, const char * name);

// Calls a function stored in the registry with index `regidx`. Makes the call
// using `lua_pcall`, passing in debug.traceback automagically.
// Returns 1 if the function was called successfully. Returns 0 and
// prints the traceback otherwise.
extern int luax_call(int regidx, int narg, int nres);
extern int luax_require(const char * name, int nres);

typedef struct {
  const char * name;
  int id;
} luax_ModuleFunc;
extern int luax_loadmodule(int tableidx, const char * name, luax_ModuleFunc * functions);

extern void luax_init();
extern void luax_deinit();

#endif
