#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

void timer0_init(void);
uint32_t timer0_get(void);
void delay_mks(uint32_t mks);

#endif
