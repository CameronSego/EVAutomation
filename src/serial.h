#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>

#define SERIAL_ESCAPE_CHAR ('%')
#define SERIAL_PACKET_START ('s')

#define SERIAL_ERR_PAST_END 0x01
#define SERIAL_ERR_CORRUPTED 0x02
/* Data Serialization Module
 *
 * Places all integers into big-endian format before sending
 * The maximum size written by each function is 2*sizeof(xxx)
 *
 * C Type    Type Code (xxx) 
 * uint8_t   u8              
 * uint16_t  u16             
 * uint32_t  u32             
 * uint64_t  u64             
 * int8_t    i8              
 * int16_t   i16             
 * int32_t   i32             
 * int64_t   i64             
 * float     f32             
 * double    f64             
 *
 * All `serial_write_xxx()` functions take a pointer to an 
 * array of bytes to write to, and a value to write (of type xxx).
 * They then return the next location to write to.
 *
 * All `serial_read_xxx()` functions take a pointer to an array of
 * bytes to be read from, and a value's address to read into (of type xxx *).
 * They then return the next location to read from.
 *
 * David Petrizze - 10/19/2015
 */

typedef struct {
  jmp_buf errorh;
  const uint8_t * c;
  const uint8_t * src;
  const uint8_t * end;
} serial_ReadContext;

typedef struct {
  jmp_buf errorh;
  uint8_t * c;
  uint8_t * dst;
  uint8_t * end;
} serial_WriteContext;

int serial_BeginRead(serial_ReadContext * ctx, const uint8_t * src, size_t src_size);
int serial_BeginWrite(serial_WriteContext * ctx, uint8_t * dst, size_t dst_size);

size_t serial_bytes_read(const serial_ReadContext * ctx);
size_t serial_bytes_written(const serial_WriteContext * ctx);

void serial_write_u8 (serial_WriteContext * ctx, uint8_t val);
void serial_write_u16(serial_WriteContext * ctx, uint16_t val);
void serial_write_u32(serial_WriteContext * ctx, uint32_t val);
//void serial_write_u64(serial_WriteContext * ctx, uint64_t val);
uint8_t serial_read_u8  (serial_ReadContext * ctx);
uint16_t serial_read_u16 (serial_ReadContext * ctx);
uint32_t serial_read_u32 (serial_ReadContext * ctx);
//uint64_t serial_read_u64 (serial_ReadContext * ctx);

//float serial_read_f32 (serial_ReadContext * ctx);
double serial_read_f64 (serial_ReadContext * ctx);
//void serial_write_f32(serial_WriteContext * ctx, float val);
void serial_write_f64(serial_WriteContext * ctx, double val);

void serial_encode_packet(serial_WriteContext * ctx, const uint8_t * src, uint16_t src_size);
size_t serial_decode_packet(serial_ReadContext * ctx, uint8_t * dst, uint16_t dst_size);

#endif
