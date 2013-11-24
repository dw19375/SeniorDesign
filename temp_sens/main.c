#include <msp430g2333.h>
#include <stdint.h>
#include "delay.h"
#include "onewire.h"
#include "ds18b20.h"
#include "xbee_uart.h"
#include "xbee_net.h"

#define DATA_SEND_T 2l			// Number of seconds between data samples
#define MY_IP "130"
#define PRECISION 1				// Set precesion of temperature sensor to 10-bit (1)

void timer_delay_ms( uint32_t t );

// Delay time for timer delays
uint32_t delay_time = 0;

/*
 * main.c
 */
int main(void) {
	int16_t temp = 0;
	temp_data data = {TEMP_DATA, 0, 0};

	WDTCTL = WDTPW + WDTHOLD; //Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	// Timer A0 setup
	CCR0 = 8000;
	TACTL = TASSEL_2 + MC_1;          		  // SMCLK, upmode, /1

	// Initialize UART
	uart_init();

	// Initialize 1-wire port and set precision to 10 bits.
	temp_init();
	set_precision( PRECISION );

	// Enable interrupts
	_BIS_SR(GIE);

	// Initialize XBee
	if( !xbee_init( MY_IP ) )
	{
		while( 1 )
		{
			// Get temperature
			start_conversion();
			timer_delay_ms( TEMP_READ_DELAY << PRECISION );
			temp = get_temp();

			// Send temp data on wifi
			data.temp = temp >> ( 3-PRECISION );

			xbee_tx_packet( 100, (uint8_t*)&data, sizeof(data) ); // MAIN_IP

			timer_delay_ms(DATA_SEND_T * 1000);
		}
	}
}

/*
 * Uses timer to delay t milliseconds
 */
void timer_delay_ms( uint32_t t )
{
	CCTL0 |= CCIE;                             // CCR0 interrupt enabled

	delay_time = t;
	_BIS_SR(LPM0_bits + GIE);                 // Enter LPM0 w/ interrupt

	CCTL0 &= ~CCIE;                             // CCR0 interrupt disabled
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

		// Receive the next byte
		uart_recv_next_byte();
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

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	/*
	 * This is intended to interrupt every ~1ms (8 MHz clock)
	 */
	if( delay_time )
	{
		delay_time--;

		if( !delay_time )
			_bic_SR_register_on_exit( LPM0_bits );
	}
}
