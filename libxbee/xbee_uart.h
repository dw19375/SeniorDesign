/*
 * xbee_uart.h
 *
 *  Created on: Nov 12, 2013
 *      Author: Dan Whisman
 */

#ifndef XBEE_UART_H_
#define XBEE_UART_H_

#include <stdint.h>


/*
 * Exported Functions
 */
void uart_send_frame( uint8_t* buf );
void uart_transmit_next_byte();
uint8_t calculate_checksum( uint8_t* buf, uint8_t len );
uint8_t verify_checksum( uint8_t* buf, uint8_t len );


#endif /* XBEE_UART_H_ */
