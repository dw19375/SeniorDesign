/*
 * main_board.h
 *
 *  Created on: Nov 19, 2013
 *      Author: dan
 */

#ifndef MAIN_BOARD_H_
#define MAIN_BOARD_H_

/*
 * Constants
 */
#define NUM_ROOMS 2
#define PRECESION 1				// Set precesion of temperature sensor to 10-bit (1)
#define LOOP_DELAY 10l			// Seconds between main loop iterations

/*
 * Functions
 */
void main_board_packet_rx_handler();
void parse_user_packet( uint8_t* p );
void parse_temp_packet( uint8_t* p );
uint8_t IP2room( uint8_t ip );
void timer_delay_ms( uint32_t t );

#endif /* MAIN_BOARD_H_ */
