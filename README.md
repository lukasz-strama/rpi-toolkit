# Lightweight GPIO Library for Raspberry Pi 4B

**rpi\_gpio** is a portable, single-header C library designed for low-level GPIO control on the Raspberry Pi 4B. It features a **dual-mode architecture**:

1.  **Host Mode (PC/Fedora):** Simulates GPIO operations by printing logs to the console (Mocking).
2.  **Target Mode (Raspberry Pi):** Controls physical hardware using direct register access via `/dev/gpiomem`.

## Features

  * **Zero Dependencies:** No `libgpiod`, `wiringPi`, or `bcm2835` required. Just standard C libraries.
  * **Direct Memory Access (MMIO):** Uses `mmap` for high-performance register manipulation.
  * **Safe User-Space Access:** Uses `/dev/gpiomem`, allowing execution without `sudo` (root) privileges.
  * **Portable:** Compile the same code on your x86_64 and your Raspberry Pi without any changes.

## Integration

This is an **STB-style single-header library**. To use it, you must define the implementation macro in **one** source file before including the header.

### `main.c` Example

```c
#include <stdio.h>
#include <unistd.h> // for sleep

// Define this ONLY in one .c file (e.g., main.c)
#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

int main() {
    // 1. Initialize the library
    if (gpio_init() != 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return 1;
    }

    // 2. Setup Pin 18 as Output
    pin_mode(18, OUTPUT);

    // 3. Toggle LED
    for (int i = 0; i < 5; i++) {
        digital_write(18, HIGH);
        sleep(1);
        digital_write(18, LOW);
        sleep(1);
    }

    // 4. Cleanup memory mapping
    gpio_cleanup();
    return 0;
}
```

## Compilation

### 1\. On x86/x64 PC (Host)

The library automatically detects the x86/x64 architecture and switches to **Simulation Mode**.

```bash
gcc main.c -o app
./app
```

**Output:**

```text
MOCK: gpio_init() called. Simulation mode active.
Starting blink program on Pin 18...
MOCK: Pin 18 set to OUTPUT
MOCK: Pin 18 set to HIGH
...
```

### 2\. On Raspberry Pi (Target)

The library detects the ARM architecture and switches to **Hardware Mode**.

```bash
gcc main.c -o app
./app
```

**Output:**

```text
(The LED connected to GPIO 18 starts blinking)
```

## API Reference

| Function | Description |
| :--- | :--- |
| `int gpio_init(void)` | Initializes the memory map. Must be called first. Returns 0 on success. |
| `void gpio_cleanup(void)` | Unmaps memory and closes file descriptors. Call before exiting. |
| `void pin_mode(int pin, int mode)` | Sets pin direction. Modes: `INPUT` (0) or `OUTPUT` (1). |
| `void digital_write(int pin, int val)` | Sets pin state. Values: `LOW` (0) or `HIGH` (1). |
| `int digital_read(int pin)` | Reads current pin level. Returns 0 or 1. |

## Important Hardware Notes

  * **Pin Numbering:** This library uses **BCM numbering** (GPIO numbers), not physical header pin numbers. (e.g., GPIO 18 is physical pin 12).
  * **Pull-up/Pull-down Resistors:** This library does **not** configure internal pull-up/pull-down resistors.
      * *Recommendation:* When using buttons (`INPUT`), always use an external pull-up or pull-down resistor on your breadboard to avoid floating signals.

## Troubleshooting

**Error: `Can't open /dev/gpiomem: Permission denied`**

  * Ensure your user is in the `gpio` group (`sudo usermod -aG gpio $USER`).
  * Try running with `sudo ./app` (though usually not required).

**Linker Error: `undefined reference to 'gpio_init'`**

  * You forgot to add `#define RPI_GPIO_IMPLEMENTATION` before `#include "rpi_gpio.h"` in your `main.c`.
