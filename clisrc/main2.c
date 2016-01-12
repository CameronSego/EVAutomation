
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
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <pthread.h>

#define FUNCTION_LOG    0x00
#define FUNCTION_FILTER 0x01

#define CAN0 0x0
#define CAN1 0x1

int device_fd = -1;

void senddata(uint8_t * data, size_t data_size)
{
  printf("Sending: { ");
  for(int i = 0 ; i < data_size-1 ; i ++)
    printf("0x%02X, ", data[i]);
  printf("0x%02X } (%u)\n", data[data_size-1], data_size);
  fflush(stdout);

  write(device_fd, data, data_size);
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
  else
  {
    for(unsigned i = 0 ; i < 8 ; i ++) {
      data[14 + i] = 0xFF;
    }
  }

  senddata(data, 22);
  //printf("Filtering on port id %d!\n", portid);
  //printf("arbid: 0x%04X\n", arbid);
  //printf("arbid_mask: 0x%04X\n", arbid_mask);
  //fflush(stdout);
  return 0;
}

bool LogThread_stop = false;
pthread_mutex_t LogThread_stop_mutex;

void * LogThread(void * lp)
{
  const char * logfile_path = lp;
  FILE * outfile = fopen(logfile_path, "w");

  printf("LOGGER: Flushing tty object.\n");
  fflush(stdout);
  tcflush(device_fd, TCIFLUSH);
  if(outfile)
  {
    while(true) {
      uint8_t dbytes[4 + 2 + 8];
      ssize_t n = read(device_fd, dbytes, sizeof(dbytes)/sizeof(*dbytes));
      /*
      for(unsigned i = 0 ; i < n ; i ++)
        printf("%02X ", dbytes[i]);
      printf("(%u)\n", n);
      fflush(stdout);
      */
      //printf("TOTALLY GOT HERE\n");
      uint32_t timestamp = 
        dbytes[0] | 
        (dbytes[1] << 8) | 
        (dbytes[2] << 16) | 
        (dbytes[3] << 24);
      //printf("timestamp: %u\n", timestamp);
      uint16_t arbid = 
        dbytes[4] | 
        (dbytes[5] << 8);
      //printf("arbid: %u\n", arbid);
      fprintf(outfile, "%u:0x%04X:", timestamp, arbid);
      for(unsigned i = 0 ; i < 8 ; i ++) {
        fprintf(outfile, "%02X ", dbytes[6 + i]);
      }
      fprintf(outfile, "\n");
      fflush(outfile);

      bool stop = false;
      pthread_mutex_lock(&LogThread_stop_mutex);
      stop = LogThread_stop;
      pthread_mutex_unlock(&LogThread_stop_mutex);

      if(stop) break;
    }
  }
  else
  {
    printf("LOGGER: Failed to open '%s' for writing.\n", logfile_path);
    fflush(stdout);
  }
  printf("LOGGER: Flushing tty object.\n");
  fflush(stdout);
  tcflush(device_fd, TCIFLUSH);

  return 0;
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
    // Open the device for read/write
    device_fd = open(argv[2], O_RDWR);
    if(device_fd == -1) {
      printf("Failed to open '%s' for read/write.\n", argv[2]);
      exit(1);
    }
    printf("Opened '%s' for read/write. fd = %d\n", argv[2], device_fd);

    // Set up baud rate of 460800
    struct termios options;
    tcgetattr(device_fd, &options);
    cfsetispeed(&options, B460800);
    cfsetospeed(&options, B460800);
    options.c_oflag &= ~OPOST;
    tcsetattr(device_fd, TCSANOW, &options);

    // Initialize the stop signal to false
    LogThread_stop = false;
    pthread_mutex_init(&LogThread_stop_mutex, NULL);

    // Tell the device to clear log/filter entries
    char reset_c = 0x02;
    senddata(&reset_c, 1);

    sleep(1);

    // Flush possibly existing buffer
    tcflush(device_fd, TCIOFLUSH);

    // Start logging
    pthread_t log_thread;
    if(pthread_create(&log_thread, NULL, LogThread, argv[3]) == 0)
      printf("Spawned log process.\n");
    else
      printf("Failed to spawn log process.\n");
    fflush(stdout);

    // Execute test
    exec_script(argv[1]);

    // Signal log thread to stop
    pthread_mutex_lock(&LogThread_stop_mutex);
    LogThread_stop = true;
    pthread_mutex_unlock(&LogThread_stop_mutex);

    // Wait for log thread to stop
    printf("Waiting for log process to exit...\n");
    pthread_join(log_thread, NULL);

    // Destroy mutex
    pthread_mutex_destroy(&LogThread_stop_mutex);

    // Flush io again, close device
    tcflush(device_fd, TCIOFLUSH);
    close(device_fd);
  }
  else
  {
    printf("Usage: CANSniffer [script] [device] [logfile]\n");
  }

  return 0;
}

