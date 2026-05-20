/*
 * dht22_driver.h
 *
 *  Created on: 6 maj 2026
 *      Author: silnymaciej
 */

#ifndef DHT22_INTERRUPT_DRIVER_H_
#define DHT22_INTERRUPT_DRIVER_H_

#include "pin_mux.h"
#include "board.h"
#include "delay_us.h"

#define DHT22_1_GPIO 					BOARD_DHT22_INITPINS_dht22_GPIO
#define DHT22_1_PIN					    BOARD_DHT22_INITPINS_dht22_GPIO_PIN


typedef struct{
	GPIO_Type* base;
	uint8_t pin;

	volatile bool measures_ready;

	bool trigger_did;

	bool trigger_delay;

	volatile bool is_measuring;

	volatile uint32_t response_start_time;

	volatile uint8_t bits_itter;

	uint32_t trigger_start_time;

	volatile uint8_t handshake_step;

	volatile uint8_t bit_tab[5];
}DHT22_Sensor;



typedef enum{
	GND_ERROR = 112,
	VCC_ERROR = 445,
	CHECK_SUM_ERROR = 554
}g_Sensor_Check_t;


uint32_t DHT22_Get_Temperature_And_RH(DHT22_Sensor* dht22);
void DHT22_Interrupt(DHT22_Sensor* dht22);
void DHT22_Set_Structure(DHT22_Sensor* dht22, GPIO_Type* base, uint8_t pin);
void DHT22_Process_And_Print_Sensor_Data(uint32_t raw_data, const char* sensor_name);

#endif /* DHT22_INTERRUPT_DRIVER_H_ */
