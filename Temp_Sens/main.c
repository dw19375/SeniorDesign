#include <msp430g2333.h>
#include <stdint.h>
#include "delay.h"
#include "onewire.h"
#include "ds18b20.h"
#include "lcd20.h"

/*
 * main.c
 */
int main(void) {
	onewire_t ow;
	int i, xpos = 0, begin_xpos;
	uint8_t scratchpad[9];
	int16_t temp = 0;

	WDTCTL = WDTPW + WDTHOLD; //Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	ow.port_out = &P2OUT;
	ow.port_in = &P2IN;
	ow.port_ren = &P2REN;
	ow.port_dir = &P2DIR;
	ow.pin = BIT4;

	// Initialize temp. sensor to 10-bit resolution
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0xbe); // read scratchpad command
	for (i = 0; i < 9; i++)
		scratchpad[i] = onewire_read_byte(&ow);

	scratchpad[4] = ((scratchpad[4])&0x9F) | 0x20;		// Set to x01x xxxx for 10-bit resolution

	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0x4E); // write scratchpad command
	for (i = 2; i <= 4; i++)
		onewire_write_byte(&ow, scratchpad[i]);


	lcdinit();
	prints("Temp: ");
	xpos += 6;
//	gotoXy(12,0);
//	prints(" deg. C");

	begin_xpos = xpos;
	while( 1 )
	{
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0x44); // convert T command
		onewire_line_high(&ow);
		DELAY_MS(200); // at least 187.5 ms for the 10-bit resolution
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0xbe); // read scratchpad command
		for (i = 0; i < 9; i++) scratchpad[i] = onewire_read_byte(&ow);

		temp = scratchpad[1] << 8 | scratchpad[0];

		gotoXy(xpos,0);
		xpos += integerToLcd( temp >> 4 );	// Divide by 16 to display integer part
		xpos += dec2ToLcd( temp >> 2 );		// Want fractional part in lowest two bits

		gotoXy(xpos,0);
		prints(" deg. C");

		xpos = begin_xpos;
	
//		_BIS_SR(LPM0_bits + GIE);
	}
	return 0;
}
