"""Python bindings for rpi-toolkit C library.

Provides access to GPIO, software PWM, hardware PWM, and timer functionality
for Raspberry Pi 4B via ctypes wrapper around libtoolkit.so.

Usage:
    from rpi_toolkit import gpio_init, pin_mode, digital_write, OUTPUT, HIGH

    gpio_init()
    pin_mode(21, OUTPUT)
    digital_write(21, HIGH)
"""
import ctypes
import os
import sys

# Load the shared library
# Assumes libtoolkit.so is in the same directory as this script
lib_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "libtoolkit.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError(f"Could not find {lib_path}. Did you run 'make'?")

_lib = ctypes.CDLL(lib_path)

# Public API - controls what 'from rpi_toolkit import *' exports
__all__ = [
    # Constants
    'INPUT', 'OUTPUT', 'LOW', 'HIGH',
    'ALT0', 'ALT1', 'ALT2', 'ALT3', 'ALT4', 'ALT5',
    # Types
    'SimpleTimer',
    # GPIO functions
    'gpio_init', 'gpio_cleanup', 'pin_mode', 'gpio_set_function',
    'digital_write', 'digital_read',
    # Timer functions
    'timer_set', 'timer_expired', 'timer_tick',
    'millis', 'micros', 'delay_ms', 'delay_us',
    # Software PWM functions
    'pwm_init', 'pwm_init_freq', 'pwm_write', 'pwm_stop',
    # Hardware PWM functions
    'hpwm_init', 'hpwm_set', 'hpwm_stop',
    # Real-time functions (optional jitter reduction)
    'set_realtime_priority', 'pin_to_core', 'get_cpu_count',
]

# Constants
INPUT = 0
OUTPUT = 1
LOW = 0
HIGH = 1
ALT0 = 4
ALT1 = 5
ALT2 = 6
ALT3 = 7
ALT4 = 3
ALT5 = 2

# ---------------------------------------------------------------------------
# Type Definitions
# ---------------------------------------------------------------------------

class SimpleTimer(ctypes.Structure):
    """Timer state structure matching C simple_timer_t."""
    _fields_ = [
        ("next_expiry", ctypes.c_uint64),
        ("interval", ctypes.c_uint64)
    ]

# Function Signatures

# int gpio_init(void);
_lib.gpio_init.argtypes = []
_lib.gpio_init.restype = ctypes.c_int

# void gpio_cleanup(void);
_lib.gpio_cleanup.argtypes = []
_lib.gpio_cleanup.restype = None

# void pin_mode(int pin, int mode);
_lib.pin_mode.argtypes = [ctypes.c_int, ctypes.c_int]
_lib.pin_mode.restype = None

# void gpio_set_function(int pin, int function);
_lib.gpio_set_function.argtypes = [ctypes.c_int, ctypes.c_int]
_lib.gpio_set_function.restype = None

# void digital_write(int pin, int value);
_lib.digital_write.argtypes = [ctypes.c_int, ctypes.c_int]
_lib.digital_write.restype = None

# int digital_read(int pin);
_lib.digital_read.argtypes = [ctypes.c_int]
_lib.digital_read.restype = ctypes.c_int

# void timer_set(simple_timer_t* t, uint64_t interval_ms);
_lib.timer_set.argtypes = [ctypes.POINTER(SimpleTimer), ctypes.c_uint64]
_lib.timer_set.restype = None

# bool timer_expired(simple_timer_t* t);
_lib.timer_expired.argtypes = [ctypes.POINTER(SimpleTimer)]
_lib.timer_expired.restype = ctypes.c_bool

# bool timer_tick(simple_timer_t* t);
_lib.timer_tick.argtypes = [ctypes.POINTER(SimpleTimer)]
_lib.timer_tick.restype = ctypes.c_bool

# uint64_t millis(void);
_lib.millis.argtypes = []
_lib.millis.restype = ctypes.c_uint64

# uint64_t micros(void);
_lib.micros.argtypes = []
_lib.micros.restype = ctypes.c_uint64

# void delay_ms(uint64_t ms);
_lib.delay_ms.argtypes = [ctypes.c_uint64]
_lib.delay_ms.restype = None

# void delay_us(uint64_t us);
_lib.delay_us.argtypes = [ctypes.c_uint64]
_lib.delay_us.restype = None

# int pwm_init(int pin);
_lib.pwm_init.argtypes = [ctypes.c_int]
_lib.pwm_init.restype = ctypes.c_int

# int pwm_init_freq(int pin, int freq_hz);
_lib.pwm_init_freq.argtypes = [ctypes.c_int, ctypes.c_int]
_lib.pwm_init_freq.restype = ctypes.c_int

# void pwm_write(int pin, int duty);
_lib.pwm_write.argtypes = [ctypes.c_int, ctypes.c_int]
_lib.pwm_write.restype = None

# void pwm_stop(int pin);
_lib.pwm_stop.argtypes = [ctypes.c_int]
_lib.pwm_stop.restype = None

# int hpwm_init(void);
_lib.hpwm_init.argtypes = []
_lib.hpwm_init.restype = ctypes.c_int

# void hpwm_set(int pin, int freq_hz, int duty_per_mille);
_lib.hpwm_set.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
_lib.hpwm_set.restype = None

# void hpwm_stop(void);
_lib.hpwm_stop.argtypes = []
_lib.hpwm_stop.restype = None

# int set_realtime_priority(void);
_lib.set_realtime_priority.argtypes = []
_lib.set_realtime_priority.restype = ctypes.c_int

# int pin_to_core(int core_id);
_lib.pin_to_core.argtypes = [ctypes.c_int]
_lib.pin_to_core.restype = ctypes.c_int

# int get_cpu_count(void);
_lib.get_cpu_count.argtypes = []
_lib.get_cpu_count.restype = ctypes.c_int

# ---------------------------------------------------------------------------
# GPIO Functions
# ---------------------------------------------------------------------------

def gpio_init():
    """Initialize GPIO subsystem. Returns 0 on success, -1 on error."""
    return _lib.gpio_init()

def gpio_cleanup():
    """Release GPIO resources."""
    _lib.gpio_cleanup()

def pin_mode(pin, mode):
    """Set pin direction (INPUT or OUTPUT)."""
    _lib.pin_mode(pin, mode)

def gpio_set_function(pin, function):
    """Set pin alternate function (ALT0-ALT5)."""
    _lib.gpio_set_function(pin, function)

def digital_write(pin, value):
    """Write digital output (LOW or HIGH)."""
    _lib.digital_write(pin, value)

def digital_read(pin):
    """Read digital input. Returns LOW or HIGH."""
    return _lib.digital_read(pin)

# ---------------------------------------------------------------------------
# Timer Functions
# ---------------------------------------------------------------------------

def timer_set(timer, interval_ms):
    """Initialize or reset a timer with the given interval in milliseconds."""
    _lib.timer_set(ctypes.byref(timer), interval_ms)

def timer_expired(timer):
    """Check if timer has expired (does not reset). Returns bool."""
    return _lib.timer_expired(ctypes.byref(timer))

def timer_tick(timer):
    """Check if timer has expired and auto-advance. Returns bool."""
    return _lib.timer_tick(ctypes.byref(timer))

def millis():
    """Get monotonic time in milliseconds."""
    return _lib.millis()

def micros():
    """Get monotonic time in microseconds."""
    return _lib.micros()

def delay_ms(ms):
    """Busy-wait delay in milliseconds."""
    _lib.delay_ms(ms)

def delay_us(us):
    """Busy-wait delay in microseconds."""
    _lib.delay_us(us)

# ---------------------------------------------------------------------------
# Software PWM Functions
# ---------------------------------------------------------------------------

def pwm_init(pin):
    """Initialize software PWM on a pin at 100 Hz. Returns 0 on success."""
    return _lib.pwm_init(pin)

def pwm_init_freq(pin, freq_hz):
    """Initialize software PWM on a pin at specified frequency. Returns 0 on success."""
    return _lib.pwm_init_freq(pin, freq_hz)

def pwm_write(pin, duty):
    """Set PWM duty cycle (0-100%)."""
    _lib.pwm_write(pin, duty)

def pwm_stop(pin):
    """Stop PWM on a pin and release resources."""
    _lib.pwm_stop(pin)

# ---------------------------------------------------------------------------
# Hardware PWM Functions
# ---------------------------------------------------------------------------

def hpwm_init():
    """Initialize hardware PWM controller. Requires root. Returns 0 on success."""
    return _lib.hpwm_init()

def hpwm_set(pin, freq_hz, duty_per_mille):
    """Set hardware PWM output. Pins: 12, 13 (ALT0), 18, 19 (ALT5). Duty in per-mille (0-1000)."""
    _lib.hpwm_set(pin, freq_hz, duty_per_mille)

def hpwm_stop():
    """Stop hardware PWM and release resources."""
    _lib.hpwm_stop()

# ---------------------------------------------------------------------------
# Real-Time Functions (Optional Jitter Reduction)
# ---------------------------------------------------------------------------

def set_realtime_priority():
    """Set SCHED_FIFO real-time scheduling with max priority.
    
    This tells Linux to give your process highest priority and not preempt it
    for other normal processes. Only kernel interrupts can interrupt your code.
    
    Returns: 0 on success, -1 on error (requires root/sudo)
    """
    return _lib.set_realtime_priority()

def pin_to_core(core_id):
    """Pin the current thread to a specific CPU core.
    
    Prevents scheduler from migrating your thread between cores,
    eliminating cache misses and reducing jitter from core switching.
    
    Args:
        core_id: CPU core to pin to (0-3 on RPi 4)
    
    Returns: 0 on success, -1 on error
    
    Tip: Combine with isolcpus kernel parameter for best results.
         Add "isolcpus=3" to /boot/cmdline.txt, then use pin_to_core(3).
    """
    return _lib.pin_to_core(core_id)

def get_cpu_count():
    """Get the number of available CPU cores.
    
    Returns: Number of CPU cores (4 on RPi 4), or -1 on error
    """
    return _lib.get_cpu_count()
