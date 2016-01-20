#ifndef PC_COMM_H
#define PC_COMM_H

#include <stddef.h>
#include <stdint.h>

/* PC Communication Module
 * 
 * Initializes and manages communication with the pc over UART.
 * Utilizes DMA, UART1, and GPIOB[0:1]
 * Initialized via `pc_comm_init()`
 * 
 * 
 * Packet header
 * ['%']['b']
 * 
 * Outgoing update
 * <HEADER>[u32 N][f64 Number0][f64 Number1]...[f64 Number<N-1>]
 * 
 * Incoming update
 * <HEADER>[u32 N][f64 Number0][f64 Number1]...[f64 Number<N-1>]
 *
 * The module will continually spit out outgoing packets, call `pc_comm_update`
 * to change the content of these packets.
 * 
 * David Petrizze - 10/19/2015
 */

#define MAX_OUTGOING_NUMBERS   4
#define MAX_INCOMING_NUMBERS   4
#define OUTGOING_PACKET_SIZE (1 + sizeof(uint32_t) + sizeof(double)*MAX_OUTGOING_NUMBERS)
#define INCOMING_PACKET_SIZE (1 + sizeof(uint32_t) + sizeof(double)*MAX_INCOMING_NUMBERS)

size_t pc_comm_send(const char * data, size_t data_size);
extern void pc_comm_on_receive(const char * data, size_t data_size);

void pc_comm_init(void);

#endif
