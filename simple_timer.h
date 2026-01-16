/*
 * simple_timer.h - Single-header C library for non-blocking timing
 *
 * Usage:
 *   #define SIMPLE_TIMER_IMPLEMENTATION
 *   #include "simple_timer.h"
 *
 *   ...
 *   simple_timer_t t;
 *   timer_set(&t, 1000);
 *   if (timer_tick(&t)) { ... }  // Auto-advances timer on expiry
 *   ...
 *
 * Note: timer_expired() only checks if expired, does NOT advance the timer.
 *       Use timer_tick() for periodic events.
 */

#ifndef SIMPLE_TIMER_H
#define SIMPLE_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t next_expiry;
    uint64_t interval;
} simple_timer_t;

// API Declarations
void timer_set(simple_timer_t* t, uint64_t interval_ms);
bool timer_expired(simple_timer_t* t);  // Check only, does NOT reset
bool timer_tick(simple_timer_t* t);     // Check and auto-advance timer
uint64_t millis(void);
uint64_t micros(void);
void delay_ms(uint64_t ms);  // Busy-wait delay in milliseconds
void delay_us(uint64_t us);  // Busy-wait delay in microseconds

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_TIMER_H

#ifdef SIMPLE_TIMER_IMPLEMENTATION

#include <time.h>

uint64_t millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000) + (uint64_t)(ts.tv_nsec / 1000000);
}

uint64_t micros(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000) + (uint64_t)(ts.tv_nsec / 1000);
}

void delay_ms(uint64_t ms) {
    uint64_t start = millis();
    while (millis() - start < ms) {
        // Busy wait
    }
}

void delay_us(uint64_t us) {
    uint64_t start = micros();
    while (micros() - start < us) {
        // Busy wait
    }
}

void timer_set(simple_timer_t* t, uint64_t interval_ms) {
    t->interval = interval_ms;
    t->next_expiry = millis() + interval_ms;
}

bool timer_expired(simple_timer_t* t) {
    uint64_t now = millis();
    if (now >= t->next_expiry) {
        return true;
    }
    return false;
}

bool timer_tick(simple_timer_t* t) {
    uint64_t now = millis();
    if (now >= t->next_expiry) {
        // Skip missed intervals to prevent cascading catch-up ticks
        // This is important for scientific applications where losing
        // timing due to system load should not cause burst behavior
        while (t->next_expiry <= now) {
            t->next_expiry += t->interval;
        }
        return true;
    }
    return false;
}

#endif // SIMPLE_TIMER_IMPLEMENTATION
