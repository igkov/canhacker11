#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

void uart_init(uint32_t baudrate);
void uart_tx(void);

#endif
