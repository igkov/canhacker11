#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comn.h"

/*
	0 - Успешно
	1 - Ошибка открытия порта
	2 - Прочая ошибка (ошибка настройки)
 */
int com_init(pcom_struct_t com, int com_number, DWORD baudrate) {
	DCB    dcb;
	COMMTIMEOUTS CommTimeouts;
	char acComName[20];
	int iOldBaud;
	memset(com, 0x00, sizeof(com_struct_t));
		com->com_number = com_number;
	com->baudrate = baudrate;
	// Создаем имя устройства для открытия и открываем его:
	sprintf(acComName, "\\\\.\\COM%d", com_number);
	com->hcom = CreateFile(acComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	// Проверяем успешность
	if(com->hcom == INVALID_HANDLE_VALUE)
		return 1;
	// Получение статуса COM-порта и настройка параметров соединения.
	GetCommState(com->hcom, &dcb);
	iOldBaud = dcb.BaudRate;
	dcb.BaudRate    = com->baudrate;
	dcb.ByteSize    = 8;
	dcb.StopBits    = ONESTOPBIT;
	dcb.Parity      = NOPARITY;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fOutX       = FALSE;
	dcb.fInX        = FALSE;
	dcb.fNull       = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	// Установка новых параметров соединения:
	if (iOldBaud != baudrate)
		if(SetCommState(com->hcom, &dcb) == 0)
			return 2;
	//
	if (SetCommMask(com->hcom, EV_RXCHAR | EV_TXEMPTY) == 0)
		return 3;
	// Настройка таймаутов чтения/записи:
	CommTimeouts.ReadIntervalTimeout         =    5;
	CommTimeouts.ReadTotalTimeoutMultiplier  =    0;
	CommTimeouts.ReadTotalTimeoutConstant    =    200;
	CommTimeouts.WriteTotalTimeoutMultiplier =    0;
	CommTimeouts.WriteTotalTimeoutConstant   =    0;
	SetCommTimeouts(com->hcom, &CommTimeouts);
	// Очищаем буфера COM:
	if (PurgeComm(com->hcom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR) == 0)
		return 4;
	return 0;
}

int com_settimeout(pcom_struct_t com, int n, int m) {
	COMMTIMEOUTS CommTimeouts;
	// Настройка таймаутов чтения/записи:
	CommTimeouts.ReadIntervalTimeout         =    1;
	CommTimeouts.ReadTotalTimeoutMultiplier  =    n;
	CommTimeouts.ReadTotalTimeoutConstant    =    m;
	CommTimeouts.WriteTotalTimeoutMultiplier =    0;
	CommTimeouts.WriteTotalTimeoutConstant   =    0;
	SetCommTimeouts(com->hcom, &CommTimeouts);
	return 0;
}

int com_deinit(pcom_struct_t com) {
	if (CloseHandle(com->hcom) == 0)
		return 1;
	return 0;
}

int com_putchar(pcom_struct_t com, char c) {
	unsigned long realsize;
	if (WriteFile(com->hcom, &c, 1, &realsize, NULL) == 0)
		return 1;
	return 0;
}

int com_putblock(pcom_struct_t com, char *buff, int len) {
	unsigned long realsize;
	unsigned long offset = 0;
	if (WriteFile(com->hcom, buff, len, &realsize, NULL) == 0)
		return 1;
	if (realsize != len)
		return 2;
	return 0;
}

int com_putstr(pcom_struct_t com, char *str) {
	return com_putblock(com, str, strlen(str));
}

int com_control(pcom_struct_t com, unsigned char line) {
	int ret;
	if(line & DTR_LINE)
		ret = EscapeCommFunction(com->hcom, SETDTR);
	else
		ret = EscapeCommFunction(com->hcom, CLRDTR);
	if(line & RTS_LINE)
		ret = EscapeCommFunction(com->hcom, SETRTS);
	else
		ret = EscapeCommFunction(com->hcom, CLRRTS);
	return ret;
}

int com_getblock(pcom_struct_t com, char *buff, int maxlen, unsigned long *realsize) {
	unsigned long offset = 0, i;
	unsigned long timeout = 5;
	while (timeout) {
		ReadFile(com->hcom, &buff[offset], maxlen, realsize, NULL);
		for (i = 0; i < *realsize; i++) {
			if (buff[offset + i] == '\r') {
				return 0;
			}
		}
		offset += *realsize;
		timeout--;
	}
	if (timeout)
		return 1;
	else
		return 0;
}

int com_getblock_simple(pcom_struct_t com, char *buff, int maxlen, unsigned long *realsize) {
	ReadFile(com->hcom, buff, maxlen, realsize, NULL);
	return 0;
}

int com_getchar(pcom_struct_t com, char *ch) {
	unsigned long realsize;
	return com_getblock(com, ch, 1, &realsize);
}
