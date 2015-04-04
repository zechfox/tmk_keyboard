#ifndef _uart_included_h_
#define _uart_included_h_

#include <stdint.h>
#include <stdbool.h>
// These buffers may be any size from 2 to 256 bytes.
#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 40
void uart_init(uint32_t baud);
void uart_putchar(uint8_t c);
uint8_t uart_getchar(void);
uint8_t uart_available(void);
#ifdef NO_SUART_PORT
char rcv_deq(void);
void rcv_clear(void);
char * get_rx_buf_tail(void);
char * get_rx_buf(void);
#endif
#endif
