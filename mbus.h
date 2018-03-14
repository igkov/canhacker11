#ifndef __MBUS_H__
#define __MBUS_H__

void mbus_init(int sniff);
void mbus_init_DEV(void);
void mbus_msend(uint8_t *data, int len);
void mbus_msend_slave(uint8_t *data, int len);

#endif
