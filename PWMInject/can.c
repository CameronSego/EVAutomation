
#include "can.h"

#include <TM4C1294NCPDT.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <driverlib\can.h>
#include <driverlib\interrupt.h>

float can_steering_angle = 0.0f;

static tCANBitClkParms CANBitClk = {
  .ui32SyncPropPhase1Seg = 2,
  .ui32Phase2Seg = 1,
  .ui32SJW = 1,
  .ui32QuantumPrescaler = 8
};

void CAN0_Handler(void)
{
  uint32_t objid = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
  
  // Read the message out of the message object, and
  // clear the interrupt.
  uint8_t data[8];
  tCANMsgObject sMsgObjectRx;
  sMsgObjectRx.pui8MsgData = data;
  sMsgObjectRx.ui32MsgLen = 8;
  CANMessageGet(CAN0_BASE, objid, &sMsgObjectRx, true);
  
  if(sMsgObjectRx.ui32MsgID == 0x10)
  {
    uint16_t sval = (data[6] << 8) | data[7];
    
    if(data[4] & 0x80)
    {
      can_steering_angle = -(float) (sval - 0x8000) / (0xA000 - 0x8000);
    }
    else
    {
      can_steering_angle = (float) (sval - 0x8000) / (0xA000 - 0x8000);
    }
  }
}

void CAN1_Handler(void)
{
  uint32_t objid = CANIntStatus(CAN1_BASE, CAN_INT_STS_CAUSE);
  
  // Read the message out of the message object, and
  // clear the interrupt.
  uint8_t data[8];
  tCANMsgObject sMsgObjectRx;
  // This isn't very well documented in the reference, but this field must be set
  sMsgObjectRx.pui8MsgData = data;
  sMsgObjectRx.ui32MsgLen = 8;
  CANMessageGet(CAN1_BASE, objid, &sMsgObjectRx, true);
  
}

static void SetReceiveAll(uint32_t canbase, size_t fifolen, void (*interrupt)(void))
{
  CANInit(canbase);
  CANBitTimingSet(canbase, &CANBitClk);
  CANEnable(canbase);
  
  tCANMsgObject sMsgObjectRx;
  // Configure a receive object.
  sMsgObjectRx.ui32MsgID = 0x000;
  sMsgObjectRx.ui32MsgIDMask = 0x000;
  sMsgObjectRx.ui32Flags = MSG_OBJ_USE_ID_FILTER | MSG_OBJ_FIFO | MSG_OBJ_RX_INT_ENABLE;
  sMsgObjectRx.ui32MsgLen = 8;
  sMsgObjectRx.pui8MsgData = 0;
  //
  // The first fifolen - 1 message objects have the MSG_OBJ_FIFO set to indicate
  // that they are part of a FIFO.
  //
  for(int i = 1 ; i < fifolen ; i ++) 
    CANMessageSet(canbase, i, &sMsgObjectRx, MSG_OBJ_TYPE_RX);
  //
  // Last message object does not have the MSG_OBJ_FIFO set to indicate that
  // this is the last message.
  //
  sMsgObjectRx.ui32Flags = MSG_OBJ_USE_ID_FILTER | MSG_OBJ_RX_INT_ENABLE;
  CANMessageSet(canbase, fifolen, &sMsgObjectRx, MSG_OBJ_TYPE_RX);
  
  CANIntRegister(canbase, interrupt);
  CANIntEnable(canbase, CAN_INT_MASTER);
}

void can_Init()
{
  SYSCTL->RCGCGPIO |= 0x03; // GPIOA and GPIOB
  SYSCTL->RCGCCAN |= 0x03; // CAN1 and CAN0
  
  // Incoming can on GPIOA, CAN0
  // Pin 0,1 digital enable
  GPIOA_AHB->DEN |= 0x03; // B0000.0011
  // Pin 0 input, pin 1 output
  GPIOA_AHB->DIR |= 0x02; // B0000.0010
  // Pin 0,1 low
  GPIOA_AHB->AFSEL |= 0x03; // B0000.0011
  // Clear and set port mux for GPIOA 0,1
  GPIOA_AHB->PCTL &= ~0x000000FF;
  GPIOA_AHB->PCTL |= 0x00000077;
  
  // Incoming can on GPIOB, CAN1
  // Pin 0,1 digital enable
  GPIOB_AHB->DEN |= 0x03; // B0000.0011
  // Pin 0 input, pin 1 output
  GPIOB_AHB->DIR |= 0x02; // B0000.0010
  // Pin 0,1 low
  GPIOB_AHB->AFSEL |= 0x03; // B0000.0011
  // Clear and set port mux for GPIOB 4,5
  GPIOB_AHB->PCTL &= ~0x000000FF;
  GPIOB_AHB->PCTL |= 0x00000077;
  
  IntMasterEnable();
  SetReceiveAll(CAN0_BASE, 8, CAN0_Handler);
  SetReceiveAll(CAN1_BASE, 8, CAN1_Handler);
}
