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
#include "canisp.h"

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
	printf("canopen.exe PORT [FILENAME]\r\n");
	printf("  PORT   - Decimal COM-port number\r\n");
	printf("Sample: comlog.exe 6 frw.bin\r\n");
	printf("  6       - COM6 open\r\n");
	printf("  frw.bin - optional file name\r\n");
	printf("            (default loading test/test.bin)\r\n");
	return;
}

int main(int argc, char **argv) {
	unsigned char data[2048];  // buffer for protocol i/o
	unsigned char frw[0x8000]; // firmware image
	unsigned long size = 0;
	uint32_t id = 0;
	int ret;
	int i;
	int speed = '3'; // 100kbit/s
	int port = 7;
	char ch;
	char *filename;
	
	SYSTEMTIME SystemTime;
	// Parameters:
	if (argc == 1) {
		help();
		return 0;
	}
	if (argc >= 2) {
		port = atoi(argv[1]);
	}
	if (argc >= 3) {
		filename = argv[2];
	} else {
		filename = "test/test.bin";
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
	// Ctrl-C event:
	if(SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) == FALSE) {
		printf("ERROR: Control Handler not installed!\r\n"); 
		ret = __LINE__;
		goto ret_uninit;
	}
	
	printf("Read Device Type\r\n");
	ret = isp_get_device_type(data);
	if (ret) {
		printf("isp_get_device_type() return %d\r\n", ret);
	}

	printf("Read Device ID\r\n");
	{
		uint32_t id;
		ret = isp_get_partid(&id);
		if (ret) {
			printf("isp_get_partid() return %d\r\n", ret);
		}
		printf("id = %08x\r\n", id);
	}
	
	switch (id) {
	case 0x1421102B:
		printf("Controller type: LPC11C12FBD48/301\r\n");
		size = 0x4000;
		break;
	case 0x1440102B:
		printf("Controller type: LPC11C14FBD48/301\r\n");
		size = 0x8000;
		break;
	case 0x1431102B:
		printf("Controller type: LPC11C22FBD48/301\r\n");
		size = 0x4000;
		break;
	case 0x1430102B:
		printf("Controller type: LPC11C24FBD48/301!\r\n");
		size = 0x8000;
		break;
	default:
		printf("Controller type: unknown!\r\n");
		size = 0x0000;
		goto ret_uninit;
	}

	if (1) {
		FILE *f;
		int frw_size;
		printf("Loading file \"%s\"\r\n", filename);
		if ((f = fopen(filename, "rb")) == 0) {
			printf("ERROR: fopen() error!\r\n");
			com_deinit(&com);
			return __LINE__;
		} 
		frw_size = fread(frw, 1, 0x8000, f);
		fclose(f);
		
		if (frw_size > size) {
			printf("ERROR: firmware too big for this controller!\r\n");
			goto ret_uninit;
		}
		
		printf("Firmware size: %db\r\n", frw_size);
		
		size = frw_size;
		if (size & 0xFF) {
			memset(&frw[size], 0xFF, 0x100-(size & 0xFF));
			size = (size & 0xFF00) + 0x100;
		}
		
		// Set crc
		printf("Setup CRC in firmware image...\r\n");
		((uint32_t*)frw)[7] = 
			-( ((uint32_t*)frw)[0] + 
			   ((uint32_t*)frw)[1] + 
			   ((uint32_t*)frw)[2] + 
			   ((uint32_t*)frw)[3] + 
			   ((uint32_t*)frw)[4] + 
			   ((uint32_t*)frw)[5] + 
			   ((uint32_t*)frw)[6]    );
		printf("CRC = %08x;\r\n", ((uint32_t*)frw)[7]);
	}
	
	printf("Unlock\r\n");
	ret = isp_unlock();
	if (ret) {
		printf("isp_unlock() return %d\r\n", ret);
	}
	
	printf("Erase Prepare\r\n");
	ret = isp_prepare(0, 7);
	if (ret) {
		printf("isp_prepare() return %d\r\n", ret);
	}

	printf("Erase\r\n");
	ret = isp_erase(0, 7);
	if (ret) {
		printf("isp_erase() return %d\r\n", ret);
	}
	
	for (i=0; i < size; i += 256) {
		//printf("Write %d\r\n", i);
		printf("Programming %d%%...\r", (int)((i*100)/size));
		
		//printf("Write to RAM\r\n");
		ret = isp_write_ram(0x10000800, &frw[i], 256);
		if (ret) {
			printf("isp_write_ram() return %d\r\n", ret);
			exit(0);
		}
		//printf("Write Prepare\r\n");
		ret = isp_prepare(0, 7);
		if (ret) {
			printf("isp_prepare() return %d\r\n", ret);
			exit(0);
		}
		//printf("Copy RAM to Flash\r\n");
		ret = isp_ram_to_flash(0x10000800, i, 256);
		if (ret) {
			printf("isp_ram_to_flash() return %d\r\n", ret);
			exit(0);
		}
		//
		// TODO: check write operation
		//
	}
	
	printf("\n");
	printf("Program finish!\r\n");

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
