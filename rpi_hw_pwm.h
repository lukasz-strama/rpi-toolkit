/*
 * rpi_hw_pwm.h - Single-header C library for Hardware PWM on Raspberry Pi 4B
 *
 * Dependencies:
 *   - rpi_gpio.h (must be included before or linked)
 *   - Requires root privileges (sudo) to access /dev/mem
 *
 * Usage:
 *   #define RPI_HW_PWM_IMPLEMENTATION
 *   #include "rpi_hw_pwm.h"
 *
 *   ...
 *   hpwm_init();
 *   hpwm_set(18, 50, 150); // Pin 18, 50Hz, 15.0% duty
 *   ...
 *   hpwm_stop();
 */

#ifndef RPI_HW_PWM_H
#define RPI_HW_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

// API Declarations
int hpwm_init(void);
void hpwm_set(int pin, int freq_hz, int duty_per_mille);
void hpwm_stop(void);

#ifdef __cplusplus
}
#endif

#endif // RPI_HW_PWM_H

#ifdef RPI_HW_PWM_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Platform Detection
#if defined(__aarch64__) || defined(__arm__)
    #define RPI_HW_PWM_PLATFORM_RPI
#else
    #define RPI_HW_PWM_PLATFORM_HOST
#endif

#ifdef RPI_HW_PWM_PLATFORM_RPI
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>

    #define PERIPHERAL_BASE 0xFE000000
    #define PWM_OFFSET      0x20C000
    #define CLK_OFFSET      0x101000
    #define BLOCK_SIZE      (4*1024)

    #define PWM_CTL  0
    #define PWM_STA  1
    #define PWM_DMAC 2
    #define PWM_RNG1 4
    #define PWM_DAT1 5
    #define PWM_FIF1 6
    #define PWM_RNG2 8
    #define PWM_DAT2 9

    #define CM_PWMCTL 40
    #define CM_PWMDIV 41

    #define CM_PASSWD (0x5A << 24)

    static volatile uint32_t *pwm_map = NULL;
    static volatile uint32_t *clk_map = NULL;
    static int mem_fd_hw = -1;

#endif

int hpwm_init(void) {
#ifdef RPI_HW_PWM_PLATFORM_HOST
    printf("MOCK: hpwm_init() called.\n");
    return 0;
#else
    if ((mem_fd_hw = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        perror("Can't open /dev/mem (Need sudo?)");
        return -1;
    }

    // Map PWM
    pwm_map = (volatile uint32_t *)mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd_hw,
        PERIPHERAL_BASE + PWM_OFFSET
    );

    if (pwm_map == MAP_FAILED) {
        perror("mmap PWM error");
        close(mem_fd_hw);
        return -1;
    }

    // Map Clock Manager
    clk_map = (volatile uint32_t *)mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd_hw,
        PERIPHERAL_BASE + CLK_OFFSET
    );

    if (clk_map == MAP_FAILED) {
        perror("mmap CLK error");
        munmap((void*)pwm_map, BLOCK_SIZE);
        close(mem_fd_hw);
        return -1;
    }

    // Configure PWM Clock to 1 MHz
    // Stop PWM clock
    clk_map[CM_PWMCTL] = CM_PASSWD | 1; // Kill clock
    usleep(100);

    // Wait for busy flag
    while (clk_map[CM_PWMCTL] & 0x80) usleep(1);

    // Set Divisor for RPi 4B: Oscillator = 54 MHz
    // To get 1 MHz: 54 MHz / 54 = 1 MHz
    // DIVI = 54, DIVF = 0
    clk_map[CM_PWMDIV] = CM_PASSWD | (54 << 12) | 0;

    // Enable Clock (Source = Oscillator = 1)
    clk_map[CM_PWMCTL] = CM_PASSWD | 16 | 1; // Enable + Source 1
    usleep(100);

    return 0;
#endif
}

void hpwm_set(int pin, int freq_hz, int duty_per_mille) {
    // Parameter validation
    if (freq_hz <= 0) return;
    if (duty_per_mille < 0) duty_per_mille = 0;
    if (duty_per_mille > 1000) duty_per_mille = 1000;

#ifdef RPI_HW_PWM_PLATFORM_HOST
    printf("MOCK: HW PWM set on Pin %d to %d Hz, Duty %d/1000\n", pin, freq_hz, duty_per_mille);
#else
    if (!pwm_map || !clk_map) return;

    // Set Pin to ALT5 (PWM)
    gpio_set_function(pin, ALT5);

    // Determine Channel
    // PWM0: 12, 18
    // PWM1: 13, 19
    int channel = 0;
    if (pin == 13 || pin == 19) channel = 1;

    // Calculate Range and Data
    // Clock is 1MHz.
    // Range = 1,000,000 / freq
    uint32_t range = 1000000 / freq_hz;
    uint32_t data = (uint64_t)range * duty_per_mille / 1000;

    if (channel == 0) {
        // Disable Channel 1 first
        pwm_map[PWM_CTL] &= ~(1 << 0);
        usleep(10);
        
        pwm_map[PWM_RNG1] = range;
        pwm_map[PWM_DAT1] = data;

        // Enable Channel 1 in MS Mode (Mark-Space)
        // Bit 0: Enable, Bit 7: MS Mode
        pwm_map[PWM_CTL] |= (1 << 7) | (1 << 0);
    } else {
        // Disable Channel 2 first
        pwm_map[PWM_CTL] &= ~(1 << 8);
        usleep(10);

        pwm_map[PWM_RNG2] = range;
        pwm_map[PWM_DAT2] = data;

        // Enable Channel 2 in MS Mode
        // Bit 8: Enable, Bit 15: MS Mode
        pwm_map[PWM_CTL] |= (1 << 15) | (1 << 8);
    }
#endif
}

void hpwm_stop(void) {
#ifdef RPI_HW_PWM_PLATFORM_HOST
    printf("MOCK: hpwm_stop() called.\n");
#else
    if (pwm_map) {
        // Disable both channels
        pwm_map[PWM_CTL] = 0;
        munmap((void*)pwm_map, BLOCK_SIZE);
        pwm_map = NULL;
    }
    if (clk_map) {
        munmap((void*)clk_map, BLOCK_SIZE);
        clk_map = NULL;
    }
    if (mem_fd_hw >= 0) {
        close(mem_fd_hw);
        mem_fd_hw = -1;
    }
#endif
}

#endif // RPI_HW_PWM_IMPLEMENTATION
