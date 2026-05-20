#ifndef DELAY_US_H_
#define DELAY_US_H_

#include <stdint.h>
#include "peripherals.h"

extern volatile uint32_t us;

void delay_us(uint32_t n);

#endif /* DELAY_US_H_ */
