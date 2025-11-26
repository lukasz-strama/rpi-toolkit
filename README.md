# rpi-toolkit

**rpi-toolkit** is a collection of portable, single-header C libraries designed for embedded systems labs on the Raspberry Pi 4B.

## Modules

| File | Description | Features |
| :--- | :--- | :--- |
| **`rpi_gpio.h`** | Hardware Abstraction Layer | Direct register access (MMIO), auto-mocking on PC, no root required (`/dev/gpiomem`). |
| **`simple_timer.h`** | Non-blocking Timing | `CLOCK_MONOTONIC` based, drift-free periodic execution, replaces `sleep()`. |

-----

## Quick Start

This example demonstrates the power of this toolkit: **Multitasking in a single thread.**
The LED blinks every 500ms, but the "Sensor" is read every 100ms. No threads, no blocking.

### `main.c`

```c
#include <stdio.h>
#include <unistd.h>

// 1. Implement the libraries (only in ONE .c file)
#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"

// Constants
#define LED_PIN 18
#define BLINK_INTERVAL 500
#define SENSOR_INTERVAL 100

int main() {
    // --- Setup ---
    if (gpio_init() != 0) {
        return 1;
    }
    
    pin_mode(LED_PIN, OUTPUT);
    
    // Timer instances
    simple_timer_t led_timer;
    simple_timer_t sensor_timer;
    
    // Start timers
    timer_set(&led_timer, BLINK_INTERVAL);
    timer_set(&sensor_timer, SENSOR_INTERVAL);

    int led_state = LOW;
    printf("System started. Press Ctrl+C to exit.\n");

    // --- Super Loop ---
    while (1) {
        
        // Task 1: Blink LED (Low Priority)
        if (timer_expired(&led_timer)) {
            led_state = !led_state;
            digital_write(LED_PIN, led_state);
            printf("[TASK 1] LED toggled to %d\n", led_state);
            
            // Restart timer for next cycle
            timer_set(&led_timer, BLINK_INTERVAL); 
        }

        // Task 2: Read "Sensors" (High Priority)
        if (timer_expired(&sensor_timer)) {
            // Simulate reading a button or sensor
            // int val = digital_read(BUTTON_PIN);
            printf("   [TASK 2] Scanning sensors...\n");
            
            timer_set(&sensor_timer, SENSOR_INTERVAL);
        }

        // CPU Idle (prevent 100% CPU usage in simple loops)
        usleep(100); 
    }

    gpio_cleanup();
    return 0;
}
```

-----

## Compilation

You can compile this identically on your Host PC and the Target RPi.

**1. On x86_64 Host PC (Simulation Mode):**

```bash
gcc main.c -o app
./app
```

*Output:*

```text
MOCK: gpio_init() called...
System started.
   [TASK 2] Scanning sensors...
   [TASK 2] Scanning sensors...
   [TASK 2] Scanning sensors...
   [TASK 2] Scanning sensors...
   [TASK 2] Scanning sensors...
[TASK 1] LED toggled to 1
MOCK: Pin 18 set to HIGH
```

**2. On Raspberry Pi (Hardware Mode):**

```bash
gcc main.c -o app
./app
```

*Output:* The physical LED will blink at 1Hz, while the console prints sensor logs at 10Hz.

-----

## Documentation

### `rpi_gpio.h`

  * **Init:** `gpio_init()`, `gpio_cleanup()`
  * **Control:** `pin_mode(pin, mode)`, `digital_write(pin, val)`, `digital_read(pin)`
  * **Note:** Uses `/dev/gpiomem`. Ensure your user is in the `gpio` group.

### `simple_timer.h`

  * **Types:** `simple_timer_t`
  * **Control:** `timer_set(&t, ms)`, `timer_expired(&t)`
  * **Utils:** `millis()`
  * **Note:** Uses `CLOCK_MONOTONIC` to prevent issues when the system clock updates via NTP/WiFi.

-----

## ⚖️ License

MIT License - Feel free to use this in your university projects.