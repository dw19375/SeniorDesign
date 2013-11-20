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
 * Function declarations
 */
uint8_t xbee_init();
void frame_rx_handler();


#endif /* XBEE_NET_H_ */
