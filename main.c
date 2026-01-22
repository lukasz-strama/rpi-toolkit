#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"
#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"
#define RPI_PWM_IMPLEMENTATION
#include "rpi_pwm.h"
#define RPI_HW_PWM_IMPLEMENTATION
#include "rpi_hw_pwm.h"
#include <unistd.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Configuration Constants
 * ---------------------------------------------------------------------------*/
#define LED_PIN         21
#define SW_PWM_PIN      18
#define HW_PWM_PIN      12

#define BLINK_INTERVAL_MS       500
#define SENSOR_POLL_INTERVAL_MS 100
#define PWM_UPDATE_INTERVAL_MS  1000
#define DEMO_DURATION_MS        5000

#define SERVO_FREQ_HZ   50
#define SERVO_NEUTRAL   75   /* 7.5% duty in per-mille */
#define PWM_STEP        25

#define LOOP_SLEEP_US   1000

int main() {
    /* -----------------------------------------------------------------------
     * Initialization
     * -----------------------------------------------------------------------*/
    if (gpio_init() != 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return 1;
    }

    printf("Starting Non-Blocking GPIO Blink on Pin %d...\n", LED_PIN);
    printf("Starting Software PWM on Pin %d...\n", SW_PWM_PIN);
    printf("Starting Hardware PWM on Pin %d...\n", HW_PWM_PIN);
    
    pin_mode(LED_PIN, OUTPUT);
    
    if (pwm_init(SW_PWM_PIN) != 0) {
        fprintf(stderr, "Failed to init PWM\n");
    }

    if (hpwm_init() != 0) {
        fprintf(stderr, "Failed to init HW PWM\n");
    }

    /* Set HW PWM to 50Hz (Servo), 7.5% duty (Neutral) */
    hpwm_set(HW_PWM_PIN, SERVO_FREQ_HZ, SERVO_NEUTRAL);

    simple_timer_t blink_timer;
    simple_timer_t sensor_timer;
    simple_timer_t pwm_timer;

    timer_set(&blink_timer, BLINK_INTERVAL_MS);
    timer_set(&sensor_timer, SENSOR_POLL_INTERVAL_MS);
    timer_set(&pwm_timer, PWM_UPDATE_INTERVAL_MS);

    int led_state = LOW;
    int pwm_duty = 0;

    /* -----------------------------------------------------------------------
     * Main Loop
     * -----------------------------------------------------------------------*/
    uint64_t start_time = millis();
    while (millis() - start_time < DEMO_DURATION_MS) {
        
        if (timer_tick(&blink_timer)) {
            led_state = !led_state;
            digital_write(LED_PIN, led_state);
            printf("Blink! LED is %s\n", led_state ? "HIGH" : "LOW");
        }

        if (timer_tick(&sensor_timer)) {
            /* Sensor polling placeholder */
        }

        if (timer_tick(&pwm_timer)) {
            pwm_duty += PWM_STEP;
            if (pwm_duty > 100) {
                pwm_duty = 0;
            }
            pwm_write(SW_PWM_PIN, pwm_duty);
            
            /* Scale 0-100 to 0-1000 for HW PWM */
            hpwm_set(HW_PWM_PIN, SERVO_FREQ_HZ, pwm_duty * 10);
        }

        /* Minimal sleep to prevent CPU hogging */
        usleep(LOOP_SLEEP_US); 
    }

    /* -----------------------------------------------------------------------
     * Cleanup
     * -----------------------------------------------------------------------*/
    pwm_stop(SW_PWM_PIN);
    hpwm_stop();
    gpio_cleanup();
    printf("Done.\n");
    return 0;
}
