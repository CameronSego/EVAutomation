
#include <TM4C1294NCPDT.h>
#include <stdbool.h>
#include <string.h>

#include <driverlib\fpu.h>
#include <inc\hw_uart.h>
#include <driverlib\uart.h>
#include "can.h"

void led_Init()
{
  SYSCTL->RCGCGPIO |= 0x20; // GPIOF
  
  // Pins [4:0] digital enable
  GPIOF_AHB->DEN |= 0x11;
  // Pins [4:0] output
  GPIOF_AHB->DIR |= 0x11;
}
void led_Set(bool a, bool b)
{
  uint32_t dval = 0;
  if(a) dval |= 0x01;
  if(b) dval |= 0x10;
  GPIOF_AHB->DATA &= ~(0x11);
  GPIOF_AHB->DATA |= (dval & 0x11);
}
void led_Byte(uint8_t byte)
{
  for(int b = 0 ; b < 8 ; b ++)
  {
    led_Set(false, false);
    for(unsigned i = 0 ; i < 800000 ; i ++);
    if(byte & (1 << b))
      led_Set(false, true);
    else
      led_Set(true, false);
    for(unsigned i = 0 ; i < 400000 ; i ++);
  }
  led_Set(false, false);
}

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
  
  uint64_t baud = 460800;
  
  // Disable UART2
  UART2->CTL &= ~0x1;
  // Clear and set integer baud-rate divisor
  UART2->IBRD &= ~0xFFFF;
  UART2->IBRD |= (16000000lu / (16lu * baud)) & 0xFFFF;
  // Clear and set fractional baud-rate divisor
  UART2->FBRD &= ~0x3F;
  UART2->FBRD |= (((16000000lu * 128lu) / (16lu * baud) + 1)/2) & 0x3F;
  // Clear and set line control (8-bit, no FIFOs, 1 stop bit)
  UART2->LCRH &= ~0xFF;
  UART2->LCRH |= 0x70;
  // Clear and set clock source (PIOSC)
  UART2->CC &= ~0xF;
  UART2->CC |= 0x05;
  // Enable DMA on Tx line
  //UART2->DMACTL |= 0x2;
  // Enable interrupts for data received
  //UART2->IM |= 0x10;
  // Enable loopback operation
  //UART2->CTL |= 0x80;
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

#define FUNCTION_LOG    0x00
#define FUNCTION_FILTER 0x01
#define FUNCTION_RESET  0x02

void LogHandler(const CanPacket * packet)
{
  uint8_t buf[14];
  buf[0] = 0x00;
  buf[1] = 0x00;
  buf[2] = 0x00;
  buf[3] = 0x00;
  buf[4] = packet->arbid & 0xFF;
  buf[5] = packet->arbid >> 8;
  
  for(unsigned i = 0 ; i < 8 ; i++)
  {
    buf[i + 6] = packet->data[i];
  }
  
  pcsr_WriteData(buf, 14);
}

void HardFault_Handler(void)
{
  led_Set(true, true);
  while(1) {}
}

int main(void)
{
  FPUEnable();
  
  led_Init();
  
  can_Init();
  pcsr_Init();
  
  //can_SetLogging(0, 0x001, 0x3FF, LogHandler);
  //uint8_t new_data[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  //uint8_t new_data_mask[] = { 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF };
  //can_SetFiltering(0, 0x001, 0x3FF, new_data, new_data_mask);
  
  //uint8_t data[] = { 0xFF, 0x00, 0xFF, 0x00, '\n' };
  //pcsr_WriteData("READY!\r\n", 8);
  
  while(UARTCharsAvail(UART2_BASE))
  {
    led_Byte(UARTCharGetNonBlocking(UART2_BASE));
  }
  
  while(1)
  {
    uint8_t buf[22];
    pcsr_ReadData(buf, 1);
    if(buf[0] == FUNCTION_LOG)
    {
      pcsr_ReadData(buf + 1, 5);
      uint16_t arbid = buf[2] + (buf[3] << 8);
      uint16_t arbid_mask = buf[4] + (buf[5] << 8);
      can_SetLogging(buf[1], arbid, arbid_mask, LogHandler);
    }
    else if(buf[0] == FUNCTION_FILTER)
    {
      pcsr_ReadData(buf + 1, 21);
      uint16_t arbid = buf[2] + (buf[3] << 8);
      uint16_t arbid_mask = buf[4] + (buf[5] << 8);
      can_SetFiltering(buf[1], arbid, arbid_mask, buf + 6, buf + 14);
    }
    else if(buf[0] == FUNCTION_RESET)
    {
      can_ResetFunctions();
      //led_Byte(buf[0]);
    }
  }
}
