#include <delay_us.h>

volatile uint32_t us = 0;


void SYSTICK_TIMER_REFERENCE_IRQHANDLER(void){
    if(us){
        us--;
    }
}

void delay_us(uint32_t n){
    us = n;
    while(us) {
        __asm("nop");
    }
}
