/*
	SPEED.exe
	
	ћинимальное приложение дл€ автоматического определени€ скорости CAN-шины.
	
	igorkov / fsp@igorkov.org / 2018
 */
#include <stdio.h>
#include <windows.h>
#include "comn.h"

FILE *f;
com_struct_t com;
int term = 0;

BOOL CtrlHandler( DWORD type ) {
	switch( type ) {
	case CTRL_C_EVENT:
		printf("\nCTRL-C detected, BREAK!\r\n");
		term = 1;
		return TRUE;
	default:
		return FALSE; 
	} 
}

void help(void) {
	printf("comlog.exe PORT\r\n");
	printf("  PORT   - Decimal COM-port number\r\n");
	return;
}

typedef struct {
	char c;
	char res1;
	char res2;
	char res3;
	char *name;
} SPEED_T;

SPEED_T list[] = {
	//{ '0', 0, 0, 0, "10Kbit/s", },
	{ '1', 0, 0, 0, "20Kbit/s", },
	{ '2', 0, 0, 0, "50Kbit/s", },
	{ '3', 0, 0, 0, "100Kbit/s", },
	{ '4', 0, 0, 0, "125Kbit/s", },
	{ '5', 0, 0, 0, "250Kbit/s", },
	{ '6', 0, 0, 0, "500Kbit/s", },
	//{ '7', 0, 0, 0, "800Kbit/s", },
	{ '8', 0, 0, 0, "1Mbit/s", },
	{ '9', 0, 0, 0, "95.238Kbit/s", },
	{ 'a', 0, 0, 0, "83.333Kbit/s", },
	{ 'b', 0, 0, 0, "47.619Kbit/s", },
	{ 'c', 0, 0, 0, "33.333Kbit/s", },
	//{ 'd', 0, 0, 0, "5Kbit/s", },
	{ 0, 0, 0, 0, (void*)0, },
};


int main(int argc, char **argv) {
	unsigned char data[2048];
	unsigned long size = 0;
	unsigned long count = 0;
	int ret;
	int i;
	int speed = '6';
	int port = 7;
	char name[20];
	char ch;
	// Parameters:
	if (argc == 1) {
		help();
		return 0;
	}
	if (argc >= 2) {
		port = atoi(argv[1]);
	}
	// Open port:
	if (ret = com_init(&com, port, 115200)) {
		printf("ERROR: com_init() return %d!\r\n", ret);
		return __LINE__;
	}
	// ѕравильна€ инициализаци€ линий RTS/DTR:
	com_control(&com, DTR_LINE|RTS_LINE);
	Sleep(100);
	com_control(&com, DTR_LINE);
	Sleep(100);
	com_control(&com, 0);
	Sleep(500);
	// ѕорт открыт и проинициализирован:
	printf("Com port #%d opened!\r\n", port);

	// Info (hw):
	ret = com_putstr(&com, "v\r");
	if (ret) {
		printf("ERROR: com_putstr(\"v\"):%d error %d\r\n", __LINE__, ret);
		ret = __LINE__;
		goto ret_point;
	}
	memset(data, 0, sizeof(data));
	ret = com_getblock(&com, data, sizeof(data), &size);
	if (ret) {
		printf("ERROR: com_getblock():%d error %d\r\n", __LINE__, ret);
		ret = __LINE__;
		goto ret_point;
	}
	if (data[0] != 'v') {
		printf("ERROR: command \"v\" return error %d!\r\n", ret);
		ret = __LINE__;
		goto ret_point;
	}
	while (data[size] != '\r' && size > 1) size--;
	data[size] = 0x00;
	printf("Device Name:    %s\r\n", &data[1]);
	// Info (version):
	ret = com_putstr(&com, "V\r");
	if (ret) {
		printf("ERROR: com_putstr(\"V\"):%d error %d\r\n", __LINE__, ret);
		ret = __LINE__;
		goto ret_point;
	}
	memset(data, 0, sizeof(data));
	ret = com_getblock(&com, data, sizeof(data), &size);
	if (ret) {
		printf("ERROR: com_getblock():%d error %d\r\n", __LINE__, ret);
		ret = __LINE__;
		goto ret_point;
	}
	if (data[0] != 'V') {
		printf("ERROR: command \"V\" return error %d!\r\n", ch);
		ret = __LINE__;
		goto ret_point;
	}
	while (data[size] != '\r' && size > 1) size--;
	data[size] = 0x00;
	printf("Device Version: %s\r\n", &data[1]);
	// Info (serial):
	ret = com_putstr(&com, "N\r");
	if (ret) {
		printf("ERROR: com_putstr(\"N\"):%d error %d\r\n", __LINE__, ret);
		ret = __LINE__;
		goto ret_point;
	}
	memset(data, 0, sizeof(data));
	ret = com_getblock(&com, data, sizeof(data), &size);
	if (ret) {
		printf("ERROR: com_getblock():%d error %d\r\n", __LINE__, ret);
		ret = __LINE__;
		goto ret_point;
	}
	if (data[0] != 'N') {
		printf("ERROR: command \"N\" return error %d!\r\n", ch);
		ret = __LINE__;
		goto ret_point;
	}
	while (data[size] != '\r' && size > 1) size--;
	data[size] = 0x00;
	printf("Device S/N:     %s\r\n", &data[1]);
	
	speed = 0;
	while (list[speed].c) {
		unsigned long realsize;
	
		// Reset
		ret = com_putstr(&com, "C\r");
		if (ret) {
			printf("ERROR: com_putstr(\"C\"):%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_point;
		}
		ret = com_getchar(&com, &ch);
		if (ret) {
			printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_point;
		}
		if (ch != '\r') {
			printf("ERROR: command \"C\" return error %d!\r\n", ch);
			ret = __LINE__;
			goto ret_point;
		}
		
		// Set CAN speed:
		sprintf(data, "S%c\r", list[speed].c);
		ret = com_putstr(&com, data);
		if (ret) {
			printf("ERROR: com_putstr(\"S%c\"):%d error %d\r\n", speed, __LINE__, ret);
			ret = __LINE__;
			goto ret_point;
		}
		ret = com_getchar(&com, &ch);
		if (ret) {
			printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_point;
		}
		if (ch != '\r') {
			printf("ERROR: command \"S%c\" return error %d!\r\n", speed, ch);
			ret = __LINE__;
			goto ret_point;
		}
		
		// Goto LISTEN mode (receive):
		ret = com_putstr(&com, "L\r"); // start recv
		if (ret) {
			printf("ERROR: com_putstr(\"O\"):%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_point;
		}
		ret = com_getchar(&com, &ch);
		if (ret) {
			printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_point;
		}
		if (ch != '\r') {
			printf("ERROR: command \"O\" return error %d!\r\n", ch);
			ret = __LINE__;
			goto ret_point;
		}

		com_getblock_simple(&com, data, sizeof(data), &realsize);
		if (realsize > 16) {
			break;
		}
		
		printf(".");
		Sleep(100);
		speed++;
	}
	
	if (list[speed].c == 0) {
		goto ret_uninit;
	}
	
	printf("\r\n");
	printf("CAN speed detected: '%c' or %s!\r\n", list[speed].c, list[speed].name);
	
	// Exit:
ret_ok:
	ret = 0;
ret_uninit:
	ret = com_putstr(&com, "C\r"); // Reset
	if (ret) {
		printf("ERROR: com_putstr(\"C\"):%d error %d\r\n", __LINE__, ret);
	}
ret_point:
	fclose(f);
	com_deinit(&com);
	return ret;
}
