/**
 * @file rpi_pwm.h
 * @brief Software PWM via dedicated threads for Raspberry Pi.
 *
 * Single-header library. Define RPI_PWM_IMPLEMENTATION in exactly one
 * translation unit before including this file.
 *
 * Requires rpi_gpio.h and pthread (-pthread linker flag).
 */

#ifndef RPI_PWM_H
#define RPI_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize software PWM on a pin at 100 Hz.
 * @param pin BCM pin number.
 * @return 0 on success, -1 on error.
 */
int pwm_init(int pin);

/**
 * @brief Initialize software PWM on a pin at specified frequency.
 * @param pin BCM pin number.
 * @param freq_hz PWM frequency in Hz.
 * @return 0 on success, -1 on error.
 */
int pwm_init_freq(int pin, int freq_hz);

/**
 * @brief Set PWM duty cycle.
 * @param pin BCM pin number.
 * @param duty Duty cycle (0-100%).
 */
void pwm_write(int pin, int duty);

/**
 * @brief Stop PWM on a pin and release resources.
 * @param pin BCM pin number.
 */
void pwm_stop(int pin);

#ifdef __cplusplus
}
#endif

#endif /* RPI_PWM_H */

#ifdef RPI_PWM_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#if defined(__aarch64__) || defined(__arm__)
    #define RPI_PWM_PLATFORM_RPI
#else
    #define RPI_PWM_PLATFORM_HOST
#endif

#define PWM_DEFAULT_FREQ_HZ 100
#define PWM_DUTY_MIN        0
#define PWM_DUTY_MAX        100

/** Clamp duty cycle to valid range [0, 100]. */
#define PWM_CLAMP_DUTY(d)   ((d) < PWM_DUTY_MIN ? PWM_DUTY_MIN : ((d) > PWM_DUTY_MAX ? PWM_DUTY_MAX : (d)))

#ifdef RPI_PWM_PLATFORM_RPI
    #include <pthread.h>
    #include <unistd.h>
    #include <stdbool.h>

    #define MAX_PWM_PINS 8

    typedef struct {
        int pin;
        volatile int duty;
        volatile int period_us;
        volatile bool running;
        pthread_t thread;
        bool active;
    } pwm_pin_t;

    static pwm_pin_t pwm_pins[MAX_PWM_PINS] = {0};
    static pthread_mutex_t pwm_mutex = PTHREAD_MUTEX_INITIALIZER;

    /**
     * PWM thread main loop:
     * - Reads volatile duty cycle and period values
     * - Generates PWM signal by toggling pin HIGH/LOW
     * - Handles edge cases: 0% duty (always LOW) and 100% duty (always HIGH)
     */
    void* pwm_thread_func(void* arg) {
        pwm_pin_t* p = (pwm_pin_t*)arg;

        while (p->running) {
            int d = p->duty;
            int period = p->period_us;

            if (d <= PWM_DUTY_MIN) {
                /* 0% duty: keep pin LOW for entire period */
                digital_write(p->pin, LOW);
                usleep(period);
            } else if (d >= PWM_DUTY_MAX) {
                /* 100% duty: keep pin HIGH for entire period */
                digital_write(p->pin, HIGH);
                usleep(period);
            } else {
                /* Proportional duty: calculate on/off times */
                int on_time = (period * d) / PWM_DUTY_MAX;
                int off_time = period - on_time;

                digital_write(p->pin, HIGH);
                usleep(on_time);
                digital_write(p->pin, LOW);
                usleep(off_time);
            }
        }
        return NULL;
    }
#endif

int pwm_init_freq(int pin, int freq_hz) {
#ifdef RPI_PWM_PLATFORM_HOST
    printf("MOCK: PWM initialized on Pin %d at %d Hz\n", pin, freq_hz);
    return 0;
#else
    if (freq_hz <= 0) freq_hz = PWM_DEFAULT_FREQ_HZ;
    
    pthread_mutex_lock(&pwm_mutex);
    
    int slot = -1;
    for (int i = 0; i < MAX_PWM_PINS; i++) {
        if (pwm_pins[i].active && pwm_pins[i].pin == pin) {
            pthread_mutex_unlock(&pwm_mutex);
            return 0;
        }
        if (!pwm_pins[i].active && slot == -1) {
            slot = i;
        }
    }

    if (slot == -1) {
        pthread_mutex_unlock(&pwm_mutex);
        fprintf(stderr, "PWM Error: Max pins reached\n");
        return -1;
    }

    pin_mode(pin, OUTPUT);
    
    pwm_pins[slot].pin = pin;
    pwm_pins[slot].duty = 0;
    pwm_pins[slot].period_us = 1000000 / freq_hz;
    pwm_pins[slot].running = true;
    pwm_pins[slot].active = true;

    if (pthread_create(&pwm_pins[slot].thread, NULL, pwm_thread_func, &pwm_pins[slot]) != 0) {
        perror("PWM Error: Failed to create thread");
        pwm_pins[slot].active = false;
        pthread_mutex_unlock(&pwm_mutex);
        return -1;
    }

    pthread_mutex_unlock(&pwm_mutex);
    return 0;
#endif
}

int pwm_init(int pin) {
    return pwm_init_freq(pin, PWM_DEFAULT_FREQ_HZ);
}

void pwm_write(int pin, int duty) {
    duty = PWM_CLAMP_DUTY(duty);

#ifdef RPI_PWM_PLATFORM_HOST
    printf("MOCK: PWM on Pin %d updated to %d%%\n", pin, duty);
#else
    pthread_mutex_lock(&pwm_mutex);
    for (int i = 0; i < MAX_PWM_PINS; i++) {
        if (pwm_pins[i].active && pwm_pins[i].pin == pin) {
            pwm_pins[i].duty = duty;
            pthread_mutex_unlock(&pwm_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&pwm_mutex);
#endif
}

void pwm_stop(int pin) {
#ifdef RPI_PWM_PLATFORM_HOST
    printf("MOCK: PWM stopped on Pin %d\n", pin);
#else
    pthread_mutex_lock(&pwm_mutex);
    for (int i = 0; i < MAX_PWM_PINS; i++) {
        if (pwm_pins[i].active && pwm_pins[i].pin == pin) {
            pwm_pins[i].running = false;
            pthread_mutex_unlock(&pwm_mutex);
            
            pthread_join(pwm_pins[i].thread, NULL);
            
            pthread_mutex_lock(&pwm_mutex);
            digital_write(pin, LOW);
            pwm_pins[i].active = false;
            pthread_mutex_unlock(&pwm_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&pwm_mutex);
#endif
}

#endif /* RPI_PWM_IMPLEMENTATION */
