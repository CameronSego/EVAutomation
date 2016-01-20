
#include "dssp.h"

unsigned min(unsigned a, unsigned b)
{
  return (a > b) ? b : a;
}

static int dssp_base_in(dssp_state_t * st, char ch)
{
  if(st->dssp_escape)
  {
    st->dssp_escape = false;
    if(ch == 's')
    {
      return DSSP_START;
    }
    else
    {
      return ch;
    }
  }
  else
  {
    if(ch == '%')
    {
      st->dssp_escape = true;
      return DSSP_NOTHING;
    }
    else return ch;
  }
}

void dssp_in(dssp_state_t * st, char ch)
{
  int c = dssp_base_in(st, ch);
  if(c == DSSP_NOTHING)
  {
  }
  else if(c == DSSP_START)
  {
    st->read_pos = 0;
    st->state = DSSP_STATE_SIZE_MSB;
  }
  else
  {
    if(st->state == DSSP_STATE_SIZE_MSB)
    {
      st->packet_data_size = 0;
      st->packet_data_size |= c << 8;
      st->state = DSSP_STATE_SIZE_LSB;
    }
    else if(st->state == DSSP_STATE_SIZE_LSB)
    {
      st->packet_data_size |= c;
      if(st->packet_data_size <= 200 && st->packet_data_size > 0)
        st->state = DSSP_STATE_DATA;
      else
        st->state = DSSP_STATE_NULL;
    }
    else if(st->state == DSSP_STATE_DATA)
    {
      st->packet_data[st->read_pos] = c;
      st->read_pos ++;
      if(st->read_pos == st->packet_data_size)
        st->state = DSSP_STATE_NULL;
    }
  }
}

size_t dssp_get_packet(dssp_state_t * st, char * buf, size_t buf_size)
{
  // A packet is available when the forcasted size is equal to the read size,
  // and both parameters are nonzero
  if(st->read_pos == st->packet_data_size && st->packet_data_size > 0)
  {
    size_t wsize = min(st->packet_data_size, buf_size);
    memcpy(buf, st->packet_data, wsize);
    st->packet_data_size = 0;
    st->read_pos = 0;
    return wsize;
  }
  return 0;
}

