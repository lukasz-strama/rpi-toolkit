# rpi-toolkit

Header-only C libraries for low-latency GPIO control on Raspberry Pi 4B, with Python bindings.

## Modules

| Module | Description |
|:-------|:------------|
| `rpi_gpio.h` | Direct memory-mapped I/O (MMIO) via `/dev/gpiomem` |
| `simple_timer.h` | `CLOCK_MONOTONIC`-based timing with µs precision |
| `rpi_pwm.h` | Multi-threaded software PWM on any GPIO pin |
| `rpi_hw_pwm.h` | DMA-based hardware PWM (requires root) |
| `rpi_realtime.h` | Optional jitter reduction (SCHED_FIFO, CPU affinity) |
| `rpi_toolkit.py` | ctypes wrapper for Python integration |

## Performance

GPIO toggle benchmark (10,000 iterations, Raspberry Pi 4B):

| Method | Frequency | Latency Factor |
|:-------|----------:|---------------:|
| rpi-toolkit (C) | 39.2 MHz | 1× (baseline) |
| rpi-toolkit (Python) | 140 kHz | 279× |
| lgpio (C) | 525 kHz | 74× |
| gpiozero (Python) | 37 kHz | 1055× |

## Usage

```c
#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"

#define RPI_PWM_IMPLEMENTATION
#include "rpi_pwm.h"

int main(void) {
    gpio_init();
    pwm_init(18);

    for (int duty = 0; duty <= 100; duty += 5) {
        pwm_write(18, duty);
        delay_us(50000);
    }

    pwm_stop(18);
    gpio_cleanup();
    return 0;
}
```

## Build

```bash
gcc main.c -o app -pthread
./app
```

Python (shared library):

```bash
make
python3 main.py
```

Hardware PWM requires `sudo`.

## API

### rpi_gpio.h

```c
int  gpio_init(void);                         // Returns 0 on success, -1 on error
void gpio_cleanup(void);
void pin_mode(int pin, int mode);             // INPUT=0, OUTPUT=1
void gpio_set_function(int pin, int func);    // ALT0-ALT5 for peripheral modes
void digital_write(int pin, int value);       // LOW=0, HIGH=1
int  digital_read(int pin);
```

### simple_timer.h

```c
void     timer_set(simple_timer_t *t, uint64_t interval_ms);
bool     timer_expired(simple_timer_t *t);    // Check only, does not reset
bool     timer_tick(simple_timer_t *t);       // Check and auto-advance (drift-compensated)
uint64_t millis(void);
uint64_t micros(void);
void     delay_ms(uint64_t ms);               // Busy-wait
void     delay_us(uint64_t us);               // Busy-wait
```

### rpi_pwm.h

```c
int  pwm_init(int pin);                       // 100 Hz default
int  pwm_init_freq(int pin, int freq_hz);     // Custom frequency
void pwm_write(int pin, int duty);            // 0-100%
void pwm_stop(int pin);
```

### rpi_hw_pwm.h

```c
int  hpwm_init(void);                                   // Returns 0 on success
void hpwm_set(int pin, int freq_hz, int duty_permille); // Duty in ‰ (0-1000)
void hpwm_stop(void);
```

Supported pins: 12, 13 (ALT0), 18, 19 (ALT5).

### rpi_realtime.h (Optional Jitter Reduction)

```c
int set_realtime_priority(void);  // Set SCHED_FIFO max priority (requires root)
int pin_to_core(int core_id);     // Pin thread to CPU core (0-3 on RPi 4)
int get_cpu_count(void);          // Get number of CPU cores
```

## Minimizing Jitter (Optional)

For timing-critical applications, you can reduce jitter using three techniques:

### 1. Real-Time Priority (SCHED_FIFO)

Switch to real-time scheduling to prevent other processes from interrupting your code:

```c
#define RPI_REALTIME_IMPLEMENTATION
#include "rpi_realtime.h"

int main() {
    set_realtime_priority();  // Requires sudo
    // ... timing-critical code ...
}
```

Python:
```python
from rpi_toolkit import set_realtime_priority
set_realtime_priority()  # Requires sudo
```

### 2. CPU Pinning (Core Affinity)

Prevent the scheduler from migrating your thread between cores:

```c
pin_to_core(3);  // Pin to core 3
```

Python:
```python
from rpi_toolkit import pin_to_core
pin_to_core(3)
```

### 3. Core Isolation (Kernel-Level)

For maximum effect, isolate a CPU core so Linux doesn't run **any** other processes on it:

1. Edit `/boot/cmdline.txt` (or `/boot/firmware/cmdline.txt` on newer OS)
2. Add `isolcpus=3` to the end of the existing line (do NOT create a new line)
3. Reboot

After reboot, core 3 is reserved. Combine with `pin_to_core(3)` for best results:

```c
int main() {
    set_realtime_priority();  // SCHED_FIFO
    pin_to_core(3);           // Use isolated core
    // Your code now runs with minimal jitter
}
```

> **Note**: To undo core isolation, remove `isolcpus=3` from cmdline.txt and reboot.

## BCM Pinout

| BCM | Phy | Function | | Phy | BCM | Function |
|:---:|:---:|:---------|---|:---:|:---:|:---------|
| — | 1 | 3.3V | | 2 | — | 5V |
| 2 | 3 | SDA | | 4 | — | 5V |
| 3 | 5 | SCL | | 6 | — | GND |
| 4 | 7 | GPCLK0 | | 8 | 14 | TXD |
| — | 9 | GND | | 10 | 15 | RXD |
| 17 | 11 | GPIO | | 12 | 18 | PWM0 |
| 27 | 13 | GPIO | | 14 | — | GND |
| 22 | 15 | GPIO | | 16 | 23 | GPIO |
| — | 17 | 3.3V | | 18 | 24 | GPIO |
| 10 | 19 | MOSI | | 20 | — | GND |
| 9 | 21 | MISO | | 22 | 25 | GPIO |
| 11 | 23 | SCLK | | 24 | 8 | CE0 |
| — | 25 | GND | | 26 | 7 | CE1 |
| 0 | 27 | ID_SD | | 28 | 1 | ID_SC |
| 5 | 29 | GPIO | | 30 | — | GND |
| 6 | 31 | GPIO | | 32 | 12 | PWM0 |
| 13 | 33 | PWM1 | | 34 | — | GND |
| 19 | 35 | PWM1 | | 36 | 16 | GPIO |
| 26 | 37 | GPIO | | 38 | 20 | GPIO |
| — | 39 | GND | | 40 | 21 | GPIO |

BCM = Broadcom pin numbering. Phy = physical header position (1-40).

## License

MIT