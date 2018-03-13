#include <LPC11xx.h>
#include "uart.h"
#include "fifo.h"

uint8_t uart_tx_empty;
extern fifo_t fifo_in;
extern fifo_t fifo_out;

void uart_init(uint32_t baudrate) {
	uint32_t Fdiv;

	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);

	LPC_IOCON->PIO1_6 &= ~0x07;     /* UART I/O config */
	LPC_IOCON->PIO1_6 |=  0x01;     /* UART RXD */
	LPC_IOCON->PIO1_7 &= ~0x07;
	LPC_IOCON->PIO1_7 |=  0x01;     /* UART TXD */

	NVIC_DisableIRQ(UART_IRQn);

	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<12); /* Enable UART clock */
	LPC_SYSCON->UARTCLKDIV     = 0x1;     /* divided by 1 */

	LPC_UART->LCR = 0x83;      /* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ((SystemCoreClock/LPC_SYSCON->UARTCLKDIV)/16)/baudrate; /*baud rate */
	LPC_UART->DLM = Fdiv / 256;
	LPC_UART->DLL = Fdiv % 256;
	LPC_UART->LCR = 0x03;      /* DLAB = 0 */
	LPC_UART->FCR = 0x07;      /* Enable and reset TX and RX FIFO. */

	/* Enable the UART Interrupt */
	uart_tx_empty = 1;
	NVIC_EnableIRQ(UART_IRQn);
	LPC_UART->IER = 0x01 | 0x02;
}

void uart_tx(void) {
	uint8_t ch;
	if (uart_tx_empty == 1)	{
		if (fifo_get(&fifo_out, &ch) == 0) {
			uart_tx_empty = 0;
			LPC_UART->THR = ch;
		}
	}
}

void UART_IRQHandler(void) {
	uint8_t ch;
	uint8_t iir;
	iir = (LPC_UART->IIR >> 1) & 0x07;
	if (iir == 0x03) {
		if (LPC_UART->LSR & 0x01) {
			goto uart_new_data;
		}
	}
	else if (iir == 0x02) {
uart_new_data:
		ch = LPC_UART->RBR;
		fifo_put(&fifo_in, ch);
	}
	else if (iir == 0x01) {
		if (LPC_UART->LSR & 0x20) {
			if (fifo_get(&fifo_out, &ch) == 0) {
				LPC_UART->THR = ch;
			} else {
				uart_tx_empty = 1;
			}
		}
	}
	return;
}
