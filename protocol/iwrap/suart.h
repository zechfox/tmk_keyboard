#ifndef SUART
#define SUART
#ifdef NO_SUART_PORT
#define xmit(value) uart_putchar(value)
#else
void xmit(uint8_t);
#endif
uint8_t rcvr(void);
uint8_t recv(void);

#endif	/* SUART */
