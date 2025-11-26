/*
 * rpi_gpio.h - Single-header C library for Raspberry Pi 4B GPIO control
 *
 * Usage:
 *   #define RPI_GPIO_IMPLEMENTATION
 *   #include "rpi_gpio.h"
 *
 *   ...
 *   gpio_init();
 *   pin_mode(18, OUTPUT);
 *   digital_write(18, HIGH);
 *   ...
 */

#ifndef RPI_GPIO_H
#define RPI_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

// API Declarations
int gpio_init(void);
void gpio_cleanup(void);
void pin_mode(int pin, int mode);
void gpio_set_function(int pin, int function);
void digital_write(int pin, int value);
int digital_read(int pin);

// Constants
#define INPUT 0
#define OUTPUT 1
#define ALT0 4
#define ALT1 5
#define ALT2 6
#define ALT3 7
#define ALT4 3
#define ALT5 2

#define LOW 0
#define HIGH 1

#ifdef __cplusplus
}
#endif

#endif // RPI_GPIO_H

#ifdef RPI_GPIO_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Architecture Detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define RPI_GPIO_PLATFORM_HOST
#elif defined(__aarch64__) || defined(__arm__)
    #define RPI_GPIO_PLATFORM_RPI
#else
    // Default to HOST if unknown, for safety
    #define RPI_GPIO_PLATFORM_HOST
#endif

#ifdef RPI_GPIO_PLATFORM_RPI
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>

    #define BLOCK_SIZE (4*1024)

    // Global variables for GPIO access
    static volatile uint32_t *gpio_map = NULL;
    static int mem_fd = -1;

#endif // RPI_GPIO_PLATFORM_RPI

int gpio_init(void) {
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: gpio_init() called. Simulation mode active.\n");
    return 0;
#else
    // Open /dev/gpiomem
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        perror("Can't open /dev/gpiomem");
        return -1;
    }

    // mmap GPIO
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
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Pin %d set to %s\n", pin, mode == INPUT ? "INPUT" : "OUTPUT");
#else
    if (!gpio_map) return;
    
    // GPFSELn register selection
    // Each register controls 10 pins. 3 bits per pin.
    // GPFSEL0: Pins 0-9
    // GPFSEL1: Pins 10-19
    // ...
    int reg_index = pin / 10;
    int bit_offset = (pin % 10) * 3;

    volatile uint32_t* fsel_reg = gpio_map + reg_index;
    
    uint32_t val = *fsel_reg;
    val &= ~(0b111 << bit_offset); // Clear 3 bits
    if (mode == OUTPUT) {
        val |= (0b001 << bit_offset); // Set to Output (001)
    }
    // Input is 000, so we just leave it cleared
    *fsel_reg = val;
#endif
}

void gpio_set_function(int pin, int function) {
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Pin %d set to Function %d\n", pin, function);
#else
    if (!gpio_map) return;

    int reg_index = pin / 10;
    int bit_offset = (pin % 10) * 3;

    volatile uint32_t* fsel_reg = gpio_map + reg_index;
    
    uint32_t val = *fsel_reg;
    val &= ~(0b111 << bit_offset); // Clear 3 bits
    val |= (function << bit_offset); // Set function
    *fsel_reg = val;
#endif
}

void digital_write(int pin, int value) {
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Pin %d set to %s\n", pin, value == HIGH ? "HIGH" : "LOW");
#else
    if (!gpio_map) return;

    // GPSETn / GPCLRn
    // Register 0 controls pins 0-31
    // Register 1 controls pins 32-53
    int reg_offset = (pin < 32) ? 0 : 1;
    int shift = (pin < 32) ? pin : (pin - 32);

    if (value == HIGH) {
        // GPSET0 is at offset 0x1C. 0x1C / 4 = 7.
        volatile uint32_t* set_reg = gpio_map + 7 + reg_offset;
        *set_reg = (1 << shift);
    } else {
        // GPCLR0 is at offset 0x28. 0x28 / 4 = 10.
        volatile uint32_t* clr_reg = gpio_map + 10 + reg_offset;
        *clr_reg = (1 << shift);
    }
#endif
}

int digital_read(int pin) {
#ifdef RPI_GPIO_PLATFORM_HOST
    printf("MOCK: Reading Pin %d (returning LOW)\n", pin);
    return LOW;
#else
    if (!gpio_map) return LOW;

    // GPLEV0 is at offset 0x34. 0x34 / 4 = 13.
    int reg_offset = (pin < 32) ? 0 : 1;
    int shift = (pin < 32) ? pin : (pin - 32);
    
    volatile uint32_t* lev_reg = gpio_map + 13 + reg_offset;
    return (*lev_reg & (1 << shift)) ? HIGH : LOW;
#endif
}

#endif // RPI_GPIO_IMPLEMENTATION
