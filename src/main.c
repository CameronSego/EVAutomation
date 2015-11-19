
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
  
  can_Init();
  
  //can_ReadInit(0x400, 0x7F8, 4);
  CanPacket packet = {
    .arbid = 0x30,
    // Right Mirror Move Right
    //.data = { 0x00, 0x00, 0x00, 0x01, 0x10, 0x2C, 0x80, 0x20 },
    // Unlock doors
    .data = { 0x00, 0x00, 0x00, 0x01, 0x10, 0x28, 0xA0, 0x20 },
    //.data = { 0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE, 0xC0, 0xFF },
  };
  //can_Loopback();
  //CanPacket packet2;
  //can_ReadBlock(&packet2);
  
  while(1) {
    //UART0->DR = 'a';
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
