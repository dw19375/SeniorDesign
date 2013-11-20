#include <msp430g2333.h>
#include <stdint.h>
#include "delay.h"
#include "onewire.h"
#include "ds18b20.h"
#include "lcd20.h"
//#include "spi.h"

uint8_t bytes_rx = 0;

/*
 * main.c
 */
int main(void) {
	int xpos = 0, begin_xpos;
	int16_t temp = 0;

	WDTCTL = WDTPW + WDTHOLD; //Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	P1DIR |= 0x48;

	// Initialize 1-wire port and set precision to 10 bits.
	temp_init();
	set_precision( 1 );

	__delay_cycles(20000000);

	lcdinit();

//	lcdData('H');
//	lcdData('e');
	// Initialize SPI port
//	spi_init();

//	lcdData('l');
//	lcdData('l');
	prints("Temp: ");
	xpos += 6;
//	gotoXy(12,0);
//	prints(" deg. C");

	// Clear the interrupt flags
//	IFG2 = 0;
//	__bis_SR_register(GIE);

	begin_xpos = xpos;
//	lcdData('o');
	while( 1 )
	{
		int i;
		volatile uint8_t* rx_data;

//		gotoXy(0,1);
//		lcdData('R');
//		gotoXy(0,2);

//		uint8_t buf[22] = {0x7E, 0x00, 0x04, 0x08, 0x07, 'T', 'P', 0x00};
//		buf[7] = calculate_checksum(buf+3,4);

//		spi_send_frame(buf);

//		for(i=0; i<64; i++)
//		{
//			lcdData(' ');
//		}

//		__delay_cycles(100000);

//		for( i=0; i<8; i++ )
//		{
//			P1OUT &= ~(BIT0);
//			__delay_cycles(100);
//
//			UCA0TXBUF = buf[i];
//
//			__delay_cycles(100);
//			P1OUT |= BIT0;
//		}
//
//		for( ; i<40; i++ )
//		{
//			P1OUT &= ~(BIT0);
//			__delay_cycles(100);
//
//			UCA0TXBUF = 0x00;
//
//			__delay_cycles(100);
//			P1OUT |= BIT0;
//		}


//		gotoXy(1,1);
//		lcdData('x');
//		gotoXy(0,3);

//		while( !(rx_data=spi_get_frame()) )		// Wait until we receive whole frame
//		{
//			__delay_cycles(100000);
//		}

		// Write frame just received to LCD.
//		gotoXy(2,1);
//		prints("x: ");
//		for( i=0; i < rx_data_len && i < MAX_BUF_LEN; i++ )
//		{
//			hex2Lcd( rx_data[i] );
//		}

		temp = read_temp();

		gotoXy(xpos,0);
		xpos += integerToLcd( temp >> 4 );	// Divide by 16 to display integer part
		xpos += dec2ToLcd( temp >> 2 );		// Want fractional part in lowest two bits

		gotoXy(xpos,0);
		prints(" deg. C");

		xpos = begin_xpos;
	
//		_BIS_SR(LPM0_bits + GIE);
	}
//	return 0;
}

/*
 * Interrupt handler for SPI receive
 */
//#pragma vector=USCIAB0RX_VECTOR
//__interrupt void USCIA0RX_ISR(void)
//{
//	if( IFG2 & UCA0RXIFG )
//	{
//		// Clear the interrupt flag
//		IFG2 &= ~UCA0RXIFG;
//
////		if( !(IE2 & UCA0TXIE) )
////			hex2Lcd( UCA0RXBUF );
//
//		if( bytes_rx < 40 )
//		{
//			bytes_rx++;
//			hex2Lcd( UCA0RXBUF );
//		}
//
//	    // Receive next byte of frame
//	    //spi_recv_frame();
//
//	    // Receive next byte of frame, will return if nothing to send
//	    //spi_transmit_frame();
//	}
//  //hex2Lcd( UCA0RXBUF );
//}
//
///*
// * Interrupt handler for SPI transmit
// */
//#pragma vector=USCIAB0TX_VECTOR
//__interrupt void USCIA0TX_ISR(void)
//{
//	if( IFG2 & UCA0TXIFG )
//	{
//		// Clear the interrupt flag
//		IFG2 &= ~UCA0TXIFG;
//
//		// Receive next byte of frame
//		//spi_transmit_frame();
//	}
//	P1OUT ^= 0x40;
//	//hex2Lcd( UCA0TXBUF );
//}
