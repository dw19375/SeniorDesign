/*
 * libtemp.c
 *
 * Library to configure and read the DS18B20 temperature sensor.
 * This requires lib1wire to communicate with the DS18B20.
 * Currently, this only works for one device on the 1-wire bus.
 *
 *  Created on: Oct 26, 2013
 *      Author: Dan Whisman
 */

/*
 * Includes
 */
#include "ds18b20.h"
#include "delay.h"
#include "onewire.h"

/*
 * Defines
 */
#define OW_PORT_OUT  	&P2OUT
#define OW_PORT_IN		&P2IN
#define OW_PORT_REN		&P2REN
#define OW_PORT_DIR		&P2DIR
#define OW_PIN			BIT4

#define TEMP_READ_DELAY 94		// Time (in ms) to wait for a 9-bit temperature read


/*
 * Global variables
 */

static onewire_t ow;
static int precision = 1;

/*
 * Returns the temperature as read from the DS18B20
 */
int16_t read_temp()
{
	start_conversion();
	delay_temp_read();
	return get_temp();
}

/*
 * Starts a temperature read
 */
void start_conversion()
{
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0x44); // convert T command
	onewire_line_high(&ow);
}

/*
 * Delays as long as necessary for accurate temperature read
 */
void delay_temp_read()
{
	DELAY_MS( TEMP_READ_DELAY << precision );
}

/*
 * Reads the scratchpad and returns the temperature
 */
int16_t get_temp()
{
	uint8_t buf[9];

	read_scratchpad( buf );
	return buf[1] << 8 | buf[0];
}

/*
 * Sets precision to decimal indicated
 * 3 = 12-bit
 * 2 = 11-bit
 * 1 = 10-bit
 * 0 =  9-bit
 */
void set_precision( int prec )
{
	uint8_t buf[9];

	if( prec >= 0 && prec <= 3 )
	{
		precision = prec;	// Set divider for delay timing
		read_scratchpad( buf );

		buf[4] &= 0x9F;					// Clear bits 6 and 5.
		buf[4] |= (prec << 5) & 0x60;	// Set bits 6 and 5 to prec.


		write_scratchpad( buf );
	}
}

/*
 * Reads the scratchpad memory into buf
 */
void read_scratchpad( uint8_t* buf )
{
	int i;

	if( NULL != buf )
	{
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0xbe); // read scratchpad command
		for (i = 0; i < 9; i++)
			buf[i] = onewire_read_byte(&ow);
	}
}

/*
 * Writes bytes 2, 3, and 4 of buf into the scratchpad
 */
void write_scratchpad( uint8_t* buf )
{
	int i;

	if( NULL != buf )
	{
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0x4E); // write scratchpad command
		for (i = 2; i <= 4; i++)
			onewire_write_byte(&ow, buf[i]);
	}
}
