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
#define TIMEOUT 500				// Timeout for ACK's in ms

#define NUM_ROOMS 2
#define PRECISION 1				// Set precesion of temperature sensor to 10-bit (1)
#define LOOP_DELAY 1l			// Seconds between main loop iterations
#define FAN_DELAY 3				// Number of loops to delay before activating fan.

#define PR_TEMP_THRESHOLD 3		// Threshold for priority room (in quarters of deg. C)
#define TEMP_THRESHOLD	  6		// Threshold for non-priority rooms

#define OFF 0
#define AC 1
#define HEAT 2

#define HEAT_PORT_OUT	P1OUT
#define HEAT_PORT_DIR 	P1DIR
#define HEAT_PIN		BIT3

#define AC_PORT_OUT		P2OUT
#define AC_PORT_DIR 	P2DIR
#define AC_PIN			BIT6

#define FAN_PORT_OUT  	P2OUT
#define FAN_PORT_DIR  	P2DIR
#define FAN_PIN			BIT7

/*
 * Functions
 */
void display_data();
uint8_t set_active( );
void set_vent( uint8_t room, uint8_t state );
void main_board_packet_rx_handler();
void parse_user_packet( volatile uint8_t* p );
void parse_temp_packet( volatile uint8_t* p );
void parse_ack( volatile uint8_t* p );
uint8_t IP2room( uint8_t ip );
void timer_delay_ms( uint32_t t );

#endif /* MAIN_BOARD_H_ */
