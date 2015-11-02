
#include "can.h"

#include <tm4c123gh6pm.h>
#include <string.h>
#include <stdbool.h>

#include <inc\hw_can.h>
#include <inc\hw_ints.h>
#include <inc\hw_nvic.h>
//#include <int\hw_memmap.h>
#include <inc\hw_sysctl.h>
#include <inc\hw_types.h>
#include <driverlib\can.h>
#include <driverlib\debug.h>
#include <driverlib\interrupt.h>

/*
void can_init()
{
  SYSCTL->RCGCGPIO |= 2; // GPIOB
  SYSCTL->RCGCCAN |= 1; // GPIOB
  
  // Pin 4,5 digital enable
  GPIOB->DEN |= 0x18;
  // Pin 4,5 output
  GPIOB->DIR |= 0x18;
  // Pin 4,5 low
  //GPIOB->DATA |= 0x18;
  // Pin 4,5 alternate functionality (UART1 Tx)
  GPIOB->AFSEL |= 0x18;
  // Clear and set port mux for GPIOB pin 1
  GPIOB->PCTL &= ~0x000FF000;
  GPIOB->PCTL |= 0x00088000;
  
  tCANBitClkParms CANBitClk = {
    .ui32SyncPropPhase1Seg = 2,
    .ui32Phase2Seg = 1,
    .ui32SJW = 1,
    .ui32QuantumPrescaler = 1
  };
  tCANMsgObject sMsgObjectRx;
  tCANMsgObject sMsgObjectTx;
  //uint8_t pui8BufferIn[8];
  uint8_t pui8BufferOut[8] = { 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xC0, 0xFF };
  //
  // Reset the state of all the message objects and the state of the CAN
  // module to a known state.
  //
  CANInit(CAN0_BASE);
  //
  // Configure the controller for 1 Mbit operation.
  //
  CANBitTimingSet(CAN0_BASE, &CANBitClk);
  //
  // Take the CAN0 device out of INIT state.
  //
  CANEnable(CAN0_BASE);
  //
  // Configure a receive object.
  //
  sMsgObjectRx.ui32MsgID = (0x400);
  sMsgObjectRx.ui32MsgIDMask = 0x7f8;
  sMsgObjectRx.ui32Flags = MSG_OBJ_USE_ID_FILTER | MSG_OBJ_FIFO;
  sMsgObjectRx.pui8MsgData = 0;
  //
  // The first three message objects have the MSG_OBJ_FIFO set to indicate
  // that they are part of a FIFO.
  //
  CANMessageSet(CAN0_BASE, 1, &sMsgObjectRx, MSG_OBJ_TYPE_RX);
  CANMessageSet(CAN0_BASE, 2, &sMsgObjectRx, MSG_OBJ_TYPE_RX);
  CANMessageSet(CAN0_BASE, 3, &sMsgObjectRx, MSG_OBJ_TYPE_RX);
  //
  // Last message object does not have the MSG_OBJ_FIFO set to indicate that
  // this is the last message.
  //
  sMsgObjectRx.ui32Flags = MSG_OBJ_USE_ID_FILTER;
  CANMessageSet(CAN0_BASE, 4, &sMsgObjectRx, MSG_OBJ_TYPE_RX);
  //
  // Configure and start transmit of message object.
  //
  sMsgObjectTx.ui32MsgID = 0x400;
  sMsgObjectTx.ui32Flags = 0;
  sMsgObjectTx.ui32MsgLen = 8;
  sMsgObjectTx.pui8MsgData = pui8BufferOut;
  CANMessageSet(CAN0_BASE, 2, &sMsgObjectTx, MSG_OBJ_TYPE_TX);
  //
  // Wait for new data to become available.
  //
  while((CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT) & 1) == 0)
  {
    //
    // Read the message out of the message object.
    //
    CANMessageGet(CAN0_BASE, 1, &sMsgObjectRx, true);
  }
  //
  // Process new data in sMsgObjectRx.pucMsgData.
  //
  uint8_t muh_data[8];
  memcpy(muh_data, sMsgObjectRx.pui8MsgData, 8);
}
*/
void can_init()
{
}
