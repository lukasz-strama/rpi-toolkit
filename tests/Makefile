CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -I..

# Test executables
TESTS = test_rpi_gpio test_simple_timer test_rpi_pwm test_rpi_hw_pwm test_integration

.PHONY: all clean run run_all

all: $(TESTS)

test_rpi_gpio: test_rpi_gpio.c unity_mini.h ../rpi_gpio.h
	$(CC) $(CFLAGS) -o $@ test_rpi_gpio.c

test_simple_timer: test_simple_timer.c unity_mini.h ../simple_timer.h
	$(CC) $(CFLAGS) -o $@ test_simple_timer.c

test_rpi_pwm: test_rpi_pwm.c unity_mini.h ../rpi_gpio.h ../rpi_pwm.h
	$(CC) $(CFLAGS) -o $@ test_rpi_pwm.c

test_rpi_hw_pwm: test_rpi_hw_pwm.c unity_mini.h ../rpi_gpio.h ../rpi_hw_pwm.h
	$(CC) $(CFLAGS) -o $@ test_rpi_hw_pwm.c

test_integration: test_integration.c unity_mini.h ../rpi_gpio.h ../simple_timer.h ../rpi_pwm.h ../rpi_hw_pwm.h
	$(CC) $(CFLAGS) -o $@ test_integration.c

run: all
	@echo "========================================"
	@echo "Running All C Tests"
	@echo "========================================"
	@failed=0; \
	for test in $(TESTS); do \
		echo ""; \
		echo ">>> Running $$test <<<"; \
		./$$test || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	echo "========================================"
	@echo "Test Suite Complete"
	@echo "========================================"

run_all: run
	@echo ""
	@echo ">>> Running Python Tests <<<"
	@cd .. && python3 -m pytest tests/test_rpi_toolkit.py -v --tb=short

clean:
	rm -f $(TESTS)
