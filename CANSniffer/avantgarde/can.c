
#include "can.h"
#include "list.h"
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

// Lists for the different Arb IDs.
struct list *id_4D;
struct list *id_11A;
struct list *id_130;
struct list *id_139;
struct list *id_156;
struct list *id_165;
struct list *id_167;
struct list *id_171;
struct list *id_178;
struct list *id_202;
struct list *id_179;
struct list *id_204;
struct list *id_185;
struct list *id_25C;
struct list *id_1A0;
struct list *id_200;
struct list *id_230;
struct list *id_25A;
struct list *id_25B;
struct list *id_270;
struct list *id_280;
struct list *id_312;
struct list *id_352;
struct list *id_365;
struct list *id_366;
struct list *id_367;
struct list *id_368;
struct list *id_369;
struct list *id_410;
struct list *id_421;
struct list *id_42D;
struct list *id_42F;
struct list *id_43E;
struct list *id_440;
struct list *id_472;
struct list *id_473;
struct list *id_474;
struct list *id_475;
struct list *id_476;
struct list *id_477;
struct list *id_595;

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

void Push_Message(uint32_t _ID, uint8_t data[8])
{
	switch(_ID)
	{
		case 0x4D:
		push(id_4D, data);
		break;
 
	case 0x11A:
		push(id_11A, data);
		break;
 
	case 0x130:
		push(id_130, data);
		break;
 
	case 0x139:
		push(id_139, data);
		break;
 
	case 0x156:
		push(id_156, data);
		break;
 
	case 0x165:
		push(id_165, data);
		break;
 
	case 0x167:
		push(id_167, data);
		break;
 
	case 0x171:
		push(id_171, data);
		break;
 
	case 0x178:
		push(id_178, data);
		break;
 
	case 0x202:
		push(id_202, data);
		break;
 
	case 0x179:
		push(id_179, data);
		break;
 
	case 0x204:
		push(id_204, data);
		break;
 
	case 0x185:
		push(id_185, data);
		break;
 
	case 0x25C:
		push(id_25C, data);
		break;
 
	case 0x1A0:
		push(id_1A0, data);
		break;
 
	case 0x200:
		push(id_200, data);
		break;
 
	case 0x230:
		push(id_230, data);
		break;
 
	case 0x25A:
		push(id_25A, data);
		break;
 
	case 0x25B:
		push(id_25B, data);
		break;
 
	case 0x270:
		push(id_270, data);
		break;
 
	case 0x280:
		push(id_280, data);
		break;
 
	case 0x312:
		push(id_312, data);
		break;
 
	case 0x352:
		push(id_352, data);
		break;
 
	case 0x365:
		push(id_365, data);
		break;
 
	case 0x366:
		push(id_366, data);
		break;
 
	case 0x367:
		push(id_367, data);
		break;
 
	case 0x368:
		push(id_368, data);
		break;
 
	case 0x369:
		push(id_369, data);
		break;
 
	case 0x410:
		push(id_410, data);
		break;
 
	case 0x421:
		push(id_421, data);
		break;
 
	case 0x42D:
		push(id_42D, data);
		break;
 
	case 0x42F:
		push(id_42F, data);
		break;
 
	case 0x43E:
		push(id_43E, data);
		break;
 
	case 0x440:
		push(id_440, data);
		break;
 
	case 0x472:
		push(id_472, data);
		break;
 
	case 0x473:
		push(id_473, data);
		break;
 
	case 0x474:
		push(id_474, data);
		break;
 
	case 0x475:
		push(id_475, data);
		break;
 
	case 0x476:
		push(id_476, data);
		break;
 
	case 0x477:
		push(id_477, data);
		break;
 
	case 0x595:
		push(id_595, data);
		break;
	}
}

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
  
  Push_Message(sMsgObjectRx.ui32MsgID, sMsgObjectRx.pui8MsgData);
	
  Pop_Message(sMsgObjectRx.ui32MsgID, data);
  sMsgObjectRx.pui8MsgData = data;
	
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

	// Init for the lists
	init_list(id_4D, 0x4D);
	init_list(id_11A, 0x11A);
	init_list(id_130, 0x130);
	init_list(id_139, 0x139);
	init_list(id_156, 0x156);
	init_list(id_165, 0x165);
	init_list(id_167, 0x167);
	init_list(id_171, 0x171);
	init_list(id_178, 0x178);
	init_list(id_202, 0x202);
	init_list(id_179, 0x179);
	init_list(id_204, 0x204);
	init_list(id_185, 0x185);
	init_list(id_25C, 0x25C);
	init_list(id_1A0, 0x1A0);
	init_list(id_200, 0x200);
	init_list(id_230, 0x230);
	init_list(id_25A, 0x25A);
	init_list(id_25B, 0x25B);
	init_list(id_270, 0x270);
	init_list(id_280, 0x280);
	init_list(id_312, 0x312);
	init_list(id_352, 0x352);
	init_list(id_365, 0x365);
	init_list(id_366, 0x366);
	init_list(id_367, 0x367);
	init_list(id_368, 0x368);
	init_list(id_369, 0x369);
	init_list(id_410, 0x410);
	init_list(id_421, 0x421);
	init_list(id_42D, 0x42D);
	init_list(id_42F, 0x42F);
	init_list(id_43E, 0x43E);
	init_list(id_440, 0x440);
	init_list(id_472, 0x472);
	init_list(id_473, 0x473);
	init_list(id_474, 0x474);
	init_list(id_475, 0x475);
	init_list(id_476, 0x476);
	init_list(id_477, 0x477);
	init_list(id_595, 0x595);
	
	
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
