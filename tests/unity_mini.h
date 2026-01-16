/*
 * unity_mini.h - Minimal C Unit Testing Framework
 * 
 * A lightweight, single-header testing framework for C validation tests.
 * Designed for embedded systems and portable code testing.
 */

#ifndef UNITY_MINI_H
#define UNITY_MINI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Test statistics */
static int unity_tests_run = 0;
static int unity_tests_passed = 0;
static int unity_tests_failed = 0;
static const char* unity_current_test = NULL;
static int unity_current_test_failed = 0;

/* ANSI color codes for terminal output */
#define UNITY_COLOR_RED     "\033[31m"
#define UNITY_COLOR_GREEN   "\033[32m"
#define UNITY_COLOR_YELLOW  "\033[33m"
#define UNITY_COLOR_RESET   "\033[0m"

/* Assertion macros */

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("  %s[FAIL]%s %s:%d: Assertion failed: %s\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, #condition); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("  %s[FAIL]%s %s:%d: Expected %lld but got %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(expected), (long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) TEST_ASSERT_EQUAL(expected, actual)

#define TEST_ASSERT_EQUAL_UINT64(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("  %s[FAIL]%s %s:%d: Expected %llu but got %llu\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (unsigned long long)(expected), (unsigned long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            printf("  %s[FAIL]%s %s:%d: Values should not be equal: %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(expected)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    do { \
        if ((actual) <= (threshold)) { \
            printf("  %s[FAIL]%s %s:%d: Expected > %lld but got %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(threshold), (long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual) \
    do { \
        if ((actual) < (threshold)) { \
            printf("  %s[FAIL]%s %s:%d: Expected >= %lld but got %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(threshold), (long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    do { \
        if ((actual) >= (threshold)) { \
            printf("  %s[FAIL]%s %s:%d: Expected < %lld but got %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(threshold), (long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_OR_EQUAL(threshold, actual) \
    do { \
        if ((actual) > (threshold)) { \
            printf("  %s[FAIL]%s %s:%d: Expected <= %lld but got %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(threshold), (long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_WITHIN(delta, expected, actual) \
    do { \
        long long diff = (long long)(expected) - (long long)(actual); \
        if (diff < 0) diff = -diff; \
        if (diff > (delta)) { \
            printf("  %s[FAIL]%s %s:%d: Expected %lld ± %lld but got %lld\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, \
                   (long long)(expected), (long long)(delta), (long long)(actual)); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(pointer) \
    do { \
        if ((pointer) == NULL) { \
            printf("  %s[FAIL]%s %s:%d: Pointer should not be NULL\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(pointer) \
    do { \
        if ((pointer) != NULL) { \
            printf("  %s[FAIL]%s %s:%d: Pointer should be NULL\n", \
                   UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__); \
            unity_current_test_failed = 1; \
        } \
    } while(0)

#define TEST_PASS() \
    do { /* no-op, test passes by default */ } while(0)

#define TEST_FAIL_MESSAGE(msg) \
    do { \
        printf("  %s[FAIL]%s %s:%d: %s\n", \
               UNITY_COLOR_RED, UNITY_COLOR_RESET, __FILE__, __LINE__, msg); \
        unity_current_test_failed = 1; \
    } while(0)

/* Test runner macros */

#define RUN_TEST(test_func) \
    do { \
        unity_current_test = #test_func; \
        unity_current_test_failed = 0; \
        unity_tests_run++; \
        printf("Running: %s... ", #test_func); \
        fflush(stdout); \
        test_func(); \
        if (unity_current_test_failed) { \
            unity_tests_failed++; \
            printf("\n"); \
        } else { \
            unity_tests_passed++; \
            printf("%s[PASS]%s\n", UNITY_COLOR_GREEN, UNITY_COLOR_RESET); \
        } \
    } while(0)

#define UNITY_BEGIN() \
    do { \
        unity_tests_run = 0; \
        unity_tests_passed = 0; \
        unity_tests_failed = 0; \
        printf("\n%s========================================%s\n", UNITY_COLOR_YELLOW, UNITY_COLOR_RESET); \
        printf("Running Tests...\n"); \
        printf("%s========================================%s\n\n", UNITY_COLOR_YELLOW, UNITY_COLOR_RESET); \
    } while(0)

#define UNITY_END() \
    ( \
        printf("\n%s========================================%s\n", UNITY_COLOR_YELLOW, UNITY_COLOR_RESET), \
        printf("Tests: %d | ", unity_tests_run), \
        printf("%sPassed: %d%s | ", UNITY_COLOR_GREEN, unity_tests_passed, UNITY_COLOR_RESET), \
        printf("%sFailed: %d%s\n", \
               unity_tests_failed > 0 ? UNITY_COLOR_RED : UNITY_COLOR_GREEN, \
               unity_tests_failed, UNITY_COLOR_RESET), \
        printf("%s========================================%s\n\n", UNITY_COLOR_YELLOW, UNITY_COLOR_RESET), \
        unity_tests_failed \
    )

#endif /* UNITY_MINI_H */
