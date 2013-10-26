/*
 * LCD 20x4
 * Created by Jeff Lawrence for ECE 445 senior design project
 */

#ifndef LCD20x4_H_
#define LCD20x4_H_


#include <msp430g2333.h>
#define  EN BIT5
#define  RS BIT0

void waitlcd(unsigned int x);

void lcdinit(void);
int integerToLcd(int integer );
int dec2ToLcd( int integer );
void lcdData(unsigned char l);
void prints(char *s);
void gotoXy(unsigned char  x,unsigned char y);


#endif /* LCD20x4_H_ */
