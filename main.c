#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"
#include <unistd.h>
#include <stdio.h>

int main() {
    if (gpio_init() != 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return 1;
    }

    int pin = 21;
    printf("Starting GPIO Blink on Pin %d...\n", pin);
    
    pin_mode(pin, OUTPUT);

    for (int i = 0; i < 5; i++) {
        printf("Blinking...\n");
        digital_write(pin, HIGH);
        sleep(1);
        digital_write(pin, LOW);
        sleep(1);
    }

    gpio_cleanup();
    printf("Done.\n");
    return 0;
}
