#include <msp430.h>
#include <stdint.h>
#include "delay.h"
#include "onewire.h"
#include "lcd16.h"

/*
 * main.c
 */
int main(void) {
	onewire_t ow;
	int i;
	uint8_t scratchpad[9];
	int16_t temp = 0;

	WDTCTL = WDTPW + WDTHOLD; //Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	// For LCD
	P1DIR = 0xFF;
	P1OUT = 0x00;

	ow.port_out = &P2OUT;
	ow.port_in = &P2IN;
	ow.port_ren = &P2REN;
	ow.port_dir = &P2DIR;
	ow.pin = BIT3;

	lcdinit();
	prints("Temp: ");
	gotoXy(12,0);
	prints(" deg. C");

	while( 1 )
	{
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0x44); // convert T command
		onewire_line_high(&ow);
		DELAY_MS(800); // at least 750 ms for the default 12-bit resolution
		onewire_reset(&ow);
		onewire_write_byte(&ow, 0xcc); // skip ROM command
		onewire_write_byte(&ow, 0xbe); // read scratchpad command
		for (i = 0; i < 9; i++) scratchpad[i] = onewire_read_byte(&ow);

		temp = scratchpad[1] << 8 | scratchpad[0];

		gotoXy(7,0);
		integerToLcd( temp );
	
//		_BIS_SR(LPM0_bits + GIE);
	}
	return 0;
}
