/**
 * @file rpi_gpio.h
 * @brief GPIO control via direct memory-mapped I/O for Raspberry Pi 4B.
 *
 * Single-header library. Define RPI_GPIO_IMPLEMENTATION in exactly one
 * translation unit before including this file.
 *
 * Uses /dev/gpiomem (no root required). On x86/x64, provides mock
 * implementations for development.
 */

#ifndef RPI_GPIO_H
#define RPI_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/** @name Pin Modes */
/**@{*/
#define INPUT  0
#define OUTPUT 1
#define ALT0   4
#define ALT1   5
#define ALT2   6
#define ALT3   7
#define ALT4   3
#define ALT5   2
/**@}*/

/** @name Logic Levels */
/**@{*/
#define LOW  0
#define HIGH 1
/**@}*/

#define GPIO_PIN_MIN 0
#define GPIO_PIN_MAX 53

/** @name Register Calculation Constants */
/**@{*/
#define GPIO_PINS_PER_FSEL_REG 10
#define GPIO_PINS_PER_BANK     32
#define FSEL_BITS_PER_PIN      3
#define FSEL_MASK              0b111
/**@}*/

/** @name Helper Macros */
/**@{*/
/** Function select register index for a pin. */
#define GPIO_FSEL_REG(pin)    ((pin) / GPIO_PINS_PER_FSEL_REG)
/** Bit offset within function select register. */
#define GPIO_FSEL_SHIFT(pin)  (((pin) % GPIO_PINS_PER_FSEL_REG) * FSEL_BITS_PER_PIN)
/** Bank index (0 or 1) for pins 0-31 or 32-53. */
#define GPIO_BANK(pin)        ((pin) < GPIO_PINS_PER_BANK ? 0 : 1)
/** Bit position within a bank register. */
#define GPIO_BIT(pin)         ((pin) < GPIO_PINS_PER_BANK ? (pin) : ((pin) - GPIO_PINS_PER_BANK))
/** Validate pin is in valid range. */
#define GPIO_VALID_PIN(pin)   ((pin) >= GPIO_PIN_MIN && (pin) <= GPIO_PIN_MAX)
/**@}*/

/**
 * @brief Initialize GPIO subsystem.
 * @return 0 on success, -1 on error.
 */
int gpio_init(void);

/**
 * @brief Release GPIO resources.
 */
void gpio_cleanup(void);

/**
 * @brief Set pin direction.
 * @param pin BCM pin number (0-53).
 * @param mode INPUT or OUTPUT.
 */
void pin_mode(int pin, int mode);

/**
 * @brief Set pin alternate function.
 * @param pin BCM pin number (0-53).
 * @param function ALT0-ALT5 for peripheral modes.
 */
void gpio_set_function(int pin, int function);

/**
 * @brief Write digital output.
 * @param pin BCM pin number.
 * @param value LOW or HIGH.
 */
void digital_write(int pin, int value);

/**
 * @brief Read digital input.
 * @param pin BCM pin number.
 * @return LOW or HIGH.
 */
int digital_read(int pin);

#ifdef __cplusplus
}
#endif

#endif /* RPI_GPIO_H */

#ifdef RPI_GPIO_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define RPI_GPIO_PLATFORM_HOST
#elif defined(__aarch64__) || defined(__arm__)
    #define RPI_GPIO_PLATFORM_RPI
#else
    #define RPI_GPIO_PLATFORM_HOST
#endif

#ifdef RPI_GPIO_PLATFORM_RPI
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>

    #define BLOCK_SIZE (4*1024)

    #define GPSET0  7
    #define GPSET1  8
    #define GPCLR0  10
    #define GPCLR1  11
    #define GPLEV0  13
    #define GPLEV1  14

    static volatile uint32_t *gpio_map = NULL;
    static int mem_fd = -1;

#endif

int gpio_init(void) {
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: gpio_init() called. Simulation mode active.\n");
    return 0;
#else
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        perror("Can't open /dev/gpiomem");
        return -1;
    }

    gpio_map = (volatile uint32_t *)mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        0
    );

    if (gpio_map == MAP_FAILED) {
        perror("mmap error");
        close(mem_fd);
        return -1;
    }
    return 0;
#endif
}

void gpio_cleanup(void) {
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: gpio_cleanup() called.\n");
#else
    if (gpio_map) {
        munmap((void*)gpio_map, BLOCK_SIZE);
        gpio_map = NULL;
    }
    if (mem_fd >= 0) {
        close(mem_fd);
        mem_fd = -1;
    }
#endif
}

void pin_mode(int pin, int mode) {
    if (!GPIO_VALID_PIN(pin)) return;
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Pin %d set to %s\n", pin, mode == INPUT ? "INPUT" : "OUTPUT");
#else
    if (!gpio_map) return;

    volatile uint32_t* fsel_reg = gpio_map + GPIO_FSEL_REG(pin);
    int shift = GPIO_FSEL_SHIFT(pin);

    uint32_t val = *fsel_reg;
    val &= ~(FSEL_MASK << shift);
    if (mode == OUTPUT) {
        val |= (1 << shift);  /* OUTPUT = 001 */
    }
    *fsel_reg = val;
#endif
}

void gpio_set_function(int pin, int function) {
    if (!GPIO_VALID_PIN(pin)) return;
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Pin %d set to Function %d\n", pin, function);
#else
    if (!gpio_map) return;

    volatile uint32_t* fsel_reg = gpio_map + GPIO_FSEL_REG(pin);
    int shift = GPIO_FSEL_SHIFT(pin);

    uint32_t val = *fsel_reg;
    val &= ~(FSEL_MASK << shift);
    val |= (function << shift);
    *fsel_reg = val;
#endif
}

void digital_write(int pin, int value) {
    if (!GPIO_VALID_PIN(pin)) return;
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Pin %d set to %s\n", pin, value == HIGH ? "HIGH" : "LOW");
#else
    if (!gpio_map) return;

    int bank = GPIO_BANK(pin);
    int bit = GPIO_BIT(pin);

    if (value == HIGH) {
        volatile uint32_t* set_reg = gpio_map + GPSET0 + bank;
        *set_reg = (1 << bit);
    } else {
        volatile uint32_t* clr_reg = gpio_map + GPCLR0 + bank;
        *clr_reg = (1 << bit);
    }
#endif
}

int digital_read(int pin) {
    if (!GPIO_VALID_PIN(pin)) return LOW;
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Reading Pin %d (returning LOW)\n", pin);
    return LOW;
#else
    if (!gpio_map) return LOW;

    int bank = GPIO_BANK(pin);
    int bit = GPIO_BIT(pin);

    volatile uint32_t* lev_reg = gpio_map + GPLEV0 + bank;
    return (*lev_reg & (1 << bit)) ? HIGH : LOW;
#endif
}

#endif /* RPI_GPIO_IMPLEMENTATION */
