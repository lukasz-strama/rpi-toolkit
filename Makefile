CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

TARGET = rpi_gpio_app

all: $(TARGET)

$(TARGET): main.c rpi_gpio.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET)
