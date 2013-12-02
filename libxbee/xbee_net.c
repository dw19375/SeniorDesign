/*
 * xbee_net.c
 *
 *  Created on: Nov 12, 2013
 *      Author: Dan Whisman
 */


#include "xbee_net.h"
//#include "lcd20.h"

/*
 * Global Variables
 */
static volatile uint8_t cts = 0;		// 0 if clear to send or stores error code
static volatile uint8_t mstat = 0;		// Modem status from XBee.  2 if associated to AP and can send packets

/*
 * Local function declarations
 */
//void print_frame( uint8_t* buf );
void frame_rx_handler();
void set_IP_addr_mode();
void set_SSID();
void set_encryption_type();
void set_encryption_password();
void set_mask();
void set_IP_address( char* ip );
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

//			lcdData('|');
//			lcdData(buf[5]);
//			lcdData(buf[6]);
//			hex2Lcd(buf[7]);
			break;

		case 0x89:			// Tx Status
			// Have received response, can send new frames now
			cts = 0;
//			print_frame( (uint8_t*) buf );
			break;

		case 0x8A:			// Modem status
			mstat = buf[4];
		case 0xFE:			// Frame Error
//			print_frame( (uint8_t*) buf );
			break;

		case 0xB0:			// Rx packet
			packet_rx_handler( 0x00 );
			break;

		default:			// Don't care about other frames received
			break;
	}

}


/*
 * Defines a handler for received UDP packets or calls the handler
 *
 * Inputs:
 * 		void (*new_handler)():		If non-NULL, will set a new function to call when
 * 									a packet is received.
 * 									If NULL, will call user defined handler
 */
void packet_rx_handler( void (*new_handler)() )
{
	static void (*handler)() = 0x00;

	if( 0x00 == new_handler )
	{
		/*
		 * Call user defined handler
		 */
		if( handler )
		{
			(*handler)();
		}
	}
	else
	{
		handler = new_handler;
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
		// Put IP address in packet
		p[8] = ip;

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

// Initialize sleep parameters
void sleep_init()
{
	uint8_t buf[] = {0x7E, 0x00, 0x06, 0x08, 0, 'S', 'O', 0x1, 0x40, 0};

	while(cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );			// Stay here until frame received by XBee
}

// Set XBee to sleep
void sleep_set()
{
	uint8_t buf[] = {0x7E, 0x00, 0x05, 0x08, 0, 'S', 'M', 4, 0};

	while(cts);
	cts = 1;
	send_data(buf);
	while( cts == 1 );			// Stay here until frame received by XBee
}

/*
 * Initializes UART and XBee network parameters
 * Returns 0 on success, non-zero on failure
 * ip is string of lowest 8 bits of IP address, represented
 * as a decimal string
 */
uint8_t xbee_init( char* ip )
{
	uint8_t ret = 0;

	// Set the frame receive handler
	frame_recv_handler( frame_rx_handler );

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

#if USE_DHCP == 1
				if( !(ret = cts) )
				{
					set_IP_address( ip );
#if USE_GM == 1
					if( !(ret = cts) )
					{
						set_mask();
						if( !(ret = cts) )
						{
							set_gateway();
						}
					}
#endif /* USE_GM == 1 */
				}
#endif /* USE_DHCP = 1 */
			}
		}
	}


	return ret;
}

// Sets IP addressing mode
void set_IP_addr_mode()
{
	uint8_t buf[] = {0x7E, 0x00, 0x05, 0x08, 0, 'M', 'A', USE_DHCP, 0};

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

// ip is a 3 character string containing lowest 3 digits of IP
// address of node, e.g., ip = "128" for 192.168.1.128
void set_IP_address( char* ip )
{
	uint8_t buf[] = IP_ADDR_STR;

	buf[0] = 0x7E;
	buf[1] = 0;
	buf[2] = IP_ADDR_STR_LEN;
	buf[3] = 0x08;

	if( ip )
	{
		buf[17] = ip[0];
		buf[18] = ip[1];
		buf[19] = ip[2];
	}

	while( cts );
	cts = 1;
	send_data( buf );
	while( cts == 1 );
}

#if USE_GM == 1
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
#endif /* USE_GM == 1 */

// Prints frame to LCD, for debugging purposes
//void print_frame( uint8_t* buf )
//{
//	int i;
//
//	lcdData('|');
//
//	// Length stored in buf[2]
//	for( i = 3; i < buf[2]; i++ )
//		hex2Lcd( buf[i] );
//}
