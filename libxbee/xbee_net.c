/*
 * xbee_net.c
 *
 *  Created on: Nov 12, 2013
 *      Author: Dan Whisman
 */


#include "xbee_net.h"
#include "lcd20.h"

/*
 * Global Variables
 */
static volatile uint8_t cts = 1;		// 0 if clear to send or stores error code

/*
 * Local function declarations
 */
void print_frame( uint8_t* buf );
void set_IP_addr_mode();
void set_SSID();
void set_encryption_type();
void set_encryption_password();
void set_IP_address();
void set_gateway();

/*
 * Function called to handle received frames from XBee
 */
void frame_rx_handler()
{
	volatile uint8_t* buf = uart_get_frame();		// Get pointer to data just received, length in buf[2].

	switch( buf[3] )		// buf[3] contains API frame identifier
	{
		case 0x88:			// AT command response
		case 0x89:			// Tx Status
			// Have received response, can send new frames now
			cts = 0;
		case 0x8A:			// Modem status
		case 0xB0:			// Rx packet
		case 0xFE:			// Frame Error
			print_frame( (uint8_t*) buf );
			break;
		default:			// Don't care about other frames received
			break;
	}
}

/*
 * Initializes UART and XBee network parameters
 * Returns 0 on success, non-zero on failure
 */
uint8_t xbee_init()
{
	uint8_t ret = 0;

//	uart_init();

	set_IP_addr_mode();

	if( !(ret = cts) )	// Yes, it's supposed to be a single '=', cts is set everytime we transmit data
	{
		set_SSID();
		if( !(ret = cts) )
		{
			set_encryption_type();
			if( !(ret = cts) )
			{
				set_encryption_password();
				if( !(ret = cts) )
				{
					set_IP_address();
					if( !(ret = cts) )
					{
						set_gateway();
					}
				}
			}
		}
	}


	return ret;
}

// Sets IP addressing mode
void set_IP_addr_mode()
{
	uint8_t buf[] = {0x7E, 0x00, 0x05, 0x08, 0, 'M', 'A', 0x01, 0};

	while(!cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );			// Stay here until frame received by XBee
}

// Sets Access point SSID
void set_SSID()
{
	uint8_t buf[] = {0x7E, 0, 9, 0x08, 0, 'I', 'D', 'E', 'R', 'R', 'O', 'R', 0}; // SSID is "ERROR"

	while(!cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );
}

void set_encryption_type()
{
	uint8_t buf[] = {0x7E, 0, 5, 0x08, 0, 'E', 'E', 2, 0};		// Encryption is WPA2

	while(!cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );
}

void set_encryption_password()
{
	// Yes, this is my wi-fi password, come mooch off my Internet if you want....
	uint8_t buf[] = {0x7E, 0, 12, 0x08, 0, 'P', 'K', 't', 'o', 'm', 'a', 'h', 'a', 'w', 'k', 0};

	while(!cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );
}

void set_IP_address()
{
	uint8_t buf[] = IP_ADDR_STR;

	buf[0] = 0x7E;
	buf[1] = 0;
	buf[2] = IP_ADDR_STR_LEN;
	buf[3] = 0x08;

	while( !cts );
	cts = 1;
	send_data( buf );
	while( cts == 1 );
}

void set_gateway()
{
	uint8_t buf[] = GATEWAY_STR;

	buf[0] = 0x7E;
	buf[1] = 0;
	buf[2] = GATEWAY_STR_LEN;
	buf[3] = 0x08;

	while( !cts );
	cts = 1;
	send_data( buf );
	while( cts == 1 );
}

// Prints frame to LCD, for debugging purposes
void print_frame( uint8_t* buf )
{
	int i;

	lcdData('|');
	lcdData('|');

	// Length stored in buf[2]
	for( i = 3; i < buf[2] + 3; i++ )
		hex2Lcd( buf[i] );
}
