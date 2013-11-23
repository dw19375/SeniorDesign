
/*
 * Includes
 */
#include <msp430.h> 
#include <stdint.h>
#include "lcd20.h"
#include "ds18b20.h"
#include "xbee_net.h"
#include "xbee_uart.h"
#include "main_board.h"

/**************************************************************************************************
 * Global Data
 */
/*
 * NUM_ROOMS is 2 right now (see main_board.h)
 * The index into temperature and humidity arrays is the room number
 */
static int temperature[NUM_ROOMS] = {88, 88};			// Initialize temperature to 22 deg. C
static int humidity[NUM_ROOMS] = {50, 50};				// Initialize humidity to 50%
static uint8_t priority_room = 0;
static uint8_t system = 0;								// 0 for off, 1 for AC, 2 for heat
static uint8_t ack_timer = 0;							// Counter for ACK timeouts


/*
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	// Timer A0 setup
	CCR0 = 8000;
	TACTL = TASSEL_2 + MC_1;          		  // SMCLK, upmode, /1

	// Initialize UART
	uart_init();

	// Initialize 1-wire port and set precision to 10 bits.
	temp_init();
	set_precision( PRECESION );

	// Initialize LCD
	lcdinit();

	// Enable interrupts
	_BIS_SR(GIE);

	// Initialize XBee
	if( !xbee_init( MY_IP ) )
	{
		while( 1 )
		{
			// Get temperature
			start_conversion();
			timer_delay_ms( TEMP_READ_DELAY << PRECESION );
			temp = get_temp();

			// Send temp data on wifi
			data.temp = temp >> 2;

			xbee_tx_packet( MAIN_IP, (uint8_t*)&data, sizeof(data) );

			timer_delay_ms(LOOP_DELAY * 1000);
		}
	}
	
	return 0;
}

/*
 * Called when we have received a UDP packet.
 * Parses packet
 * 	If it is an ACK, resets the timeout counter
 * 	If it is temperature data, updates local values
 */
void main_board_packet_rx_handler()
{
	// Get pointer to packet
	volatile uint8_t* p = uart_get_frame();

	// Check if packet is valid, data starts at p[14]
	if( p && p[2] > 14 && verify_checksum((uint8_t*)p+3, p[2]-3) )
	{
		// data starts at p[14]
		uint16_t type = *((int*)(&p[14]));		// Get int at p[14] and p[15], with proper byte order

		switch(type)
		{
			case TEMP_DATA:
				parse_temp_packet( p );
				break;
			case USER_CMD:
				parse_user_packet( p );
				break;
			case ACK:
				ack_timer = 0;
				break;
			default:
				break;
		}
	}
}

/*
 * Parses a user packet that was just received.
 *
 * Inputs:
 * 		p - pointer to XBee receive frame - see XBee datasheet, pg. 92 (Rx Packet IPv4)
 * 		Can assume p is valid and receive frame is valid
 */
void parse_user_packet( uint8_t* p )
{

}

/*
 * Parses a packet from a temperature sensor
 *
 * Inputs:
 * 		p - pointer to XBee receive frame - see XBee datasheet, pg. 92 (Rx Packet IPv4)
 * 		Can assume p is valid and receive frame is valid
 */
void parse_temp_packet( uint8_t* p )
{
	temp_data* q = (temp_data*)(p+14);
	uint8_t rm = IP2room( p[7] );		// Source address is at offset 4, lowest byte at offset 7

	temperature[rm] = q->temp;
	humidity[rm] = q->humid;
}

/*
 * Function to convert the lower 8 bits of the IP address to the room number.
 * Returns 0 on error - don't want to return -1 because we should always return
 * something valid so the program doesn't crash.
 *
 * Inputs:
 * 		ip - lowest 8 bits of received IP address.
 *
 * 	Return value:
 * 		Room number corresponding to IP address or 0 on error.
 */
uint8_t IP2room( uint8_t ip )
{
	uint8_t retval = 0;

	// One IP for each temp sensor, and one for vent controller
	if( ip >= MAIN_IP && ip < MAIN_IP + 2*NUM_ROOMS )
	{
		retval = (ip - MAIN_IP) >> 1;		// Return (ip - MAIN_IP)/2
	}

	return retval;
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
