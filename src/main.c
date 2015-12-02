
#include <tm4c123gh6pm.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "pc_comm.h"
#include "can.h"

/*
int count = 0;

void TIMER0A_Handler()
{
  // Clear interrupt
  TIMER0->ICR |= 1;
  
  char data[] = { 'A', 'S', 'D', 'F', 'A', 'S', 'D', 'F' };
  pc_comm_send(data, 8);
  //pc_comm_read_update(nums, 3);
  
  unsigned waittime = 20000;
  // Kill TIMER0 A
  TIMER0->CTL &= ~0x1;
  TIMER0->TAILR = waittime;
  // Reenable TIMER0A, also enable stalling (for debugging)
  TIMER0->CTL |= 0x3;
}

void pc_comm_on_receive(const char * data, size_t data_size)
{
  count ++;
}
*/

void pc_ReadData(uint8_t * buf, size_t size)
{
  for(unsigned i = 0 ; i < size ; i ++)
  {
    // Wait while the receive FIFO is empty
    while(UART0->FR & 0x10) {}
    buf[i] = UART0->DR;
  }
}
void pc_WriteData(const uint8_t * buf, size_t size)
{
  for(unsigned i = 0 ; i < size ; i ++)
  {
    // Wait while transmit FIFO is full
    while(UART0->FR & 0x20) {}
    UART0->DR = buf[i];
  }
}

void led_Init()
{
  SYSCTL->RCGCGPIO |= 0x20; // GPIOF
  
  // Pins [1:3] digital enable
  GPIOF->DEN |= 0x0E;
  // Pins [1:3] output
  GPIOF->DIR |= 0x0E;
  // Pins [0:1] high
  //GPIOF->DATA |= 0x0E;
}
void led_Set(bool r, bool g, bool b)
{
  uint32_t dval = 0;
  if(r) dval |= 0x2;
  if(b) dval |= 0x4;
  if(g) dval |= 0x8;
  GPIOF->DATA &= ~(0xE);
  GPIOF->DATA |= (dval & 0xE);
}
void led_Byte(uint8_t byte)
{
  for(int b = 0 ; b < 8 ; b ++)
  {
    led_Set(false, false, false);
    for(unsigned i = 0 ; i < 500000 ; i ++);
    if(byte & (1 << b))
      led_Set(false, false, true);
    else
      led_Set(true, false, false);
    for(unsigned i = 0 ; i < 1000000 ; i ++);
  }
  led_Set(true, true, true);
}

int main()
{
  SYSCTL->RCGCGPIO |= 0x01; // GPIOA
  SYSCTL->RCGCUART |= 0x01; // UART0
  
  // Pins [0:1] digital enable
  GPIOA->DEN |= 0x3;
  // Pins [0:1] output
  GPIOA->DIR |= 0x3;
  // Pins [0:1] high
  //GPIOA->DATA |= 0x3;
  // Pins [0:1] alternate functionality (UART0 Rx/Tx)
  GPIOA->AFSEL |= 0x3;
  // Clear and set port mux for GPIOB pins [0:1]
  GPIOA->PCTL &= ~0xFF;
  GPIOA->PCTL |= 0x11;
  
  // Disable UART0
  UART0->CTL &= ~0x1;
  // Clear and set integer baud-rate divisor
  UART0->IBRD &= ~0xFFFF;
  UART0->IBRD |= (16000000 / (16u * 9600u)) & 0xFFFF;
  // Clear and set fractional baud-rate divisor
  UART0->FBRD &= ~0x3F;
  UART0->FBRD |= (((16000000u * 128u) / (16u * 9600u) + 1)/2) & 0x3F;
  // Clear and set line control (8-bit, no FIFOs, 1 stop bit)
  UART0->LCRH &= ~0xFF;
  UART0->LCRH |= 0x70;
  // Clear and set clock source (PIOSC)
  UART0->CC &= ~0xF;
  UART0->CC |= 0x5;
  // Enable DMA on Tx line
  // UART0->DMACTL |= 0x2;
  // Enable interrupts for data received
  // UART0->IM |= 0x10;
  // Enable loopback operation
  // UART0->CTL |= 0x80;
  // Enable UART0
  UART0->CTL |= 0x1;
  
  led_Init();
  can_Init();
  
  //can_ReadInit(0x400, 0x7F8, 4);
  
  CanPacket packet = {
    .arbid = 0x165,
    // Right Mirror Move Right
    //.data = { 0x00, 0x00, 0x00, 0x01, 0x10, 0x2C, 0x80, 0x20 },
    // Unlock doors
    .data = { 0x20, 0xC0, 0x00, 0x00, 0x10, 0x65, 0x00, 0x00 },
  };
  
  can_Loopback();
  //CanPacket packet2;
  //can_ReadBlock(&packet2);
  
  led_Set(true, false, false);
  
  while(1)
  {
    //uint8_t buf[12];
    //pc_ReadData(buf, 2);
    //led_Byte(buf[0]);
    //led_Byte(buf[1]);
    
    //UART0->DR = 'a';
    for(unsigned i = 0 ; i < 50000 ; i ++);
    can_Inject(&packet);
  }
  
  /*
  //FPUEnable();
  
  pc_comm_init();
  count = 0;
  
  // Enable clocking to peripherals
  SYSCTL->RCGCTIMER |= 1; // TIMER0
  
  // Enable TIMER0A Interrupt
  NVIC->ISER[0] |= (1 << TIMER0A_IRQn);
  
  // Kill TIMER0A
  TIMER0->CTL &= ~0x1;
  // 32 Bit timer configuration for TIMER0
  TIMER0->CFG = 0;
  // Regular, one-shot timer mode
  TIMER0->TAMR = 1;
  // .5 seconds at 16MHz
  TIMER0->TAILR = 8000000;
  // TIMER0A timeout interrupt mask
  TIMER0->IMR = 1;
  // Enable TIMER0A, enable debug stalling
  TIMER0->CTL |= 0x3;
  
  while(1){};
  */
}
