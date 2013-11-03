/*
 * spi.c
 *
 *  Created on: Oct 30, 2013
 *      Author: Dan Whisman
 *
 *  Note: The interrupt handlers must be defined in the top level project.
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
volatile uint8_t rx_buf[MAX_BUF_LEN] = {0, 0};
volatile uint8_t rx_data_len = 0;				// Length of Rx data in rx_buf
volatile uint8_t have_rx_data = 0;

// Pointer to buffer to send
const uint8_t* to_send = 0x00;

/*
 * Initialization of SPI
 */
void spi_init()
{
	P1SEL |= BIT1 + BIT2 + BIT4;
	P1SEL2 |= BIT1 + BIT2 + BIT4;
	UCA0CTL0 = UCCKPL + UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 0x00;                           // /1024
	UCA0BR1 = 0x04;                              //
	UCA0MCTL = 0;                             // No modulation
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

	// Don't enable Tx interrupts just yet, wait until we send something
	IE2 |= UCA0RXIE;               			  // Enable USCI0 RX interrupt

}


/*
 * Receives a frame one byte at a time, from UCA0RXBUF.
 * Will write to global buffer incrementally data part of frame and checksum
 * This is intended to be called from an Rx interrupt.
 *
 * If there is more data than fits in the buffer, will ignore all bytes
 * afterward.
 *
 * Assuming UCA0RXBUF contains valid data
 */
void spi_recv_frame()
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
			if( rx_data_len < MAX_BUF_LEN )
			{
				rx_buf[rx_data_len] = UCA0RXBUF;
				rx_data_len++;
			}
			break;
	}

	/*
	 * If we have received all bytes (numbytes = total-1 -- haven't accounted
	 * for byte just received), reset numbytes to zero and write to checksum.
	 */
	if( numbytes >= len+3 ) // len >= 0, will set after numbytes = 2.
	{
		numbytes = 0;
		have_rx_data = 1;
	}
	else
	{
		numbytes++;

		// If not actively transmitting, may need to send data to receive it
		// Transmit interrupt is enabled only if we are actively transmitting relevant data
		// If it is clear, we will need to transmit our own
		if( (!(UCA0STAT & UCBUSY)) && (!(IE2 & UCA0TXIE)) )
		{
			UCA0TXBUF = 0xFF;
		}

		//while (!(IFG2 & UCA0TXIFG));    // USCI_A0 TX buffer ready?
		//UCA0TXBUF = 0xFF;				// Write to Tx buffer to start receiving next byte
	}

}

/*
 * Returns pointer to received frame if unread, 0x00 if not
 */
volatile uint8_t* spi_get_frame()
{
	volatile uint8_t* ret = 0x00;

	if( have_rx_data )
	{
		ret = rx_buf;
		have_rx_data = 0;
	}
	else if( (!(UCA0STAT & UCBUSY)) && (!(IE2 & UCA0TXIE)) )
	{
		UCA0TXBUF = 0xFF;
		// May need to start transmission to start receiving
		// UCA0TXBUF = 0xFF;
	}
	else
	{
		P1OUT ^= 0x40;
	}

	return ret;
}

/*
 * Sends a frame passed in - must have correct header format!
 * This is intended to be called from an interrupt routine, once
 * for each byte sent.  Length of buffer is int from first two bytes
 * of buf.
 *
 * Length of packet is in to_send[1] and to_send[2],
 * Don't make length more than 251 bytes.
 */
void spi_transmit_frame()
{
	static uint8_t totx = 0;			// Number of bytes to send total
	static uint8_t numbytes = 0;		// Number of bytes sent / index of next byte to send

	if( to_send )
	{
		if( numbytes == 0 )
		{
			totx = to_send[2] + 4;			// LSB of length in XBee data frame

//			// Clear Tx interrupt flag and enable interrupt
//			IFG2 &= ~UCA0TXIFG;
//			IE2 |= UCA0TXIE;
		}

		if( numbytes == totx )
		{
			numbytes = 0;

			// Have sent everything, set to_send to NULL to indicate so
			to_send = 0x00;

			// Disable Tx interrupt so we can receive without being bothered
			IE2 &= ~UCA0TXIE;
		}
		else
		{
			while(UCA0STAT & UCBUSY);		// Wait until SPI port not busy

			UCA0TXBUF = to_send[numbytes];
			numbytes++;

			__delay_cycles(100);
		}
	}
}

/*
 * Send an SPI frame pointed to by buf
 */
void spi_send_frame( const uint8_t* buf )
{
	// Keep local copy of pointer
	to_send = buf;

	if( buf )
	{
		// Clear Tx interrupt flag and enable interrupt
		IFG2 &= ~UCA0TXIFG;
		IE2 |= UCA0TXIE;
	}

	// Transmit first frame
	spi_transmit_frame();
}
