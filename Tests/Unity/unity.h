/* ============================================================================
 * Unity - Lightweight C Test Framework (Minimal Embedded Version)
 * Adapted for STM32 High Voltage Team project testing
 * ============================================================================ */
#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* ---- Test counters ---- */
static int unity_tests_run    = 0;
static int unity_tests_passed = 0;
static int unity_tests_failed = 0;
static const char* unity_current_test = "";

/* ---- Color codes for terminal output ---- */
#ifdef _WIN32
  #define UNITY_COLOR_RED    ""
  #define UNITY_COLOR_GREEN  ""
  #define UNITY_COLOR_YELLOW ""
  #define UNITY_COLOR_RESET  ""
#else
  #define UNITY_COLOR_RED    "\033[31m"
  #define UNITY_COLOR_GREEN  "\033[32m"
  #define UNITY_COLOR_YELLOW "\033[33m"
  #define UNITY_COLOR_RESET  "\033[0m"
#endif

/* ---- Core macros ---- */
#define RUN_TEST(func) do { \
    unity_current_test = #func; \
    unity_tests_run++; \
    printf("  Running %-60s ", #func); \
    func(); \
    unity_tests_passed++; \
    printf("[PASS]\n"); \
} while(0)

#define TEST_ASSERT(condition) do { \
    if (!(condition)) { \
        printf("[FAIL]\n    Assertion failed: %s\n    at %s:%d\n", \
               #condition, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_MESSAGE(condition, msg) do { \
    if (!(condition)) { \
        printf("[FAIL]\n    %s\n    Assertion: %s\n    at %s:%d\n", \
               msg, #condition, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL(expected, actual) do { \
    long long _e = (long long)(expected); \
    long long _a = (long long)(actual); \
    if (_e != _a) { \
        printf("[FAIL]\n    Expected: %lld  Actual: %lld\n    at %s:%d\n", \
               _e, _a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_UINT16(expected, actual) do { \
    uint16_t _e = (uint16_t)(expected); \
    uint16_t _a = (uint16_t)(actual); \
    if (_e != _a) { \
        printf("[FAIL]\n    Expected: %u  Actual: %u\n    at %s:%d\n", \
               (unsigned)_e, (unsigned)_a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_INT16(expected, actual) do { \
    int16_t _e = (int16_t)(expected); \
    int16_t _a = (int16_t)(actual); \
    if (_e != _a) { \
        printf("[FAIL]\n    Expected: %d  Actual: %d\n    at %s:%d\n", \
               (int)_e, (int)_a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_UINT32(expected, actual) do { \
    uint32_t _e = (uint32_t)(expected); \
    uint32_t _a = (uint32_t)(actual); \
    if (_e != _a) { \
        printf("[FAIL]\n    Expected: %u  Actual: %u\n    at %s:%d\n", \
               (unsigned)_e, (unsigned)_a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_HEX32(expected, actual) do { \
    uint32_t _e = (uint32_t)(expected); \
    uint32_t _a = (uint32_t)(actual); \
    if (_e != _a) { \
        printf("[FAIL]\n    Expected: 0x%08X  Actual: 0x%08X\n    at %s:%d\n", \
               _e, _a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_UINT8(expected, actual) do { \
    uint8_t _e = (uint8_t)(expected); \
    uint8_t _a = (uint8_t)(actual); \
    if (_e != _a) { \
        printf("[FAIL]\n    Expected: 0x%02X  Actual: 0x%02X\n    at %s:%d\n", \
               (unsigned)_e, (unsigned)_a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_TRUE(condition)  TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("[FAIL]\n    Expected non-NULL pointer\n    at %s:%d\n", \
               __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("[FAIL]\n    Expected NULL pointer\n    at %s:%d\n", \
               __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) do { \
    float _d = (float)(delta); \
    float _e = (float)(expected); \
    float _a = (float)(actual); \
    if (fabs(_e - _a) > _d) { \
        printf("[FAIL]\n    Expected: %f +/- %f  Actual: %f\n    at %s:%d\n", \
               _e, _d, _a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) do { \
    long long _t = (long long)(threshold); \
    long long _a = (long long)(actual); \
    if (!(_a > _t)) { \
        printf("[FAIL]\n    Expected > %lld  Actual: %lld\n    at %s:%d\n", \
               _t, _a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) do { \
    long long _t = (long long)(threshold); \
    long long _a = (long long)(actual); \
    if (!(_a < _t)) { \
        printf("[FAIL]\n    Expected < %lld  Actual: %lld\n    at %s:%d\n", \
               _t, _a, __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) do { \
    if (memcmp((expected), (actual), (len)) != 0) { \
        printf("[FAIL]\n    Memory mismatch (%d bytes)\n    at %s:%d\n", \
               (int)(len), __FILE__, __LINE__); \
        unity_tests_failed++; \
        unity_tests_passed--; \
        return; \
    } \
} while(0)

/* ---- Suite management ---- */
#define UNITY_BEGIN_SUITE(name) do { \
    printf("\n========================================\n"); \
    printf("  TEST SUITE: %s\n", name); \
    printf("========================================\n"); \
} while(0)

#define UNITY_END_SUITE() do { \
    /* nothing */ \
} while(0)

/* ---- Final summary ---- */
static inline int unity_print_summary(void) {
    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("  Total:  %d\n", unity_tests_run);
    printf("  Passed: %d\n", unity_tests_passed);
    printf("  Failed: %d\n", unity_tests_failed);
    printf("========================================\n");
    if (unity_tests_failed == 0) {
        printf("  ALL TESTS PASSED!\n");
    } else {
        printf("  *** %d TEST(S) FAILED ***\n", unity_tests_failed);
    }
    printf("========================================\n\n");
    return unity_tests_failed;
}

#endif /* UNITY_H */
