/*
	CANHACKER port
	
	Портирование проекта CANHACKER на платформу LPC11Cxx.
	
	igorkov / fsp@igorkov.org / 2016

	Site: igorkov.org/bcomp11
 */
#include <stdint.h>
#include <LPC11xx.H>

void leds_init(void) {
	LPC_SYSCON->SYSAHBCLKCTRL |= (1UL <<  6);
	LPC_GPIO2->DIR  |=  (1UL<<9)|(1UL<<10);
	LPC_GPIO2->DATA &= ~((1UL<<9)|(1UL<<10));
}

void led_red(int state) {
	if (state)
		LPC_GPIO2->DATA |=  (1UL<<9);
	else
		LPC_GPIO2->DATA &= ~(1UL<<9);
}

void led_green(int state) {
	if (state)
		LPC_GPIO2->DATA |=  (1UL<<10);
	else
		LPC_GPIO2->DATA &= ~(1UL<<10);
}

void ProtectDelay(void) {
	int n;
	for (n = 0; n < 100000; n++) { __NOP(); } 
}

int main (void) {
	leds_init();
	while (1) {
		led_red(1);
		led_green(0);
		ProtectDelay();
		ProtectDelay();
		ProtectDelay();
		led_red(0);
		led_green(1);
		ProtectDelay();
		ProtectDelay();
		ProtectDelay();
	}
}
