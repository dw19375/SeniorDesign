/*
 * spi.h
 *
 *  Created on: Oct 30, 2013
 *      Author: dan
 */

#ifndef SPI_H_
#define SPI_H_

#include <msp430g2333.h>
#include <stdint.h>

/*
 * Defines
 */
#ifndef IPADDR
#define IPADDR C0A80180	// 192.168.1.128
#endif

#define MAX_BUF_LEN 18	// size of Rx buffer to allocate per frame (in bytes)


#endif /* SPI_H_ */
