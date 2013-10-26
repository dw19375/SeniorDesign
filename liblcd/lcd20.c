/*
 * 20x4 HD44780 interface control code
 * created by Jeff Lawrence for ECE 445 senior design project
 */
#include "lcd20.h"

void lcdcmd(unsigned char Data) {
	unsigned char Bit3_hold = 0x00;
	unsigned char Bit210_hold = 0x00;

	// data lines set for 2.1, 2.2, 2.3, 2.5
	// enable = 1.5, rs= 1.0

	// set enable and RS
	P1OUT &= ~RS; // ~RS = ~BIT0 = 0xFE
	P1OUT &= ~EN; // ~EN = ~BIT5 = 0xBF

	// send the upper bits
	P2OUT &= 0xD1; 						// clear data lines 0xD1
	Bit3_hold = (Data >> 2) & 0x20; 	// get upper bit 3 into the temp
	Bit210_hold = (Data >> 3) & 0x0E; 	// get upper bits 2,1,0 into the temp
	P2OUT |= (Bit3_hold + Bit210_hold); // combine the temps into the output
	P1OUT |= EN;						// set enable
	waitlcd(2);
	P1OUT &= ~EN;

	// send the lower bits
	P2OUT &= 0xD1;
	// clear data lines 0xD1
	Bit3_hold = (Data << 2) & 0x20;		// get the lower bit 3 into the temp
	Bit210_hold = (Data << 1) & 0x0E;// get the lower bits 2,1,0 into the temp
	P2OUT |= (Bit3_hold + Bit210_hold); // combine temps into output
	P1OUT |= EN;
	waitlcd(2);
	P1OUT &= ~EN;

}
void lcdData(unsigned char l) {
	unsigned char Bit3_hold = 0x00;
	unsigned char Bit210_hold = 00;
	P1OUT |= RS;
	P1OUT &= ~EN;

	// send the upper bits
	P2OUT &= 0xD1; 						// clear data lines 0xD1
	Bit3_hold = (l >> 2) & 0x20; 			// get upper bit 3 into the temp
	Bit210_hold = (l >> 3) & 0x0E; 		// get upper bits 2,1,0 into the temp
	P2OUT |= (Bit3_hold + Bit210_hold); 	// combine the temps into the output
	P1OUT |= EN;							// set enable
	waitlcd(2);
	P1OUT &= ~EN;

	// send the lower bits
	P2OUT &= 0xD1;
	// clear data lines 0xD1
	Bit3_hold = (l << 2) & 0x20;			// get the lower bit 3 into the temp
	Bit210_hold = (l << 1) & 0x0E;	// get the lower bits 2,1,0 into the temp
	P2OUT |= (Bit3_hold + Bit210_hold); 	// combine temps into output
	P1OUT |= EN;
	waitlcd(2);
	P1OUT &= ~EN;

}

void lcdinit(void) {

	// LCD pin directions
	P1DIR |= 0x21;
	P1OUT &= ~0x21;
	P2DIR |= 0x2E;
	P2OUT &= ~0x2E;

	P1OUT &= ~RS;
	P1OUT &= ~EN;
	P2OUT |= 0x06;
	waitlcd(40);
	P1OUT |= EN;
	P1OUT &= ~EN;
	waitlcd(5);
	P1OUT |= EN;
	P1OUT &= ~EN;
	waitlcd(5);
	P1OUT |= EN;
	P1OUT &= ~EN;
	waitlcd(2);

	P2OUT &= 0xF4;
	P1OUT |= EN;
	P1OUT &= ~EN;
	lcdcmd(0x28);   //set data length 4 bit 2 line
	waitlcd(250);
	lcdcmd(0x0E);   // set display on cursor on blink on
	waitlcd(250);
	lcdcmd(0x01); // clear lcd
	waitlcd(250);
	lcdcmd(0x06);  // cursor shift direction
	waitlcd(250);
	lcdcmd(0x80);  //set ram address
	waitlcd(250);

}

void waitlcd(volatile unsigned int x) {
	volatile unsigned int i;
	for (x; x > 1; x--) {
		for (i = 0; i <= 110; i++)
			;
	}
}

void prints(char *s) {

	while (*s) {
		lcdData(*s);
		s++;
	}
}

void gotoXy(unsigned char x, unsigned char y) {
	if (x < 40) {
		if (y)
			x |= 0x40;
		x |= 0x80;
		lcdcmd(x);
	}

}

int integerToLcd( int integer )
{
	unsigned char thousands,hundreds,tens,ones, ret=1;

	// Handle negative numbers
	if( integer < 0 )
	{
		lcdData('-');
		integer *= -1;
		ret++;
	}

	// Limit to four digits
	if( integer > 9999 )
		integer = 9999;

	thousands = integer / 1000;

	if( thousands )
	{
		lcdData(thousands + 0x30);
		ret++;
	}

	hundreds = ((integer - thousands*1000)-1) / 100;

	if( hundreds )
	{
		lcdData( hundreds + 0x30);
		ret++;
	}

	tens=(integer%100)/10;

	if( tens )
	{
		lcdData( tens + 0x30);
		ret++;
	}

	// Always print ones digit.
	ones=integer%10;

	lcdData( ones + 0x30);

	return ret;
}

/*
 * Displays fractional part of fixed point integer as decimal
 * The fractional part must be in the lowest 2 bits of the word passed in
 * Will print up to two digits (three including the dot)
 *
 * returns number of characters printed
 */
int dec2ToLcd( int integer )
{
	// Always display the decimal point
	lcdData('.');

	// Make sure integer is positive
	if( integer < 0 )
		integer *= -1;

	integer &= 0x03;		// Clear all but lowest two bits

	switch( integer )
	{
		case 0x00:		// 0.00
			lcdData('0');
			lcdData('0');
			break;
		case 0x01:		// 0.25
			lcdData('2');
			lcdData('5');
			break;
		case 0x02:		// 0.5
			lcdData('5');
			lcdData('0');
			break;
		case 0x03:		// 0.75
			lcdData('7');
			lcdData('5');
			break;
	}

	return 3;
}
