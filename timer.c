#include <LPC11xx.h>
#include "timer.h"

void timer0_init(void) {
	LPC_SYSCON->SYSAHBCLKCTRL |= (1UL<<9);
	// timer init:
	LPC_TMR32B0->TC  = 0;
	LPC_TMR32B0->PR  = 48; // 1MHz ticks freq
	LPC_TMR32B0->TCR = 0x01;
}

uint32_t timer0_get(void) {
	return LPC_TMR32B0->TC;
}

void delay_mks(uint32_t mks) {
	uint32_t mksStart = LPC_TMR32B0->TC;
	while ((LPC_TMR32B0->TC-mksStart) < mks);
}
