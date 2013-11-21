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

/*
 * Function declarations
 */
uint8_t xbee_init();
void frame_rx_handler();


#endif /* XBEE_NET_H_ */
