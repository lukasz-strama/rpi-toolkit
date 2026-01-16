/*
 * test_rpi_hw_pwm.c - Validation tests for rpi_hw_pwm.h
 *
 * These tests validate the Hardware PWM library in EMULATION MODE.
 * Focus: valid pin validation, duty per-mille clamping, frequency handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "unity_mini.h"

#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define RPI_HW_PWM_IMPLEMENTATION
#include "rpi_hw_pwm.h"

/* ============================================================================
 * HARDWARE PWM INITIALIZATION TESTS
 * ============================================================================ */

void test_hpwm_init_returns_success(void) {
    int result = hpwm_init();
    TEST_ASSERT_EQUAL_INT(0, result);
    hpwm_stop();
}

void test_hpwm_stop_no_crash(void) {
    hpwm_init();
    hpwm_stop();
    TEST_PASS();
}

void test_hpwm_stop_without_init(void) {
    // Stop without init should not crash
    hpwm_stop();
    TEST_PASS();
}

void test_hpwm_double_init(void) {
    int r1 = hpwm_init();
    int r2 = hpwm_init();
    TEST_ASSERT_EQUAL_INT(0, r1);
    TEST_ASSERT_EQUAL_INT(0, r2);
    hpwm_stop();
}

void test_hpwm_double_stop(void) {
    hpwm_init();
    hpwm_stop();
    hpwm_stop();  // Should not crash
    TEST_PASS();
}

void test_hpwm_multiple_init_stop_cycles(void) {
    for (int i = 0; i < 10; i++) {
        int result = hpwm_init();
        TEST_ASSERT_EQUAL_INT(0, result);
        hpwm_stop();
    }
}

/* ============================================================================
 * HARDWARE PWM SET - VALID PINS
 * ============================================================================ */

void test_hpwm_set_pin_12(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(12, 1000, 500);  // Pin 12, 1kHz, 50% duty
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_pin_13(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(13, 1000, 500);  // Pin 13, 1kHz, 50% duty
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_pin_18(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 1000, 500);  // Pin 18, 1kHz, 50% duty
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_pin_19(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(19, 1000, 500);  // Pin 19, 1kHz, 50% duty
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_all_valid_pins(void) {
    gpio_init();
    hpwm_init();
    
    int valid_pins[] = {12, 13, 18, 19};
    for (int i = 0; i < 4; i++) {
        hpwm_set(valid_pins[i], 1000, 500);
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * HARDWARE PWM SET - INVALID PINS
 * ============================================================================ */

void test_hpwm_set_invalid_pin_0(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(0, 1000, 500);  // Should be silently ignored
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_invalid_pin_17(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(17, 1000, 500);  // Not a HW PWM pin
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_invalid_pin_negative(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(-1, 1000, 500);
    hpwm_set(-100, 1000, 500);
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_invalid_pin_large(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(100, 1000, 500);
    hpwm_set(1000, 1000, 500);
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_many_invalid_pins(void) {
    gpio_init();
    hpwm_init();
    
    int invalid_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 27};
    int num_invalid = sizeof(invalid_pins) / sizeof(invalid_pins[0]);
    
    for (int i = 0; i < num_invalid; i++) {
        hpwm_set(invalid_pins[i], 1000, 500);  // All should be ignored
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * HARDWARE PWM SET - DUTY PER-MILLE
 * ============================================================================ */

void test_hpwm_set_duty_zero(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 1000, 0);  // 0% duty
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_duty_max(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 1000, 1000);  // 100% duty (1000 per-mille)
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_duty_half(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 1000, 500);  // 50% duty
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_duty_negative_clamped(void) {
    gpio_init();
    hpwm_init();
    // Negative duty should be clamped to 0
    hpwm_set(18, 1000, -1);
    hpwm_set(18, 1000, -100);
    hpwm_set(18, 1000, -2147483648);  // INT_MIN
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_duty_over_max_clamped(void) {
    gpio_init();
    hpwm_init();
    // Duty > 1000 should be clamped to 1000
    hpwm_set(18, 1000, 1001);
    hpwm_set(18, 1000, 2000);
    hpwm_set(18, 1000, 10000);
    hpwm_set(18, 1000, 2147483647);  // INT_MAX
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_all_duty_values(void) {
    gpio_init();
    hpwm_init();
    // Test duty values from 0 to 1000
    for (int duty = 0; duty <= 1000; duty += 10) {
        hpwm_set(18, 1000, duty);
    }
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * HARDWARE PWM SET - FREQUENCY
 * ============================================================================ */

void test_hpwm_set_freq_1hz(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 1, 500);  // 1 Hz
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_freq_50hz(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 50, 75);  // 50 Hz, 7.5% (servo)
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_freq_1khz(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 1000, 500);  // 1 kHz
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_freq_high(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, 100000, 500);  // 100 kHz
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_freq_zero_ignored(void) {
    gpio_init();
    hpwm_init();
    // freq <= 0 should return early
    hpwm_set(18, 0, 500);
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_freq_negative_ignored(void) {
    gpio_init();
    hpwm_init();
    hpwm_set(18, -1, 500);
    hpwm_set(18, -100, 500);
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * HARDWARE PWM LIFECYCLE TESTS
 * ============================================================================ */

void test_hpwm_set_before_init(void) {
    gpio_init();
    // Set before init should not crash (returns early due to null check)
    hpwm_set(18, 1000, 500);
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_set_after_stop(void) {
    gpio_init();
    hpwm_init();
    hpwm_stop();
    hpwm_set(18, 1000, 500);  // Should not crash
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_multiple_set_calls(void) {
    gpio_init();
    hpwm_init();
    
    for (int i = 0; i < 100; i++) {
        hpwm_set(18, 50 + i, i % 1001);
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_channel_switching(void) {
    gpio_init();
    hpwm_init();
    
    // Alternate between Channel 0 (pins 12, 18) and Channel 1 (pins 13, 19)
    for (int i = 0; i < 50; i++) {
        hpwm_set(12, 1000, 250);  // Channel 0
        hpwm_set(13, 1000, 500);  // Channel 1
        hpwm_set(18, 1000, 750);  // Channel 0
        hpwm_set(19, 1000, 1000); // Channel 1
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_init_stop_with_gpio(void) {
    // Full lifecycle with GPIO
    for (int i = 0; i < 5; i++) {
        gpio_init();
        hpwm_init();
        hpwm_set(18, 50, 75);
        hpwm_set(12, 1000, 500);
        hpwm_stop();
        gpio_cleanup();
    }
    TEST_PASS();
}

/* ============================================================================
 * STRESS TESTS
 * ============================================================================ */

void test_hpwm_stress_rapid_set(void) {
    gpio_init();
    hpwm_init();
    
    for (int i = 0; i < 10000; i++) {
        hpwm_set(18, 1000 + (i % 1000), i % 1001);
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_stress_init_stop_cycles(void) {
    gpio_init();
    
    for (int i = 0; i < 50; i++) {
        hpwm_init();
        hpwm_set(18, 1000, 500);
        hpwm_stop();
    }
    
    gpio_cleanup();
    TEST_PASS();
}

void test_hpwm_stress_all_pins_rapid(void) {
    gpio_init();
    hpwm_init();
    
    int pins[] = {12, 13, 18, 19};
    
    for (int i = 0; i < 1000; i++) {
        for (int p = 0; p < 4; p++) {
            hpwm_set(pins[p], 50 + (i % 950), i % 1001);
        }
    }
    
    hpwm_stop();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_hpwm_init_returns_success);
    RUN_TEST(test_hpwm_stop_no_crash);
    RUN_TEST(test_hpwm_stop_without_init);
    RUN_TEST(test_hpwm_double_init);
    RUN_TEST(test_hpwm_double_stop);
    RUN_TEST(test_hpwm_multiple_init_stop_cycles);
    
    // Valid pins tests
    RUN_TEST(test_hpwm_set_pin_12);
    RUN_TEST(test_hpwm_set_pin_13);
    RUN_TEST(test_hpwm_set_pin_18);
    RUN_TEST(test_hpwm_set_pin_19);
    RUN_TEST(test_hpwm_set_all_valid_pins);
    
    // Invalid pins tests
    RUN_TEST(test_hpwm_set_invalid_pin_0);
    RUN_TEST(test_hpwm_set_invalid_pin_17);
    RUN_TEST(test_hpwm_set_invalid_pin_negative);
    RUN_TEST(test_hpwm_set_invalid_pin_large);
    RUN_TEST(test_hpwm_set_many_invalid_pins);
    
    // Duty per-mille tests
    RUN_TEST(test_hpwm_set_duty_zero);
    RUN_TEST(test_hpwm_set_duty_max);
    RUN_TEST(test_hpwm_set_duty_half);
    RUN_TEST(test_hpwm_set_duty_negative_clamped);
    RUN_TEST(test_hpwm_set_duty_over_max_clamped);
    RUN_TEST(test_hpwm_set_all_duty_values);
    
    // Frequency tests
    RUN_TEST(test_hpwm_set_freq_1hz);
    RUN_TEST(test_hpwm_set_freq_50hz);
    RUN_TEST(test_hpwm_set_freq_1khz);
    RUN_TEST(test_hpwm_set_freq_high);
    RUN_TEST(test_hpwm_set_freq_zero_ignored);
    RUN_TEST(test_hpwm_set_freq_negative_ignored);
    
    // Lifecycle tests
    RUN_TEST(test_hpwm_set_before_init);
    RUN_TEST(test_hpwm_set_after_stop);
    RUN_TEST(test_hpwm_multiple_set_calls);
    RUN_TEST(test_hpwm_channel_switching);
    RUN_TEST(test_hpwm_init_stop_with_gpio);
    
    // Stress tests
    RUN_TEST(test_hpwm_stress_rapid_set);
    RUN_TEST(test_hpwm_stress_init_stop_cycles);
    RUN_TEST(test_hpwm_stress_all_pins_rapid);
    
    return UNITY_END();
}
