/*
 * test_rpi_gpio.c - Validation tests for rpi_gpio.h
 *
 * These tests validate the GPIO library in EMULATION MODE.
 * Focus: boundary conditions, error handling, lifecycle management.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity_mini.h"

#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

/* ============================================================================
 * GPIO INITIALIZATION TESTS
 * ============================================================================ */

void test_gpio_init_returns_success(void) {
    int result = gpio_init();
    TEST_ASSERT_EQUAL_INT(0, result);
    gpio_cleanup();
}

void test_gpio_cleanup_does_not_crash(void) {
    gpio_init();
    gpio_cleanup();
    // If we got here, it didn't crash
    TEST_PASS();
}

void test_gpio_cleanup_without_init(void) {
    // Calling cleanup without init should not crash
    gpio_cleanup();
    TEST_PASS();
}

void test_gpio_multiple_init_cleanup_cycles(void) {
    // Stress test: multiple init/cleanup cycles
    for (int i = 0; i < 10; i++) {
        int result = gpio_init();
        TEST_ASSERT_EQUAL_INT(0, result);
        gpio_cleanup();
    }
}

void test_gpio_double_init(void) {
    // Double init should work (not crash)
    int r1 = gpio_init();
    int r2 = gpio_init();
    TEST_ASSERT_EQUAL_INT(0, r1);
    TEST_ASSERT_EQUAL_INT(0, r2);
    gpio_cleanup();
}

void test_gpio_double_cleanup(void) {
    // Double cleanup should not crash
    gpio_init();
    gpio_cleanup();
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * PIN MODE TESTS - BOUNDARY CONDITIONS
 * ============================================================================ */

void test_pin_mode_all_valid_pins_as_output(void) {
    gpio_init();
    // All valid pins: 0-53
    for (int pin = GPIO_PIN_MIN; pin <= GPIO_PIN_MAX; pin++) {
        pin_mode(pin, OUTPUT);
        // No crash = success
    }
    gpio_cleanup();
    TEST_PASS();
}

void test_pin_mode_all_valid_pins_as_input(void) {
    gpio_init();
    for (int pin = GPIO_PIN_MIN; pin <= GPIO_PIN_MAX; pin++) {
        pin_mode(pin, INPUT);
    }
    gpio_cleanup();
    TEST_PASS();
}

void test_pin_mode_invalid_pin_negative(void) {
    gpio_init();
    // Negative pin should be silently ignored (no crash)
    pin_mode(-1, OUTPUT);
    pin_mode(-100, OUTPUT);
    pin_mode(-2147483648, OUTPUT);  // INT_MIN
    gpio_cleanup();
    TEST_PASS();
}

void test_pin_mode_invalid_pin_too_high(void) {
    gpio_init();
    // Pins above 53 should be silently ignored
    pin_mode(54, OUTPUT);
    pin_mode(100, OUTPUT);
    pin_mode(1000, OUTPUT);
    pin_mode(2147483647, OUTPUT);  // INT_MAX
    gpio_cleanup();
    TEST_PASS();
}

void test_pin_mode_boundary_pins(void) {
    gpio_init();
    // Test exact boundary pins
    pin_mode(0, OUTPUT);   // First valid pin
    pin_mode(53, OUTPUT);  // Last valid pin
    gpio_cleanup();
    TEST_PASS();
}

void test_pin_mode_without_init(void) {
    // pin_mode without gpio_init should not crash
    pin_mode(18, OUTPUT);
    TEST_PASS();
}

void test_pin_mode_rapid_mode_switching(void) {
    gpio_init();
    // Rapidly switch modes on same pin
    for (int i = 0; i < 1000; i++) {
        pin_mode(18, OUTPUT);
        pin_mode(18, INPUT);
    }
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * GPIO SET FUNCTION TESTS - ALT MODES
 * ============================================================================ */

void test_gpio_set_function_all_alt_modes(void) {
    gpio_init();
    int alt_modes[] = {ALT0, ALT1, ALT2, ALT3, ALT4, ALT5};
    int num_modes = sizeof(alt_modes) / sizeof(alt_modes[0]);
    
    for (int pin = 0; pin <= 27; pin++) {
        for (int m = 0; m < num_modes; m++) {
            gpio_set_function(pin, alt_modes[m]);
        }
    }
    gpio_cleanup();
    TEST_PASS();
}

void test_gpio_set_function_invalid_pin(void) {
    gpio_init();
    gpio_set_function(-1, ALT0);
    gpio_set_function(54, ALT0);
    gpio_set_function(100, ALT5);
    gpio_cleanup();
    TEST_PASS();
}

void test_gpio_set_function_without_init(void) {
    gpio_set_function(18, ALT5);
    TEST_PASS();
}

/* ============================================================================
 * DIGITAL WRITE TESTS
 * ============================================================================ */

void test_digital_write_all_valid_pins_high(void) {
    gpio_init();
    for (int pin = GPIO_PIN_MIN; pin <= GPIO_PIN_MAX; pin++) {
        pin_mode(pin, OUTPUT);
        digital_write(pin, HIGH);
    }
    gpio_cleanup();
    TEST_PASS();
}

void test_digital_write_all_valid_pins_low(void) {
    gpio_init();
    for (int pin = GPIO_PIN_MIN; pin <= GPIO_PIN_MAX; pin++) {
        pin_mode(pin, OUTPUT);
        digital_write(pin, LOW);
    }
    gpio_cleanup();
    TEST_PASS();
}

void test_digital_write_invalid_pin_negative(void) {
    gpio_init();
    digital_write(-1, HIGH);
    digital_write(-100, LOW);
    gpio_cleanup();
    TEST_PASS();
}

void test_digital_write_invalid_pin_too_high(void) {
    gpio_init();
    digital_write(54, HIGH);
    digital_write(100, LOW);
    digital_write(1000, HIGH);
    gpio_cleanup();
    TEST_PASS();
}

void test_digital_write_without_init(void) {
    digital_write(18, HIGH);
    digital_write(18, LOW);
    TEST_PASS();
}

void test_digital_write_rapid_toggle(void) {
    gpio_init();
    pin_mode(18, OUTPUT);
    // Rapid toggle stress test
    for (int i = 0; i < 10000; i++) {
        digital_write(18, HIGH);
        digital_write(18, LOW);
    }
    gpio_cleanup();
    TEST_PASS();
}

void test_digital_write_boundary_pins(void) {
    gpio_init();
    // First and last valid pins
    pin_mode(0, OUTPUT);
    digital_write(0, HIGH);
    digital_write(0, LOW);
    
    pin_mode(53, OUTPUT);
    digital_write(53, HIGH);
    digital_write(53, LOW);
    gpio_cleanup();
    TEST_PASS();
}

void test_digital_write_pins_31_32_boundary(void) {
    // Test the register boundary (pins < 32 use GPSET0, pins >= 32 use GPSET1)
    gpio_init();
    pin_mode(31, OUTPUT);
    pin_mode(32, OUTPUT);
    
    digital_write(31, HIGH);
    digital_write(32, HIGH);
    digital_write(31, LOW);
    digital_write(32, LOW);
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * DIGITAL READ TESTS
 * ============================================================================ */

void test_digital_read_returns_low_in_emulation(void) {
    gpio_init();
    for (int pin = 0; pin <= 27; pin++) {
        pin_mode(pin, INPUT);
        int value = digital_read(pin);
        // In emulation mode, digital_read always returns LOW
        TEST_ASSERT_EQUAL_INT(LOW, value);
    }
    gpio_cleanup();
}

void test_digital_read_invalid_pin_negative(void) {
    gpio_init();
    int value = digital_read(-1);
    TEST_ASSERT_EQUAL_INT(LOW, value);
    gpio_cleanup();
}

void test_digital_read_invalid_pin_too_high(void) {
    gpio_init();
    int value = digital_read(54);
    TEST_ASSERT_EQUAL_INT(LOW, value);
    
    value = digital_read(100);
    TEST_ASSERT_EQUAL_INT(LOW, value);
    gpio_cleanup();
}

void test_digital_read_without_init(void) {
    // Should return LOW and not crash
    int value = digital_read(18);
    TEST_ASSERT_EQUAL_INT(LOW, value);
}

void test_digital_read_all_valid_pins(void) {
    gpio_init();
    for (int pin = GPIO_PIN_MIN; pin <= GPIO_PIN_MAX; pin++) {
        int value = digital_read(pin);
        TEST_ASSERT_EQUAL_INT(LOW, value);
    }
    gpio_cleanup();
}

void test_digital_read_pins_31_32_boundary(void) {
    gpio_init();
    pin_mode(31, INPUT);
    pin_mode(32, INPUT);
    
    int v31 = digital_read(31);
    int v32 = digital_read(32);
    
    TEST_ASSERT_EQUAL_INT(LOW, v31);
    TEST_ASSERT_EQUAL_INT(LOW, v32);
    gpio_cleanup();
}

/* ============================================================================
 * CONSTANTS VALIDATION TESTS
 * ============================================================================ */

void test_constants_input_output_values(void) {
    TEST_ASSERT_EQUAL_INT(0, INPUT);
    TEST_ASSERT_EQUAL_INT(1, OUTPUT);
}

void test_constants_high_low_values(void) {
    TEST_ASSERT_EQUAL_INT(0, LOW);
    TEST_ASSERT_EQUAL_INT(1, HIGH);
}

void test_constants_alt_function_values(void) {
    // Verify ALT function values match BCM2711 specification
    TEST_ASSERT_EQUAL_INT(4, ALT0);
    TEST_ASSERT_EQUAL_INT(5, ALT1);
    TEST_ASSERT_EQUAL_INT(6, ALT2);
    TEST_ASSERT_EQUAL_INT(7, ALT3);
    TEST_ASSERT_EQUAL_INT(3, ALT4);
    TEST_ASSERT_EQUAL_INT(2, ALT5);
}

void test_constants_pin_range(void) {
    TEST_ASSERT_EQUAL_INT(0, GPIO_PIN_MIN);
    TEST_ASSERT_EQUAL_INT(53, GPIO_PIN_MAX);
}

/* ============================================================================
 * STRESS TESTS
 * ============================================================================ */

void test_stress_full_gpio_cycle(void) {
    gpio_init();
    
    // Full cycle on all pins
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int pin = 0; pin <= 27; pin++) {
            pin_mode(pin, OUTPUT);
            digital_write(pin, HIGH);
            digital_write(pin, LOW);
            pin_mode(pin, INPUT);
            digital_read(pin);
        }
    }
    
    gpio_cleanup();
    TEST_PASS();
}

void test_stress_many_operations(void) {
    gpio_init();
    
    // 100k operations
    for (int i = 0; i < 100000; i++) {
        digital_write(18, i % 2);
    }
    
    gpio_cleanup();
    TEST_PASS();
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_gpio_init_returns_success);
    RUN_TEST(test_gpio_cleanup_does_not_crash);
    RUN_TEST(test_gpio_cleanup_without_init);
    RUN_TEST(test_gpio_multiple_init_cleanup_cycles);
    RUN_TEST(test_gpio_double_init);
    RUN_TEST(test_gpio_double_cleanup);
    
    // Pin mode tests
    RUN_TEST(test_pin_mode_all_valid_pins_as_output);
    RUN_TEST(test_pin_mode_all_valid_pins_as_input);
    RUN_TEST(test_pin_mode_invalid_pin_negative);
    RUN_TEST(test_pin_mode_invalid_pin_too_high);
    RUN_TEST(test_pin_mode_boundary_pins);
    RUN_TEST(test_pin_mode_without_init);
    RUN_TEST(test_pin_mode_rapid_mode_switching);
    
    // GPIO set function tests
    RUN_TEST(test_gpio_set_function_all_alt_modes);
    RUN_TEST(test_gpio_set_function_invalid_pin);
    RUN_TEST(test_gpio_set_function_without_init);
    
    // Digital write tests
    RUN_TEST(test_digital_write_all_valid_pins_high);
    RUN_TEST(test_digital_write_all_valid_pins_low);
    RUN_TEST(test_digital_write_invalid_pin_negative);
    RUN_TEST(test_digital_write_invalid_pin_too_high);
    RUN_TEST(test_digital_write_without_init);
    RUN_TEST(test_digital_write_rapid_toggle);
    RUN_TEST(test_digital_write_boundary_pins);
    RUN_TEST(test_digital_write_pins_31_32_boundary);
    
    // Digital read tests
    RUN_TEST(test_digital_read_returns_low_in_emulation);
    RUN_TEST(test_digital_read_invalid_pin_negative);
    RUN_TEST(test_digital_read_invalid_pin_too_high);
    RUN_TEST(test_digital_read_without_init);
    RUN_TEST(test_digital_read_all_valid_pins);
    RUN_TEST(test_digital_read_pins_31_32_boundary);
    
    // Constants validation
    RUN_TEST(test_constants_input_output_values);
    RUN_TEST(test_constants_high_low_values);
    RUN_TEST(test_constants_alt_function_values);
    RUN_TEST(test_constants_pin_range);
    
    // Stress tests
    RUN_TEST(test_stress_many_operations);
    RUN_TEST(test_stress_full_gpio_cycle);
    
    return UNITY_END();
}
