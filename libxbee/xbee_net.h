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

/*
 * Function declarations
 */
uint8_t xbee_init( char* ip );
void packet_rx_handler( void (*new_handler)() );
void xbee_tx_packet( uint8_t ip, uint8_t* buf, uint8_t len );


#endif /* XBEE_NET_H_ */
