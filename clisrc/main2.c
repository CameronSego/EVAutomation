
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>

#define FUNCTION_LOG    0x00
#define FUNCTION_FILTER 0x01

#define CAN0 0x0
#define CAN1 0x1

void senddata(uint8_t * data, size_t data_size)
{
  printf("Sending: { ");
  for(int i = 0 ; i < data_size-1 ; i ++)
    printf("0x%02X, ", data[i]);
  printf("0x%02X } (%u)\n", data[data_size-1], data_size);
}

int lua_Log(lua_State * L)
{
  const char * options[] = { "CAN0", "CAN1", 0 };
  int portid = luaL_checkoption(L, 1, 0, options);

  uint8_t data[6];
  data[0] = FUNCTION_LOG;
  data[1] = portid;

  uint16_t arbid = luaL_checkinteger(L, 2);
  data[2] = arbid & 0xFF;
  data[3] = (arbid >> 8) & 0xFF;

  uint16_t arbid_mask = luaL_optinteger(L, 3, 0x3FF);
  data[4] = arbid_mask & 0xFF;
  data[5] = (arbid_mask >> 8) & 0xFF;

  senddata(data, 6);
  printf("Logging on port id %d!\n", portid);
  printf("arbid: 0x%04X\n", arbid);
  printf("arbid_mask: 0x%04X\n", arbid_mask);
  fflush(stdout);
  return 0;
}
int lua_Filter(lua_State * L)
{
  const char * options[] = { "CAN0", "CAN1", 0 };
  int portid = luaL_checkoption(L, 1, 0, options);

  uint8_t data[22];
  data[0] = FUNCTION_FILTER;
  data[1] = portid;

  uint16_t arbid = luaL_checkinteger(L, 2);
  data[2] = arbid & 0xFF;
  data[3] = (arbid >> 8) & 0xFF;

  uint16_t arbid_mask = luaL_optinteger(L, 3, 0x3FF);
  data[4] = arbid_mask & 0xFF;
  data[5] = (arbid_mask >> 8) & 0xFF;

  luaL_checktype(L, 4, LUA_TTABLE);

  for(unsigned i = 0 ; i < 8 ; i ++) {
    lua_rawgeti(L, 4, i + 1);
    data[6 + i] = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  if(lua_type(L, 5) == LUA_TTABLE) {
    for(unsigned i = 0 ; i < 8 ; i ++) {
      lua_rawgeti(L, 5, i + 1);
      data[14 + i] = lua_tointeger(L, -1);
      lua_pop(L, 1);
    }
  }

  senddata(data, 22);
  printf("Filtering on port id %d!\n", portid);
  printf("arbid: 0x%04X\n", arbid);
  printf("arbid_mask: 0x%04X\n", arbid_mask);
  fflush(stdout);
  return 0;
}

int main(int argc, char ** argv)
{
  int pid = fork();

  if(pid == 0)
    system("cat /dev/ttyS4");

  printf("Spawned cat process with pid %d\n", pid);

  if(argc == 2)
  {
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, lua_Log); lua_setglobal(L, "Log");
    lua_pushcfunction(L, lua_Filter); lua_setglobal(L, "Filter");

    const char * scriptfile = argv[1];
    printf("Executing script: '%s'\n", scriptfile);

    luaL_dofile(L, scriptfile);
  }
  else
  {
    printf("Please specify a script to run.\n");
  }

  return 0;
}

