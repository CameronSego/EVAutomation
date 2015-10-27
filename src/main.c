
#include <tm4c123gh6pm.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "pc_comm.h"

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

int main()
{
  SCB->CPACR |= (0xF << 20);
  __asm("DSB");
  __asm("ISB");
  
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
}
