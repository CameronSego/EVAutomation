
#include <tm4c123gh6pm.h>
#include <assert.h>
#include <math.h>
#include "bb_comm.h"
#include "serial.h"

static struct
{
  double numbers[MAX_OUTGOING_NUMBERS];
  uint32_t n_numbers;
} data_out = { {0}, 3 };
static struct
{
  double numbers[MAX_INCOMING_NUMBERS];
  uint32_t n_numbers;
} data_in = { {0}, 3 };

static uint8_t dma_out_a[OUTGOING_PACKET_SIZE*2];
static uint8_t dma_out_b[OUTGOING_PACKET_SIZE*2];
static uint8_t inbuf[INCOMING_PACKET_SIZE*2];
static size_t inbuf_pos = 0;

struct UDMAChannelControl {
  void * SRCENDP;
  void * DSTENDP;
  uint32_t CHCTL;
  uint32_t _0_;
};
volatile static struct UDMAChannelControl UDMAChannels[64] __attribute__((aligned(1024)));

static void dma_out_a_init(size_t out_size) {
  assert(out_size <= 1024);
  UDMAChannels[9].SRCENDP = dma_out_a + out_size - 1;
  UDMAChannels[9].DSTENDP = (void*)&UART1->DR;
  UDMAChannels[9].CHCTL = 0xC0008003;
  UDMAChannels[9].CHCTL |= (out_size - 1) << 4;
  
  // Start DMA transfer on channel 9
  UDMA->ENASET |= (1<<9);
};
static void dma_out_b_init(size_t out_size) {
  assert(out_size <= 1024);
  UDMAChannels[32+9].SRCENDP = dma_out_b + out_size - 1;
  UDMAChannels[32+9].DSTENDP = (void*)&UART1->DR;
  UDMAChannels[32+9].CHCTL = 0xC0008003;
  UDMAChannels[32+9].CHCTL |= (out_size - 1) << 4;
  
  // Start DMA transfer on channel 9
  UDMA->ENASET |= (1<<9);
};

static size_t write_update(uint8_t * buffer)
{
  uint8_t tmpbuffer[OUTGOING_PACKET_SIZE];
  
  serial_WriteContext ctx;
  if(serial_BeginWrite(&ctx, tmpbuffer, OUTGOING_PACKET_SIZE)) {
    return 0;
  }
  serial_write_u32(&ctx, data_out.n_numbers);
  for(size_t i = 0 ; i < data_out.n_numbers ; i ++) {
    serial_write_f64(&ctx, data_out.numbers[i]);
  }
  size_t psize = serial_bytes_written(&ctx);
  
  if(serial_BeginWrite(&ctx, buffer, OUTGOING_PACKET_SIZE*2)) {
    return 0;
  }
  serial_encode_packet(&ctx, tmpbuffer, psize);
  return serial_bytes_written(&ctx);
}

void UART1_Handler(void)
{
  if(UDMA->CHIS & (1<<9))
  {
    // Clear this interrupt
    UDMA->CHIS |= (1<<9);
    // The uDMA controller is now using the regular channel 9, so initialize it
    if(!(UDMA->ALTSET & (1<<9)))
    {
      dma_out_a_init(write_update(dma_out_a));
    }
    else
    {
      dma_out_b_init(write_update(dma_out_b));
    }
  }
  if(UART1->MIS & 0x10)
  {
    // The interrupt was called because data was received
    volatile uint8_t val = UART1->DR; // This also clears the interrupt
    
    if((inbuf_pos == 0 && val != SERIAL_ESCAPE_CHAR) || 
       (inbuf_pos == 1 && val != SERIAL_PACKET_START) ||
       (inbuf_pos >= INCOMING_PACKET_SIZE)) {
      inbuf_pos = 0;
      return;
    }
    
    inbuf[inbuf_pos] = val;
    inbuf_pos ++;
    
    if(inbuf_pos > 4)
    {
      serial_ReadContext ctx;
      if(serial_BeginRead(&ctx, inbuf, inbuf_pos)) {
        // Decoding error
        return;
      }
      
      uint8_t buffer[INCOMING_PACKET_SIZE];
      serial_decode_packet(&ctx, buffer, INCOMING_PACKET_SIZE);
      
      if(serial_BeginRead(&ctx, buffer, INCOMING_PACKET_SIZE)) {
        // Deserialization error
        inbuf_pos = 0;
        return;
      }
      data_in.n_numbers = serial_read_u32(&ctx);
      if(data_in.n_numbers >= MAX_INCOMING_NUMBERS)
        data_in.n_numbers = MAX_INCOMING_NUMBERS;
      for(int i = 0 ; i < data_in.n_numbers ; i ++)
        data_in.numbers[i] = serial_read_f64(&ctx);
      
      inbuf_pos = 0;
      return;
    }
  }
}

/*
size_t bb_comm_read_update(double * dst, size_t num)
{
  // TODO: Check to make sure there's data
  
  const uint8_t * c = dma_in_a + 2;
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
void bb_comm_update(double * numbers, size_t num)
{
  if(num > MAX_OUTGOING_NUMBERS)
    num = MAX_OUTGOING_NUMBERS;
  for(int i = 0 ; i < num ; i ++)
    data_out.numbers[i] = numbers[i];
  data_out.n_numbers = num;
}

void bb_comm_init(void)
{
  data_out.n_numbers = 0;
  data_in.n_numbers = 0;
  
  // Enable clocking to required peripherals
  SYSCTL->RCGCGPIO |= 2; // GPIOB
  SYSCTL->RCGCDMA |= 1; // UDMA
  SYSCTL->RCGCUART |= 2; // UART1
  
  NVIC->ISER[1] |= (1 << (UDMA_IRQn-32));
  NVIC->ISER[0] |= (1 << (UART1_IRQn));
  
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
  // Clear and set line control (8-bit, FIFOs, 2 stop bits)
  UART1->LCRH &= ~0xFF;
  UART1->LCRH |= 0x70;
  // Clear and set clock source (PIOSC)
  UART1->CC &= ~0xF;
  UART1->CC |= 0x5;
  // Enable DMA on Tx line
  UART1->DMACTL |= 0x2;
  // Enable interrupts for data received
  UART1->IM |= 0x10;
  // Enable loopback operation
  UART1->CTL |= 0x80;
  // Enable UART1
  UART1->CTL |= 0x1;
  
  // Configure DMA
  UDMA->CFG = 1; // MASTEREN
  UDMA->CTLBASE = (uint32_t)&UDMAChannels;
  
  UDMA->PRIOSET |= (1<<8); // 8 is high priority
  UDMA->ALTCLR |= (1<<8); // primary control structure for channel 8
  UDMA->USEBURSTCLR |= (1<<8); // 8 responds to single and burst requests
  UDMA->REQMASKCLR |= (1<<8); // 8's requests are unmasked
  // Channel 8 routed through UART1 Tx
  UDMA->CHMAP1 &= ~0x0F;
  UDMA->CHMAP1 |= 0x01;
  
  UDMA->PRIOSET |= (1<<9); // 9 is high priority
  UDMA->ALTCLR |= (1<<9); // primary control structure for channel 9
  UDMA->USEBURSTCLR |= (1<<9); // 9 responds to single and burst requests
  UDMA->REQMASKCLR |= (1<<9); // 9's requests are unmasked
  // Channel 9 routed through UART1 Tx
  UDMA->CHMAP1 &= ~0xF0;
  UDMA->CHMAP1 |= 0x10;
  
  dma_out_a_init(write_update(dma_out_a));
}
