/*
 * test_integration.c - Integration tests for rpi-toolkit
 *
 * These tests validate multi-module interactions and full lifecycle
 * scenarios in EMULATION MODE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "unity_mini.h"

#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"

#define RPI_PWM_IMPLEMENTATION
#include "rpi_pwm.h"

#define RPI_HW_PWM_IMPLEMENTATION
#include "rpi_hw_pwm.h"

/* ============================================================================
 * FULL LIFECYCLE TESTS
 * ============================================================================ */

void test_full_gpio_pwm_lifecycle(void) {
    // Complete lifecycle as shown in main.c
    int result = gpio_init();
    TEST_ASSERT_EQUAL_INT(0, result);
    
    pin_mode(21, OUTPUT);
    digital_write(21, HIGH);
    digital_write(21, LOW);
    
    result = pwm_init(18);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    result = hpwm_init();
    TEST_ASSERT_EQUAL_INT(0, result);
    
    hpwm_set(12, 50, 75);
    
    pwm_write(18, 50);
    
    pwm_stop(18);
    hpwm_stop();
    gpio_cleanup();
    
    TEST_PASS();
}

void test_full_lifecycle_multiple_cycles(void) {
    for (int cycle = 0; cycle < 5; cycle++) {
        gpio_init();
        
        pin_mode(17, OUTPUT);
        pin_mode(18, OUTPUT);
        pin_mode(22, OUTPUT);
        
        pwm_init(18);
        hpwm_init();
        
        for (int i = 0; i < 10; i++) {
            digital_write(17, i % 2);
            pwm_write(18, i * 10);
            hpwm_set(12, 50, i * 100);
        }
        
        simple_timer_t t;
        timer_set(&t, 5);
        while (!timer_tick(&t)) {
            delay_us(100);
        }
        
        pwm_stop(18);
        hpwm_stop();
        gpio_cleanup();
    }
    
    TEST_PASS();
}

/* ============================================================================
 * GPIO + TIMER INTEGRATION
 * ============================================================================ */

void test_gpio_with_timer_blink(void) {
    gpio_init();
    pin_mode(21, OUTPUT);
    
    simple_timer_t t;
    timer_set(&t, 10);  // 10ms interval
    
    int state = LOW;
    uint64_t start = millis();
    int toggle_count = 0;
    
    while (millis() - start < 100) {  // Run for 100ms
        if (timer_tick(&t)) {
            state = !state;
            digital_write(21, state);
            toggle_count++;
        }
        delay_us(500);  // Small delay
    }
    
    // Should have toggled ~9-10 times
    TEST_ASSERT_GREATER_OR_EQUAL(7, toggle_count);
    TEST_ASSERT_LESS_OR_EQUAL(12, toggle_count);
    
    gpio_cleanup();
}

void test_gpio_with_timer_multiple_pins(void) {
    gpio_init();
    
    int pins[] = {17, 18, 22, 23};
    int num_pins = 4;
    
    for (int i = 0; i < num_pins; i++) {
        pin_mode(pins[i], OUTPUT);
    }
    
    simple_timer_t timers[4];
    for (int i = 0; i < num_pins; i++) {
        timer_set(&timers[i], 10 + i * 5);  // Different intervals
    }
    
    int states[4] = {0};
    uint64_t start = millis();
    
    while (millis() - start < 100) {
        for (int i = 0; i < num_pins; i++) {
            if (timer_tick(&timers[i])) {
                states[i] = !states[i];
                digital_write(pins[i], states[i]);
            }
        }
        delay_us(500);
    }
    
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * PWM + TIMER INTEGRATION
 * ============================================================================ */

void test_pwm_fade_with_timer(void) {
    gpio_init();
    pwm_init(18);
    
    simple_timer_t t;
    timer_set(&t, 5);  // 5ms interval
    
    int duty = 0;
    int step = 5;
    uint64_t start = millis();
    
    while (millis() - start < 100) {
        if (timer_tick(&t)) {
            duty += step;
            if (duty > 100) {
                duty = 100;
                step = -5;
            } else if (duty < 0) {
                duty = 0;
                step = 5;
            }
            pwm_write(18, duty);
        }
        delay_us(500);
    }
    
    pwm_stop(18);
    gpio_cleanup();
    TEST_PASS();
}

void test_hw_pwm_servo_sweep_with_timer(void) {
    gpio_init();
    hpwm_init();
    
    simple_timer_t t;
    timer_set(&t, 20);  // 20ms interval (servo update rate)
    
    int duty = 50;   // Start at 5% (servo min)
    int step = 10;
    uint64_t start = millis();
    
    while (millis() - start < 200) {
        if (timer_tick(&t)) {
            hpwm_set(18, 50, duty);  // 50Hz for servo
            
            duty += step;
            if (duty >= 100) {
                duty = 100;
                step = -10;
            } else if (duty <= 50) {
                duty = 50;
                step = 10;
            }
        }
        delay_us(500);
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * MULTI-MODULE CONCURRENT OPERATIONS
 * ============================================================================ */

void test_gpio_swpwm_hwpwm_concurrent(void) {
    gpio_init();
    
    // Multiple GPIO pins
    pin_mode(17, OUTPUT);
    pin_mode(21, OUTPUT);
    
    // Software PWM
    pwm_init(18);
    pwm_init(22);
    
    // Hardware PWM
    hpwm_init();
    
    simple_timer_t gpio_timer, swpwm_timer, hwpwm_timer;
    timer_set(&gpio_timer, 10);
    timer_set(&swpwm_timer, 15);
    timer_set(&hwpwm_timer, 20);
    
    int gpio_state = 0;
    int sw_duty = 0;
    int hw_duty = 0;
    
    uint64_t start = millis();
    
    while (millis() - start < 150) {
        if (timer_tick(&gpio_timer)) {
            gpio_state = !gpio_state;
            digital_write(17, gpio_state);
            digital_write(21, !gpio_state);
        }
        
        if (timer_tick(&swpwm_timer)) {
            sw_duty = (sw_duty + 10) % 101;
            pwm_write(18, sw_duty);
            pwm_write(22, 100 - sw_duty);
        }
        
        if (timer_tick(&hwpwm_timer)) {
            hw_duty = (hw_duty + 50) % 1001;
            hpwm_set(12, 50, hw_duty);
            hpwm_set(19, 50, 1000 - hw_duty);
        }
        
        delay_us(500);
    }
    
    pwm_stop(18);
    pwm_stop(22);
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * CLEANUP SEQUENCE TESTS
 * ============================================================================ */

void test_cleanup_order_pwm_first(void) {
    gpio_init();
    pwm_init(18);
    hpwm_init();
    
    // Cleanup in order: PWM, HW PWM, GPIO
    pwm_stop(18);
    hpwm_stop();
    gpio_cleanup();
    
    TEST_PASS();
}

void test_cleanup_order_gpio_first(void) {
    gpio_init();
    pwm_init(18);
    hpwm_init();
    
    // Cleanup in reverse order (should still work)
    gpio_cleanup();
    hpwm_stop();
    pwm_stop(18);
    
    TEST_PASS();
}

void test_cleanup_partial(void) {
    gpio_init();
    pwm_init(18);
    hpwm_init();
    
    // Only cleanup some modules
    pwm_stop(18);
    // Don't call hpwm_stop or gpio_cleanup
    
    // Should still be able to use remaining modules
    hpwm_set(12, 50, 500);
    digital_write(21, HIGH);
    
    hpwm_stop();
    gpio_cleanup();
    
    TEST_PASS();
}

/* ============================================================================
 * ERROR RECOVERY TESTS
 * ============================================================================ */

void test_reinit_after_cleanup(void) {
    gpio_init();
    pwm_init(18);
    hpwm_init();
    
    pwm_stop(18);
    hpwm_stop();
    gpio_cleanup();
    
    // Reinitialize everything
    TEST_ASSERT_EQUAL_INT(0, gpio_init());
    TEST_ASSERT_EQUAL_INT(0, pwm_init(18));
    TEST_ASSERT_EQUAL_INT(0, hpwm_init());
    
    // Use them
    pin_mode(21, OUTPUT);
    digital_write(21, HIGH);
    pwm_write(18, 50);
    hpwm_set(12, 50, 500);
    
    // Cleanup again
    pwm_stop(18);
    hpwm_stop();
    gpio_cleanup();
    
    TEST_PASS();
}

void test_operations_after_cleanup(void) {
    gpio_init();
    gpio_cleanup();
    
    // Operations after cleanup should not crash
    pin_mode(18, OUTPUT);
    digital_write(18, HIGH);
    digital_read(18);
    
    TEST_PASS();
}

/* ============================================================================
 * TIMING PRECISION WITH MULTIPLE MODULES
 * ============================================================================ */

void test_timing_precision_under_load(void) {
    gpio_init();
    pwm_init(18);
    
    simple_timer_t t;
    timer_set(&t, 10);  // 10ms interval
    
    int tick_count = 0;
    uint64_t start = millis();
    
    // Run for 200ms with GPIO and PWM operations
    while (millis() - start < 200) {
        if (timer_tick(&t)) {
            tick_count++;
            digital_write(21, tick_count % 2);
            pwm_write(18, (tick_count * 5) % 101);
        }
        delay_us(100);
    }
    
    // Should have ~20 ticks (200ms / 10ms)
    TEST_ASSERT_GREATER_OR_EQUAL(18, tick_count);
    TEST_ASSERT_LESS_OR_EQUAL(22, tick_count);
    
    pwm_stop(18);
    gpio_cleanup();
}

/* ============================================================================
 * STRESS TESTS
 * ============================================================================ */

void test_stress_full_system(void) {
    for (int iteration = 0; iteration < 20; iteration++) {
        gpio_init();
        
        // Initialize multiple modules
        for (int i = 0; i < 4; i++) {
            pin_mode(i + 17, OUTPUT);
        }
        
        pwm_init(17);
        pwm_init(18);
        hpwm_init();
        
        // Rapid operations
        for (int i = 0; i < 100; i++) {
            digital_write(17, i % 2);
            digital_write(18, (i + 1) % 2);
            pwm_write(17, i % 101);
            pwm_write(18, (100 - i % 101));
            hpwm_set(12, 50 + i, i % 1001);
        }
        
        pwm_stop(17);
        pwm_stop(18);
        hpwm_stop();
        gpio_cleanup();
    }
    
    TEST_PASS();
}

void test_stress_timer_with_all_modules(void) {
    gpio_init();
    pwm_init(18);
    hpwm_init();
    
    simple_timer_t t1, t2, t3;
    timer_set(&t1, 5);
    timer_set(&t2, 7);
    timer_set(&t3, 11);
    
    int c1 = 0, c2 = 0, c3 = 0;
    uint64_t start = millis();
    
    while (millis() - start < 100) {
        if (timer_tick(&t1)) {
            c1++;
            digital_write(21, c1 % 2);
        }
        if (timer_tick(&t2)) {
            c2++;
            pwm_write(18, (c2 * 10) % 101);
        }
        if (timer_tick(&t3)) {
            c3++;
            hpwm_set(12, 50, (c3 * 100) % 1001);
        }
        delay_us(500);
    }
    
    // Verify reasonable tick counts
    TEST_ASSERT_GREATER_THAN(10, c1);
    TEST_ASSERT_GREATER_THAN(8, c2);
    TEST_ASSERT_GREATER_THAN(5, c3);
    
    pwm_stop(18);
    hpwm_stop();
    gpio_cleanup();
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    UNITY_BEGIN();
    
    // Full lifecycle tests
    RUN_TEST(test_full_gpio_pwm_lifecycle);
    RUN_TEST(test_full_lifecycle_multiple_cycles);
    
    // GPIO + Timer integration
    RUN_TEST(test_gpio_with_timer_blink);
    RUN_TEST(test_gpio_with_timer_multiple_pins);
    
    // PWM + Timer integration
    RUN_TEST(test_pwm_fade_with_timer);
    RUN_TEST(test_hw_pwm_servo_sweep_with_timer);
    
    // Multi-module concurrent
    RUN_TEST(test_gpio_swpwm_hwpwm_concurrent);
    
    // Cleanup sequence tests
    RUN_TEST(test_cleanup_order_pwm_first);
    RUN_TEST(test_cleanup_order_gpio_first);
    RUN_TEST(test_cleanup_partial);
    
    // Error recovery
    RUN_TEST(test_reinit_after_cleanup);
    RUN_TEST(test_operations_after_cleanup);
    
    // Timing precision
    RUN_TEST(test_timing_precision_under_load);
    
    // Stress tests
    RUN_TEST(test_stress_full_system);
    RUN_TEST(test_stress_timer_with_all_modules);
    
    return UNITY_END();
}
