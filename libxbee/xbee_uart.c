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
uint8_t* to_send = 0x00;				// Pointer to buffer to send.
volatile uint8_t rx_buf[RX_BUF_LEN];	// Receive buffer
uint8_t rx_data_len = 0;

/*
 * Initialization function -- STILL NEED TO ENABLE GLOBAL INTERRUPTS AFTER CALLING
 */
void uart_init()
{
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;
	P1SEL = BIT1 + BIT2 ;                   // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                  // P1.1 = RXD, P1.2=TXD
	UCA0CTL0 = 0;							// LSB first
	UCA0CTL1 = UCSSEL_2;                    // SMCLK
	UCA0BR0 = 0x41;                         // 8MHz 9600    833 = 0x341
	UCA0BR1 = 0x03;                         // 8MHz 9600
	UCA0MCTL = UCBRS1;                      // Modulation UCBRSx = 2
	UCA0CTL1 &= ~UCSWRST;                   // **Initialize USCI state machine**

	IE2 |= UCA0RXIE + UCA0TXIE;             // Enable USCI_A0 RX interrupt
}

/*
 * This function is called when a frame has been received.  If the argument
 * is not NULL, sets the new handler to that function, otherwise, it calls
 * the handler already present.
 */
void frame_recv_handler( void (*new_handler)() )
{
	void (*handler)() = 0x00;

	if( 0x00 == new_handler )
	{
		if( handler )
		{
			(*handler)();
		}
	}
	else
	{
		handler = new_handler;
	}
}


volatile uint8_t* uart_get_frame()
{
	volatile uint8_t* ret = 0x00;

	if( rx_data_len )
		ret = rx_buf;

	return ret;
}

/*
 * Receives next byte of API frame.
 * This is meant to be called from the Rx interrupt
 */
void uart_recv_next_byte()
{
	static uint16_t len = 0;			// Length of whole frame
	static uint8_t numbytes = 0;		// Number of bytes received thus far

	switch( numbytes )
	{
		case 0:
			// First byte received should be 0x7E, if not, silently ignore
			if( UCA0RXBUF != 0x7E )
				return;

			len = 0;
			rx_data_len = 0;

			break;
		case 1:
			// Received MSB of length
			len = ((uint16_t)UCA0RXBUF) << 8;
			break;
		case 2:
			// Received LSB of length
			len |= UCA0RXBUF;
			break;
		default:
			// Copy into Rx buffer, if there is room
			if( numbytes < RX_BUF_LEN )
			{
				rx_buf[numbytes] = UCA0RXBUF;
			}
			break;
	}

	/*
	 * If we have received all bytes (numbytes = total-1 -- haven't accounted
	 * for byte just received), reset numbytes to zero and write to checksum.
	 */
	if( numbytes >= len+3 ) // len >= 0, will set after numbytes = 2.
	{
		rx_data_len = numbytes+1;
		numbytes = 0;

		// Call handler for received frame
		frame_recv_handler( 0x00 );
	}
	else
	{
		numbytes++;
	}

}

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
