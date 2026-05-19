/*
 * dht22_driver.h
 *
 *  Created on: 6 maj 2026
 *      Author: silnymaciej
 */

/*
*
*
*  WORK IN PROGRESS
*
*/

#ifndef DHT22_INTERRUPT_DRIVER_H_
#define DHT22_INTERRUPT_DRIVER_H_

#include "pin_mux.h"
#include "board.h"
#include "delay_us.h"

#define DHT22_GPIO 					BOARD_DHT22_INITPINS_dht22_GPIO
#define DHT22_PIN_MASK 				BOARD_DHT22_INITPINS_dht22_GPIO_PIN_MASK
#define DHT22_PIN					BOARD_DHT22_INITPINS_dht22_GPIO_PIN

typedef enum{
	GND_ERROR = 112,
	VCC_ERROR = 445,
	CHECK_SUM_ERROR = 554
}g_Sensor_Check_t;


uint32_t DHT22_Get_Temperature_And_RH(void);


#endif /* DHT22_INTERRUPT_DRIVER_H_ */
