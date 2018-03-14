#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <Shlwapi.h>

#include "event.h"
#include "comn.h"

char *ini_name = "./config.ini";

int com_number = 0;
int com_speed = 0;

int can_init = 0;
int can_speed = 1;
int can_mode = 0;

int mbus_init = 0;
int mbus_busy = 0;
int mbus_sniffer = 0;

typedef struct {
	int interval;
	int delay;
	int addr;
	int len;
	uint8_t data[8];
} *pcan_t, can_t;

typedef struct {
	int interval;
	int delay;
	int len;
	uint8_t data[32];
} *pmbus_t, mbus_t;

mbus_t mbus_list[1000];
can_t can_list[1000];

int htoi(char *s) {
	int hexdigit;
	int inhex;
	int i = 0;
	int n = 0;
	if( s[i] == '0' && ( s[i+1] == 'x' || s[i+1] == 'X' ) ) {
		i += 2;
	}
	inhex = 1;
	for( ; inhex == 1; ++i) {
		if(s[i] >='0' && s[i] <='9')
			hexdigit= s[i] - '0';
		else if(s[i] >='a' && s[i] <='f')
			hexdigit= s[i] -'a' + 10;
		else if(s[i] >='A' && s[i] <='F')
			hexdigit= s[i] -'A' + 10;
		else {
			inhex = 0;
		}
		if(inhex == 1) {
			n = 16 * n + hexdigit;
		}
	}
	return n;
}

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

unsigned long count_can = 0;
unsigned long count_mbus = 0;

int ev_mbus(int param) {
	unsigned char data[256];
	int offset = 0;
	int i;
	int ret;

	// info:
	//printf("ev_mbus(): param = %d                                               \r\n", param);
	count_mbus++;
	
	// make command
	sprintf(data, "X");
	offset = 1;
	for (i=0; i<mbus_list[param].len; i++) {
		sprintf(&data[offset], "%02x", mbus_list[param].data[i]);
		offset += 2;
	}
	sprintf(&data[offset], "\r");
	//printf("DATA: %s\r\n", data);
	
	// send
	ret = com_putstr(&com, data);
	if (ret) {
		printf("ERROR: com_putstr(\"X\"):%d error %d\r\n", __LINE__, ret);
	}
	
	return mbus_list[param].interval;
}

int ev_can(int param) {
	unsigned char data[256];
	int offset = 0;
	int i;
	int ret;

	// info:
	//printf("ev_can(): param = %d                                                \r\n", param);
	count_can++;
	
	// make command
	sprintf(data, "t");
	offset = 1;
	sprintf(&data[offset], "%03x", can_list[param].addr);
	offset += 3;
	sprintf(&data[offset], "%1x", can_list[param].len);
	offset += 1;
	for (i=0; i<can_list[param].len; i++) {
		sprintf(&data[offset], "%02x", can_list[param].data[i]);
		offset += 2;
	}
	sprintf(&data[offset], "\r");
	//printf("DATA: %s\r\n", data);
	
	// send
	ret = com_putstr(&com, data);
	if (ret) {
		printf("ERROR: com_putstr(\"C\"):%d error %d\r\n", __LINE__, ret);
	}
	return can_list[param].interval;
}

int main(int argc, char **argv) {
	int i;
	int ret;
	int ecnt = 0;
	char section[64];
	char str[128];
	SYSTEMTIME SystemTime;
	unsigned char data[2048];
	unsigned long size = 0;
	unsigned long count = 0;
	
	GetPrivateProfileString("CANHACKER","INIT",     "0", str, 127, ini_name);
	can_init = atoi(str);
	GetPrivateProfileString("CANHACKER","SPEED",    "500001", str, 127, ini_name);
	can_speed = atoi(str);
	
	switch (can_speed) {
	// 0 - 10Kbit/s
	case 10000:
		can_speed = '0';
		break;
	// 1 - 20Kbit/s
	case 20000:
		can_speed = '1';
		break;
	// 2 - 50Kbit/s
	case 50000:
		can_speed = '2';
		break;
	// 3 - 100Kbit/s
	case 100000:
		can_speed = '3';
		break;
	// 4 - 125Kbit/s
	case 125000:
		can_speed = '4';
		break;
	// 5 - 250Kbit/s
	case 250000:
		can_speed = '5';
		break;
	// 6 - 500Kbit/s
	case 500000:
		can_speed = '6';
		break;
	// 7 - 800Kbit/s
	case 800000:
		can_speed = '7';
		break;
	// 8 - 1Mbit/s
	case 1000000:
		can_speed = '8';
		break;
	// 9 - 95.238Kbit/s
	case 95238:
		can_speed = '9';
		break;
	// a - 83.333Kbit/s
	case 83333:
		can_speed = 'a';
		break;
	// b - 47.619Kbit/s
	case 47619:
		can_speed = 'b';
		break;
	// c - 33.333Kbit/s
	case 33333:
		can_speed = 'c';
		break;
	// d - 5Kbit/s
	case 5000:
		can_speed = 'd';
		break;
	default:
		printf("ERROR: incorrect CAN speed %d\r\n", can_speed);
		return 0;
	}
	
	GetPrivateProfileString("CANHACKER","PORT",      "6",      str, 127, ini_name);
	com_number = atoi(str);
	GetPrivateProfileString("CANHACKER","PORT_SPEED","115200",      str, 127, ini_name);
	com_speed = atoi(str);
	GetPrivateProfileString("CANHACKER","MODE",      "NONE", str, 127, ini_name);
	
	if (strcmp(str, "NONE") == 0) {
		can_mode = 0;
	} else
	if (strcmp(str, "SNIFFER") == 0) {
		can_mode = 1;
	} else
	if (strcmp(str, "LISTEN") == 0) {
		can_mode = 2;
	} else {
		can_mode = 0;
	}
	
	GetPrivateProfileString("MBUS","INIT",          "0",      str, 127, ini_name);
	mbus_init = atoi(str);
	GetPrivateProfileString("MBUS","BUSY",          "MASTER", str, 127, ini_name);
	if (strcmp(str, "MASTER") == 0) {
		mbus_busy = 0;
	} else 
	if (strcmp(str, "SLAVE") == 0) {
		mbus_busy = 1;
	} else {
		mbus_busy = 0;
	} 
	GetPrivateProfileString("MBUS","SNIFFER",       "1",      str, 127, ini_name);
	mbus_sniffer = atoi(str);
	
	for (i=0; i<999; i++) {
		int j;
		sprintf(section, "CAN_%03d", i);
		GetPrivateProfileString(section,"DELAY",         "-1",   str, 127, ini_name);
		can_list[i].delay = atoi(str);
		if (strcmp(str, "-1") == 0 ||
			strcmp(str, "0") == 0) {
			continue;
		}
		GetPrivateProfileString(section,"INTERVAL",      "0",    str, 127, ini_name);
		can_list[i].interval = atoi(str);
		GetPrivateProfileString(section,"ADDR",          "0",    str, 127, ini_name);
		can_list[i].addr = htoi(str);
		GetPrivateProfileString(section,"LEN",           "0",    str, 127, ini_name);
		can_list[i].len = atoi(str);
		if (can_list[i].len > 8) {
			printf("ERROR: can %d len error (len %d > 8)!", i, can_list[i].len);
			continue;
		}
		GetPrivateProfileString(section,"DATA",          "0",    str, 127, ini_name);
		for (j=0; j < can_list[i].len && j*3 < strlen(str)-1; j++) {
			int value = htoi(&str[j*3]);
			can_list[i].data[j] = value;
		}
	}
	
	for (i=0; i<999; i++) {
		int j;
		sprintf(section, "MBUS_%03d", i);
		GetPrivateProfileString(section,"DELAY",         "-1",   str, 127, ini_name);
		mbus_list[i].delay = atoi(str);
		if (strcmp(str, "-1") == 0 ||
			strcmp(str, "0") == 0) {
			continue;
		}
		GetPrivateProfileString(section,"INTERVAL",      "0",    str, 127, ini_name);
		mbus_list[i].interval = atoi(str);
		GetPrivateProfileString(section,"LEN",           "0",    str, 127, ini_name);
		mbus_list[i].len = atoi(str);
		if (mbus_list[i].len > 32) {
			printf("ERROR: mbus %d len error (len %d > 32)!", i, mbus_list[i].len);
			continue;
		}
		GetPrivateProfileString(section,"DATA",          "0",    str, 127, ini_name);
		for (j=0; j < mbus_list[i].len && j*3 < strlen(str)-1; j++) {
			int value = htoi(&str[j*3]);
			mbus_list[i].data[j] = value;
		}
	}
	
	// -------------
	// Init hw:
	// -------------
	
	if (com_number) {
		// Open port:
		if (ret = com_init(&com, com_number, com_speed)) {
			printf("ERROR: com_init() return %d!\r\n", ret);
			return 0;
		}
		// Правильная инициализация линий RTS/DTR:
		com_control(&com, DTR_LINE|RTS_LINE);
		Sleep(100);
		com_control(&com, DTR_LINE);
		Sleep(100);
		com_control(&com, 0);
		Sleep(500);
		// Порт открыт и проинициализирован:
		printf("Com port #%d opened!\r\n", com_number);
	} else {
		printf("ERROR: incorrect com-port number!\r\n");
		return 0;
	}

	// Create log:
	// yyyymmdd_hhmmss date stamp:
	GetLocalTime(&SystemTime);
	sprintf(str, "log%04d%02d%02d_%02d%02d%02d.txt", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
	if ((f = fopen(str, "wb+")) == 0) {
		printf("ERROR: fopen() error!\r\n");
		com_deinit(&com);
		return __LINE__;
	}
	fseek(f, 0, SEEK_SET);
	printf("Log file \"%s\" created!\r\n", str);

	if (can_init) {
		// Info (hw):
		ret = com_putstr(&com, "v\r");
		if (ret) {
			printf("ERROR: com_putstr(\"v\"):%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		memset(data, 0, sizeof(data));
		ret = com_getblock(&com, data, sizeof(data), &size);
		if (ret) {
			printf("ERROR: com_getblock():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		if (data[0] != 'v') {
			printf("WARNING: command \"v\" return error %d, NO INIT mode!\r\n", ret);
			goto no_init;
		}
		while (data[size] != '\r' && size > 1) size--;
		data[size] = 0x00;
		printf("Device Name:    %s\r\n", &data[1]);
		// Info (version):
		ret = com_putstr(&com, "V\r");
		if (ret) {
			printf("ERROR: com_putstr(\"V\"):%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		memset(data, 0, sizeof(data));
		ret = com_getblock(&com, data, sizeof(data), &size);
		if (ret) {
			printf("ERROR: com_getblock():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		if (data[0] != 'V') {
			printf("ERROR: command \"V\" error!\r\n");
			ret = __LINE__;
			goto ret_uninit;
		}
		while (data[size] != '\r' && size > 1) size--;
		data[size] = 0x00;
		printf("Device Version: %s\r\n", &data[1]);
		// Info (serial):
		ret = com_putstr(&com, "N\r");
		if (ret) {
			printf("ERROR: com_putstr(\"N\"):%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		memset(data, 0, sizeof(data));
		ret = com_getblock(&com, data, sizeof(data), &size);
		if (ret) {
			printf("ERROR: com_getblock():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		if (data[0] != 'N') {
			printf("ERROR: command \"N\" error!\r\n");
			ret = __LINE__;
			goto ret_uninit;
		}
		while (data[size] != '\r' && size > 1) size--;
		data[size] = 0x00;
		printf("Device S/N:     %s\r\n", &data[1]);

		// CAN
		if (can_mode) {
			char ch;
			// Reset
			ret = com_putstr(&com, "C\r");
			if (ret) {
				printf("ERROR: com_putstr(\"C\"):%d error %d\r\n", __LINE__, ret);
				ret = __LINE__;
				goto ret_uninit;
			}
			ret = com_getchar(&com, &ch);
			if (ret) {
				printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
				ret = __LINE__;
				goto ret_uninit;
			}
			if (ch != '\r') {
				printf("ERROR: command \"C\" error!\r\n");
				ret = __LINE__;
				goto ret_uninit;
			}
			// Set CAN speed:
			sprintf(data, "S%c\r", can_speed);
			ret = com_putstr(&com, data);
			if (ret) {
				printf("ERROR: com_putstr(\"S%c\"):%d error %d\r\n", can_speed, __LINE__, ret);
				ret = __LINE__;
				goto ret_uninit;
			}
			ret = com_getchar(&com, &ch);
			if (ret) {
				printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
				ret = __LINE__;
				goto ret_uninit;
			}
			if (ch != '\r') {
				printf("ERROR: command \"S%c\" return error %d!\r\n", can_speed, ch);
				ret = __LINE__;
				goto ret_uninit;
			}
			if (can_mode == 1) {
				// Goto NORMAL mode (receive):
				ret = com_putstr(&com, "O\r"); // start recv
				if (ret) {
					printf("ERROR: com_putstr(\"O\"):%d error %d\r\n", __LINE__, ret);
					ret = __LINE__;
					goto ret_uninit;
				}
			} else 
			if (can_mode == 2) {
				// Goto LISTEN mode (receive):
				ret = com_putstr(&com, "L\r"); // start recv
				if (ret) {
					printf("ERROR: com_putstr(\"L\"):%d error %d\r\n", __LINE__, ret);
					ret = __LINE__;
					goto ret_uninit;
				}
			} else {
				printf("ERROR: unknown CAN mode!\r\n");
				goto ret_uninit;
			}
			ret = com_getchar(&com, &ch);
			if (ret) {
				printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
				ret = __LINE__;
				goto ret_uninit;
			}
			if (ch != '\r') {
				printf("ERROR: command \"O\" return error %d!\r\n", ch);
				ret = __LINE__;
				goto ret_uninit;
			}
		}
	}
	
	// MBUS
	if (mbus_init) {
		char ch;
		// no DEV-signal
		sprintf(data, "I%c%c\r", mbus_sniffer?'1':'0', '0');
		ret = com_putstr(&com, data);
		if (ret) {
			printf("ERROR: com_putstr(\"S%c\"):%d error %d\r\n", can_speed, __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		ret = com_getchar(&com, &ch);
		if (ret) {
			printf("ERROR: com_getchar():%d error %d\r\n", __LINE__, ret);
			ret = __LINE__;
			goto ret_uninit;
		}
		if (ch != '\r') {
			printf("ERROR: command \"I\" return error %d!\r\n", ch);
			ret = __LINE__;
			goto ret_uninit;
		}
	}

	// Init events system:
	event_init();
	
	// Add async events:
	for (i=1; i<999; i++) {
		if (mbus_init) {
			if (mbus_list[i].delay >= 0) {
				//printf("mbus %d present, len = %d, interval = %d, delay = %d, data = %02x %02x %02x %02x ...!\r\n", 
				//	i, mbus_list[i].len, mbus_list[i].interval, mbus_list[i].delay,
				//	mbus_list[i].data[0], mbus_list[i].data[1], mbus_list[i].data[2], mbus_list[i].data[3]);
				
				event_set(ecnt, ev_mbus, mbus_list[i].delay+100, i);
				ecnt++;
			}
			if (ecnt >= MAX_EVENT) {
				printf("ERROR: too many events (max = %d)!\r\n", MAX_EVENT);
				break;
			}
		}
		
		if (can_mode) {
			if (can_list[i].delay >= 0) {
				//printf("can %d present, len = %d, addr = %03x interval = %d, delay = %d, data = %02x %02x %02x %02x ...!\r\n", 
				//	i, can_list[i].len, can_list[i].addr, can_list[i].interval, can_list[i].delay,
				//	can_list[i].data[0], can_list[i].data[1], can_list[i].data[2], can_list[i].data[3]);
				
				event_set(ecnt, ev_can, can_list[i].delay+100, i);
				ecnt++;
			}
		}
		
		if (ecnt >= MAX_EVENT) {
			printf("ERROR: too many events (max = %d)!\r\n", MAX_EVENT);
			break;
		}
	}

no_init:
	// Ctrl-C event:
	if(SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) == FALSE) {
		printf("ERROR: Control Handler not installed!\r\n"); 
		goto ret_uninit;
	}
	
	// Receive:
	while(1) {
		com_getblock_simple(&com, data, sizeof(data), &size);
		if (size) {
			fwrite(data, 1, size, f);
		}
		count += size;
		printf("Received: %5ub, send CAN %3u, send MBUS %3u\r", count, count_can, count_mbus);
		if (term) {
			printf("\n");
			for (i=0; i<MAX_EVENT; i++) {
				event_unset(i);
			}
			goto ret_ok;
		}
	}

ret_ok:
	//printf();
ret_uninit:
	com_deinit(&com);
	fclose(f);
	return 0;
}
