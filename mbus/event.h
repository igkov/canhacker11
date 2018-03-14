#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdint.h>
#define MAX_EVENT 150

typedef int (*timer_event_f)(int param);

typedef struct {
	uint32_t cmp_val;
	timer_event_f pfunc;
	int param;
} timer_event_t, *ptimer_event_t;

void event_init(void);
int event_set(uint32_t slot, timer_event_f pfunc, uint32_t delay, int param);
int event_unset(uint32_t slot);

void delay_ms(uint32_t msDelay);
uint32_t get_ms_timer(void);

#endif // __EVENT_H__
