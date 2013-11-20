
/*
 * Includes
 */
#include <msp430.h> 
#include <stdint.h>
#include "lcd20.h"
#include "ds18b20.h"
#include "main_board.h"

/**************************************************************************************************
 * Global Data
 */
/*
 * NUM_ROOMS is 2 right now (see main_board.h)
 * The index into temperature and humidity arrays is the room number
 */
static int temperature[NUM_ROOMS] = {88, 88};			// Initialize temperature to 22 deg. C
static uint8_t humidity[NUM_ROOMS] = {50, 50};				// Initialize humidity to 50%
static uint8_t priority_room = 0;


/*
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // Run all the initialization functions for the xbee, temp. sensor, etc.
	
	return 0;
}

/*
 * Parses a user packet that was just received.
 *
 * Inputs:
 * 		p - pointer to XBee receive frame - see XBee datasheet, pg. 92 (Rx Packet IPv4)
 * 		    The receive frame starts at byte 3 (i.e. it does not include the initial 0x7E
 * 		    or the two byte length)
 */
void parse_user_packet( uint8_t* p )
{
	if( p )
	{

	}
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
	if( ip >= IP_START && ip < IP_START + 2*NUM_ROOMS )
	{
		retval = (ip - IP_START) >> 1;		// Return (ip - IP_START)/2
	}

	return retval;
}
