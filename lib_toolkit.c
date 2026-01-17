/*
 * lib_toolkit.c - Shared library entry point for Python bindings
 *
 * This file bundles all the single-header implementations into one
 * compilation unit so we can build a .so file for ctypes.
 */

/* Required for pthread_setaffinity_np in rpi_realtime.h */
#define _GNU_SOURCE

#define RPI_GPIO_IMPLEMENTATION
#include "rpi_gpio.h"

#define SIMPLE_TIMER_IMPLEMENTATION
#include "simple_timer.h"

#define RPI_PWM_IMPLEMENTATION
#include "rpi_pwm.h"

#define RPI_HW_PWM_IMPLEMENTATION
#include "rpi_hw_pwm.h"

#define RPI_REALTIME_IMPLEMENTATION
#include "rpi_realtime.h"

