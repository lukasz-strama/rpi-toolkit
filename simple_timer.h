/**
 * @file simple_timer.h
 * @brief Non-blocking timer and delay utilities using CLOCK_MONOTONIC.
 *
 * Single-header library. Define SIMPLE_TIMER_IMPLEMENTATION in exactly one
 * translation unit before including this file.
 */

#ifndef SIMPLE_TIMER_H
#define SIMPLE_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Timer state structure.
 */
typedef struct {
    uint64_t next_expiry;  /**< Next expiry timestamp in ms. */
    uint64_t interval;     /**< Timer interval in ms. */
} simple_timer_t;

/**
 * @brief Initialize or reset a timer.
 * @param t Pointer to timer structure.
 * @param interval_ms Timer interval in milliseconds.
 */
void timer_set(simple_timer_t* t, uint64_t interval_ms);

/**
 * @brief Check if timer has expired (does not reset).
 * @param t Pointer to timer structure.
 * @return true if expired.
 */
bool timer_expired(simple_timer_t* t);

/**
 * @brief Check if timer has expired and auto-advance.
 *
 * Advances the timer by its interval, compensating for drift.
 * Skips missed intervals to prevent burst behavior.
 *
 * @param t Pointer to timer structure.
 * @return true if expired (timer is advanced).
 */
bool timer_tick(simple_timer_t* t);

/**
 * @brief Get monotonic time in milliseconds.
 * @return Milliseconds since system boot.
 */
uint64_t millis(void);

/**
 * @brief Get monotonic time in microseconds.
 * @return Microseconds since system boot.
 */
uint64_t micros(void);

/**
 * @brief Busy-wait delay in milliseconds.
 * @param ms Delay duration.
 */
void delay_ms(uint64_t ms);

/**
 * @brief Busy-wait delay in microseconds.
 * @param us Delay duration.
 */
void delay_us(uint64_t us);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_TIMER_H */

#ifdef SIMPLE_TIMER_IMPLEMENTATION

#include <time.h>

/** @name Time Unit Conversions */
/**@{*/
#define MS_PER_SEC  1000
#define US_PER_SEC  1000000
#define NS_PER_MS   1000000
#define NS_PER_US   1000
/**@}*/

uint64_t millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * MS_PER_SEC) + (uint64_t)(ts.tv_nsec / NS_PER_MS);
}

uint64_t micros(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * US_PER_SEC) + (uint64_t)(ts.tv_nsec / NS_PER_US);
}

void delay_ms(uint64_t ms) {
    uint64_t start = millis();
    while (millis() - start < ms) {
        /* Busy wait */
    }
}

void delay_us(uint64_t us) {
    uint64_t start = micros();
    while (micros() - start < us) {
        /* Busy wait */
    }
}

void timer_set(simple_timer_t* t, uint64_t interval_ms) {
    t->interval = interval_ms;
    t->next_expiry = millis() + interval_ms;
}

bool timer_expired(simple_timer_t* t) {
    return millis() >= t->next_expiry;
}

bool timer_tick(simple_timer_t* t) {
    uint64_t now = millis();
    if (now >= t->next_expiry) {
        while (t->next_expiry <= now) {
            t->next_expiry += t->interval;
        }
        return true;
    }
    return false;
}

#endif /* SIMPLE_TIMER_IMPLEMENTATION */
