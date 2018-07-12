/*
	CANOPEN.exe
	
	Тестовое приложение для работы по интерфейсу CANOPEN.
	Зачатки модуля программирования для LPC11Cxx.
	
	igorkov / fsp@igorkov.org / 2016
 */
#include <stdio.h>
#include <stdint.h>
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
	printf("canopen.exe PORT SPEED\r\n");
	printf("  PORT   - Decimal COM-port number\r\n");
	printf("Sample: comlog.exe 9\r\n");
	printf("  9 - COM9 open\r\n");
	return;
}

typedef struct {
	int addr;
	int len;
	uint8_t data[8];
} *pcan_t, can_t; 

int can_send(pcan_t can_packet) {
	unsigned char data[128];
	unsigned long offset = 0;
	int i;
	int ret;
	
	printf("SEND DATA: ADDR=%04x LEN=%d DATA=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
		can_packet->addr, can_packet->len, 
		can_packet->data[0], can_packet->data[1], can_packet->data[2], can_packet->data[3], 
		can_packet->data[4], can_packet->data[5], can_packet->data[6], can_packet->data[7]);

	// make command
	sprintf(data, "t");
	offset = 1;
	sprintf(&data[offset], "%03x", can_packet->addr);
	offset += 3;
	sprintf(&data[offset], "%1x", can_packet->len);
	offset += 1;
	for (i = 0; i < can_packet->len; i++) {
		sprintf(&data[offset], "%02x", can_packet->data[i]);
		offset += 2;
	}
	sprintf(&data[offset], "\r");
	//printf("CAN DATA: %s\n", data);
	// send
	ret = com_putstr(&com, data);
	if (ret) {
		printf("ERROR: com_putstr(\"C\"):%d error %d\r\n", __LINE__, ret);
	}
	com_getblock_simple(&com, data, 1, &offset);
	if (offset != 1 && 
		data[0] != '\r') {
		printf("ERROR: Bad canhacker command answer ad %d!\r\n", __LINE__);
	}

	return ret;
}

int ch2i(char sym) {
	if (sym <= '9' && sym >= '0') {
		return sym-'0';
	} else
	if (sym <= 'F' && sym >= 'A') {
		return sym-'A'+10;
	} else
	if (sym <= 'f' && sym >= 'a') {
		return sym-'a'+10;
	} else {
		return 0;
	}
}

int can_decode(char *str, pcan_t can_packet) {
	if (str[0] == 't') {
		//tiiiLDDDDDDDDDDDDDDDD[CR]
		can_packet->addr = ch2i(str[1]) * 256 + ch2i(str[2]) * 16 + ch2i(str[3]);
		can_packet->len  = ch2i(str[4]);
		can_packet->data[0] = ch2i(str[5]) * 16 + ch2i(str[6]);
		can_packet->data[1] = ch2i(str[7]) * 16 + ch2i(str[8]);
		can_packet->data[2] = ch2i(str[9]) * 16 + ch2i(str[10]);
		can_packet->data[3] = ch2i(str[11]) * 16 + ch2i(str[12]);
		can_packet->data[4] = ch2i(str[13]) * 16 + ch2i(str[14]);
		can_packet->data[5] = ch2i(str[15]) * 16 + ch2i(str[16]);
		can_packet->data[6] = ch2i(str[17]) * 16 + ch2i(str[18]);
		can_packet->data[7] = ch2i(str[19]) * 16 + ch2i(str[20]);

		printf("RECV DATA: ADDR=%04x LEN=%d DATA=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			can_packet->addr, can_packet->len, 
			can_packet->data[0], can_packet->data[1], can_packet->data[2], can_packet->data[3], 
			can_packet->data[4], can_packet->data[5], can_packet->data[6], can_packet->data[7]);

		return 0;
	} else {
		return 1;
	}
}

int main(int argc, char **argv) {
	unsigned char data[2048];
	unsigned char frw[0x4000];
	unsigned long size = 0;
	unsigned long count = 0;
	int offset = 0;
	int ret;
	int i;
	int speed = '3'; // 100kbit/s
	int port = 7;
	char name[20];
	char ch;
	SYSTEMTIME SystemTime;
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
	// Правильная инициализация линий RTS/DTR:
	com_control(&com, DTR_LINE|RTS_LINE);
	Sleep(100);
	com_control(&com, DTR_LINE);
	Sleep(100);
	com_control(&com, 0);
	Sleep(500);
	// Порт открыт и проинициализирован:
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
#if 0
	// Ctrl-C event:
	if(SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) == FALSE) {
		printf("ERROR: Control Handler not installed!\r\n"); 
		ret = __LINE__;
		goto ret_uninit;
	}
#endif

	printf("Unlock\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Unlock
		packet.data[0] = 0x2B; // Command
		packet.data[1] = 0x00; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x5A; // Data[0]
		packet.data[5] = 0x5A; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}

	printf("Memory Read Address (set)\r\n");
	// Send
	{
		can_t packet;
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Memory Read Address (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x10; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}

	printf("Memory Read Length (set)\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Memory Read Length (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x11; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x40; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
	
	printf("Read memory / Read Init Data\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Read memory
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x50; // Index (Minor)
		packet.data[2] = 0x1F; // Index (Major)
		packet.data[3] = 0x01; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
	
goto no_read;
	offset = 0;
	for (i = 0; offset < 0x4000-10; i += 4) {
		printf("Read memory / Read SegmentData %d\r\n", i);
		// Send
		{
			can_t packet;
			
			packet.addr = 0x600 + 0x7D;
			packet.len = 8;
			// Read memory
			packet.data[0] = 0x60; // Command
			
			if (i%8 == 4) {
				packet.data[0] |= 0x10; // flipped
			}
			
			packet.data[1] = 0x00; // Index (Minor)
			packet.data[2] = 0x00; // Index (Major)
			packet.data[3] = 0x00; // Sub Index
			packet.data[4] = 0x00; // Data[0]
			packet.data[5] = 0x00; // Data[1]
			packet.data[6] = 0x00; // Data[2]
			packet.data[7] = 0x00; // Data[3]
			
			can_send(&packet);
		}
		// Receive:
		{
			memset(data, 0, sizeof(data));
			com_getblock_simple(&com, data, sizeof(data), &size);
			//printf("ANSWER: %s\r\n", data);
			
			can_t packet;
			can_decode(data, &packet);
			
			if (packet.data[0] == 0x80) break;
			if (term) break;
			
			memcpy(&frw[offset], &packet.data[1], 7);
			offset+=7;
		}
	}
	
	{
		FILE *f;
		if ((f = fopen("out.bin", "wb+")) == 0) {
			printf("ERROR: fopen() error!\r\n");
			com_deinit(&com);
			return __LINE__;
		} 
		fwrite(frw, 1, offset, f);
		fclose(f); 
	}

	if (term == 0) {
		goto ret_ok;
	}
	
	
	
	
	
no_read:
	printf("Memory Write Address (set)\r\n");
	// Send
	{
		can_t packet;
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Memory Read Address (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x15; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0] // 0x10001000
		packet.data[5] = 0x10; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x10; // Data[3]
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
	
	//Try:
	//TX: 67d 21 50 1f 01 00 00 00 00
	//and receive:
	//RX: 67d 60 50 1f 01 00 00 00 00
	
	printf("Memory write (Write Init Data)\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		//
		packet.data[0] = 0x21; // Command
		packet.data[1] = 0x50; // Index (Minor)
		packet.data[2] = 0x1f; // Index (Major)
		packet.data[3] = 0x01; // Sub Index
		packet.data[4] = 0x04; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		
		// @TODO
		printf("\r\n"); 
		printf("ANSWER: %s\r\n", data); 
		printf("strlen() = %d\r\n", strlen(data));
		printf("\r\n"); 
		// may be here received req...
		
		can_t packet;
		can_decode(data, &packet);
	}

#if 1
	printf("Memory write (Write Segment Data)\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		//
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x50; // Index (Minor)
		packet.data[2] = 0x1f; // Index (Major)
		packet.data[3] = 0x01; // Sub Index
		packet.data[4] = 0x5A; // Data[0]
		packet.data[5] = 0x5A; // Data[1]
		packet.data[6] = 0xA5; // Data[2]
		packet.data[7] = 0xA5; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
#else
	printf("Memory write (Write Segment Data)\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		//
		packet.data[0] = 0x60; // Command
		packet.data[1] = 0xA5; // Index (Minor)
		packet.data[2] = 0x5A; // Index (Major)
		packet.data[3] = 0xA5; // Sub Index
		packet.data[4] = 0x5A; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
#endif

	// http://host.lpcware.com/content/forum/ccan-isp-write-ram-failure

	printf("\r\n\r\nREADING!!!!\r\n\r\n");

	printf("Memory Read Address (set)\r\n");
	// Send
	{
		can_t packet;
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Memory Read Address (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x10; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x10; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x10; // Data[3]
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}

	printf("Memory Read Length (set)\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Memory Read Length (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x11; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x04; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
	
	printf("Read memory / Read Init Data\r\n");
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Read memory
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x50; // Index (Minor)
		packet.data[2] = 0x1F; // Index (Major)
		packet.data[3] = 0x01; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		//printf("ANSWER: %s\r\n", data);
		
		can_t packet;
		can_decode(data, &packet);
	}
	
	printf("Read memory / Read SegmentData %d\r\n", i);
	// Send
	{
		can_t packet;
		
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		// Read memory
		packet.data[0] = 0x60; // Command
		packet.data[1] = 0x00; // Index (Minor)
		packet.data[2] = 0x00; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		can_send(&packet);
	}
	// Receive:
	{
		memset(data, 0, sizeof(data));
		com_getblock_simple(&com, data, sizeof(data), &size);
		
		can_t packet;
		can_decode(data, &packet);
	}

#if 0
		// DEVICE TYPE
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x00; // Index (Minor)
		packet.data[2] = 0x10; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
#elif 0
		// Part Identification Number
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x18; // Index (Minor)
		packet.data[2] = 0x10; // Index (Major)
		packet.data[3] = 0x02; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
#elif 0
		// Memory Read Address
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x10; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
#elif 0
		// Memory Read Address (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x10; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x10; // Data[3]
#elif 0
		// Memory Read Length
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x11; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
#elif 0
		// Memory Read Length (set)
		packet.data[0] = 0x23; // Command
		packet.data[1] = 0x11; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x04; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
#elif 0
		// Read memory
		packet.data[0] = 0x40; // Command
		packet.data[1] = 0x50; // Index (Minor)
		packet.data[2] = 0x1F; // Index (Major)
		packet.data[3] = 0x01; // Sub Index
		packet.data[4] = 0x00; // Data[0]
		packet.data[5] = 0x00; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
		
		//???
		// 80 1f 50 01 11 00 09 06
		// 80 - Abort
		// Data: 06090011
		// 0906 - Additional code
		// 00 - Error code 
		// 06 - Error class
#elif 0
		// Unlock
		packet.data[0] = 0x2B; // Command
		packet.data[1] = 0x00; // Index (Minor)
		packet.data[2] = 0x50; // Index (Major)
		packet.data[3] = 0x00; // Sub Index
		packet.data[4] = 0x5A; // Data[0]
		packet.data[5] = 0x5A; // Data[1]
		packet.data[6] = 0x00; // Data[2]
		packet.data[7] = 0x00; // Data[3]
#endif
	

	
	// Exit:
ret_ok:
	ret = 0;
ret_uninit:
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
ret_point:
	com_deinit(&com);
	return ret;
}
