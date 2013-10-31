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

/*
 * XBee IPv4 Rx frame has
 * 11 bytes header + data (max 8 bytes) + 1 byte checksum = 20 bytes
 * Leave 1 extra byte for good measure.
 */
#define MAX_BUF_LEN 21	// size of Rx buffer to allocate per frame (in bytes)



/*
 * Global Variables
 */
uint8_t rx_buf[MAX_BUF_LEN] = {0, 0};
uint8_t rx_data_len = 0;				// Length of Rx data in rx_buf
uint8_t have_rx_data = 0;

#endif /* SPI_H_ */
