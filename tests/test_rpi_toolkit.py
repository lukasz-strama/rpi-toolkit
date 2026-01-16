"""
test_rpi_toolkit.py - Validation tests for the Python wrapper

These tests validate the rpi_toolkit.py Python bindings in EMULATION MODE.
Focus: type handling, wrapper correctness, exception safety.
"""

import pytest
import sys
import os
import time
import ctypes

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from rpi_toolkit import (
    # Constants
    INPUT, OUTPUT, LOW, HIGH,
    ALT0, ALT1, ALT2, ALT3, ALT4, ALT5,
    # Types
    SimpleTimer,
    # GPIO functions
    gpio_init, gpio_cleanup, pin_mode, gpio_set_function,
    digital_write, digital_read,
    # Timer functions
    timer_set, timer_expired, timer_tick,
    millis, micros, delay_ms, delay_us,
    # Software PWM functions
    pwm_init, pwm_init_freq, pwm_write, pwm_stop,
    # Hardware PWM functions
    hpwm_init, hpwm_set, hpwm_stop,
)


# ============================================================================
# CONSTANTS VALIDATION TESTS
# ============================================================================

class TestConstants:
    """Validate that Python constants match C header values."""
    
    def test_input_output_values(self):
        assert INPUT == 0
        assert OUTPUT == 1
    
    def test_high_low_values(self):
        assert LOW == 0
        assert HIGH == 1
    
    def test_alt_function_values(self):
        """Verify ALT function values match BCM2711 specification."""
        assert ALT0 == 4
        assert ALT1 == 5
        assert ALT2 == 6
        assert ALT3 == 7
        assert ALT4 == 3
        assert ALT5 == 2


# ============================================================================
# GPIO WRAPPER TESTS
# ============================================================================

class TestGPIOWrapper:
    """Test GPIO wrapper functions."""
    
    def test_gpio_init_returns_int(self):
        result = gpio_init()
        assert isinstance(result, int)
        assert result == 0
        gpio_cleanup()
    
    def test_gpio_cleanup_no_exception(self):
        gpio_init()
        gpio_cleanup()  # Should not raise
    
    def test_gpio_cleanup_without_init(self):
        gpio_cleanup()  # Should not raise
    
    def test_gpio_multiple_init_cleanup(self):
        for _ in range(10):
            assert gpio_init() == 0
            gpio_cleanup()
    
    def test_pin_mode_accepts_constants(self):
        gpio_init()
        pin_mode(18, OUTPUT)
        pin_mode(18, INPUT)
        gpio_cleanup()
    
    def test_pin_mode_accepts_int(self):
        gpio_init()
        pin_mode(18, 0)  # INPUT
        pin_mode(18, 1)  # OUTPUT
        gpio_cleanup()
    
    def test_pin_mode_all_valid_pins(self):
        gpio_init()
        for pin in range(54):  # 0-53
            pin_mode(pin, OUTPUT)
        gpio_cleanup()
    
    def test_pin_mode_boundary_pins(self):
        gpio_init()
        pin_mode(0, OUTPUT)   # First valid
        pin_mode(53, OUTPUT)  # Last valid
        gpio_cleanup()
    
    def test_gpio_set_function_all_alt_modes(self):
        gpio_init()
        alt_modes = [ALT0, ALT1, ALT2, ALT3, ALT4, ALT5]
        for mode in alt_modes:
            gpio_set_function(18, mode)
        gpio_cleanup()
    
    def test_digital_write_accepts_high_low(self):
        gpio_init()
        pin_mode(18, OUTPUT)
        digital_write(18, HIGH)
        digital_write(18, LOW)
        gpio_cleanup()
    
    def test_digital_write_accepts_int(self):
        gpio_init()
        pin_mode(18, OUTPUT)
        digital_write(18, 1)
        digital_write(18, 0)
        gpio_cleanup()
    
    def test_digital_write_rapid_toggle(self):
        gpio_init()
        pin_mode(18, OUTPUT)
        for i in range(1000):
            digital_write(18, i % 2)
        gpio_cleanup()
    
    def test_digital_read_returns_int(self):
        gpio_init()
        pin_mode(18, INPUT)
        value = digital_read(18)
        assert isinstance(value, int)
        gpio_cleanup()
    
    def test_digital_read_returns_low_in_emulation(self):
        gpio_init()
        for pin in range(28):
            pin_mode(pin, INPUT)
            assert digital_read(pin) == LOW
        gpio_cleanup()


# ============================================================================
# TIMER WRAPPER TESTS
# ============================================================================

class TestTimerWrapper:
    """Test timer wrapper functions."""
    
    def test_millis_returns_int(self):
        result = millis()
        assert isinstance(result, int)
    
    def test_millis_positive(self):
        assert millis() > 0
    
    def test_millis_monotonically_increasing(self):
        prev = millis()
        for _ in range(100):
            curr = millis()
            assert curr >= prev
            prev = curr
    
    def test_micros_returns_int(self):
        result = micros()
        assert isinstance(result, int)
    
    def test_micros_positive(self):
        assert micros() > 0
    
    def test_micros_monotonically_increasing(self):
        prev = micros()
        for _ in range(100):
            curr = micros()
            assert curr >= prev
            prev = curr
    
    def test_micros_greater_than_millis(self):
        m = millis()
        u = micros()
        # micros should be roughly millis * 1000
        assert u >= m * 1000 - 10000
    
    def test_delay_ms_blocks(self):
        start = millis()
        delay_ms(50)
        elapsed = millis() - start
        assert elapsed >= 50
    
    def test_delay_ms_precision(self):
        start = millis()
        delay_ms(100)
        elapsed = millis() - start
        assert 100 <= elapsed <= 130
    
    def test_delay_ms_zero(self):
        start = millis()
        delay_ms(0)
        elapsed = millis() - start
        assert elapsed < 10
    
    def test_delay_us_blocks(self):
        start = micros()
        delay_us(10000)  # 10ms
        elapsed = micros() - start
        assert elapsed >= 10000
    
    def test_delay_us_precision(self):
        start = micros()
        delay_us(50000)  # 50ms
        elapsed = micros() - start
        assert 50000 <= elapsed <= 60000


class TestSimpleTimer:
    """Test SimpleTimer struct and related functions."""
    
    def test_simple_timer_struct_creation(self):
        t = SimpleTimer()
        assert hasattr(t, 'next_expiry')
        assert hasattr(t, 'interval')
    
    def test_simple_timer_struct_fields(self):
        t = SimpleTimer()
        t.next_expiry = 12345
        t.interval = 1000
        assert t.next_expiry == 12345
        assert t.interval == 1000
    
    def test_timer_set_initializes(self):
        t = SimpleTimer()
        timer_set(t, 1000)
        assert t.interval == 1000
        assert t.next_expiry > 0
    
    def test_timer_set_various_intervals(self):
        t = SimpleTimer()
        for interval in [0, 1, 10, 100, 1000, 10000]:
            timer_set(t, interval)
            assert t.interval == interval
    
    def test_timer_expired_returns_bool(self):
        t = SimpleTimer()
        timer_set(t, 1000)
        result = timer_expired(t)
        assert isinstance(result, bool)
    
    def test_timer_expired_false_before_interval(self):
        t = SimpleTimer()
        timer_set(t, 1000)  # 1 second
        assert timer_expired(t) == False
    
    def test_timer_expired_true_after_interval(self):
        t = SimpleTimer()
        timer_set(t, 10)  # 10ms
        time.sleep(0.020)  # 20ms
        assert timer_expired(t) == True
    
    def test_timer_expired_does_not_reset(self):
        t = SimpleTimer()
        timer_set(t, 10)
        time.sleep(0.020)
        assert timer_expired(t) == True
        assert timer_expired(t) == True  # Still expired
    
    def test_timer_tick_returns_bool(self):
        t = SimpleTimer()
        timer_set(t, 1000)
        result = timer_tick(t)
        assert isinstance(result, bool)
    
    def test_timer_tick_false_before_interval(self):
        t = SimpleTimer()
        timer_set(t, 1000)
        assert timer_tick(t) == False
    
    def test_timer_tick_true_after_interval(self):
        t = SimpleTimer()
        timer_set(t, 10)
        time.sleep(0.020)
        assert timer_tick(t) == True
    
    def test_timer_tick_auto_advances(self):
        t = SimpleTimer()
        timer_set(t, 10)
        time.sleep(0.020)
        assert timer_tick(t) == True
        # After tick, should not be expired again immediately
        assert timer_tick(t) == False
    
    def test_timer_tick_multiple_intervals(self):
        t = SimpleTimer()
        timer_set(t, 20)  # 20ms
        
        tick_count = 0
        start = millis()
        
        while millis() - start < 100:
            if timer_tick(t):
                tick_count += 1
            time.sleep(0.001)
        
        # Should have ~4-5 ticks
        assert 3 <= tick_count <= 6


# ============================================================================
# SOFTWARE PWM WRAPPER TESTS
# ============================================================================

class TestPWMWrapper:
    """Test software PWM wrapper functions."""
    
    def test_pwm_init_returns_int(self):
        gpio_init()
        result = pwm_init(18)
        assert isinstance(result, int)
        assert result == 0
        pwm_stop(18)
        gpio_cleanup()
    
    def test_pwm_init_freq_returns_int(self):
        gpio_init()
        result = pwm_init_freq(18, 500)
        assert isinstance(result, int)
        assert result == 0
        pwm_stop(18)
        gpio_cleanup()
    
    def test_pwm_write_accepts_duty(self):
        gpio_init()
        pwm_init(18)
        for duty in [0, 25, 50, 75, 100]:
            pwm_write(18, duty)
        pwm_stop(18)
        gpio_cleanup()
    
    def test_pwm_write_clamping(self):
        gpio_init()
        pwm_init(18)
        # These should be clamped, not raise
        pwm_write(18, -10)
        pwm_write(18, 150)
        pwm_stop(18)
        gpio_cleanup()
    
    def test_pwm_stop_no_exception(self):
        gpio_init()
        pwm_init(18)
        pwm_stop(18)  # Should not raise
        gpio_cleanup()
    
    def test_pwm_stop_without_init(self):
        gpio_init()
        pwm_stop(18)  # Should not raise
        gpio_cleanup()
    
    def test_pwm_multiple_pins(self):
        gpio_init()
        pins = [17, 18, 22, 23]
        for pin in pins:
            assert pwm_init(pin) == 0
        for pin in pins:
            pwm_write(pin, 50)
        for pin in pins:
            pwm_stop(pin)
        gpio_cleanup()
    
    def test_pwm_init_stop_cycles(self):
        gpio_init()
        for _ in range(10):
            pwm_init(18)
            pwm_write(18, 50)
            pwm_stop(18)
        gpio_cleanup()


# ============================================================================
# HARDWARE PWM WRAPPER TESTS
# ============================================================================

class TestHWPWMWrapper:
    """Test hardware PWM wrapper functions."""
    
    def test_hpwm_init_returns_int(self):
        result = hpwm_init()
        assert isinstance(result, int)
        assert result == 0
        hpwm_stop()
    
    def test_hpwm_set_valid_pins(self):
        gpio_init()
        hpwm_init()
        for pin in [12, 13, 18, 19]:
            hpwm_set(pin, 1000, 500)
        hpwm_stop()
        gpio_cleanup()
    
    def test_hpwm_set_invalid_pins(self):
        gpio_init()
        hpwm_init()
        # Invalid pins should be silently ignored
        for pin in [0, 1, 17, 20, 100]:
            hpwm_set(pin, 1000, 500)
        hpwm_stop()
        gpio_cleanup()
    
    def test_hpwm_set_duty_range(self):
        gpio_init()
        hpwm_init()
        for duty in [0, 250, 500, 750, 1000]:
            hpwm_set(18, 1000, duty)
        hpwm_stop()
        gpio_cleanup()
    
    def test_hpwm_set_duty_clamping(self):
        gpio_init()
        hpwm_init()
        # Should be clamped, not raise
        hpwm_set(18, 1000, -10)
        hpwm_set(18, 1000, 1500)
        hpwm_stop()
        gpio_cleanup()
    
    def test_hpwm_set_frequency_range(self):
        gpio_init()
        hpwm_init()
        for freq in [1, 50, 1000, 10000, 100000]:
            hpwm_set(18, freq, 500)
        hpwm_stop()
        gpio_cleanup()
    
    def test_hpwm_stop_no_exception(self):
        hpwm_init()
        hpwm_stop()  # Should not raise
    
    def test_hpwm_stop_without_init(self):
        hpwm_stop()  # Should not raise
    
    def test_hpwm_init_stop_cycles(self):
        gpio_init()
        for _ in range(10):
            hpwm_init()
            hpwm_set(18, 50, 75)
            hpwm_stop()
        gpio_cleanup()


# ============================================================================
# TYPE CONVERSION TESTS
# ============================================================================

class TestTypeConversion:
    """Test that Python types are correctly converted to C types."""
    
    def test_pin_as_float_rejected(self):
        gpio_init()
        # ctypes correctly rejects float arguments - this is proper type safety
        try:
            pin_mode(18.7, OUTPUT)  # type: ignore
            gpio_cleanup()
            assert False, "Expected ctypes to reject float"
        except (ctypes.ArgumentError, TypeError):
            pass  # Expected behavior
        gpio_cleanup()
    
    def test_large_values(self):
        gpio_init()
        pwm_init(18)
        
        # Large duty values should be clamped
        pwm_write(18, 999999)
        pwm_write(18, -999999)
        
        pwm_stop(18)
        gpio_cleanup()
    
    def test_timer_large_interval(self):
        t = SimpleTimer()
        timer_set(t, 999999999)
        assert t.interval == 999999999


# ============================================================================
# INTEGRATION TESTS
# ============================================================================

class TestPythonIntegration:
    """Integration tests using Python wrapper."""
    
    def test_full_gpio_lifecycle(self):
        assert gpio_init() == 0
        
        pin_mode(21, OUTPUT)
        digital_write(21, HIGH)
        digital_write(21, LOW)
        
        pin_mode(21, INPUT)
        value = digital_read(21)
        assert value == LOW
        
        gpio_cleanup()
    
    def test_full_pwm_lifecycle(self):
        gpio_init()
        
        assert pwm_init(18) == 0
        assert hpwm_init() == 0
        
        pwm_write(18, 50)
        hpwm_set(12, 50, 500)
        
        pwm_stop(18)
        hpwm_stop()
        gpio_cleanup()
    
    def test_timer_based_blink(self):
        gpio_init()
        pin_mode(21, OUTPUT)
        
        t = SimpleTimer()
        timer_set(t, 10)
        
        state = LOW
        toggle_count = 0
        start = millis()
        
        while millis() - start < 100:
            if timer_tick(t):
                state = HIGH if state == LOW else LOW
                digital_write(21, state)
                toggle_count += 1
            time.sleep(0.001)
        
        assert 7 <= toggle_count <= 12
        gpio_cleanup()
    
    def test_pwm_fade(self):
        gpio_init()
        pwm_init(18)
        
        for duty in range(0, 101, 5):
            pwm_write(18, duty)
        for duty in range(100, -1, -5):
            pwm_write(18, duty)
        
        pwm_stop(18)
        gpio_cleanup()
    
    def test_concurrent_operations(self):
        gpio_init()
        
        pin_mode(17, OUTPUT)
        pin_mode(21, OUTPUT)
        pwm_init(18)
        hpwm_init()
        
        for i in range(50):
            digital_write(17, i % 2)
            digital_write(21, (i + 1) % 2)
            pwm_write(18, i % 101)
            hpwm_set(12, 50, (i * 10) % 1001)
        
        pwm_stop(18)
        hpwm_stop()
        gpio_cleanup()


# ============================================================================
# STRESS TESTS
# ============================================================================

class TestPythonStress:
    """Stress tests for Python wrapper."""
    
    def test_stress_many_gpio_operations(self):
        gpio_init()
        pin_mode(18, OUTPUT)
        
        for i in range(10000):
            digital_write(18, i % 2)
        
        gpio_cleanup()
    
    def test_stress_timer_rapid_set(self):
        t = SimpleTimer()
        for i in range(1000):
            timer_set(t, i % 100 + 1)
    
    def test_stress_pwm_rapid_changes(self):
        gpio_init()
        pwm_init(18)
        
        for i in range(1000):
            pwm_write(18, i % 101)
        
        pwm_stop(18)
        gpio_cleanup()
    
    def test_stress_full_system_cycles(self):
        for _ in range(20):
            gpio_init()
            pwm_init(18)
            hpwm_init()
            
            for i in range(50):
                digital_write(21, i % 2)
                pwm_write(18, i % 101)
                hpwm_set(12, 50, i % 1001)
            
            pwm_stop(18)
            hpwm_stop()
            gpio_cleanup()


# ============================================================================
# EDGE CASE TESTS  
# ============================================================================

class TestEdgeCases:
    """Edge case and boundary condition tests."""
    
    def test_boundary_pin_0(self):
        gpio_init()
        pin_mode(0, OUTPUT)
        digital_write(0, HIGH)
        digital_write(0, LOW)
        pin_mode(0, INPUT)
        digital_read(0)
        gpio_cleanup()
    
    def test_boundary_pin_53(self):
        gpio_init()
        pin_mode(53, OUTPUT)
        digital_write(53, HIGH)
        digital_write(53, LOW)
        pin_mode(53, INPUT)
        digital_read(53)
        gpio_cleanup()
    
    def test_timer_zero_interval(self):
        t = SimpleTimer()
        timer_set(t, 0)
        # Should be immediately expired
        assert timer_expired(t) == True
    
    def test_pwm_duty_exactly_0(self):
        gpio_init()
        pwm_init(18)
        pwm_write(18, 0)
        pwm_stop(18)
        gpio_cleanup()
    
    def test_pwm_duty_exactly_100(self):
        gpio_init()
        pwm_init(18)
        pwm_write(18, 100)
        pwm_stop(18)
        gpio_cleanup()
    
    def test_hpwm_duty_exactly_0(self):
        gpio_init()
        hpwm_init()
        hpwm_set(18, 1000, 0)
        hpwm_stop()
        gpio_cleanup()
    
    def test_hpwm_duty_exactly_1000(self):
        gpio_init()
        hpwm_init()
        hpwm_set(18, 1000, 1000)
        hpwm_stop()
        gpio_cleanup()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
