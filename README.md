# rpi-toolkit

**rpi-toolkit** is a collection of portable, single-header C libraries designed for embedded systems on the Raspberry Pi 4B. 
It includes a Python wrapper for scripting.

## Modules

| File | Description | Features |
| :--- | :--- | :--- |
| **`rpi_gpio.h`** | Hardware Abstraction Layer | Direct register access (MMIO), auto-mocking on PC, no root required (`/dev/gpiomem`). |
| **`simple_timer.h`** | Timing & Delays | `CLOCK_MONOTONIC` based, drift-free periodic execution, plus microsecond precision for sensors. |
| **`rpi_pwm.h`** | Software PWM | Multi-threaded PWM generation on any GPIO pin. Replaces `softPwm` from WiringPi. |
| **`rpi_hw_pwm.h`** | Hardware PWM | Jitter-free DMA-based PWM using `/dev/mem`. Requires root (`sudo`). |
| **`rpi_toolkit.py`**| Python Wrapper | `ctypes` binding to use all above C libraries directly in Python. |

---

## Performance Benchmarks

Comparison of `rpi-toolkit` against standard Raspberry Pi GPIO methods. 
Tests performed on Raspberry Pi 4B, toggling a GPIO pin 10,000 times.

| Method | Language | Library | Frequency | Speedup Factor |
| :--- | :--- | :--- | :--- | :--- |
| **rpi-toolkit** | **C** | **Direct MMIO** | **39,215,686 Hz (~39 MHz)** | **1x (Baseline)** |
| rpi-toolkit | Python | ctypes wrapper | 140,381 Hz (~140 kHz) | 279x slower |
| lgpio | C | Linux Kernel API | 525,292 Hz (~0.5 MHz) | 74x slower |
| gpiozero | Python | Standard Lib | 37,167 Hz (~37 kHz) | 1,055x slower |
| pinctrl | Bash | Sysfs/Pinctrl | 164 Hz | 238,766x slower |

**Conclusion:** `rpi-toolkit` in native C is approximately **74x faster** than the standard `lgpio` C library because it bypasses the Linux Kernel overhead using Direct Memory Access (DMA/MMIO).
Even the Python wrapper is **~3.8x faster** than the standard `gpiozero` library.

## Quick Start

This example demonstrates multitasking: blinking an LED, reading sensors, and pulsing a PWM pin simultaneously.

```c
#include <stdio.h>
#include <unistd.h>

// 1. Implement the libraries (only in ONE .c file)
#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"

#define RPI_PWM_IMPLEMENTATION
#include "rpi_pwm.h"

// Define this if you need Hardware PWM (Requires sudo)
// #define RPI_HW_PWM_IMPLEMENTATION
// #include "rpi_hw_pwm.h"

int main() {
    // Setup
    gpio_init();
    pwm_init(18); // Start Software PWM on Pin 18

    printf("System started. Press Ctrl+C to exit.\n");

    while (1) {
        // Fade LED logic using PWM
        for (int i = 0; i <= 100; i += 5) {
            pwm_write(18, i);
            delay_us(50000); // Wait 50ms (using busy-wait for precision)
        }
        for (int i = 100; i >= 0; i -= 5) {
            pwm_write(18, i);
            delay_us(50000);
        }
    }

    pwm_stop(18);
    gpio_cleanup();
    return 0;
}
```

## Compilation

Since `rpi_pwm.h` uses threads, you must link with the `pthread` library.

### 1. On x86_64 (Simulation Mode):

```Bash
gcc main.c -o app -pthread
./app
```

Output:

```Plaintext
MOCK: gpio_init() called...
MOCK: PWM initialized on Pin 18
MOCK: PWM on Pin 18 updated to 5%
...
```

2. On Raspberry Pi (Hardware Mode):

```Bash
gcc main.c -o app -pthread
./app
```

(Note: If using `rpi_hw_pwm.h`, run with `sudo ./app`)

## Python Support

You can use these libraries in Python thanks to the included ctypes wrapper. This gives you the syntax of Python with the performance of C.

### 1. Build the Shared Library

```Bash
make
```

### 2. Run the Python Script

```Bash
python3 main.py
```

(Note: If using `hpwm_*` functions, run `sudo python3 main.py`)

## API Reference

simple_timer.h

```
timer_set(&t, ms): Start/Reset a timer.

timer_tick(&t): Returns true if expired and automatically advances the timer (drift-free).

micros(): Returns uptime in microseconds.

delay_us(us): precise delay (busy-wait) for timing-critical protocols.
```

rpi_pwm.h (Software PWM)

```
pwm_init(pin): Starts a background thread for PWM on selected pin.

pwm_write(pin, duty): Sets duty cycle (0-100).

pwm_stop(pin): Stops the thread and cleans up.
```

rpi_hw_pwm.h (Hardware PWM)

```
hpwm_init(): Initializes DMA/Clock for jitter-free PWM.

hpwm_set(pin, freq, duty_permille): Sets frequency (Hz) and duty (0-1000).

hpwm_stop(): Disables controller.
```

rpi_gpio.h

```
pin_mode(pin, mode), digital_write(pin, val), digital_read(pin).
```

## GPIO Pinout Reference (BCM vs Physical)

| BCM (Code) | Role        | Phy |   | Phy | Role        | BCM (Code) |
| :---:      | :---        | :---: |---| :---: | :---        | :---:      |
| **-** | **3.3V** |  1  | . |  2  | **5V** | **-** |
| **2** | SDA (I2C)   |  3  | . |  4  | **5V** | **-** |
| **3** | SCL (I2C)   |  5  | . |  6  | **GND** | **-** |
| **4** | GPCLK0      |  7  | . |  8  | TXD (UART)  | **14** |
| **-** | **GND** |  9  | . | 10  | RXD (UART)  | **15** |
| **17** | GPIO        | 11  | . | 12  | PWM0 / GPIO | **18** |
| **27** | GPIO        | 13  | . | 14  | **GND** | **-** |
| **22** | GPIO        | 15  | . | 16  | GPIO        | **23** |
| **-** | **3.3V** | 17  | . | 18  | GPIO        | **24** |
| **10** | MOSI (SPI)  | 19  | . | 20  | **GND** | **-** |
| **9** | MISO (SPI)  | 21  | . | 22  | GPIO        | **25** |
| **11** | SCLK (SPI)  | 23  | . | 24  | CE0 (SPI)   | **8** |
| **-** | **GND** | 25  | . | 26  | CE1 (SPI)   | **7** |
| **0** | ID_SD       | 27  | . | 28  | ID_SC       | **1** |
| **5** | GPIO        | 29  | . | 30  | **GND** | **-** |
| **6** | GPIO        | 31  | . | 32  | PWM0 / GPIO | **12** |
| **13** | PWM1 / GPIO | 33  | . | 34  | **GND** | **-** |
| **19** | PWM1 / MISO | 35  | . | 36  | GPIO        | **16** |
| **26** | GPIO        | 37  | . | 38  | MOSI / GPIO | **20** |
| **-** | **GND** | 39  | . | 40  | SCLK / GPIO | **21** |

* **BCM:** The number you use in `pin_mode(X, ...)` and `digital_write(X, ...)`.
* **Phy:** The physical pin number on the board header (1-40).

## License

MIT License - Feel free to use this in your university projects.