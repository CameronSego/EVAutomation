
#include "serial.h"

#include <setjmp.h>
#include <string.h>

int serial_BeginRead(serial_ReadContext * ctx, const uint8_t * src, size_t src_size)
{
  ctx->c = src;
  ctx->src = src;
  ctx->end = src + src_size;
  return setjmp(ctx->errorh);
}
int serial_BeginWrite(serial_WriteContext * ctx, uint8_t * dst, size_t dst_size)
{
  ctx->c = dst;
  ctx->dst = dst;
  ctx->end = dst + dst_size;
  return setjmp(ctx->errorh);
}

size_t serial_bytes_read(const serial_ReadContext * ctx)
{
  return ctx->c - ctx->src;
}
size_t serial_bytes_written(const serial_WriteContext * ctx)
{
  return ctx->c - ctx->dst;
}

static void ctx_write(serial_WriteContext * ctx, uint8_t val)
{
  if(ctx->c == ctx->end) longjmp(ctx->errorh, SERIAL_ERR_PAST_END);
  *ctx->c = val;
  ctx->c ++;
}
static uint8_t ctx_read(serial_ReadContext * ctx)
{
  if(ctx->c == ctx->end) longjmp(ctx->errorh, SERIAL_ERR_PAST_END);
  uint8_t val = *ctx->c;
  ctx->c ++;
  return val;
}

void serial_write_u8(serial_WriteContext * ctx, uint8_t val)
{
  ctx_write(ctx, val);
}
uint8_t serial_read_u8(serial_ReadContext * ctx)
{
  return ctx_read(ctx);
}
void serial_write_u16(serial_WriteContext * ctx, uint16_t val)
{
  uint8_t * bytes = (uint8_t*)&val;
  ctx_write(ctx, bytes[1]);
  ctx_write(ctx, bytes[0]);
}
uint16_t serial_read_u16(serial_ReadContext * ctx)
{
  union { uint16_t u; uint8_t b[8]; } val;
  val.b[1] = ctx_read(ctx);
  val.b[0] = ctx_read(ctx);
  return val.u;
}
void serial_write_u32(serial_WriteContext * ctx, uint32_t val)
{
  uint8_t * bytes = (uint8_t*)&val;
  ctx_write(ctx, bytes[3]);
  ctx_write(ctx, bytes[2]);
  ctx_write(ctx, bytes[1]);
  ctx_write(ctx, bytes[0]);
}
uint32_t serial_read_u32(serial_ReadContext * ctx)
{
  union { uint32_t u; uint8_t b[8]; } val;
  val.b[3] = ctx_read(ctx);
  val.b[2] = ctx_read(ctx);
  val.b[1] = ctx_read(ctx);
  val.b[0] = ctx_read(ctx);
  return val.u;
}
void serial_write_f64(serial_WriteContext * ctx, double val)
{
  uint8_t * bytes = (uint8_t*)&val;
  ctx_write(ctx, bytes[0]);
  ctx_write(ctx, bytes[1]);
  ctx_write(ctx, bytes[2]);
  ctx_write(ctx, bytes[3]);
  ctx_write(ctx, bytes[4]);
  ctx_write(ctx, bytes[5]);
  ctx_write(ctx, bytes[6]);
  ctx_write(ctx, bytes[7]);
}
double serial_read_f64(serial_ReadContext * ctx)
{
  union { double d; uint8_t b[8]; } val;
  val.b[0] = ctx_read(ctx);
  val.b[1] = ctx_read(ctx);
  val.b[2] = ctx_read(ctx);
  val.b[3] = ctx_read(ctx);
  val.b[4] = ctx_read(ctx);
  val.b[5] = ctx_read(ctx);
  val.b[6] = ctx_read(ctx);
  val.b[7] = ctx_read(ctx);
  return val.d;
}

void serial_encode_packet(serial_WriteContext * ctx, const uint8_t * src, uint16_t src_size)
{
  ctx_write(ctx, SERIAL_ESCAPE_CHAR);
  ctx_write(ctx, SERIAL_PACKET_START);
  serial_write_u16(ctx, src_size);
  for(int i = 0 ; i < src_size ; i ++) {
    if(src[i] == SERIAL_ESCAPE_CHAR)
      ctx_write(ctx, SERIAL_ESCAPE_CHAR);
    ctx_write(ctx, src[i]);
  }
}
size_t serial_decode_packet(serial_ReadContext * ctx, uint8_t * dst, uint16_t dst_size)
{
  if(ctx_read(ctx) != SERIAL_ESCAPE_CHAR)
    longjmp(ctx->errorh, SERIAL_ERR_CORRUPTED);
  if(ctx_read(ctx) != SERIAL_PACKET_START)
    longjmp(ctx->errorh, SERIAL_ERR_CORRUPTED);
  
  uint16_t len = serial_read_u16(ctx);
  if(len > dst_size) 
    longjmp(ctx->errorh, SERIAL_ERR_PAST_END);
  
  for(int i = 0 ; i < len ; i ++)
  {
    uint8_t val = ctx_read(ctx);
    if(val == SERIAL_ESCAPE_CHAR)
    {
      val = ctx_read(ctx);
      
      if(val == SERIAL_PACKET_START)
        // Incomplete packet, it contains a start code
        longjmp(ctx->errorh, SERIAL_ERR_CORRUPTED);
    }
    dst[i] = val;
  }
  
  return len;
}
