/* LCD 20x4
 * created by Jeff Lawrence for ECE 445 senior design project
 */

#ifndef LCD20x4_H_
#define LCD20x4_H_


#include <msp430g2333.h>
#define  EN BIT5
#define  RS BIT0

void waitlcd(unsigned int x);

void lcdinit(void);
void integerToLcd(int integer );
void lcdData(unsigned char l);
void prints(char *s);
void gotoXy(unsigned char  x,unsigned char y);
#endif
