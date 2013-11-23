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

/*
 * Functions
 */
void main_board_packet_rx_handler();
void parse_user_packet( uint8_t* p );
void parse_temp_packet( uint8_t* p );
uint8_t IP2room( uint8_t ip );

#endif /* MAIN_BOARD_H_ */
