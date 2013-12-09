#include <msp430.h> 
#include "onewire.h"
#include "xbee_uart.h"
#include "xbee_net.h"

#define OW_PORT_OUT  	&P2OUT
#define OW_PORT_IN		&P2IN
#define OW_PORT_REN		&P2REN
#define OW_PORT_DIR		&P2DIR
#define OW_PIN			BIT4

#define V_THRESHOLD		550
#define BATTERY_CHECK_T 4			// Battery voltage checked at least this often (in seconds).
#define MAX_PULSE_T     1500l		// Number of ms to run the servo when changing position
#define VENT_CLOSED_T   2025		// Timer half period when vent is closed
#define VENT_OPENED_T	1094		// Timer half period when vent is opened
#define MY_IP			"129"

/*
 * Local functions
 */
void read_scratchpad( uint8_t* buf );
void start_conversion();
void timer_delay_ms( uint32_t t );
void vent_packet_rx_handler();
void set_servo( uint16_t pos );

/*
 * Global variables
 */
uint32_t delay_time = 0;
uint16_t pulse_count = 0;		// Count of pulses sent to servo
uint16_t ack_to_send = 0xFFFF;
uint16_t servo_per = 0;
static onewire_t ow;

/*
 * main.c
 */
int main(void)
{
	uint8_t sp[9];
	uint16_t voltage = 0;		// Battery voltage in units of 10 mV


    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;
	
//	P1DIR |= BIT6;
	P2DIR |= BIT3 + BIT2;                     // P2.3 output for servo, P2.4 for battery LED
	P2OUT &= ~BIT3;
	P2OUT |= BIT2;
	P2SEL = 0x00;

	CCTL0 = CCIE;                             // CCR0 interrupt enabled
	CCR0 = 1094;
	TACTL = TASSEL_2 + MC_1 + ID_3;           // SMCLK, upmode, /8

	// Initialize UART
	uart_init();

	// Initialize 1-wire port
	ow.port_out = OW_PORT_OUT;
	ow.port_in = OW_PORT_IN;
	ow.port_ren = OW_PORT_REN;
	ow.port_dir = OW_PORT_DIR;
	ow.pin = OW_PIN;

	// Enable interrupts
	_BIS_SR(GIE);

	timer_delay_ms( 10000 );

	// Initialize XBee
	xbee_init( MY_IP );
	if( !xbee_init( MY_IP ) )
	{
		P2OUT &= ~BIT2;

		// Set received packet handler
		packet_rx_handler( vent_packet_rx_handler );

		// Initialize battery monitor
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0x4E); // Write scratchpad
		onewire_write_byte(&ow, 0x00); // Page 0
		onewire_write_byte(&ow, 0x08); // AD = 1, select battery voltage for A/D input
		onewire_reset(&ow);
		onewire_line_high(&ow);

		// Set the XBee to sleep mode
		sleep_init();
		sleep_set();

//		P1OUT |= BIT6;


		while( 1 )
		{
			// Check battery voltage - start conversion
			start_conversion();
	//		P2OUT ^= BIT2;

			if( ack_to_send )
			{
				// Send ACK
				ack a = {ACK, 0};
				a.seq = ack_to_send;
				xbee_tx_packet( MAIN_IP, (uint8_t*)&a, sizeof(a) );
				ack_to_send = 0;
			}

			if( servo_per )
			{
				set_servo( servo_per );
				servo_per = 0;
			}

			// May be cut short by an incoming packet, but that's OK
			timer_delay_ms( BATTERY_CHECK_T * 1000 );		// Go to sleep

			P2OUT |= BIT2;
			read_scratchpad( sp );
			voltage = (sp[4] << 8) | sp[3];

			// Check voltage
			if( voltage < V_THRESHOLD )
			{
				P2OUT |= BIT2;
			}
			else
			{
				P2OUT &= ~BIT2;
			}

//			xbee_tx_packet( 102, (uint8_t*)&voltage, sizeof(voltage) );
		}
	}
	else
	{
		while(1)
		{
			timer_delay_ms( 1000 );		// Go to sleep
			P2OUT ^= BIT2;				// Flash LED on P2.2
		}
	}
}

/*
 * Reads the scratchpad memory into buf
 */
void read_scratchpad( uint8_t* buf )
{
	int i;
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0xb8); // Recall memory, page 0
	onewire_write_byte(&ow, 0x00);

	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0xbe); // read scratchpad command
	onewire_write_byte(&ow, 0x00); // Page 0
	for (i = 0; i < 9; i++)
		buf[i] = onewire_read_byte(&ow);
}

/*
 * Starts a voltage read
 */
void start_conversion()
{
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0xB4); // convert V command
	onewire_line_high(&ow);
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
			 verify_checksum((uint8_t*)p+3, p[2]-3) )
	{
		vent_cmd* c = (vent_cmd*)(p+14);		// Typecast data portion of packet to a vent_cmd struct

		if( c->type == VENT_CMD )
		{
			if( c->cmd )
			{
				// Open vent
				servo_per = VENT_OPENED_T;
//				P1OUT &= ~BIT6;
			}
			else
			{
				// Close vent
				servo_per = VENT_CLOSED_T;
			}

			// Can't send ACK here (requires interrupts), so save seq. number to do it later
			ack_to_send = c->seq;
		}
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
	pulse_count = 1000*MAX_PULSE_T / pos;

	TACTL &= ~MC_3;		// Halt Timer
	CCR0 = pos;			// Set new period
	TACTL |= MC_1;		// Up mode

	CCTL0 |= CCIE;      // CCR0 interrupt enabled
}

/*
 * Uses timer to delay t milliseconds
 */
void timer_delay_ms( uint32_t t )
{
	if( !pulse_count )
	{
		TACTL &= ~MC_3;		// Halt Timer
		CCR0 = 1000;
		TACTL |= MC_1;		// Up mode

		delay_time = t;
		CCTL0 |= CCIE;                             // CCR0 interrupt enabled

		_BIS_SR(LPM0_bits + GIE);                 // Enter LPM0 w/ interrupt
	}
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

		if( ack_to_send ) // ack_to_send is nonzero when we need to move the servo as well.
			_bic_SR_register_on_exit( LPM0_bits );		// Get out of low power mode if we need to send an ACK
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
	}
	else if( pulse_count )
	{
		// Generate waveform on P2.3
		P2OUT ^= BIT3;
		pulse_count--;

		if( !pulse_count )
		{
			P2OUT &= ~BIT3;
//			P1OUT |= BIT6;
		}
	}
	else
	{
		CCTL0 &= ~CCIE;                             // CCR0 interrupt disabled
		_bic_SR_register_on_exit( LPM0_bits );
	}

}
