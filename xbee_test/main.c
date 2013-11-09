#include <msp430.h> 
#include <stdint.h>
#include "lcd20.h"

void uart_send_frame( uint8_t* buf );
void uart_transmit_frame();
uint8_t calculate_checksum( uint8_t* buf, uint8_t len );
uint8_t verify_checksum( uint8_t* buf, uint8_t len, uint8_t checksum );

static uint8_t* to_send;
static uint8_t bytes_rx = 0;

/*
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    if (CALBC1_1MHZ==0xFF)					// If calibration constant erased
    {
      while(1);                               // do not load, trap CPU!!
    }
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;
    P1SEL = BIT1 + BIT2 ;                   // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2 ;                  // P1.1 = RXD, P1.2=TXD
    UCA0CTL0 |= UCMSB;						// MSB first
    UCA0CTL1 |= UCSSEL_2;                   // SMCLK
    UCA0BR0 = 0x41;                         // 8MHz 9600    833 = 0x341
    UCA0BR1 = 0x03;                         // 8MHz 9600
    UCA0MCTL = UCBRS1;                      // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                   // **Initialize USCI state machine**
    IE2 |= UCA0RXIE + UCA0TXIE;             // Enable USCI_A0 RX interrupt

	uint8_t buf[22] = {0x7E, 0x00, 0x04, 0x08, 0x07, 'T', 'P', 0x00};
	buf[7] = calculate_checksum(buf+3,4);


	__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled

	
	return 0;
}

void uart_send_frame( uint8_t* buf )
{
	// Keep local copy of pointer
	to_send = buf;

	if( buf )
	{
		// Clear Tx interrupt flag
		IFG2 &= ~UCA0TXIFG;

		// Transmit first frame
		uart_transmit_frame();
	}
}

void uart_transmit_frame()
{
	static uint8_t totx = 0;			// Number of bytes to send total
	static uint8_t numbytes = 0;		// Number of bytes sent / index of next byte to send

	if( to_send )
	{
		if( numbytes == 0 )
		{
			totx = to_send[2] + 4;			// LSB of length in XBee data frame
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

			UCA0TXBUF = to_send[numbytes];
			numbytes++;
		}
	}
}


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

uint8_t verify_checksum( uint8_t* buf, uint8_t len, uint8_t checksum )
{
	return 0xFF == ( checksum + calculate_checksum(buf, len) );
}

/*
 * Interrupt handler for SPI receive
 */
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIA0RX_ISR(void)
{
	if( IFG2 & UCA0RXIFG )
	{
		// Clear the interrupt flag
		IFG2 &= ~UCA0RXIFG;

		if( bytes_rx < 40 )
		{
			bytes_rx++;
			hex2Lcd( UCA0RXBUF );
		}
	}
}

/*
 * Interrupt handler for SPI transmit
 */

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCIA0TX_ISR(void)
{
	if( IFG2 & UCA0TXIFG )
	{
		// Clear the interrupt flag
		IFG2 &= ~UCA0TXIFG;

		// Transmit next byte of frame
		uart_transmit_frame();
	}
}
