
#include <tm4c123gh6pm.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "dssp.h"
#include "pc_comm.h"
#include "serial.h"

static char dma_out[OUTGOING_PACKET_SIZE*2];

dssp_state_t dssp_state = DSSP_STATE_INIT;

inline size_t min(size_t a, size_t b)
{
  return (a > b) ? b : a;
}

struct UDMAChannelControl {
  void * SRCENDP;
  void * DSTENDP;
  uint32_t CHCTL;
  uint32_t _0_;
};
volatile static struct UDMAChannelControl UDMAChannels[64] __attribute__((aligned(1024)));

static size_t write_update(char * dst, size_t dst_size, const char * src, size_t src_size)
{
  char * c = dst;
  *c = SERIAL_ESCAPE_CHAR;
    c ++; if(c - dst == dst_size) return 0;
  *c = SERIAL_PACKET_START;
    c ++; if(c - dst == dst_size) return 0;
  *c = (src_size >> 8) & 0xFF;
    c ++; if(c - dst == dst_size) return 0;
  *c = src_size & 0xFF;
    c ++;if(c - dst == dst_size) return 0;
  for(int i = 0 ; i < src_size ; i ++)
  {
    if(src[i] == SERIAL_ESCAPE_CHAR) {
      *c = SERIAL_ESCAPE_CHAR;
      c++; if(c - dst == dst_size) return 0;
    }
    *c = src[i];
    c++; if(c - dst == dst_size) return 0;
  }
  
  return c - dst;
}

/*
void UART1_Handler(void)
{
  if(UART1->MIS & 0x10)
  {
    // The interrupt was called because data was received
    // Clear it
    UART1->ICR |= 0x10;
    
    volatile char val = UART1->DR;
    
    dssp_in(&dssp_state, val);
    
    char inbuf[INCOMING_PACKET_SIZE];
    size_t psize = dssp_get_packet(&dssp_state, inbuf, INCOMING_PACKET_SIZE);
    if(psize) {
      pc_comm_on_receive(inbuf, psize);
    }
  }
}
*/

/*
size_t pc_comm_read_update(double * dst, size_t num)
{
  // TODO: Check to make sure there's data
  
  const char * c = dma_in_a + 2;
  uint32_t num_recv;
  c = serial_read_u32(c, &num_recv);
  
  // minimum of num_recv and num
  if(num_recv < num)
    num = num_recv;
  
  for(size_t i = 0 ; i < num ; i ++) {
    serial_read_f64(c, dst+i);
  }
  
  return num;
}
*/
bool pc_comm_busy()
{
  return UDMA->ENASET & (1<<9);
}
size_t pc_comm_send(const char * data, size_t data_size)
{
  if(!(UDMA->ENASET & (1<<9)))
  {
    size_t out_size = write_update(dma_out, sizeof(dma_out)/sizeof(*dma_out), data, data_size);
    assert(out_size <= 1024);
    UDMAChannels[9].SRCENDP = dma_out + out_size - 1;
    UDMAChannels[9].DSTENDP = (void*)&UART1->DR;
    UDMAChannels[9].CHCTL = 0xC0000001;
    UDMAChannels[9].CHCTL |= (out_size - 1) << 4;
    
    // Start DMA transfer on channel 9
    UDMA->ENASET |= (1<<9);
    
    return data_size;
  }
  return 0;
}

void pc_comm_init(void)
{
  dssp_state = (dssp_state_t)DSSP_STATE_INIT;
  
  // Enable clocking to required peripherals
  SYSCTL->RCGCGPIO |= 2; // GPIOB
  SYSCTL->RCGCDMA |= 1; // UDMA
  SYSCTL->RCGCUART |= 2; // UART1
  
  //NVIC->ISER[1] |= (1 << (UDMA_IRQn-32));
  //NVIC->ISER[0] |= (1 << (UART1_IRQn));
  
  // Pin 1 digital enable
  GPIOB->DEN |= 0x2;
  // Pin 1 output
  GPIOB->DIR |= 0x2;
  // Pin 1 high
  GPIOB->DATA |= 0x2;
  // Pin 1 alternate functionality (UART1 Tx)
  GPIOB->AFSEL |= 0x2;
  // Clear and set port mux for GPIOB pin 1
  GPIOB->PCTL &= ~0xF0;
  GPIOB->PCTL |= 0x10;
  
  // Disable UART1
  UART1->CTL &= ~0x1;
  // Clear and set integer baud-rate divisor
  UART1->IBRD &= ~0xFFFF;
  UART1->IBRD |= (16000000 / (16u * 115200u)) & 0xFFFF;
  // Clear and set fractional baud-rate divisor
  UART1->FBRD &= ~0x3F;
  UART1->FBRD |= (((16000000u * 128u) / (16u * 115200u) + 1)/2) & 0x3F;
  // Clear and set line control (8-bit, no FIFOs, 1 stop bit)
  UART1->LCRH &= ~0xFF;
  UART1->LCRH |= 0x70;
  // Clear and set clock source (PIOSC)
  UART1->CC &= ~0xF;
  UART1->CC |= 0x5;
  // Enable DMA on Tx line
  UART1->DMACTL |= 0x2;
  // Enable interrupts for data received
  // UART1->IM |= 0x10;
  // Enable loopback operation
  // UART1->CTL |= 0x80;
  // Enable UART1
  UART1->CTL |= 0x1;
  
  // Configure DMA
  UDMA->CFG = 1; // MASTEREN
  UDMA->CTLBASE = (uint32_t)&UDMAChannels;
  
  UDMA->PRIOSET |= (1<<8); // 8 is high priority
  UDMA->ALTCLR |= (1<<8); // primary control structure for channel 8
  UDMA->USEBURSTCLR |= (1<<8); // 8 responds to single and burst requests
  UDMA->REQMASKCLR |= (1<<8); // 8's requests are unmasked
  // Channel 8 routed through UART1 Rx
  UDMA->CHMAP1 &= ~0x0F;
  UDMA->CHMAP1 |= 0x01;
  
  UDMA->PRIOSET |= (1<<9); // 9 is high priority
  UDMA->ALTCLR |= (1<<9); // primary control structure for channel 9
  UDMA->USEBURSTCLR |= (1<<9); // 9 responds to single and burst requests
  UDMA->REQMASKCLR |= (1<<9); // 9's requests are unmasked
  // Channel 9 routed through UART1 Tx
  UDMA->CHMAP1 &= ~0xF0;
  UDMA->CHMAP1 |= 0x10;
}
