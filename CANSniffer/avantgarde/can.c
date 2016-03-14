
#include "can.h"
#include <TM4C1294NCPDT.h>
#include <string.h>
#include <stdbool.h>

#include <inc\hw_gpio.h>
#include <inc\hw_can.h>
#include <inc\hw_ints.h>
#include <inc\hw_nvic.h>
//#include <int\hw_memmap.h>
#include <inc\hw_sysctl.h>
#include <inc\hw_types.h>
#include <driverlib\can.h>
#include <driverlib\gpio.h>
#include <driverlib\interrupt.h>
#include "driverlib\pin_map.h"
#include "driverlib\sysctl.h"

typedef struct LogEntry
{
  uint16_t arb_id;
  uint16_t arb_mask;
  can_LogCallback cb;
} LogEntry;
typedef struct FilterEntry
{
  uint16_t arb_id;
  uint16_t arb_mask;
  uint8_t data[8];
  uint8_t data_mask[8];
} FilterEntry;

static tCANBitClkParms CANBitClk = {
  .ui32SyncPropPhase1Seg = 2,
  .ui32Phase2Seg = 1,
  .ui32SJW = 1,
  .ui32QuantumPrescaler = 8
};

static int can0_txid = 24;
static int can1_txid = 24;

static LogEntry can0_log_entries[3];
static unsigned can0_log_entry_num = 0;
static LogEntry can1_log_entries[3];
static unsigned can1_log_entry_num = 0;
static FilterEntry can0_filter_entries[3];
static unsigned    can0_filter_entry_num = 0;
static FilterEntry can1_filter_entries[3];
static unsigned    can1_filter_entry_num = 0;

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
  
  for(unsigned i = 0 ; i < can0_log_entry_num ; i ++)
  {
    uint16_t arb_id = can0_log_entries[i].arb_id;
    uint16_t arb_mask = can0_log_entries[i].arb_mask;
    can_LogCallback cb = can0_log_entries[i].cb;
    if((sMsgObjectRx.ui32MsgID & arb_mask) == (arb_id & arb_mask))
    {
      CanPacket p;
      p.arbid = arb_id;
      memcpy(p.data, data, 8);
      cb(&p);
    }
  }
  for(unsigned i = 0 ; i < can0_filter_entry_num ; i ++)
  {
    uint16_t arb_id = can0_filter_entries[i].arb_id;
    uint16_t arb_mask = can0_filter_entries[i].arb_mask;
    uint8_t * new_data = can0_filter_entries[i].data;
    uint8_t * data_mask = can0_filter_entries[i].data_mask;
    if((sMsgObjectRx.ui32MsgID & arb_mask) == (arb_id & arb_mask))
    {
      for(unsigned i = 0 ; i < 8 ; i ++)
      {
        data[i] &= ~(data_mask[i]);
        data[i] |= data_mask[i] & new_data[i];
      }
    }
  }
  
  CANMessageSet(CAN1_BASE, can1_txid, &sMsgObjectRx, MSG_OBJ_TYPE_TX);
  
  can1_txid ++;
  if(can1_txid > 32)
    can1_txid = 24;
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
  
  for(unsigned i = 0 ; i < can1_log_entry_num ; i ++)
  {
    uint16_t arb_id = can1_log_entries[i].arb_id;
    uint16_t arb_mask = can1_log_entries[i].arb_mask;
    can_LogCallback cb = can1_log_entries[i].cb;
    if((sMsgObjectRx.ui32MsgID & arb_mask) == (arb_id & arb_mask))
    {
      CanPacket p;
      p.arbid = arb_id;
      memcpy(p.data, data, 8);
      cb(&p);
    }
  }
  for(unsigned i = 0 ; i < can1_filter_entry_num ; i ++)
  {
    uint16_t arb_id = can1_filter_entries[i].arb_id;
    uint16_t arb_mask = can1_filter_entries[i].arb_mask;
    uint8_t * new_data = can1_filter_entries[i].data;
    uint8_t * data_mask = can1_filter_entries[i].data_mask;
    if((sMsgObjectRx.ui32MsgID & arb_mask) == (arb_id & arb_mask))
    {
      for(unsigned i = 0 ; i < 8 ; i ++)
      {
        data[i] &= ~(data_mask[i]);
        data[i] |= data_mask[i] & new_data[i];
      }
    }
  }
  
  CANMessageSet(CAN0_BASE, can0_txid, &sMsgObjectRx, MSG_OBJ_TYPE_TX);
  
  can0_txid ++;
  if(can0_txid > 32)
    can0_txid = 24;
}

void can_Inject(const CanPacket * packet)
{
  tCANMsgObject sMsgObjectTx;
  //
  // Configure and start transmit of message object.
  //
  sMsgObjectTx.ui32MsgID = packet->arbid;
  sMsgObjectTx.ui32Flags = 0;
  sMsgObjectTx.ui32MsgLen = 8;
  sMsgObjectTx.pui8MsgData = (uint8_t *)packet->data;
  CANMessageSet(CAN1_BASE, 32, &sMsgObjectTx, MSG_OBJ_TYPE_TX);
}

void SetReceiveAll(uint32_t canbase, size_t fifolen, void (*interrupt)(void))
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

/*
static void SuperLoopback(void)
{
  //Disable auto retransmit
  CAN0->CTL |= 0x20;
  CAN1->CTL |= 0x20;
  
  CAN0->CTL |= 0x80;
  CAN0->TST |= 0x10; // Enable loopback
  CAN1->CTL |= 0x80;
  CAN1->TST |= 0x10; // Enable loopback
}
*/
void can_SetLogging(uint8_t can_id, uint16_t arb_id, uint16_t arb_mask, can_LogCallback cb)
{
  if(can_id == 0)
  {
    const unsigned max_entries = sizeof(can0_log_entries)/sizeof(*can0_log_entries);
    unsigned idx = can0_log_entry_num;
    if(idx < max_entries)
    {
      can0_log_entries[idx].arb_id = arb_id;
      can0_log_entries[idx].arb_mask = arb_mask;
      can0_log_entries[idx].cb = cb;
      can0_log_entry_num ++;
    }
  }
  else if(can_id == 1)
  {
    const unsigned max_entries = sizeof(can1_log_entries)/sizeof(*can1_log_entries);
    unsigned idx = can1_log_entry_num;
    if(idx < max_entries)
    {
      can1_log_entries[idx].arb_id = arb_id;
      can1_log_entries[idx].arb_mask = arb_mask;
      can1_log_entries[idx].cb = cb;
      can1_log_entry_num ++;
    }
  }
}
void can_SetFiltering(uint8_t can_id, uint16_t arb_id, uint16_t arb_mask, uint8_t * data, uint8_t * data_mask)
{
  if(can_id == 0)
  {
    const unsigned max_entries = sizeof(can0_filter_entries)/sizeof(*can0_filter_entries);
    unsigned idx = can0_filter_entry_num;
    if(idx < max_entries)
    {
      can0_filter_entries[idx].arb_id = arb_id;
      can0_filter_entries[idx].arb_mask = arb_mask;
      memcpy(can0_filter_entries[idx].data, data, 8);
      memcpy(can0_filter_entries[idx].data_mask, data_mask, 8);
      can0_filter_entry_num ++;
    }
  }
  else if(can_id == 1)
  {
    const unsigned max_entries = sizeof(can1_filter_entries)/sizeof(*can1_filter_entries);
    unsigned idx = can1_filter_entry_num;
    if(idx < max_entries)
    {
      can1_filter_entries[idx].arb_id = arb_id;
      can1_filter_entries[idx].arb_mask = arb_mask;
      memcpy(can1_filter_entries[idx].data, data, 8);
      memcpy(can1_filter_entries[idx].data_mask, data_mask, 8);
      can1_filter_entry_num ++;
    }
  }
}
//Quick CAN CAN CAN CAN
void can_ResetFunctions()
{
  can0_log_entry_num = 0;
  can0_filter_entry_num = 0;
  can1_log_entry_num = 0;
  can1_filter_entry_num = 0;
}
void can_Init(void)
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
  
  //IntEnable(INT_CAN0);
  //IntEnable(INT_CAN1);
  IntMasterEnable();
  SetReceiveAll(CAN0_BASE, 8, CAN0_Handler);
  SetReceiveAll(CAN1_BASE, 8, CAN1_Handler);
  
  can_ResetFunctions();
  
  //SuperLoopback();
  
  /*
  CanPacket p = {
    .arbid = 0x130,
    .data = { 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xC0, 0xFF },
  };
  
  can_Inject(&p);
  */
  
  //can_ReadBlock(&p);
}
