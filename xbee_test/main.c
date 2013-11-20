#include <msp430.h> 
#include <stdint.h>
#include "lcd20.h"
#include "xbee_uart.h"

static uint8_t bytes_rx = 0;

// Hack to get out of transparent mode
static char rx_buf[3];
static volatile uint8_t rx_ok = 0;

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

	lcdinit();
	uart_init();

	__bis_SR_register(GIE);

//	uint8_t str[8] = {'+','+','+',0x00};
//
//	uart_send_frame(str);
//
//	while( !rx_ok ); // Wait until we have received OK from xbee
//	str[0] = 'A';
//	str[1] = 'T';
//	str[2] = 'A';
//	str[3] = 'P';
//	str[4] = ' ';
//	str[5] = '1';
//	str[6] = 0x0D;
//	str[7] = 0x00;
//	uart_send_frame(str);
//
//	while( !rx_ok );
//	str[2] = 'C';
//	str[3] = 'N';
//	str[4] = ' ';
//	uart_send_frame(str);
//
//	while( !rx_ok );

	// Should be in API mode now
	uint8_t buf[22] = {0x7E, 0x00, 0x04, 0x08, 0x07, 'T', 'P', 0x00};

//	__delay_cycles(10000000);
	buf[7] = calculate_checksum(buf+3,4);

	uart_send_frame(buf);

	while(1);

	return 0;
}



/*
 * Interrupt handler for UART receive
 */
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIA0RX_ISR(void)
{
	if( IFG2 & UCA0RXIFG )
	{
		// Clear the interrupt flag
		IFG2 &= ~UCA0RXIFG;

		// Shift byte into rx_buf
		rx_buf[0] = rx_buf[1];
		rx_buf[1] = rx_buf[2];
		rx_buf[2] = UCA0RXBUF;

		if( rx_buf[0] == 'O' &&
			rx_buf[1] == 'K' &&
			rx_buf[2] == 0x0D
		   )
		{
			rx_ok = 1;
		}
		else
		{
			if( 2 != rx_ok )
				rx_ok = 0;
		}

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
		uart_transmit_next_byte();
	}
}
