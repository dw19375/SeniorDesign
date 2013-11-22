#include <msp430.h> 
#include "onewire.h"

#define BATTERY_CHECK_T 600			// Battery voltage checked at least this often (in seconds).
#define VENT_CLOSED_T   2025		// Timer half period when vent is closed
#define VENT_OPENED_T	1094		// Timer half period when vent is opened

/*
 * Local functions
 */
void timer_delay_ms( uint32_t t );
void vent_packet_rx_handler();
void set_servo( uint16_t pos );

/*
 * Global variables
 */
uint32_t delay_time = 0;
uint16_t pulse_count = 0;		// Count of pulses sent to servo

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;
	
	P1DIR |= 0x01;                            // P1.0 output
	CCTL0 = CCIE;                             // CCR0 interrupt enabled
	CCR0 = 1094;
	TACTL = TASSEL_2 + MC_1 + ID_3;           // SMCLK, upmode, /8

	// Initialize UART
	uart_init();

	// Enable interrupts
	_BIS_SR(GIE);

	// Initialize XBee
	if( !xbee_init( MY_IP ) )
	{
		// Set received packet handler
		packet_rx_handler( vent_packet_rx_handler );

		while( 1 )
		{
			// Check battery voltage

			// May be cut short by an incoming packet, but that's OK
			timer_delay_ms( BATTERY_CHECK_T * 1000 );		// Delay 10 minutes
		}
	}

	return 0;
}

/*
 * Called when we have received a UDP packet.
 * Parses packet and activates servo if necessary
 */
void vent_packet_rx_handler()
{
	// Get pointer to packet
	volatile uint8_t* p = uart_get_frame();

	// Check if packet is valid, data starts at p[14]
	if( p && p[2] > 14 && p[3] == 0xB0 &&
			p[14] == VENT_CMD && verify_checksum(p+3, p[2]-3) )
	{
		vent_cmd* c = (vent_cmd*)(p+14);		// Typecast data portion of packet to a vent_cmd struct

		if( c->cmd )
		{
			// Open vent
			set_servo( VENT_OPENED_T );
		}
		else
		{
			// Close vent
			set_servo( VENT_CLOSED_T );
		}

		// Send ACK

	}
}

/*
 * Sets the servo position
 *
 * pos is twice the period, in timer cycles, of the
 * output waveform.  1 cycle = 1us at 8MHz with clock
 * divider of 8.
 */
void set_servo( uint16_t pos )
{
	delay_time = 0;

	TACTL &= ~MC_3;		// Halt Timer
	CCR0 = pos;			// Set new period
	TACTL |= MC_1;		// Up mode
}

/*
 * Uses timer to delay t milliseconds
 */
void timer_delay_ms( uint32_t t )
{
	CCTL0 |= CCIE;                             // CCR0 interrupt enabled

	CCR0 = 1000;

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
 * Interrupt handler for UART transmit
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
	if( delay_time )
	{
		/*
		 * If delay time is set, we use this timer to delay.
		 */
		delay_time--;

		if( !delay_time )
			_bic_SR_register_on_exit( LPM0_bits );
	}
	else
	{
		_bic_SR_register_on_exit( LPM0_bits );
	}

}
