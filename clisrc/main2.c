
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

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
  //printf("Logging on port id %d!\n", portid);
  //printf("arbid: 0x%04X\n", arbid);
  //printf("arbid_mask: 0x%04X\n", arbid_mask);
  //fflush(stdout);
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
  //printf("Filtering on port id %d!\n", portid);
  //printf("arbid: 0x%04X\n", arbid);
  //printf("arbid_mask: 0x%04X\n", arbid_mask);
  //fflush(stdout);
  return 0;
}

pid_t start_logging(const char * device, const char * logfile)
{
  pid_t pid = fork();

  if(pid == 0) {

    FILE * infile = fopen(device, "rb");
    FILE * outfile = fopen(logfile, "w");

    if(infile && outfile)
    {
      while(true) {
        uint8_t dbytes[4 + 2 + 8];
        fread(dbytes, 1, sizeof(dbytes)/sizeof(*dbytes), infile);
        uint32_t timestamp = 
          dbytes[0] | 
          (dbytes[1] << 8) | 
          (dbytes[2] << 16) | 
          (dbytes[3] << 24);
        uint16_t arbid = 
          dbytes[4] | 
          (dbytes[5] << 8);
        fprintf(outfile, "%u:0x%04X:", timestamp, arbid);
        for(unsigned i = 0 ; i < 8 ; i ++) {
          fprintf(outfile, "%02X ", dbytes[6 + i]);
        }
        fprintf(outfile, "\n");
      }
    }
    else if(!infile) printf("LOGGER: Failed to open '%s' for reading.\n", device);
    else if(!outfile) printf("LOGGER: Failed to open '%s' for writing.\n", logfile);
  }

  printf("Spawned log process with pid %d\n", pid);
  return pid;
}
void exec_script(const char * filename)
{
  lua_State * L = luaL_newstate();
  luaL_openlibs(L);

  lua_pushcfunction(L, lua_Log); lua_setglobal(L, "Log");
  lua_pushcfunction(L, lua_Filter); lua_setglobal(L, "Filter");

  printf("Executing script: '%s'\n", filename);

  luaL_dofile(L, filename);
}

int main(int argc, char ** argv)
{
  if(argc >= 4)
  {
    pid_t pid = start_logging(argv[2], argv[3]);
    if(pid)
    {
      exec_script(argv[1]);
      printf("Killing child process %d\n", pid);
      kill(pid, SIGKILL);
    }
  }
  else
  {
    printf("Usage: CANSniffer [script] [device] [logfile]\n");
  }

  return 0;
}

