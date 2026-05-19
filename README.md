# High-Performance Asynchronous DHT22 Driver for NXP Microcontrollers

An optimized, fully non-blocking, interrupt-driven DHT22 (AM2302) sensor driver written in C for NXP Cortex-M microcontrollers. This driver eliminates standard blocking delays (`delay_ms`) during sensor readout, leveraging hardware timers (`CTIMER0`) and GPIO edge interrupts to ensure maximum CPU availability for concurrent application tasks.

## Key Features

* **100% Non-Blocking Design:** The CPU never wastes cycles spinning in a loop waiting for the DHT22's slow 18ms pulse or 40-bit transmission.
* **Interrupt-Driven Data Capture:** Edge-triggered GPIO interrupts dynamically decode incoming bits on the fly.
* **Ultra-Fast ISR Execution:** Employs direct bit-packing and pointer arithmetic inside the Interrupt Service Routine (ISR) to minimize interrupt latency.
* **Thread-Safe Soft-Locking:** Implements an internal memory isolation state machine (`is_measuring` flag gating) to completely eliminate race conditions between the ISR and the main loop thread.
* **Robust Error Handling:** Built-in hardware validation using 8-bit checksum verification.
* **No Floating-Point Operations:** Highly efficient integer-only calculations tailored for deeply embedded systems without an FPU.

---

## Architecture & Timing Diagram

The driver transitions through a lightweight state machine to manage the single-wire communication protocol safely without halting execution:

1. **Trigger Phase:** Pulls the data line down for 1ms via a non-blocking timestamp comparison.
2. **Handshake Phase:** Releases the line and skips the initialization pulses inside the ISR (`handshake_step < 3`).
3. **Data Acquisition Phase:** Gauges the duration of high-state pulses using `CTIMER0->TC`. If a high pulse lasts longer than 30 microseconds, a logic `1` is recorded; otherwise, it is a logic `0`.
4. **Data Processing:** Once all 40 bits are packed into `bit_tab`, the ISR locks further inputs, signals completion via `measures_ready`, and lets the user safely unpack temperature and humidity values.

---

## File Structure

* `dht22_interrupt_driver.h` — Contains hardware mapping definitions, driver configuration, and API definitions.
* `dht22_interrupt_driver.c` — Implements the state machine, asynchronous trigger, and the GPIO ISR.

---

## API Reference

### `uint32_t DHT22_Get_Temperature_And_RH(void)`
Polled in the main loop context. Manages the internal lifecycle of the sensor read operation.

**Return Values:**

| Value | Meaning | Description |
| :--- | :--- | :--- |
| `0` | `TRIGGER_ACTIVE` | Driver has just started the low-pulse trigger. |
| `1234` | `BUSY_ACQUIRING` | Sensor is currently streaming data; 40 bits are not yet complete. |
| `554` | `CHECK_SUM_ERROR` | Data payload failed the parity/checksum check. |
| `Other` | `SUCCESS_DATA` | Returns a packed 32-bit word containing raw metrics. |

---

## Integration Example (Fixed Integer Math)

Here is how to properly integrate the driver into your non-blocking main loop scheduler using pure integer arithmetic:

```c
#include <stdio.h>
#include "board.h"
#include "fsl_debug_console.h"
#include "dht22_interrupt_driver.h"

#define MEASUREMENT_INTERVAL_TICKS 2000000U // 2 seconds interval

int main(void) {
    /* Peripheral and Board Initialization */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    uint32_t last_measurement_time = 0;
    const uint32_t measurement_interval = MEASUREMENT_INTERVAL_TICKS;

    PRINTF("--- Asynchronous DHT22 System Active ---\r\n");

    while (1) {
        /* Non-blocking 2-second interval scheduler */
        if (CTIMER0->TC - last_measurement_time >= measurement_interval) {
            
            uint32_t raw_data = DHT22_Get_Temperature_And_RH();

            /* Check if the driver is still processing/waiting for the device */
            if (raw_data != 0 && raw_data != 1234) {
                
                if (raw_data == CHECK_SUM_ERROR) {
                    PRINTF("[ERROR] Checksum mismatch!\r\n");
                } else {
                    // Extract byte structures
                    uint16_t raw_humidity = (uint16_t)((raw_data >> 16) & 0xFFFF);
                    uint16_t raw_temperature = (uint16_t)(raw_data & 0xFFFF);
                    
                    // Pure Integer Calculations (No Float)
                    uint32_t hum_integral = raw_humidity / 10;
                    uint32_t hum_fractional = raw_humidity % 10;

                    int32_t temp_integral = 0;
                    int32_t temp_fractional = 0;

                    if (raw_temperature & 0x8000) {
                        uint16_t absolute_temp = raw_temperature & 0x7FFF;
                        temp_integral = -(int32_t)(absolute_temp / 10);
                        temp_fractional = (int32_t)(absolute_temp % 10);
                    } else {
                        temp_integral = (int32_t)(raw_temperature / 10);
                        temp_fractional = (int32_t)(raw_temperature % 10);
                    }

                    PRINTF("RH: %u.%u%% | Temp: %d.%d st.C\r\n", 
                           hum_integral, hum_fractional, temp_integral, temp_fractional);
                }

                /* Execution complete: update timer to start the next 2s window */
                last_measurement_time = CTIMER0->TC;
            }
        }

        /* Background Application Code Executes Here Instantly */
        // user_background_tasks();
    }
    return 0;
}
```
