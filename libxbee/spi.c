/*
 * spi.c
 *
 *  Created on: Oct 30, 2013
 *      Author: Dan Whisman
 */

/*
 * Includes
 */
#include "spi.h"

/*
 * Global variables
 */

// First two bytes are length,
// length = 0: already received packet
static uint8_t rx_buf[MAX_BUF_LEN] = {0, 0};


/*
 * Receives a frame one byte at a time, from UCA0RXBUF.
 * Will write to global buffer incrementally whole frame received,
 * but writes length when whole frame received.
 * This is intended to be called from an Rx interrupt.
 *
 * Assuming UCA0RXBUF contains valid data
 */
void recv_frame( )
{
	static uint16_t len = 0;			// Length of whole frame
	static uint8_t torx = 0;			// Number of bytes to copy into buffer
	static uint8_t numbytes = 0;		// Number of bytes received thus far

	// If numbytes == 0 or 1, haven't received whole length yet
	if( numbytes == 0 )
	{
		len = 0;
		len |= UCA0RXBUF << 8;

		// Don't copy to buffer yet, but indicate that it is invalid
		rx_buf[0] = 0;
		rx_buf[1] = 0;
	}
	// Have received whole length
	else if( numbytes == 1 )
	{
		len |= UCA0RXBUF;

		if( len > MAX_BUF_LEN )
			torx = MAX_BUF_LEN;
		else
			torx = len;
	}
	else if( numbytes < (torx-1) )
	{
		rx_buf[numbytes] = UCA0RXBUF;
	}
	// Receiving last byte now
	// when numbytes == min(MAX_BUF_LEN-1, len-1)
	else if( numbytes == torx )
	{
		// Copy len over
		rx_buf[0] = (len >> 8) & 0xFF;
		rx_buf[1] = len & 0xFF;
		rx_buf[numbytes] = UCA0RXBUF;
	}

	// Must wait for all bytes if len > MAX_BUF_LEN.  Just ignore them.

	numbytes++;

	if( numbytes == len )
		numbytes = 0;
}

/*
 * Returns pointer to received frame if unread, 0x00 if not
 */
uint8_t* get_frame()
{
	uint8_t* ret = 0x00;

	if( rx_buf[0] || rx_buf[1])
		ret = rx_buf;

	rx_buf[0] = 0;
	rx_buf[1] = 0;

	return ret;
}

/*
 * Sends a frame passed in - must have correct header format!
 * This is intended to be called from an interrupt routine, once
 * for each byte sent.  Length of buffer is int from first two bytes
 * of buf.
 *
 * Assuming it is OK to send right now
 */
void send_frame( uint8_t* buf )
{
	static uint8_t totx = 0;			// Number of bytes to send total
	static uint8_t numbytes = 0;		// Number of bytes sent / index of next byte to send

	if( numbytes == 0 )
	{
		totx |= buf[0];
	}
	else if( numbytes == 1 )
	{
		totx |= (buf[1])<<8;
	}

	if( numbytes == totx )
	{
		totx = 0;
		numbytes = 0;
	}
	else
	{
		UCA0TXBUF = buf[numbytes];
		numbytes++;
	}
}
