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
static volatile uint8_t cts = 0;		// 0 if clear to send or stores error code
static volatile uint8_t mstat = 0;		// Modem status from XBee.  2 if associated to AP and can send packets

/*
 * Local function declarations
 */
void print_frame( uint8_t* buf );
void set_IP_addr_mode();
void set_SSID();
void set_encryption_type();
void set_encryption_password();
void set_mask();
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
			if( buf[7] )
			{
				// Error has occurred
				cts = buf[7] + 1;
			}
			else
			{
				cts = 0;
			}

			lcdData('|');
			lcdData(buf[5]);
			lcdData(buf[6]);
			hex2Lcd(buf[7]);
			break;

		case 0x89:			// Tx Status
			// Have received response, can send new frames now
			cts = 0;
			print_frame( (uint8_t*) buf );
			break;

		case 0x8A:			// Modem status
			mstat = buf[4];
		case 0xB0:			// Rx packet
		case 0xFE:			// Frame Error
			print_frame( (uint8_t*) buf );
			break;
		default:			// Don't care about other frames received
			break;
	}

}


/*
 * Sends a packet over UDP to the address 192.168.1.<ip> on port 0x2616
 *
 *
 * Inputs:
 * 		ip		lowest byte of IP address of destination
 * 		buf		pointer to data to send in packet
 * 		len		length of data to send
 */
void xbee_tx_packet( uint8_t ip, uint8_t* buf, uint8_t len )
{
	//						   0   1  2   3    4   5    6   7   8    9     10    11    12  13 14
	uint8_t p[RX_BUF_LEN] = {0x7E, 0, 0, 0x20, 0, 192, 168, 1, 128, 0x26, 0x16, 0x26, 0x16, 0, 0};
	uint8_t i;

	if( buf )
	{
		// Copy data over
		for( i=0; (i<len) && ((i+15)<RX_BUF_LEN); i++ )
		{
			p[i+15] = buf[i];
		}

		// Calculate length of data
		p[2] = len+12;

		while( mstat != 2 );
		while(cts);					// Wait until clear to send
		cts = 1;					// Not clear to send anymore
		send_data(p);				// Send data
		while( cts == 1 );			// Stay here until frame received by XBee so p not popped off stack.
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
						set_mask();
						if( !(ret = cts) )
						{
							set_gateway();
						}
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

	while(cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );			// Stay here until frame received by XBee
}

// Sets Access point SSID
void set_SSID()
{
	uint8_t buf[] = {0x7E, 0, 9, 0x08, 0, 'I', 'D', 'E', 'R', 'R', 'O', 'R', 0}; // SSID is "ERROR"

	while(cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );
}

void set_encryption_type()
{
	uint8_t buf[] = {0x7E, 0, 5, 0x08, 0, 'E', 'E', 2, 0};		// Encryption is WPA2

	while(cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );
}

void set_encryption_password()
{
	// Yes, this is my wi-fi password, come mooch off my Internet if you want....
	uint8_t buf[] = {0x7E, 0, 12, 0x08, 0, 'P', 'K', 't', 'o', 'm', 'a', 'h', 'a', 'w', 'k', 0};

	while(cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );
}

void set_mask()
{
	uint8_t buf[] = MASK_STR;

	buf[0] = 0x7E;
	buf[1] = 0;
	buf[2] = MASK_STR_LEN;
	buf[3] = 0x08;

	while( cts );
	cts = 1;
	send_data( buf );
	while( cts == 1 );
}

void set_IP_address()
{
	uint8_t buf[] = IP_ADDR_STR;

	buf[0] = 0x7E;
	buf[1] = 0;
	buf[2] = IP_ADDR_STR_LEN;
	buf[3] = 0x08;

	while( cts );
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

	while( cts );
	cts = 1;
	send_data( buf );
	while( cts == 1 );
}

// Prints frame to LCD, for debugging purposes
void print_frame( uint8_t* buf )
{
	int i;

	lcdData('|');

	// Length stored in buf[2]
	for( i = 3; i < buf[2]; i++ )
		hex2Lcd( buf[i] );
}
