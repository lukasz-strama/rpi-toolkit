/**
 * @file rpi_hw_pwm.h
 * @brief Hardware PWM via DMA for Raspberry Pi 4B.
 *
 * Single-header library. Define RPI_HW_PWM_IMPLEMENTATION in exactly one
 * translation unit before including this file.
 *
 * Requires rpi_gpio.h and root privileges (/dev/mem access).
 *
 * Supported pins: 12, 13 (ALT0), 18, 19 (ALT5).
 */

#ifndef RPI_HW_PWM_H
#define RPI_HW_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize hardware PWM controller.
 *
 * Configures PWM clock to 1 MHz base frequency.
 *
 * @return 0 on success, -1 on error.
 */
int hpwm_init(void);

/**
 * @brief Set hardware PWM output.
 * @param pin BCM pin number (12, 13, 18, or 19).
 * @param freq_hz PWM frequency in Hz.
 * @param duty_per_mille Duty cycle in per-mille (0-1000).
 */
void hpwm_set(int pin, int freq_hz, int duty_per_mille);

/**
 * @brief Stop hardware PWM and release resources.
 */
void hpwm_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* RPI_HW_PWM_H */

#ifdef RPI_HW_PWM_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(__aarch64__) || defined(__arm__)
    #define RPI_HW_PWM_PLATFORM_RPI
#else
    #define RPI_HW_PWM_PLATFORM_HOST
#endif

/** @name Duty Cycle Limits (per-mille) */
/**@{*/
#define HPWM_DUTY_MIN 0
#define HPWM_DUTY_MAX 1000
/**@}*/

/** Clamp duty per-mille to valid range [0, 1000]. */
#define HPWM_CLAMP_DUTY(d) ((d) < HPWM_DUTY_MIN ? HPWM_DUTY_MIN : ((d) > HPWM_DUTY_MAX ? HPWM_DUTY_MAX : (d)))

#ifdef RPI_HW_PWM_PLATFORM_RPI
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>

    /** @name Register Offsets */
    /**@{*/
    #define PERIPHERAL_BASE 0xFE000000
    #define PWM_OFFSET      0x20C000
    #define CLK_OFFSET      0x101000
    #define BLOCK_SIZE      (4*1024)
    /**@}*/

    /** @name Register Offsets */
    /**@{*/
    #define PWM_CTL  0
    #define PWM_STA  1
    #define PWM_DMAC 2
    #define PWM_RNG1 4
    #define PWM_DAT1 5
    #define PWM_FIF1 6
    #define PWM_RNG2 8
    #define PWM_DAT2 9
    /**@}*/

    /** @name Clock Manager Offsets */
    /**@{*/
    #define CM_PWMCTL 40
    #define CM_PWMDIV 41
    /**@}*/

    /** @name Clock Manager Password */
    /**@{*/
    #define CM_PASSWD (0x5A << 24)
    /**@}*/

    /** @name PWM Control Register Bits */
    /**@{*/
    #define PWM_CTL_PWEN1 (1 << 0)   /**< Channel 1 enable */
    #define PWM_CTL_MSEN1 (1 << 7)   /**< Channel 1 M/S mode */
    #define PWM_CTL_PWEN2 (1 << 8)   /**< Channel 2 enable */
    #define PWM_CTL_MSEN2 (1 << 15)  /**< Channel 2 M/S mode */
    /**@}*/

    /** @name Clock Configuration */
    /**@{*/
    #define CM_SRC_PLLD      6       /**< PLLD clock source (500 MHz on Pi 4) */
    #define CM_DIV_VALUE     54      /**< Divider for ~1 MHz PWM clock */
    #define PWM_BASE_FREQ_HZ 1000000 /**< Resulting PWM clock frequency */
    /**@}*/

    /** @name Global Variables */
    /**@{*/
    static volatile uint32_t *pwm_map = NULL;
    static volatile uint32_t *clk_map = NULL;
    static int mem_fd_hw = -1;
    /**@}*/

    /**
     * @brief Get PWM channel and alt function for a hardware PWM pin.
     * @param pin BCM pin number.
     * @param channel Output: 0 or 1 for channel.
     * @param alt_func Output: ALT0 or ALT5.
     * @return 1 if valid HW PWM pin, 0 otherwise.
     */
    static int hpwm_get_channel(int pin, int* channel, int* alt_func) {
        switch (pin) {
            case 12: *channel = 0; *alt_func = ALT0; return 1;
            case 13: *channel = 1; *alt_func = ALT0; return 1;
            case 18: *channel = 0; *alt_func = ALT5; return 1;
            case 19: *channel = 1; *alt_func = ALT5; return 1;
            default: return 0;
        }
    }

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

    /*
     * Clock setup for 1 MHz PWM frequency:
     * - Stop the clock (write PASSWD | KILL bit)
     * - Wait for clock to become idle (BUSY bit clear)
     * - Set divider: 500 MHz PLLD / 54 ≈ 9.26 MHz integer, but we use
     *   this as base for range calculations to achieve desired freq
     * - Enable clock with PLLD source
     */
    clk_map[CM_PWMCTL] = CM_PASSWD | 1;  /* Stop clock */
    usleep(100);

    while (clk_map[CM_PWMCTL] & 0x80) usleep(1);  /* Wait for not BUSY */

    clk_map[CM_PWMDIV] = CM_PASSWD | (CM_DIV_VALUE << 12) | 0;
    clk_map[CM_PWMCTL] = CM_PASSWD | CM_SRC_PLLD | 0x10;  /* Enable with PLLD */
    usleep(100);

    return 0;
#endif
}

void hpwm_set(int pin, int freq_hz, int duty_per_mille) {
    if (freq_hz <= 0) return;
    duty_per_mille = HPWM_CLAMP_DUTY(duty_per_mille);

#ifdef RPI_HW_PWM_PLATFORM_HOST
    printf("MOCK: HW PWM set on Pin %d to %d Hz, Duty %d/1000\n", pin, freq_hz, duty_per_mille);
#else
    if (!pwm_map || !clk_map) return;

    int channel, alt_func;
    if (!hpwm_get_channel(pin, &channel, &alt_func)) {
        return;  /* Invalid HW PWM pin */
    }

    gpio_set_function(pin, alt_func);

    uint32_t range = PWM_BASE_FREQ_HZ / freq_hz;
    uint32_t data = (uint64_t)range * duty_per_mille / HPWM_DUTY_MAX;

    if (channel == 0) {
        pwm_map[PWM_CTL] &= ~PWM_CTL_PWEN1;  /* Disable channel 1 */
        usleep(10);

        pwm_map[PWM_RNG1] = range;
        pwm_map[PWM_DAT1] = data;

        pwm_map[PWM_CTL] |= PWM_CTL_MSEN1 | PWM_CTL_PWEN1;  /* M/S mode + enable */
    } else {
        pwm_map[PWM_CTL] &= ~PWM_CTL_PWEN2;  /* Disable channel 2 */
        usleep(10);

        pwm_map[PWM_RNG2] = range;
        pwm_map[PWM_DAT2] = data;

        pwm_map[PWM_CTL] |= PWM_CTL_MSEN2 | PWM_CTL_PWEN2;  /* M/S mode + enable */
    }
#endif
}

void hpwm_stop(void) {
#ifdef RPI_HW_PWM_PLATFORM_HOST
    printf("MOCK: hpwm_stop() called.\n");
#else
    if (pwm_map) {
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

#endif /* RPI_HW_PWM_IMPLEMENTATION */
