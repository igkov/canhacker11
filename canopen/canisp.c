#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comn.h"
#include "canisp.h"

extern com_struct_t com;

//
// Low level: CANHACKER interface:
//

int can_send(pcan_t can_packet) {
	unsigned char data[128];
	unsigned long offset = 0;
	int i;
	int ret;
	
	//printf("SEND DATA: ADDR=%04x LEN=%d DATA=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
	//	can_packet->addr, can_packet->len, 
	//	can_packet->data[0], can_packet->data[1], can_packet->data[2], can_packet->data[3], 
	//	can_packet->data[4], can_packet->data[5], can_packet->data[6], can_packet->data[7]);

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

static int ch2i(char sym) {
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

int can_recv(pcan_t can_packet) {
	uint8_t str[512];
	unsigned long size;
	
	memset(str, 0, sizeof(str));
	com_getblock_simple(&com, str, sizeof(str), &size); 	

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
		
		//printf("RECV DATA: ADDR=%04x LEN=%d DATA=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
		//	can_packet->addr, can_packet->len, 
		//	can_packet->data[0], can_packet->data[1], can_packet->data[2], can_packet->data[3], 
		//	can_packet->data[4], can_packet->data[5], can_packet->data[6], can_packet->data[7]);
		
		return 0;
	} else {
		return 1;
	}
}

//
// Middle Level: CanOpen SDO processing
//

int sdo_write(uint16_t index, uint8_t subindex, int len, uint8_t *data) {
	int ret;
	can_t packet;
	
	if (len <= 0 || len > 4) {
		return 1;
	}

	memset(&packet, 0x00, sizeof(packet));
	packet.addr = 0x600 + 0x7D;
	packet.len = 8;
	packet.data[0] = 0x23 | ((4-len)<<2);
	packet.data[1] = index & 0xFF;
	packet.data[2] = (index >> 8) & 0xFF;
	packet.data[3] = subindex;
	memcpy(&packet.data[4], data, len);
	ret = can_send(&packet);
	if (ret) {
		return 0x100 + ret;
	}
	memset(&packet, 0x00, sizeof(packet));
	ret = can_recv(&packet);
	if (ret) {
		return 0x200 + ret;
	}
	if (packet.addr != (0x580 + NODE_ID)) {
		return 2;
	}
	if (packet.data[0] != 0x60 ||
		packet.data[1] != (index & 0xFF) ||
		packet.data[2] != ((index>>8) & 0xFF) ||
		packet.data[3] != subindex) {
		return 3;
	}
	return 0;
}

int sdo_read(uint16_t index, uint8_t subindex, int len, uint8_t *data) {
	int ret;
	can_t packet;
	
	if (len <= 0 || len > 4) {
		return 1;
	}

	memset(&packet, 0x00, sizeof(packet));
	packet.addr = 0x600 + 0x7D;
	packet.len = 8;
	packet.data[0] = 0x40;
	packet.data[1] = (index & 0xFF);
	packet.data[2] = (index >> 8) & 0xFF;
	packet.data[3] = subindex;
	ret = can_send(&packet);
	if (ret) {
		return 0x100 + ret;
	}
	memset(&packet, 0x00, sizeof(packet));
	ret = can_recv(&packet);
	if (ret) {
		return 0x200 + ret;
	}
	if (packet.addr != 0x580 + NODE_ID) {
		return 2;
	}
	if ((packet.data[0] & 0xE3) != 0x43 ||
		packet.data[1] != (index & 0xFF) ||
		packet.data[2] != ((index>>8) & 0xFF) ||
		packet.data[3] != subindex) {
		return 3;
	}
	if (((packet.data[0] >> 2) & 0x03) != (4-len)) {
		return 4;
	}
	memcpy(data, &packet.data[4], len);
	return 0;
}

int sdo_seq_read(uint16_t index, uint8_t subindex, int len, uint8_t *data) {
	int ret;
	can_t packet;
	int offset = 0;
	int toggle = 0;
	
	memset(&packet, 0x00, sizeof(packet));
	packet.addr = 0x600 + 0x7D;
	packet.len = 8;
	packet.data[0] = 0x40;
	packet.data[1] = (index & 0xFF);
	packet.data[2] = (index >> 8) & 0xFF;
	packet.data[3] = subindex;
	ret = can_send(&packet);
	if (ret) {
		return 0x100 + ret;
	}
	memset(&packet, 0x00, sizeof(packet));
	ret = can_recv(&packet);
	if (ret) {
		return 0x200 + ret;
	}
	if (packet.addr != 0x580 + NODE_ID) {
		return 2;
	}
	if (packet.data[0] != 0x40 ||
		packet.data[1] != (index & 0xFF) ||
		packet.data[2] != ((index>>8) & 0xFF) ||
		packet.data[3] != subindex) {
		return 3;
	}
	
	while (offset < len) {
		int size = (len - offset) > 7 ? 7 : (len - offset);
		
		memset(&packet, 0x00, sizeof(packet));
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		if (toggle) {
			packet.data[0] = 0x70;
		} else {
			packet.data[0] = 0x60;
		}
		ret = can_send(&packet);
		if (ret) {
			return 0x300 + ret;
		}
		memset(&packet, 0x00, sizeof(packet));
		ret = can_recv(&packet);
		if (ret) {
			return 0x400 + ret;
		}
		if (packet.addr != 0x580 + NODE_ID) {
			return 2;
		}
		if (toggle) {
			if ((packet.data[0] & 0xF0) != 0x10) {
				return 3;
			}
		} else {
			if ((packet.data[0] & 0xF0) != 0x00) {
				return 3;
			}
		}
		memcpy(&data[offset], &packet.data[1], size);
		if (packet.data[0] & 0x01) {
			break;
		}
		toggle = toggle ? 0 : 1;
		offset += size;
	}
	return 0;
}

int sdo_seq_write(uint16_t index, uint8_t subindex, int len, uint8_t *data) {
	int ret;
	can_t packet;
	int offset = 0;
	int toggle = 0;
	
	memset(&packet, 0x00, sizeof(packet));
	packet.addr = 0x600 + 0x7D;
	packet.len = 8;
	packet.data[0] = 0x21;
	packet.data[1] = (index & 0xFF);
	packet.data[2] = (index >> 8) & 0xFF;
	packet.data[3] = subindex;
	packet.data[4] = (len >>  0) & 0xFF;
	packet.data[5] = (len >>  8) & 0xFF;
	packet.data[6] = (len >> 16) & 0xFF;
	packet.data[7] = (len >> 24) & 0xFF;
	ret = can_send(&packet);
	if (ret) {
		return 0x100 + ret;
	}
	memset(&packet, 0x00, sizeof(packet));
	ret = can_recv(&packet);
	if (ret) {
		return 0x200 + ret;
	}
	if (packet.addr != 0x580 + NODE_ID) {
		return 2;
	}
	if (packet.data[0] != 0x60 ||
		packet.data[1] != (index & 0xFF) ||
		packet.data[2] != ((index>>8) & 0xFF) ||
		packet.data[3] != subindex) {
		return 3;
	}
	
	while (offset < len) {
		// 
		// NOTE / TODO / FIX
		// 
		// Почему-то загрузчик принимает только по 4 байта в каждой посылке...
		// 
		//int size = (len - offset) > 7 ? 7 : (len - offset);
		int size = (len - offset) > 4 ? 4 : (len - offset);
		
		memset(&packet, 0x00, sizeof(packet));
		packet.addr = 0x600 + 0x7D;
		packet.len = 8;
		if (toggle) {
			packet.data[0] = 0x16;
		} else {
			packet.data[0] = 0x06;
		}
		if (offset + size == len) {
			// last
			packet.data[0] |= 0x01;
		}
		memcpy(&packet.data[1], &data[offset], size);
		ret = can_send(&packet);
		if (ret) {
			return 0x300 + ret;
		}
		memset(&packet, 0x00, sizeof(packet));
		ret = can_recv(&packet);
		if (ret) {
			return 0x400 + ret;
		}
		if (packet.addr != 0x580 + NODE_ID) {
			return 2;
		}
		if (toggle) {
			if ((packet.data[0] & 0xF0) != 0x30) {
				return 3;
			}
		} else {
			if ((packet.data[0] & 0xF0) != 0x20) {
				return 3;
			}
		}
		toggle = toggle ? 0 : 1;
		offset += size;
	}
	return 0;
}

// 
// High Level: ISP LPC interface
//

int isp_get_partid(uint32_t *id) {
	int ret;
	ret = sdo_read(SDO_PART_ID, 4, (uint8_t*)id);
	return ret;
}

int isp_get_device_type(char *type) {
	int ret;
	ret = sdo_read(SDO_DEVICE_TYPE, 4, type);
	type[4] = 0x00;
	return ret;
}

int isp_unlock(void) {
	int ret;
	uint16_t unlock = 0x5a5a;
	ret = sdo_write(SDO_UNLOCK, 2, (uint8_t*)&unlock);
	return ret;
}

int isp_write_ram(uint32_t addr, uint8_t *data, int len) {
	int ret;
	ret = sdo_write(SDO_WRITE_ADDRESS, 4, (uint8_t*)&addr);
	if (ret) {
		return ret;
	}
	ret = sdo_seq_write(SDO_DATA, len, data);
	return ret;
}

int isp_read_memory(uint32_t addr, uint8_t *data, int len) {
	int ret;
	ret = sdo_write(SDO_READ_ADDRESS, 4, (uint8_t*)&addr);
	if (ret) {
		return ret;
	}
	ret = sdo_write(SDO_READ_LENGTH, 4, (uint8_t*)&len);
	if (ret) {
		return ret;
	}
	ret = sdo_seq_read(SDO_DATA, len, data);
	return ret;
}

int isp_prepare(uint8_t start_sector, uint8_t end_sector) {
	int ret;
	uint16_t prepare = start_sector | (end_sector << 8);
	ret = sdo_write(SDO_PREPARE, 2, (uint8_t*)&prepare);
	return ret;	
}

int isp_erase(uint8_t start_sector, uint8_t end_sector) {
	int ret;
	uint16_t erase = start_sector | (end_sector << 8);
	ret = sdo_write(SDO_ERASE, 2, (uint8_t*)&erase);
	return ret;	
}

int isp_ram_to_flash(uint32_t ram_addr, uint32_t flash_addr, int len) {
	int ret;
	uint16_t len16 = len;
	ret = sdo_write(SDO_FLASH_ADDR, 4, (uint8_t*)&flash_addr);
	if (ret) {
		return ret;
	}
	ret = sdo_write(SDO_FLASH_RAM, 4, (uint8_t*)&ram_addr);
	if (ret) {
		return ret;
	}
	ret = sdo_write(SDO_FLASH_SIZE, 2, (uint8_t*)&len16);
	return ret;	
}

int isp_get_serial(uint32_t *data) {
	int ret;
	ret = sdo_read(SDO_SN_1, 4, (uint8_t*)&data[0]);
	if (ret) {
		return ret;
	}
	ret = sdo_read(SDO_SN_2, 4, (uint8_t*)&data[1]);
	if (ret) {
		return ret;
	}
	ret = sdo_read(SDO_SN_3, 4, (uint8_t*)&data[2]);
	if (ret) {
		return ret;
	}
	ret = sdo_read(SDO_SN_4, 4, (uint8_t*)&data[3]);
	return ret;
}

int isp_go(uint32_t *addr, uint8_t mode) {
	int ret;
	ret = sdo_write(SDO_EXEC_ADDR, 4, (uint8_t*)addr);
	if (ret) {
		return ret;
	}
	ret = sdo_write(SDO_EXEC_ADDR, 1, &mode);
	return ret;
}
