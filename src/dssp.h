#ifndef DSSP_H
#define DSSP_H

#include <stdbool.h>

#include <stdbool.h>
#include <string.h>

// DSSP - Dave's Serial Streaming Protocol
//
// [%][s]   start sequence
// [%][x]   character x
//
// Packet:
//   [start sequence][len_msb][len_lsb][byte<0>][byte<1>]...[byte<len-1>]
//
//   where len is the unescaped length of the data payload
//

#define DSSP_START      -0x10
#define DSSP_NOTHING    -0x11
#define DSSP_INCOMPLETE -0x12

typedef struct dssp_state
{
  bool dssp_escape;
  enum {
    DSSP_STATE_NULL, 
    DSSP_STATE_SIZE_MSB, 
    DSSP_STATE_SIZE_LSB, 
    DSSP_STATE_DATA
  } state;
  unsigned read_pos;
  char packet_data[200];
  unsigned short packet_data_size;
} dssp_state_t;

#define DSSP_STATE_INIT { \
  .dssp_escape = false, \
  .state = DSSP_STATE_NULL, \
  .read_pos = 0, \
  .packet_data = {0}, \
  .packet_data_size = 0, \
}

extern void dssp_in(dssp_state_t * st, char ch);
extern size_t dssp_get_packet(dssp_state_t * st, char * buf, size_t buf_size);

#endif
