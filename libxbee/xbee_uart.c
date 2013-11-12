/*
 * xbee_uart.c
 *
 *  Created on: Nov 12, 2013
 *      Author: Dan Whisman
 */

/*
 * Includes
 */
#include "xbee_uart.h"


/*
 * Static Global variables
 */
uint8_t* to_send = 0x00;			// Pointer to buffer to send.

/*
 * Initiates the process of sending a frame on the UART
 * This is called once.
 *
 * Frames are either XBee API frames (beginning with 0x7E,
 * or NULL terminated strings.
 */
void uart_send_frame( uint8_t* buf )
{
	// Keep local copy of pointer
	to_send = buf;

	if( buf )
	{
		// Clear Tx interrupt flag
		IFG2 &= ~UCA0TXIFG;

		// Transmit first byte
		uart_transmit_next_byte();
	}
}

/*
 * Transmits next byte of frame on UART.
 * This is meant to be called from the Tx interrupt handler for each byte.
 */
void uart_transmit_next_byte()
{
	static uint8_t totx = 0;			// Number of bytes to send total
	static uint8_t numbytes = 0;		// Number of bytes sent / index of next byte to send

	if( to_send )
	{
		if( numbytes == 0 )
		{
			/*
			 * If this is an API frame, its length is the second and third bytes of the frame
			 * Otherwise, it is a NULL terminated sequence of bytes to send.
			 */
			if( to_send[0] == 0x7E )
				totx = to_send[2] + 4;			// LSB of length in XBee data frame
			else
			{
				totx = 0;

				while( to_send[totx] && totx < 16 )
					totx++;
			}
		}

		if( numbytes == totx )
		{
			numbytes = 0;

			// Have sent everything, set to_send to NULL to indicate so
			to_send = 0x00;
		}
		else
		{
			while(UCA0STAT & UCBUSY);		// Wait until uart not busy

			// Increment numbytes before transmitting, in case we're interrupted...
			numbytes++;
			UCA0TXBUF = to_send[numbytes-1];
		}
	}
}

/*
 * Calculates checksum for buf of length len
 * Checksum is calculated as
 * 0xFF - (lowest byte of sum of all bytes in buf)
 * For XBee, checksum does not include length and delimiter bytes
 */
uint8_t calculate_checksum( uint8_t* buf, uint8_t len )
{
	uint8_t ret = 0;

	while( len )
	{
		len--;
		ret += buf[len];
	}

	return 0xFF - ret;
}

/*
 * To verify, buf must include all bytes except delimiter and length bytes,
 * buf must include checksum at end.
 */
uint8_t verify_checksum( uint8_t* buf, uint8_t len )
{
	return calculate_checksum( buf, len-1 ) == buf[len-1];
}
