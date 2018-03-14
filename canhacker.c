/*
	CANHACKER port
	
	Портирование проекта CANHACKER на платформу LPC11Cxx.
	
	igorkov / fsp@igorkov.org / 2016

	Site: igorkov.org/bcomp11
 */
#include <LPC11xx.h>
#include "type.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "can.h"
#include "timer.h"
#include "leds.h"
#include "uart.h"
#include "fifo.h"
#include "mbus.h"

void ProtectDelay(void) {
	int n;
	for (n = 0; n < 100000; n++) { __NOP(); } 
}

fifo_t fifo_in;
uint8_t data_in[256] __align(4);
fifo_t fifo_out;
uint8_t data_out[1024] __align(4);
int interface_state = 0;
int interface_state_mbus = 0;

uint8_t h2b(uint8_t _ascii) {
	if((_ascii >= '0') && (_ascii <= '9')) return(_ascii - '0'); 
	if((_ascii >= 'a') && (_ascii <= 'f')) return(_ascii - 'a' + 10); 
	if((_ascii >= 'A') && (_ascii <= 'F')) return(_ascii - 'A' + 10); 
	return 0xFF;
}

int h2v(char *str) {
	uint8_t value;
	uint8_t ret;
	value = h2b(str[0]);
	if (value == 0xFF) {
		return -1;
	}
	ret = value<<4;
	value = h2b(str[1]);
	if (value == 0xFF) {
		return -1;
	}
	ret += value;
	return ret;
}

uint8_t b2h(uint8_t _halfbyte) {
	_halfbyte &= 0x0F;
	if(_halfbyte >= 10)
		return('A' + _halfbyte - 10); 
	else
		return('0' + _halfbyte); 
}

void can_loopback(CAN_msg *canData) {
	uint32_t ms = timer0_get()/1000;
	uint32_t time = ms % 60000;
	int offset = 0;
	int i;
	uint8_t buf[64];
	if (canData->format == STANDARD_FORMAT) {
		if (canData->type == DATA_FRAME)
			buf[offset++] = 't';
		else
			buf[offset++] = 'r';
		buf[offset++] = b2h((canData->id)>>8);
		buf[offset++] = b2h((canData->id)>>4);
		buf[offset++] = b2h((canData->id));
	} else {
		if (canData->type == DATA_FRAME)
			buf[offset++] = 'T';
		else
			buf[offset++] = 'R';
		buf[offset++] = b2h((canData->id)>>28);
		buf[offset++] = b2h((canData->id)>>24);
		buf[offset++] = b2h((canData->id)>>20);
		buf[offset++] = b2h((canData->id)>>16);
		buf[offset++] = b2h((canData->id)>>12);
		buf[offset++] = b2h((canData->id)>>8);
		buf[offset++] = b2h((canData->id)>>4);
		buf[offset++] = b2h((canData->id));
	}
	buf[offset++] = b2h((canData->len));
	for (i=0;i<canData->len;i++) {
		buf[offset++] = b2h((canData->data[i])>>4);
		buf[offset++] = b2h((canData->data[i]));
	}
	buf[offset++] = b2h((time)>>12);
	buf[offset++] = b2h((time)>>8);
	buf[offset++] = b2h((time)>>4);
	buf[offset++] = b2h((time)>>0);
	buf[offset++] = '\r';
	buf[offset++] = 0;
	if(interface_state > 0) { 
		// Отправляем данные:
		fifo_puts(&fifo_out, buf);
		// Включаем светодиод:
		led_green(1);
	}
}

#define BEL 0x07

int main (void) {
	int i;
	uint8_t cmd[128];
	uint32_t speed = 0;
	// Инициализация таймера:
	timer0_init();
	//  Инициализируем очереди:
	fifo_init(&fifo_in, data_in, sizeof(data_in));
	fifo_init(&fifo_out, data_out, sizeof(data_out));
	// Инициализируем вспомогательную периферию:
	leds_init();                              // LEDs initial
	uart_init(115200);                        // UART initial
	// Основной цикл:
	while (1) {
		uint8_t ch;
		uint8_t offset = 0;
		uint32_t tmp;
		// receive + send
		while (offset < sizeof(cmd)-1) {
			if (fifo_get(&fifo_in, &ch) == 0) {
				if (ch == '\r')	{
					break;
				}
				if (ch != '\n') {
					cmd[offset] = ch;
					offset++;
				}
			}
			// Send data:
			uart_tx();
			// Sleep:
			__WFI();
		}
		// Нет данных, нечего обрабатывать:
		if (offset == 0)
			continue;
		led_red(1);
		// Обработка команд:
		switch(cmd[0]) {
		case 'V': // V[CR]
			if (offset != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			fifo_puts(&fifo_out, "V0402\r");
			break;
		case 'v': // v[CR]
			if (offset != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			fifo_puts(&fifo_out, "vLPC11C\r");
			break;
		 case 'N': // N[CR]
			if (offset != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			fifo_puts(&fifo_out, "N1234\r");
			break;
		case 'O': // O[CR]
			if (offset != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
		 	if (speed == 0) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			// Инциализируем CAN:
			if (CAN_setup(speed)) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			//CAN_wrFilter(0x21, STANDARD_FORMAT);           /* Enable reception of messages */
			CAN_noFilter(STANDARD_FORMAT);                 /* All receive */
			// Кладем "возврат":
			fifo_put(&fifo_out, '\r');
			// Заканчиваем инициализацию:
			CAN_start();
			CAN_waitReady();
		   	interface_state	= 1;
			led_green(1);
			break;
		case 'L': // L[CR]
			if (offset != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			if (speed == 0) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			// Инциализируем CAN:
			if (CAN_setup(speed)) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			//CAN_wrFilter(0x21, STANDARD_FORMAT);           /* Enable reception of messages */
			CAN_noFilter(STANDARD_FORMAT);                 /* All receive */
			// Кладем "возврат":
			fifo_put(&fifo_out, '\r');
			// Заканчиваем инициалиацию:
			CAN_testmode();
			CAN_waitReady();
			interface_state	= 2;
			led_green(1);
			break;
		case 'C': // C[CR]
			if (offset != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			if (interface_state) {
				CAN_stop();
			}
			interface_state	= 0;
			led_green(0);
			fifo_put(&fifo_out, '\r');
			break;
		case 'T': // TiiiiiiiiLDDDDDDDDDDDDDDDD[CR]
		case 't': // tiiiLDDDDDDDDDDDDDDDD[CR]
		case 'R': // RiiiiiiiiL[CR]
		case 'r': // riiiL[CR]
			// Только в NORMAL-режиме отправляем данные:
			if (interface_state != 1) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			// Первая проверка длины:
			if ((cmd[0] == 'T' && offset < 10) ||
				(cmd[0] == 't' && offset < 5) ||
				(cmd[0] == 'R' && offset != 10) ||
				(cmd[0] == 'r' && offset != 5)) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			i = 1;
			if (cmd[0] == 't' || cmd[0] == 'r') {
				tmp = h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
			} else {
				tmp = h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
				tmp = (tmp << 4) + h2b(cmd[i++]);
			}
			CAN_TxMsg.id = tmp;
			CAN_TxMsg.len = h2b(cmd[i++]);
			if (CAN_TxMsg.len > 8) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			// Вторая (точная) проверка длины:
			if ((cmd[0] == 'T' && offset != 10+CAN_TxMsg.len*2) ||
				(cmd[0] == 't' && offset !=  5+CAN_TxMsg.len*2)) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			if (cmd[0] == 't' || cmd[0] == 'T') {
				int b;
				for (b=0;b<CAN_TxMsg.len;b++) {
					tmp = h2b(cmd[i++]); 
					tmp = (tmp << 4) + h2b(cmd[i++]);
					CAN_TxMsg.data[b] = tmp;
				}
			}
			// Send:
			if (cmd[0] == 't' || cmd[0] == 'r') {
				CAN_TxMsg.format = STANDARD_FORMAT;
			} else {
				CAN_TxMsg.format = EXTENDED_FORMAT;
			}
			CAN_TxMsg.type = DATA_FRAME;
			CAN_wrMsg(&CAN_TxMsg);
			fifo_puts(&fifo_out, "\r");
			break;
		case 'S': // Sn[CR]
			if (offset != 2) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			switch(cmd[1]) {
			case '0': // CAN = 10Kbit/s
				speed = 10000;
				break;
			case '1': // CAN = 20Kbit/s
				speed = 20000;
				break;
			case '2': // CAN = 50Kbit/s
				speed = 50000;
				break;
			case '3': // CAN = 100Kbit/s
				speed = 100000;
				break;
			case '4': // CAN = 125Kbit/s
				speed = 125000;
				break;
			case '5': // CAN = 250Kbit/s
				speed = 250000;
				break;
			case '6': // CAN = 500Kbit/s
				speed = 500000;
				break;
			case '7': // CAN = 800Kbit/s
				speed = 800000;
				break;
			case '8': // CAN = 1000Kbit/s
				speed = 1000000;
				break;
			case '9': // CAN = 95.238Kbit/s
				speed = 95238;
				break;
			case 'a': // CAN = 83.338Kbit/s
				speed = 83333;
				break;
			case 'b': // CAN = 47.619Kbit/s
				speed = 47619;
				break;
			case 'c': // CAN = 33.333Kbit/s
				speed = 33333;
				break;
			case 'd': // CAN = 5Kbit/s
				speed = 5000;
				break;
			default:
				fifo_put(&fifo_out, BEL);
				break;
			}
			fifo_put(&fifo_out, '\r');
			break;
		// non can-hacker commands 'B', 'I', etc.
		case 'I': // Iab[cr]
			// Основная функция сниффера,
			// Кроме инициализации процедуры ничего не требуется.
			if (offset < 2) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			if (cmd[1] == '1') {
				mbus_init(1);
				interface_state_mbus = 1;
			} else
			if (cmd[1] == '0') {
				mbus_init(0);
				interface_state_mbus = 2;
			} else {
				// no init mbus
			}
			if (cmd[2] == '1') {
				mbus_init_DEV();
				interface_state_mbus = 3;
			}
			fifo_put(&fifo_out, '\r');
			break;
		case 'X': //Xaabbccddee...ff[CR]
			if (interface_state_mbus == 0) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			for (i = 1; i < offset; i += 2) {
				int val = h2v((char*)&cmd[i]);
				if (val == -1) {
					break;
				}
				cmd[i/2] = val;
			}
			offset = i/2;
			// Отправка данных:
			if (offset == 0) {
				mbus_init_DEV();
			} else {
				mbus_msend(cmd, offset);
			}
			// answer
			fifo_put(&fifo_out, '\r');
			break;
		case 'x': //xaabbccddee...ff[CR]
			if (interface_state_mbus == 0) {
				fifo_put(&fifo_out, BEL);
				break;
			}
			for (i = 1; i < offset; i += 2) {
				int val = h2v((char*)&cmd[i]);
				if (val == -1) {
					break;
				}
				cmd[i/2] = val;
			}
			offset = i/2;
			// Отправка данных:
			mbus_msend_slave(cmd, offset);
			// answer
			fifo_put(&fifo_out, '\r');
			break;

		// unsupport:
		case 's': //sxxyy[CR]
		case 'W': //Wrrdd[CR]
		case 'Z': //Z[CR]
		case 'A': //A[CR]
		case 'E': //E[CR]
		case 'F': //F[CR]
		case 'G': //G[CR]
		case 'M': //M[CR]
		case 'm': //m[CR]
		// other:
		default:
			//fifo_puts(&fifo_out, "\r");
			fifo_put(&fifo_out, BEL); // BEL
			break;
		}
		// Гасим светодиод:
		led_red(0);
	}
}
