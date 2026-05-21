# Asynchronous DHT22 Driver for Embedded Systems

## Project Overview

This project implements an asynchronous, non-blocking DHT22 sensor driver for microcontrollers. It utilizes external GPIO interrupts and a hardware timer (e.g., CTIMER0) to manage sensor communication. 

The primary advantage of this architecture is the complete elimination of CPU blocking. By utilizing a state machine and interrupt service routines (ISR), the system avoids `delay()` functions during the data acquisition phase. This allows the microcontroller to handle multiple sensors simultaneously while leaving the main loop free to execute other tasks.

## Key Features

* **Non-Blocking Architecture:** Communication with the sensor is handled within an interrupt routine that merely records edge timings and advances the state machine.
* **Lightweight ISR:** The interrupt service routine executes in a fraction of a microsecond, ensuring system stability even with multiple sensors active.
* **Integer-Only Mathematics:** Raw data processing is performed using fast integer operations, avoiding computationally expensive floating-point (`float`) operations.
* **Scalability:** The system uses a `DHT22_Sensor` structure, allowing the addition of new sensors simply by declaring and initializing new structure instances.

## Architecture Description

1. **`DHT22_Sensor` (Structure):** Maintains the context for each sensor instance, including state flags, timing variables, and the bit buffer.
2. **`GPIO_IRQHandler`:** The common entry point for external interrupts. It acts as a dispatcher, delegating execution to the appropriate sensor instance via pointers.
3. **`DHT22_Get_Temperature_And_RH`:** A non-blocking polling function designed to be called within the main loop. It handles the trigger sequence and evaluates the final data.

## Implementation Example (main.c)

```c
#include "dht22_interrupt_driver.h"

// 1. Declare sensor instances
static DHT22_Sensor dht22_1;
static DHT22_Sensor dht22_2;

// 2. Initialize structures. WARNING STRUCTURES MUST BE INITIALIZED BEFORE BOARD_InitBootPeripherals() !!!!!!!!
    DHT22_Set_Structure(&dht22_1, DHT22_1_GPIO, DHT22_1_PIN);
    DHT22_Set_Structure(&dht22_2, DHT22_2_GPIO, DHT22_2_PIN);

int main(void) {
    // Hardware initialization (clocks, pins, etc.)
    // ...

    uint32_t last_measurement_time_1 = 0;
    uint32_t last_measurement_time_2 = 0;
    const uint32_t measurement_interval = 2000000U; // e.g., 2 seconds

    while (1) {
        // 3. Poll Sensor 1
        if (CTIMER0->TC - last_measurement_time_1 >= measurement_interval) {
            uint32_t raw_data = DHT22_Get_Temperature_And_RH(&dht22_1);

            // 0 = measuring, 1234 = waiting/timeout
            if (raw_data != 0 && raw_data != 1234) {
                DHT22_Process_And_Print_Sensor_Data(raw_data, "SENSOR 1");
                last_measurement_time_1 = CTIMER0->TC;
            }
        }

        // 4. Poll Sensor 2
        if (CTIMER0->TC - last_measurement_time_2 >= measurement_interval) {
            uint32_t raw_data = DHT22_Get_Temperature_And_RH(&dht22_2);

            if (raw_data != 0 && raw_data != 1234) {
                DHT22_Process_And_Print_Sensor_Data(raw_data, "SENSOR 2");
                last_measurement_time_2 = CTIMER0->TC;
            }
        }
    }
    return 0;
}

// 5. Shared Interrupt Handler
void GPIO3_IRQHandler(void) {
    DHT22_Interrupt(&dht22_1);
    DHT22_Interrupt(&dht22_2);
}
```
