#include "dht22_interrupt_driver.h"
#include "pin_mux.h"
#include <string.h>
#include "fsl_debug_console.h"
#include <stdlib.h>


static const uint8_t BIT_MAP_TAB[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};


void DHT22_Interrupt(DHT22_Sensor* dht22){
	GPIO_PinClearInterruptFlag(dht22->base, dht22->pin);

	if(dht22->is_measuring && !dht22->measures_ready){
		if(dht22->handshake_step < 3){
			++dht22->handshake_step;
			return;
		}

		if(!dht22->falling_edge){
			dht22->response_start_time = CTIMER0->TC;

			dht22->falling_edge = true;

		} else{

			if((CTIMER0->TC - dht22->response_start_time) > 30U){
				*(dht22->bit_tab + (dht22->bits_itter >> 3)) |= *(BIT_MAP_TAB + (dht22->bits_itter & 7));
			}
			++dht22->bits_itter;

			dht22->falling_edge = false;

			if(dht22->bits_itter == 40){
				dht22->is_measuring = false;
				dht22->measures_ready = true;
			}
		}
	}
}

static inline void DHT22_Reset_Flags(DHT22_Sensor* dht22){

	dht22->measures_ready = false;
	dht22->is_measuring = false;
	dht22->trigger_did = false;
	dht22->trigger_delay = false;
	dht22->response_start_time = 0;
	dht22->trigger_start_time = 0;
	dht22->handshake_step = 0;
	dht22->bits_itter = 0;
}


static void DHT22_Do_Trigger(DHT22_Sensor* dht22){
	if(!dht22->trigger_delay){

		dht22->trigger_delay = true;

		dht22->base->PCOR = (1U << dht22->pin);
		dht22->base->PDDR |= (1U << dht22->pin);

		dht22->trigger_start_time = CTIMER0->TC;

	}


	if((CTIMER0->TC - dht22->trigger_start_time) >= 1000){
		dht22->trigger_delay = false;
		dht22->base->PDDR &= ~(1U << dht22->pin);

		delay_us(5U);
		GPIO_PinClearInterruptFlag(dht22->base, dht22->pin);

		dht22->is_measuring = true;
		dht22->trigger_did = true;
	}
}



void DHT22_Set_Structure(DHT22_Sensor* dht22, GPIO_Type* base, uint8_t pin){
	memset(dht22, 0, sizeof(DHT22_Sensor));

	dht22->base = base;
	dht22->pin = pin;
}




static void DHT22_Process_Sensor_Data(DHT22_Sensor* dht22,uint32_t* raw_data) {

    uint16_t raw_humidity = (uint16_t)((*raw_data >> 16) & 0xFFFF);
    uint16_t raw_temperature = (uint16_t)(*raw_data & 0xFFFF);


    dht22->rh_integral = raw_humidity / 10;
    dht22->rh_decimal = raw_humidity % 10;

    int16_t temp_signed;

    if (raw_temperature & 0x8000) {
    	temp_signed = -(int16_t)(raw_temperature & 0x7FFF);
    } else {
    	temp_signed = (int16_t)raw_temperature;
    }

    dht22->temp_integral = temp_signed / 10;
    dht22->temp_decimal = abs(temp_signed) % 10;

}

g_Sensor_DHT22_Check_t DHT22_Get_Temperature_And_RH(DHT22_Sensor* dht22){
	if(!dht22->trigger_did){
		DHT22_Do_Trigger(dht22);
		return DHT22_DATA_NOT_READY;
	}

	if(!dht22->measures_ready){
		if((CTIMER0->TC - dht22->trigger_start_time) >= 20000U){

			DHT22_Reset_Flags(dht22);

			dht22->bit_tab[0] = 0; dht22->bit_tab[1] = 0; dht22->bit_tab[2] = 0; dht22->bit_tab[3] = 0; dht22->bit_tab[4] = 0;

			return DHT22_CHECK_SUM_ERROR;
		}
		return DHT22_DATA_NOT_READY;
	}

	else {

		DHT22_Reset_Flags(dht22);

		uint32_t raw_data = 0;

		if((uint8_t)(*dht22->bit_tab + *(dht22->bit_tab + 1) + *(dht22->bit_tab + 2) + *(dht22->bit_tab + 3)) == *(dht22->bit_tab + 4)) {
			raw_data = ((uint32_t)(*(dht22->bit_tab)) << 24) | ((uint32_t)(*(dht22->bit_tab + 1)) << 16) | ((uint32_t)(*(dht22->bit_tab + 2)) << 8) | ((uint32_t)(*(dht22->bit_tab + 3)));
		} else {
			dht22->bit_tab[0] = 0; dht22->bit_tab[1] = 0; dht22->bit_tab[2] = 0; dht22->bit_tab[3] = 0; dht22->bit_tab[4] = 0;
			return DHT22_CHECK_SUM_ERROR;
		}

		dht22->bit_tab[0] = 0; dht22->bit_tab[1] = 0; dht22->bit_tab[2] = 0; dht22->bit_tab[3] = 0; dht22->bit_tab[4] = 0;

		DHT22_Process_Sensor_Data(dht22,&raw_data);

		return DHT22_DATA_READY;
	}
}
