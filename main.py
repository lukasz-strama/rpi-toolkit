"""Demo application for rpi-toolkit Python bindings.

Demonstrates GPIO blinking, software PWM, and hardware PWM using
non-blocking timers.
"""
import time
import sys
from rpi_toolkit import (
    gpio_init, gpio_cleanup, pin_mode, digital_write, 
    timer_set, timer_tick, millis, pwm_init, pwm_write, pwm_stop,
    hpwm_init, hpwm_set, hpwm_stop,
    SimpleTimer, OUTPUT, HIGH, LOW
)

# ---------------------------------------------------------------------------
# Configuration Constants
# ---------------------------------------------------------------------------
LED_PIN = 21
SW_PWM_PIN = 18
HW_PWM_PIN = 12

BLINK_INTERVAL_MS = 500
SENSOR_POLL_INTERVAL_MS = 100
PWM_UPDATE_INTERVAL_MS = 1000
DEMO_DURATION_MS = 5000

SERVO_FREQ_HZ = 50
SERVO_NEUTRAL = 75  # 7.5% duty in per-mille
PWM_STEP = 25

LOOP_SLEEP_S = 0.001


def main():
    # -----------------------------------------------------------------------
    # Initialization
    # -----------------------------------------------------------------------
    if gpio_init() != 0:
        print("Failed to initialize GPIO", file=sys.stderr)
        return 1

    print(f"Starting Python GPIO Blink on Pin {LED_PIN}...")
    print(f"Starting Python Software PWM on Pin {SW_PWM_PIN}...")
    print(f"Starting Python Hardware PWM on Pin {HW_PWM_PIN}...")

    pin_mode(LED_PIN, OUTPUT)

    if pwm_init(SW_PWM_PIN) != 0:
        print("Failed to init PWM", file=sys.stderr)

    if hpwm_init() != 0:
        print("Failed to init HW PWM", file=sys.stderr)

    # Set HW PWM to 50Hz (Servo), 7.5% duty (Neutral)
    hpwm_set(HW_PWM_PIN, SERVO_FREQ_HZ, SERVO_NEUTRAL)

    blink_timer = SimpleTimer()
    sensor_timer = SimpleTimer()
    pwm_timer = SimpleTimer()

    timer_set(blink_timer, BLINK_INTERVAL_MS)
    timer_set(sensor_timer, SENSOR_POLL_INTERVAL_MS)
    timer_set(pwm_timer, PWM_UPDATE_INTERVAL_MS)

    led_state = LOW
    pwm_duty = 0

    # -----------------------------------------------------------------------
    # Main Loop
    # -----------------------------------------------------------------------
    start_time = millis()
    while millis() - start_time < DEMO_DURATION_MS:
        
        if timer_tick(blink_timer):
            led_state = not led_state
            digital_write(LED_PIN, HIGH if led_state else LOW)
            print(f"Blink! LED is {'HIGH' if led_state else 'LOW'}")

        if timer_tick(sensor_timer):
            # Sensor polling placeholder
            pass

        if timer_tick(pwm_timer):
            pwm_duty += PWM_STEP
            if pwm_duty > 100:
                pwm_duty = 0
            pwm_write(SW_PWM_PIN, pwm_duty)
            
            # Scale 0-100 to 0-1000 for HW PWM
            hpwm_set(HW_PWM_PIN, SERVO_FREQ_HZ, pwm_duty * 10)

        # Minimal sleep to prevent CPU hogging
        time.sleep(LOOP_SLEEP_S)

    # -----------------------------------------------------------------------
    # Cleanup
    # -----------------------------------------------------------------------
    pwm_stop(SW_PWM_PIN)
    hpwm_stop()
    gpio_cleanup()
    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
