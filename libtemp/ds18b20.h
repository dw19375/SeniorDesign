/*
 * ds18b20.h
 *
 * Library to configure and read the DS18B20 temperature sensor.
 * This requires lib1wire to communicate with the DS18B20.
 * Currently, this only works for one device on the 1-wire bus.
 *
 *  Created on: Oct 26, 2013
 *      Author: Dan Whisman
 */

#ifndef DS18B20_H_
#define DS18B20_H_

/*
 * Includes needed for function declarations
 */
#include <msp430g2333.h>
#include <stdint.h>

/*
 * Global function definitions
 */

int16_t read_temp();
void start_conversion();
void delay_temp_read();
int16_t get_temp();
void set_precision( int prec );
void read_scratchpad( uint8_t* buf );
void write_scratchpad( uint8_t* buf );


#endif /* DS18B20_H_ */
