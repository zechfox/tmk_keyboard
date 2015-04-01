// TODO: Teensy support(ATMega32u4/AT90USB128)
// Fixed for Arduino Duemilanove ATmega168p by Jun Wako
/* UART Example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Version 1.0: Initial Release
// Version 1.1: Add support for Teensy 2.0, minor optimizations


#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"


static volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint8_t tx_buffer_head;
static volatile uint8_t tx_buffer_tail;
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_buffer_head;
static volatile uint8_t rx_buffer_tail;
#if !defined(__AVR_ATmega32__)
// Initialize the UART
void uart_init(uint32_t baud)
{
	cli();
	UBRR0 = (F_CPU / 4 / baud - 1) / 2;
	UCSR0A = (1<<U2X0);
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
	tx_buffer_head = tx_buffer_tail = 0;
	rx_buffer_head = rx_buffer_tail = 0;
	sei();
}

// Transmit a byte
void uart_putchar(uint8_t c)
{
	uint8_t i;

	i = tx_buffer_head + 1;
	if (i >= TX_BUFFER_SIZE) i = 0;
	while (tx_buffer_tail == i) ; // wait until space in buffer
	//cli();
	tx_buffer[i] = c;
	tx_buffer_head = i;
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0) | (1<<UDRIE0);
	//sei();
}

// Receive a byte
uint8_t uart_getchar(void)
{
        uint8_t c, i;

	while (rx_buffer_head == rx_buffer_tail) ; // wait for character
        i = rx_buffer_tail + 1;
        if (i >= RX_BUFFER_SIZE) i = 0;
        c = rx_buffer[i];
        rx_buffer_tail = i;
        return c;
}

// Return the number of bytes waiting in the receive buffer.
// Call this before uart_getchar() to check if it will need
// to wait for a byte to arrive.
uint8_t uart_available(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head >= tail) return head - tail;
	return RX_BUFFER_SIZE + head - tail;
}

// Transmit Interrupt
ISR(USART_UDRE_vect)
{
	uint8_t i;

	if (tx_buffer_head == tx_buffer_tail) {
		// buffer is empty, disable transmit interrupt
		UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	} else {
		i = tx_buffer_tail + 1;
		if (i >= TX_BUFFER_SIZE) i = 0;
		UDR0 = tx_buffer[i];
		tx_buffer_tail = i;
	}
}

// Receive Interrupt
ISR(USART_RX_vect)
{
	uint8_t c, i;

	c = UDR0;
	i = rx_buffer_head + 1;
	if (i >= RX_BUFFER_SIZE) i = 0;
	if (i != rx_buffer_tail) {
		rx_buffer[i] = c;
		rx_buffer_head = i;
	}
}
#else
// Initialize the UART
void uart_init(uint32_t baud)
{
        uint16_t baud_prescale = 0x0000;
        bool u2x_enable = false;
	cli();
#ifndef FIX_BAUD_RATE
        //enable 2X mode to get more accuracy 
        if(baud > 38400)
        {
          u2x_enable = true;
	  baud_prescale = ((F_CPU / (baud * 8UL))) - 1;
        }
        else
	  baud_prescale = ((F_CPU / (baud * 16UL))) - 1;
#else
#warning "using fix baud rate"
        //use WormFood's AVR baud rate calculator: http://www.wormfood.net/avrbaudcalc.php
        //baud rate = 0x000C, 115200@12MHz, must enable U2X to get acceptable error. 
        baud_prescale = 0x000C;  	
        u2x_enable = true;
#endif
	// Set baud rate
	UBRRL = (baud_prescale & 0xff);// Load lower 8-bits into the low byte of the UBRR register
	UBRRH = ((baud_prescale & 0xff00) >> 8); 
	/* Load upper 8-bits into the high byte of the UBRR register
	   Default frame format is 8 data bits, no parity, 1 stop bit
	 to change use UCSRC, see AVR datasheet*/ 
	tx_buffer_head = tx_buffer_tail = 0;
	rx_buffer_head = rx_buffer_tail = 0;
	// Enable receiver and transmitter and receive complete interrupt 
        UCSRA |= (u2x_enable << U2X); 
        UCSRB = ((1<<TXEN)|(1<<RXEN) | (1<<RXCIE));
	UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
	sei();
}

// Transmit a byte
void uart_putchar(uint8_t c)
{
	uint8_t i;

	i = tx_buffer_head + 1;
	if (i >= TX_BUFFER_SIZE) i = 0;
	while (tx_buffer_tail == i) ; // wait until space in buffer
	//cli();
	tx_buffer[i] = c;
	tx_buffer_head = i;
	UCSRB |= (1<<UDRIE);
	//sei();
}

// Receive a byte
uint8_t uart_getchar(void)
{
        uint8_t c, i;

	while (rx_buffer_head == rx_buffer_tail) ; // wait for character
        i = rx_buffer_tail + 1;
        if (i >= RX_BUFFER_SIZE) i = 0;
        c = rx_buffer[i];
        rx_buffer_tail = i;
        return c;
}

// Return the number of bytes waiting in the receive buffer.
// Call this before uart_getchar() to check if it will need
// to wait for a byte to arrive.
uint8_t uart_available(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head >= tail) return head - tail;
	return RX_BUFFER_SIZE + head - tail;
}

#ifdef NO_SUART_PORT
char rcv_deq(void)
{
    char c = 0;
    if (rx_buffer_head != rx_buffer_tail) {
        c = rx_buffer[rx_buffer_tail++];
        rx_buffer_tail %= RX_BUFFER_SIZE;
    }
    return c;
}
void rcv_clear(void)
{
    rx_buffer_tail = rx_buffer_head = 0;
}
char * get_rx_buf_tail(void)
{
    return (char*)(rx_buffer + rx_buffer_tail);
}
char * get_rx_buf(void)
{
    return (char*)rx_buffer;
}
#endif
// Transmit Interrupt
ISR(USART_UDRE_vect)
{
	uint8_t i;

	if (tx_buffer_head == tx_buffer_tail) {
		// buffer is empty, disable transmit interrupt
		UCSRB &= ~(1<<UDRIE);
	} else {
		i = tx_buffer_tail + 1;
		if (i >= TX_BUFFER_SIZE) i = 0;
		UDR = tx_buffer[i];
		tx_buffer_tail = i;
	}
}

// Receive Interrupt
ISR(USART_RXC_vect)
{
	uint8_t c, i;

	c = UDR;
	#ifdef NO_SUART_PORT
	static volatile uint8_t mux_state = 0xff;
    static volatile uint8_t mux_link = 0xff;
	switch (mux_state) {
        case 0xff: // SOF
            if (c == 0xbf)
                mux_state--;
            break;
        case 0xfe: // Link
            mux_state--;
            mux_link = c;
            break;
        case 0xfd: // Flags
            mux_state--;
            break;
        case 0xfc: // Length
            mux_state = c;
            break;
        case 0x00:
            mux_state = 0xff;
            mux_link = 0xff;
            break;
        default:
            if (mux_state--) {
				i = rx_buffer_head + 1;
					if (i >= RX_BUFFER_SIZE) i = 0;
					if (i != rx_buffer_tail) {
						rx_buffer[rx_buffer_head] = c;
						rx_buffer_head = i;
					}

            }
    }
	#else
	i = rx_buffer_head + 1;
	if (i >= RX_BUFFER_SIZE) i = 0;
	if (i != rx_buffer_tail) {
		rx_buffer[rx_buffer_head] = c;
		rx_buffer_head = i;
	}
	#endif
}

#endif

