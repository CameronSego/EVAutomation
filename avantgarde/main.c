
#include <TM4C1294NCPDT.h>
#include <stdbool.h>
#include <string.h>

#include <driverlib\fpu.h>
#include <inc\hw_uart.h>
#include <driverlib\uart.h>
#include "can.h"

void pcsr_Init()
{
  SYSCTL->RCGCGPIO |= 0x8;
  SYSCTL->RCGCUART |= 0x4;
  
  // Pins [4:5] digital enable
  GPIOD_AHB->DEN |= 0x30;
  // Pin 5 output
  GPIOD_AHB->DIR |= 0x20;
  // Pin 5 high
  //GPIOA->DATA |= 0x20;
  // Pins [4:5] alternate functionality (UART2 Rx/Tx)
  GPIOD_AHB->AFSEL |= 0x30;
  // Clear and set port mux for GPIOD pins [4:5]
  GPIOD_AHB->PCTL &= ~0xFF0000;
  GPIOD_AHB->PCTL |= 0x110000;
  
  // Disable UART2
  UART2->CTL &= ~0x1;
  // Clear and set integer baud-rate divisor
  UART2->IBRD &= ~0xFFFF;
  UART2->IBRD |= (16000000 / (16u * 9600u)) & 0xFFFF;
  // Clear and set fractional baud-rate divisor
  UART2->FBRD &= ~0x3F;
  UART2->FBRD |= (((16000000u * 128u) / (16u * 9600u) + 1)/2) & 0x3F;
  // Clear and set line control (8-bit, no FIFOs, 1 stop bit)
  UART2->LCRH &= ~0xFF;
  UART2->LCRH |= 0x70;
  // Clear and set clock source (PIOSC)
  UART2->CC &= ~0xF;
  UART2->CC |= 0x5;
  // Enable DMA on Tx line
  // UART2->DMACTL |= 0x2;
  // Enable interrupts for data received
  // UART2->IM |= 0x10;
  // Enable loopback operation
  UART2->CTL |= 0x80;
  // Enable UART2
  UART2->CTL |= 0x1;
}

void pcsr_ReadData(uint8_t * buf, size_t size)
{
  for(unsigned i = 0 ; i < size ; i ++)
    buf[i] = UARTCharGet(UART2_BASE);
}
void pcsr_WriteData(const uint8_t * buf, size_t size)
{
  for(unsigned i = 0 ; i < size ; i ++)
    UARTCharPut(UART2_BASE, buf[i]);
}

int main(void)
{
  FPUEnable();
  
  can_Init();
  //pcsr_Init();
  
  while(1)
  {
    //uint8_t buf[12];
    //pcsr_ReadData(buf, 12);
    //pcsr_WriteData("asdf\r\n", 6);
    /*
    if(buf[0] == 'b') {
      pc_WriteData(buf, 1);
    } else {
      pc_WriteData("ASDFASDF", 8);
    }
    */
  }
}
