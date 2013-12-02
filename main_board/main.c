
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

#define MY_IP "128"

/**************************************************************************************************
 * Global Data
 */
/*
 * NUM_ROOMS is 2 right now (see main_board.h)
 * The index into temperature and humidity arrays is the room number
 */
static int16_t temperature[NUM_ROOMS] = {88, 88};		// Initialize temperature to 22 deg. C
//static int16_t humidity[NUM_ROOMS] = {50, 50};			// Initialize humidity to 50%
static int16_t desired_temp = 88;						// Desired temperature
static uint8_t alive[NUM_ROOMS] = {0,0};				// High nibble = temp, low nibble = vent
static uint8_t vent_state[NUM_ROOMS] = {0, 0};			// State of vents of each room 0 = closed, 1 = open
static uint8_t priority_room = 0;
static uint8_t system = 0;								// 0 for off, 1 for AC, 2 for heat (set by user)
static uint8_t active = 0;								// 0 if HVAC off, 1 if on (set by system)
static volatile uint8_t ack_timer = 0;					// Counter for ACK timeouts
static volatile uint16_t last_ack = 0;					// Sequence number of last ACK received
static volatile int16_t ack_to_send = 0;				// Sequence number of ACK to send
static uint8_t ip_to_ack = 100;							// IP address to send ACK to
static uint32_t delay_time;


/*
 * main.c
 */
int main(void)
{
	uint8_t fan_counter = 0;		// Counts how many loops to time fan turning on.
	uint8_t i;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;
//	P1DIR |= BIT6;

	// Interface pins to HVAC
	HEAT_PORT_DIR |= HEAT_PIN;
	AC_PORT_DIR |= AC_PIN;
	FAN_PORT_DIR |= FAN_PIN;

	HEAT_PORT_OUT &= ~HEAT_PIN;
	AC_PORT_OUT &= ~AC_PIN;
	FAN_PORT_OUT &= ~FAN_PIN;

	// Timer A0 setup
	CCR0 = 8000;
	TACTL = TASSEL_2 + MC_1;          		  // SMCLK, upmode, /1

	// Initialize UART
	uart_init();

	// Initialize 1-wire port and set precision to 10 bits.
	temp_init();
	set_precision( PRECISION );

	// Initialize LCD
	lcdinit();

	// Initialize packet handler
	packet_rx_handler( main_board_packet_rx_handler );

	// Enable interrupts
	_BIS_SR(GIE);

	// Initialize XBee
	xbee_init( MY_IP );
	if( !xbee_init( MY_IP ) )
	{
		while( 1 )
		{
			// Get temperature of local sensor
			start_conversion();
			timer_delay_ms( TEMP_READ_DELAY << PRECISION );
			temperature[0] = get_temp() >> ( 3 - PRECISION );

			alive[0] |= 0xF0;		// we're always alive

			// Set the vents, as needed, and turn on the HVAC
			if( set_active() )
			{
				if( !active )
				{
					// Turn HVAC on
					if( system == AC )
					{
						HEAT_PORT_OUT &= ~HEAT_PIN;
						AC_PORT_OUT |= AC_PIN;
						FAN_PORT_OUT &= ~FAN_PIN;
					}
					else if( system == HEAT )
					{
						HEAT_PORT_OUT |= HEAT_PIN;
						AC_PORT_OUT &= ~AC_PIN;
						FAN_PORT_OUT &= ~FAN_PIN;
					}

					fan_counter = 1;
					active = 1;
				}
			}
			else
			{
				if( active )
				{
					// Turn HVAC off
					HEAT_PORT_OUT &= ~HEAT_PIN;
					AC_PORT_OUT &= ~AC_PIN;
					FAN_PORT_OUT &= ~FAN_PIN;

					fan_counter = 0;
					active = 0;
				}
			}

			display_data();

//			P1OUT ^= BIT6;
			timer_delay_ms(LOOP_DELAY * 1000);
//			P1OUT ^= BIT6;

			if( ack_to_send )
			{
				// Send ACK
				ack a = {ACK, 0};
				a.seq = ack_to_send;
				xbee_tx_packet( ip_to_ack, (uint8_t*)&a, sizeof(a) );
				ack_to_send = 0;
			}

			if( fan_counter )
			{
				if( FAN_DELAY == fan_counter )
				{
					// Turn on fan
					FAN_PORT_OUT |= FAN_PIN;

					fan_counter = 0;
				}
				else
				{
//					FAN_PORT_OUT &= ~FAN_PIN;
					fan_counter++;
				}
			}

			for( i=0; i < NUM_ROOMS; i++ )
			{
				if( alive[i] & 0xF0 )
				{
					// Decrement upper nibble
					alive[i] -= 0x10;
				}
			}
		}
	}
	
	return 0;
}

/*
 * Sets vents if needed
 * Returns 1 if HVAC should be on based on current temperature profile.
 */
uint8_t set_active( )
{
	uint8_t i, ret = 0;
	int16_t thres = TEMP_THRESHOLD;

	for( i=0; i < NUM_ROOMS; i++ )
	{
		if( i == priority_room )
		{
			thres = PR_TEMP_THRESHOLD;
		}

		if( system == AC )
		{
			// AC is selected

			if( temperature[i] > (desired_temp + thres) )
			{
				// Too hot, open vent to let A/C in
				set_vent( i, 1 );
				ret |= 1;
			}
			else if( temperature[i] < desired_temp - TEMP_THRESHOLD )
			{
				// Cold enough, close vent
				set_vent( i, 0 );
			}
		}
		else if( system == HEAT )
		{
			// Heat is selected
			if( temperature[i] < (desired_temp - thres) )
			{
				// Too cold, open vent
				set_vent( i, 1 );
				ret |= 1;
			}
			else if( temperature[i] > (desired_temp + TEMP_THRESHOLD) )
			{
				// Warm enough, close vent
				set_vent( i, 0 );
			}
		}
	}

	return ret;
}

/*
 * Sends a vent command to set the vent to the desired state
 * Will do nothing if it is already there.
 */
void set_vent( uint8_t room, uint8_t state )
{
	static uint8_t s = 0;
	uint8_t i;

	if( vent_state[room] != state )
	{
		// Set state
		vent_state[room] = state;

		// Get address of vent in room
		uint8_t ip = MAIN_IP + 1 + (room << 1);

		s++;

		if( !s )		// Don't use sequence number 0
			s++;

		// Construct packet
		vent_cmd v = {VENT_CMD, 0, 1};
		v.cmd = state;
		v.seq = s;

		// Send message to room and wait for ACK, retry up to 2 times
		for(i=3; last_ack != s && i!=0; i--)
		{
			xbee_tx_packet( ip, (uint8_t*)(&v), sizeof(v) );
			timer_delay_ms( TIMEOUT );
		}

		alive[room] &= 0xF0;
		alive[room] |= 0x01;
		if( last_ack != s )
		{
			alive[room] |= 0x02;
		}
	}
}

/*
 * Displays the temperature data/settings on the LCD
 */
void display_data()
{
	uint8_t i;

	gotoXy(0,0);

	prints("Set: ");
	switch( system )
	{
		case AC:
			lcdData('C');
			break;
		case HEAT:
			lcdData('H');
			break;
//		case FAN:
//			lcdData('F');
//			break;
		default:
			lcdData(' ');
			break;
	}
	lcdData(' ');
	integerToLcd( desired_temp >> 2 );
	dec2ToLcd( desired_temp );

	gotoXy(0,1);
	prints(" PR: ");
	integerToLcd( priority_room );
	lcdData(' ');
	integerToLcd( temperature[priority_room] >> 2 );
	dec2ToLcd( temperature[priority_room] );

	gotoXy(20,0);

	for( i=0; i<NUM_ROOMS; i++ )
	{
		hex2Lcd( alive[i] );
	}
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
				parse_ack( p );
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
void parse_user_packet( volatile uint8_t* p )
{
	user_cmd* u = (user_cmd*)(p+14);

	if( u->room < NUM_ROOMS )
	{
		priority_room = u->room;
	}

	if( u->sys < 3 )
	{
		system = u->sys;
	}

	desired_temp = u->temp;

	ip_to_ack = p[7];			// IP address is at offset 4, lowest byte at offset 7
	ack_to_send = u->seq;
}

void parse_ack( volatile uint8_t* p )
{
	ack* a = (ack*)(p+14);

	ack_timer = 0;
	last_ack = a->seq;
}

/*
 * Parses a packet from a temperature sensor
 *
 * Inputs:
 * 		p - pointer to XBee receive frame - see XBee datasheet, pg. 92 (Rx Packet IPv4)
 * 		Can assume p is valid and receive frame is valid
 */
void parse_temp_packet( volatile uint8_t* p )
{
	temp_data* q = (temp_data*)(p+14);
	uint8_t rm = IP2room( p[7] );		// Source address is at offset 4, lowest byte at offset 7

	temperature[rm] = q->temp;
	alive[rm] |= 0xF0;					// Set upper bits to reset timeout counter

//	humidity[rm] = q->humid;
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
 * Returns apparent temperature for given temperature and humidity
 */
//int16_t humidex( int16_t temp, int16_t humidity )
//{
//	return temp;
//}

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

		if( ack_to_send )
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
