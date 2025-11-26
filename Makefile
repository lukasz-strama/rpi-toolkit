CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

TARGET = rpi_gpio_app
LIB_TARGET = libtoolkit.so

all: $(TARGET) $(LIB_TARGET)

$(TARGET): main.c rpi_gpio.h simple_timer.h rpi_pwm.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c

$(LIB_TARGET): lib_toolkit.c rpi_gpio.h simple_timer.h rpi_pwm.h
	$(CC) $(CFLAGS) -shared -fPIC -o $(LIB_TARGET) lib_toolkit.c

clean:
	rm -f $(TARGET) $(LIB_TARGET)
