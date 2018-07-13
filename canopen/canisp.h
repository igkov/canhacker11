#ifndef __CANISP_H__
#define __CANISP_H__

#include <stdint.h>

// High Level: ISP LCP interface

int isp_get_partid(uint32_t *id);
int isp_get_device_type(char *type);
int isp_unlock(void);
int isp_write_ram(uint32_t addr, uint8_t *data, int len);
int isp_read_memory(uint32_t addr, uint8_t *data, int len);
int isp_prepare(uint8_t start_sector, uint8_t end_sector);
int isp_erase(uint8_t start_sector, uint8_t end_sector);
int isp_ram_to_flash(uint32_t ram_addr, uint32_t flash_addr, int len);
int isp_get_serial(uint32_t *data);
int isp_go(uint32_t *addr, uint8_t mode);

// Middle Level: CanOpen SDO processing:

// Max data 4b
int sdo_read(uint16_t index, uint8_t subindex, int len, uint8_t *data);
int sdo_write(uint16_t index, uint8_t subindex, int len, uint8_t *data);

// Max data ??b
int sdo_seq_read(uint16_t index, uint8_t subindex, int len, uint8_t *data);
int sdo_seq_write(uint16_t index, uint8_t subindex, int len, uint8_t *data);

// Low level: CANBUS, Canhacker driver

typedef struct {
	int addr;
	int len;
	uint8_t data[8];
} *pcan_t, can_t;

int can_send(pcan_t can_packet);
int can_recv(pcan_t can_packet);

// DEFINES:

#define NODE_ID            0x7D

#define SDO_DEVICE_TYPE    0x1000,0x00
#define SDO_PART_ID        0x1018,0x02
#define SDO_DATA           0x1F50,0x01
#define SDO_UNLOCK         0x5000,0x00
#define SDO_READ_ADDRESS   0x5010,0x00
#define SDO_READ_LENGTH    0x5011,0x00
#define SDO_WRITE_ADDRESS  0x5015,0x00
#define SDO_PREPARE        0x5020,0x00
#define SDO_ERASE          0x5030,0x00
#define SDO_BLANK_CHECK    0x5040,0x01
#define SDO_BLANK_ADDR     0x5040,0x02
#define SDO_FLASH_ADDR     0x5050,0x01
#define SDO_FLASH_RAM      0x5050,0x02
#define SDO_FLASH_SIZE     0x5050,0x03
#define SDO_CMP_ADDR1      0x5060,0x01
#define SDO_CMP_ADDR2      0x5060,0x02
#define SDO_CMP_SIZE       0x5060,0x03
#define SDO_CMP_OFFSET     0x5060,0x04
#define SDO_EXEC_ADDR      0x5070,0x01
#define SDO_EXEC_GO        0x5070,0x02
#define SDO_SN_1           0x5100,0x01
#define SDO_SN_2           0x5100,0x02
#define SDO_SN_3           0x5100,0x03
#define SDO_SN_4           0x5100,0x04

#endif // __CANISP_H__
