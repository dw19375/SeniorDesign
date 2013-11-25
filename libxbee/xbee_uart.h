/*
 * xbee_uart.h
 *
 *  Created on: Nov 12, 2013
 *      Author: Dan Whisman
 */

#ifndef XBEE_UART_H_
#define XBEE_UART_H_

#include <stdint.h>
#include <msp430.h>

/*
 * Global definitions
 */
#define RX_BUF_LEN 26

/*
 * Exported Functions
 */
void uart_init();
void send_data( uint8_t* buf );
void frame_recv_handler( void (*new_handler)() );
volatile uint8_t* uart_get_frame();
void uart_recv_next_byte();
void uart_send_frame( uint8_t* buf );
void uart_transmit_next_byte();
uint8_t calculate_checksum( uint8_t* buf, uint8_t len );
uint8_t verify_checksum( uint8_t* buf, uint8_t len );


#endif /* XBEE_UART_H_ */
