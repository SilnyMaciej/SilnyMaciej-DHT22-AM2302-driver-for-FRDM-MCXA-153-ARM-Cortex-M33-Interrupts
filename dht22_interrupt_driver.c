#include <dht22_interrupt_driver.h>
#include "pin_mux.h"



static volatile bool measures_ready = false;

static bool trigger_did = false;


static bool trigger_delay = false;

static volatile bool is_measuring = false;

static volatile uint32_t response_start_time = 0;

static volatile uint8_t bits_itter = 0;

static uint32_t trigger_start_time = 0;

static volatile uint8_t handshake_step = 0;

static volatile uint8_t bit_tab[5] = {0};


void INT_0_IRQHANDLER(void){
	if(GPIO_PinGetInterruptFlag(DHT22_GPIO, DHT22_PIN)){
		GPIO_PinClearInterruptFlag(DHT22_GPIO, DHT22_PIN);

		if (!is_measuring) {
			return;
		}

		if(!measures_ready){
			if(handshake_step < 3){
				++handshake_step;
				return;
			}

			if(DHT22_GPIO->PDIR & DHT22_PIN_MASK){
				response_start_time = CTIMER0->TC;
			} else{
				if((CTIMER0->TC - response_start_time) > 30U){
					 *(bit_tab + (bits_itter >> 3)) |= (1U << (7 - (bits_itter & 7)));
				}

				++bits_itter;

				if(bits_itter == 40){
					measures_ready = true;
					bits_itter = 0;
					handshake_step = 0;
				}
			}
		}
	}

}


static void DHT22_Do_Trigger(void){
	if(!trigger_delay){

		trigger_delay = true;

		DHT22_GPIO->PCOR = DHT22_PIN_MASK;
		DHT22_GPIO->PDDR |= DHT22_PIN_MASK;

		trigger_start_time = CTIMER0->TC;

	}


	if((CTIMER0->TC - trigger_start_time) >= 1000){
		trigger_delay = false;
		DHT22_GPIO->PDDR &= ~DHT22_PIN_MASK;

		delay_us(5U);
		GPIO_PinClearInterruptFlag(DHT22_GPIO, DHT22_PIN);

		is_measuring = true;
		trigger_did = true;
	}
}

uint32_t DHT22_Get_Temperature_And_RH(void){
	if(!trigger_did){
		DHT22_Do_Trigger();
		return 0;
	}

	if(!measures_ready) return 1234;
	else{
		is_measuring = false;

		uint32_t results = 0;

		if((uint8_t)(*bit_tab + *(bit_tab + 1) + *(bit_tab + 2) + *(bit_tab + 3)) == *(bit_tab + 4)) {
			results = ((uint32_t)(*(bit_tab)) << 24) | ((uint32_t)(*(bit_tab + 1)) << 16) | ((uint32_t)(*(bit_tab + 2)) << 8) | ((uint32_t)(*(bit_tab + 3)));
		} else results = CHECK_SUM_ERROR;

		measures_ready = false;

		trigger_did = false;

		trigger_delay = false;

		response_start_time = 0;

		trigger_start_time = 0;

		handshake_step = 0;

		bits_itter = 0;

		for(uint8_t i = 0; i < 5; i++) {
			bit_tab[i] = 0;
		}

		return results;
	}
}
