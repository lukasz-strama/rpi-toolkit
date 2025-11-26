/*
 * lib_toolkit.c - Shared library entry point for Python bindings
 *
 * This file bundles all the single-header implementations into one
 * compilation unit so we can build a .so file for ctypes.
 */

#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"

#define RPI_PWM_IMPLEMENTATION
#include "rpi_pwm.h"
