/*
 * xbee_net.h
 *
 *  Created on: Nov 12, 2013
 *      Author: Dan Whisman
 */

#ifndef XBEE_NET_H_
#define XBEE_NET_H_

#include <stdint.h>
#include "xbee_uart.h"

/*
 * Constant data
 */
#define GATEWAY_STR "~0000GW192.168.1.0"
#define GATEWAY_STR_LEN 15
#define IP_ADDR_STR "~0000MY192.168.1.128"
#define IP_ADDR_STR_LEN 17
#define MASK_STR "~0000MK255.255.255.0"
#define MASK_STR_LEN 17

#define USE_DHCP 1			// 0 for DHCP, 1 for Static IP

#define MAIN_IP	  128		// Address of main control board.
// Packet types
#define TEMP_DATA 0x0001
#define VENT_CMD  0x0002
#define USER_CMD  0x0004
#define ACK       0x8000

/*
 * Type definitions
 */
typedef struct user_cmd_s		// Jeff, please correct this
{
	uint16_t	type;
	uint16_t	seq;		// Sequence number
	int16_t		temp;		// Desired temperature
	uint16_t	room;		// Priority room
	uint16_t	sys;		// System (heat or AC) to use
}user_cmd;

typedef struct vent_cmd_s
{
	uint16_t	type;		// Type of packet being sent
	uint16_t	cmd;		// Vent command
	uint16_t	seq;		// Sequence number of packet
}vent_cmd;

typedef struct temp_data_s
{
	uint16_t	type;		// Type of packet
	int16_t		temp;		// Temperature
	int16_t		humid;		// Humidity
}temp_data;

typedef struct ack_s
{
	uint16_t	type;		// Type of packet
	uint16_t	seq;		// Sequence number to acknowledge
}ack;


/*
 * Function declarations
 */
uint8_t xbee_init( char* ip );
void packet_rx_handler( void (*new_handler)() );
void xbee_tx_packet( uint8_t ip, uint8_t* buf, uint8_t len );


#endif /* XBEE_NET_H_ */
