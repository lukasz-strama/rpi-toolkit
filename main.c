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

int main() {
    if (gpio_init() != 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return 1;
    }

    int led_pin = 21;
    int pwm_pin = 18;
    int hw_pwm_pin = 12;

    printf("Starting Non-Blocking GPIO Blink on Pin %d...\n", led_pin);
    printf("Starting Software PWM on Pin %d...\n", pwm_pin);
    printf("Starting Hardware PWM on Pin %d...\n", hw_pwm_pin);
    
    pin_mode(led_pin, OUTPUT);
    
    if (pwm_init(pwm_pin) != 0) {
        fprintf(stderr, "Failed to init PWM\n");
    }

    if (hpwm_init() != 0) {
        fprintf(stderr, "Failed to init HW PWM\n");
    }

    // Set HW PWM to 50Hz (Servo), 7.5% duty (Neutral)
    hpwm_set(hw_pwm_pin, 50, 75);

    simple_timer_t blink_timer;
    simple_timer_t sensor_timer;
    simple_timer_t pwm_timer;

    timer_set(&blink_timer, 500);
    timer_set(&sensor_timer, 100);
    timer_set(&pwm_timer, 1000); // Change PWM every second

    int led_state = LOW;
    int pwm_duty = 0;
    int pwm_step = 25;

    // Run for 5 seconds
    uint64_t start_time = millis();
    while (millis() - start_time < 5000) {
        
        if (timer_tick(&blink_timer)) {
            led_state = !led_state;
            digital_write(led_pin, led_state);
            printf("Blink! LED is %s\n", led_state ? "HIGH" : "LOW");
        }

        if (timer_tick(&sensor_timer)) {
            // printf("Checking sensors...\n"); // Commented out to reduce spam
        }

        if (timer_tick(&pwm_timer)) {
            pwm_duty += pwm_step;
            if (pwm_duty > 100) {
                pwm_duty = 0;
            }
            pwm_write(pwm_pin, pwm_duty);
            
            // Update HW PWM as well (just for demo)
            hpwm_set(hw_pwm_pin, 50, pwm_duty * 10); // Scale 0-100 to 0-1000
        }

        // minimal sleep to prevent CPU hogging
        usleep(1000); 
    }

    pwm_stop(pwm_pin);
    hpwm_stop();
    gpio_cleanup();
    printf("Done.\n");
    return 0;
}
