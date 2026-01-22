/*
 * rpi_realtime.h - Optional Real-Time and CPU Affinity Utilities
 *
 * Provides functions to minimize jitter in timing-critical applications:
 * - set_realtime_priority(): Switch to SCHED_FIFO real-time scheduling
 * - pin_to_core(): Bind the current thread to a specific CPU core
 *
 * Usage:
 *   #define RPI_REALTIME_IMPLEMENTATION
 *   #include "rpi_realtime.h"
 *
 *   int main() {
 *       set_realtime_priority();  // Requires root
 *       pin_to_core(3);           // Pin to core 3 (combine with isolcpus=3)
 *       // ... your timing-critical code ...
 *   }
 *
 * IMPORTANT: These are OPTIONAL optimizations for reducing jitter.
 * The toolkit works without them, but real-time priority and CPU pinning
 * can significantly improve timing consistency for demanding applications.
 *
 * For maximum effect, combine with Linux kernel core isolation:
 *   Edit /boot/cmdline.txt and add: isolcpus=3
 *   Then use pin_to_core(3) in your code.
 */

#ifndef RPI_REALTIME_H
#define RPI_REALTIME_H

/* _GNU_SOURCE must be defined before any system headers for pthread_setaffinity_np */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set real-time SCHED_FIFO scheduling policy with maximum priority.
 *
 * This tells the Linux scheduler to give your process the highest priority
 * and not preempt it for other normal processes. Only kernel interrupts
 * and higher-priority real-time tasks can interrupt your code.
 *
 * Returns: 0 on success, -1 on error (check errno, likely EPERM - need root)
 *
 * REQUIRES: Run with sudo/root privileges
 */
int set_realtime_priority(void);

/**
 * Pin the current thread to a specific CPU core.
 *
 * This prevents the scheduler from migrating your thread between cores,
 * which eliminates cache misses and reduces jitter from core switching.
 *
 * @param core_id  The CPU core to pin to (0-3 on RPi 4)
 * Returns: 0 on success, -1 on error
 *
 * TIP: Combine with isolcpus kernel parameter for best results.
 *      Add "isolcpus=3" to /boot/cmdline.txt, then use pin_to_core(3).
 */
int pin_to_core(int core_id);

/**
 * Get the number of available CPU cores.
 *
 * Returns: Number of CPU cores, or -1 on error
 */
int get_cpu_count(void);

#ifdef __cplusplus
}
#endif

#endif /* RPI_REALTIME_H */

/* ===========================================================================
 * Implementation
 * ===========================================================================*/
#ifdef RPI_REALTIME_IMPLEMENTATION

#include <unistd.h>
#include <string.h>

int set_realtime_priority(void) {
    struct sched_param param;

    /* Get maximum priority for SCHED_FIFO (usually 99) */
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (param.sched_priority == -1) {
        perror("rpi_realtime: sched_get_priority_max failed");
        return -1;
    }

    /* Set SCHED_FIFO policy for current process (pid=0 means self) */
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("rpi_realtime: Failed to set SCHED_FIFO (run with sudo?)");
        return -1;
    }

    printf("rpi_realtime: Set SCHED_FIFO with priority %d\n", param.sched_priority);
    return 0;
}

int pin_to_core(int core_id) {
    cpu_set_t cpuset;

    /* Validate core_id */
    int num_cores = get_cpu_count();
    if (num_cores == -1) {
        return -1;
    }
    if (core_id < 0 || core_id >= num_cores) {
        fprintf(stderr, "rpi_realtime: Invalid core_id %d (valid: 0-%d)\n",
                core_id, num_cores - 1);
        return -1;
    }

    /* Create CPU set with only the specified core */
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    /* Bind current thread to the specified core */
    pthread_t current_thread = pthread_self();
    if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("rpi_realtime: pthread_setaffinity_np failed");
        return -1;
    }

    printf("rpi_realtime: Thread pinned to core %d\n", core_id);
    return 0;
}

int get_cpu_count(void) {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count == -1) {
        perror("rpi_realtime: sysconf(_SC_NPROCESSORS_ONLN) failed");
        return -1;
    }
    return (int)count;
}

#endif /* RPI_REALTIME_IMPLEMENTATION */
