/*
	COMLOG.exe
	
	ћинимальное приложение дл€ сохранени€ данных из устройства CANBOX.
	ќткрывает указанный COM-порт, инициализирует устройство и записывает 
	все принимаемые данные из него в файл.
	
	igorkov / fsp@igorkov.org / 2016
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
	printf("comlog.exe PORT SPEED\r\n");
	printf("  PORT   - Decimal COM-port number\r\n");
	printf("  SPEED  - CAN bus speed, 1 char value:\r\n");
	printf("       0 - 10Kbit/s\r\n");
	printf("       1 - 20Kbit/s\r\n");
	printf("       2 - 50Kbit/s\r\n");
	printf("       3 - 100Kbit/s\r\n");
	printf("       4 - 125Kbit/s\r\n");
	printf("       5 - 250Kbit/s\r\n");
	printf("       6 - 500Kbit/s\r\n");
	printf("       7 - 800Kbit/s\r\n");
	printf("       8 - 1Mbit/s\r\n");
	printf("       9 - 95.238Kbit/s\r\n");
	printf("       a - 83.333Kbit/s\r\n");
	printf("       b - 47.619Kbit/s\r\n");
	printf("       c - 33.333Kbit/s\r\n");
	printf("       d - 5Kbit/s\r\n");
	printf("Sample: comlog.exe 9 6\r\n");
	printf("  9 - COM9 open\r\n");
	printf("  6 - CAN speed 500kbit/s\r\n\r\n");
	return;
}

int main(int argc, char **argv) {
	unsigned char data[2048];
	unsigned long size = 0;
	unsigned long count = 0;
	int ret;
	int speed = '6';
	int port = 7;
	char name[20];
	char ch;
	char no_init_flag = 0;
	SYSTEMTIME SystemTime;
	// Parameters:
	if (argc == 1) {
		help();
		return 0;
	}
	if (argc >= 2)
		port = atoi(argv[1]);
	if  (argc >= 3) {
		if (argv[2][0] >= '0' && argv[2][0] <= '9')
			speed = argv[2][0];
		else if (argv[2][0] >= 'a' && argv[2][0] <= 'd')
			speed = argv[2][0];
		else {
			printf("WARNING: unknown CAN speed!\r\n");
		}
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
	// Create log:
#if 0
	// UTC date stamp:
	sprintf(name, "log%u.txt", time(NULL));
#else
	// yyyymmdd_hhmmss date stamp:
	GetLocalTime(&SystemTime);
	sprintf(name, "log%04d%02d%02d_%02d%02d%02d.txt", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
#endif
	if ((f = fopen(name, "wb+")) == 0) {
		printf("ERROR: fopen() error!\r\n");
		com_deinit(&com);
		return __LINE__;
	}
	fseek(f, 0, SEEK_SET);
	printf("Log file \"%s\" created!\r\n", name);
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
#if 1
		printf("WARNING: command \"v\" return error %d, NO INIT mode!\r\n", ret);
		no_init_flag = 1;
		goto no_init;
#else
		printf("ERROR: command \"v\" return error %d!\r\n", ret);
		ret = __LINE__;
		goto ret_point;
#endif
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
	sprintf(data, "S%c\r", speed);
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
	// Goto NORMAL mode (receive):
	ret = com_putstr(&com, "O\r"); // start recv
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
no_init:
	// Ctrl-C event:
	if(SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) == FALSE) {
		printf("ERROR: Control Handler not installed!\r\n"); 
		ret = __LINE__;
		goto ret_uninit;
	}
	// Receive:
	while(1) {
		com_getblock_simple(&com, data, sizeof(data), &size);
		if (size)
			fwrite(data, 1, size, f);
		count += size;
		printf("Received:       %ub\r", count);
		if (term) {
			printf("\n");
			goto ret_ok;
		}
	}
	// Exit:
ret_ok:
	ret = 0;
ret_uninit:
	if (no_init_flag == 0) {
		ret = com_putstr(&com, "C\r"); // Reset
		if (ret) {
			printf("ERROR: com_putstr(\"C\"):%d error %d\r\n", __LINE__, ret);
		}
		ret = com_getchar(&com, &ch);
		if (ret) {
			printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
		}
		if (ch != '\r') {
			printf("ERROR: command \"C\" return error!\r\n");
		}
	}
ret_point:
	fclose(f);
	com_deinit(&com);
	return ret;
}
