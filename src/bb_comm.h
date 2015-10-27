#ifndef BB_COMM_H
#define BB_COMM_H

#include <stddef.h>
#include <stdint.h>

/* BeagleBone Communication Module
 * 
 * Initializes and manages communication with the beaglebone over UART.
 * Utilizes DMA, UART1, and GPIOB[0:1]
 * Initialized via `bb_comm_init()`
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
 * The module will continually spit out outgoing packets, call `bb_comm_update`
 * to change the content of these packets.
 * 
 * David Petrizze - 10/19/2015
 */

#define MAX_OUTGOING_NUMBERS   4
#define MAX_INCOMING_NUMBERS   4
#define OUTGOING_PACKET_SIZE (1 + sizeof(uint32_t) + sizeof(double)*MAX_OUTGOING_NUMBERS)
#define INCOMING_PACKET_SIZE (1 + sizeof(uint32_t) + sizeof(double)*MAX_INCOMING_NUMBERS)

       void bb_comm_update(double * numbers, size_t num);
extern void bb_comm_datarecv(double * numbers, size_t num);

void bb_comm_init(void);

#endif
