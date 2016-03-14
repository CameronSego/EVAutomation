
#include <tm4c123gh6pm.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "pc_comm.h"
#include "can.h"

#include "list.h"


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
