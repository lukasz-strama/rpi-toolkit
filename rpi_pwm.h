/*
 * rpi_pwm.h - Single-header C library for Software PWM on Raspberry Pi
 *
 * Dependencies:
 *   - rpi_gpio.h (must be included before or linked)
 *   - pthread (link with -pthread)
 *
 * Usage:
 *   #define RPI_PWM_IMPLEMENTATION
 *   #include "rpi_pwm.h"
 *
 *   ...
 *   pwm_init(18);
 *   pwm_write(18, 50); // 50% duty cycle
 *   ...
 *   pwm_stop(18);
 */

#ifndef RPI_PWM_H
#define RPI_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

// API Declarations
int pwm_init(int pin);
void pwm_write(int pin, int duty);
void pwm_stop(int pin);

#ifdef __cplusplus
}
#endif

#endif // RPI_PWM_H

#ifdef RPI_PWM_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

// Platform Detection
#if defined(__aarch64__) || defined(__arm__)
    #define RPI_PWM_PLATFORM_RPI
#else
    #define RPI_PWM_PLATFORM_HOST
#endif

#ifdef RPI_PWM_PLATFORM_RPI
    #include <pthread.h>
    #include <unistd.h>
    #include <stdbool.h>

    #define MAX_PWM_PINS 8

    typedef struct {
        int pin;
        volatile int duty;
        volatile bool running;
        pthread_t thread;
        bool active;
    } pwm_pin_t;

    static pwm_pin_t pwm_pins[MAX_PWM_PINS] = {0};
    static pthread_mutex_t pwm_mutex = PTHREAD_MUTEX_INITIALIZER;

    void* pwm_thread_func(void* arg) {
        pwm_pin_t* p = (pwm_pin_t*)arg;
        
        while (p->running) {
            int d = p->duty;
            
            if (d <= 0) {
                digital_write(p->pin, LOW);
                usleep(10000); // 10ms wait
            } else if (d >= 100) {
                digital_write(p->pin, HIGH);
                usleep(10000); // 10ms wait
            } else {
                // Period is 10ms = 10000us
                int on_time = d * 100;
                int off_time = 10000 - on_time;

                digital_write(p->pin, HIGH);
                usleep(on_time);
                digital_write(p->pin, LOW);
                usleep(off_time);
            }
        }
        return NULL;
    }
#endif

int pwm_init(int pin) {
#ifdef RPI_PWM_PLATFORM_HOST
    printf("MOCK: PWM initialized on Pin %d\n", pin);
    return 0;
#else
    pthread_mutex_lock(&pwm_mutex);
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_PWM_PINS; i++) {
        if (pwm_pins[i].active && pwm_pins[i].pin == pin) {
            pthread_mutex_unlock(&pwm_mutex);
            return 0; // Already initialized
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

void pwm_write(int pin, int duty) {
    if (duty < 0) duty = 0;
    if (duty > 100) duty = 100;

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
            
            // Join outside mutex to avoid deadlock
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

#endif // RPI_PWM_IMPLEMENTATION
