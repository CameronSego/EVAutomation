#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  uint32_t arbid;
  uint8_t data[8];
} CanPacket;
void can_Inject(const CanPacket * packet);
void can_ReadInit(uint32_t arbid, uint32_t arbmask, size_t fifolen);
bool can_Read(CanPacket * packet);
void can_ReadBlock(CanPacket * packet);
void can_Loopback(void);
extern void can_Init(void);

#endif
