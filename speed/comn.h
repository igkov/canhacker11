#ifndef __COMN_H__
#define __COMN_H__

#include <windows.h>

typedef struct {
	HANDLE hcom;
	DWORD baudrate;
	char com_number;
} com_struct_t, *pcom_struct_t;

#define DTR_LINE 1
#define RTS_LINE 2

int com_init(pcom_struct_t com, int com_number, DWORD baudrate);
int com_deinit(pcom_struct_t com);
int com_putchar(pcom_struct_t com, char c);
int com_putblock(pcom_struct_t com, char *buff, int len);
int com_putstr(pcom_struct_t com, char *str);
int com_control(pcom_struct_t com, unsigned char line);
int com_getblock(pcom_struct_t com, char *buff, int maxlen, unsigned long *realsize);
int com_getblock_simple(pcom_struct_t com, char *buff, int maxlen, unsigned long *realsize);
int com_getchar(pcom_struct_t com, char *ch);
int com_settimeout(pcom_struct_t com, int n, int m);

#endif //__COMN_H__
