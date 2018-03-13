#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <Shlwapi.h>

char *ini_name = "config.ini";

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
	int hexdigit,i,inhex,n;
	i = 0;
	if( s[i] == '0') {
		++i;
		if(s[i] == 'x' || s[i] == 'X') {
			++i;
		}
	}
	n = 0;
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

int main(int argc, char **argv) {
	int i;
	char section[64];
	char str[128];
	
	GetPrivateProfileString("CANHACKER","SPEED",    "500000", str, 127, ini_name);
	GetPrivateProfileString("CANHACKER","PORT",     "6",      str, 127, ini_name);
	GetPrivateProfileString("CANHACKER","MODE",     "500000", str, 127, ini_name);
	
	GetPrivateProfileString("MBUS","INIT",          "0",      str, 127, ini_name);
	GetPrivateProfileString("MBUS","BUSY",          "200",    str, 127, ini_name);
	GetPrivateProfileString("MBUS","SNIFFER",       "5",      str, 127, ini_name);
	
	for (i=1; i<999; i++) {
		int j;
		sprintf(section, "CAN_%03d", i);
		GetPrivateProfileString(section,"DELAY",         "-1",   str, 127, ini_name);
		can_list[i].delay = atoi(str);
		if (strcmp(str, "-1") == 0 ||
			strcmp(str, "0") == 0) {
			continue;
		}
		printf("A");
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
	
	for (i=1; i<999; i++) {
		int j;
		sprintf(section, "MBUS_%03d", i);
		GetPrivateProfileString(section,"DELAY",         "-1",   str, 127, ini_name);
		mbus_list[i].delay = atoi(str);
		if (strcmp(str, "-1") == 0 ||
			strcmp(str, "0") == 0) {
			continue;
		}
		printf("X");
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
	
	for (i=1; i<999; i++) {
		if (mbus_list[i].delay >= 0) {
			printf("mbus %d present!\r\n", i);
		}
		if (can_list[i].delay >= 0) {
			printf("can %d present!\r\n", i);
		}
		
	}
	
	return 0;
}
