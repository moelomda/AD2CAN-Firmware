/* ============================================================================
 * Test Main - Compiles and runs ALL test suites
 * ============================================================================
 * Each test file is included as a separate compilation unit with its own
 * static variables. This is the single entry point for all tests.
 *
 * Build: gcc -o run_tests test_main.c Mocks/mock_stm32_hal.c Mocks/mock_cmsis_os.c
 *        -I Mocks -I ../App/Inc -I ../App/Inc/HAL -I ../App/Inc/Drivers
 *        -I ../App/Inc/Tasks -lm
 * ============================================================================ */
#include <stdio.h>
#include <stdlib.h>
#include "Unity/unity.h"

/* ---- Forward declarations for all test suite runners ---- */
void run_sensor_tests(void);
void run_adc_hal_tests(void);
void run_can_hal_tests(void);
void run_tim_hal_tests(void);
void run_sensor_task_tests(void);
void run_brake_light_task_tests(void);
void run_can_rx_task_tests(void);
void run_can_tx_task_tests(void);
void run_app_init_tests(void);
void run_integration_tests(void);
void run_acceptance_tests(void);

int main(int argc, char **argv)
{
    printf("\n");
    printf("################################################################\n");
    printf("#                                                              #\n");
    printf("#        HIGH VOLTAGE TEAM - COMPLETE TEST SUITE               #\n");
    printf("#        STM32G473 Sensor Node - Full System Validation        #\n");
    printf("#                                                              #\n");
    printf("################################################################\n");

    /* ================================================================
     * PART 1: UNIT TESTS
     * ================================================================ */
    printf("\n\n");
    printf("================================================================\n");
    printf("  PART 1: UNIT TESTS\n");
    printf("================================================================\n");

    /* 1.1 Sensor calculations (pure logic, no hardware) */
    run_sensor_tests();

    /* 1.2 ADC HAL layer */
    run_adc_hal_tests();

    /* 1.3 CAN HAL layer */
    run_can_hal_tests();

    /* 1.4 Timer HAL layer */
    run_tim_hal_tests();

    /* 1.5 Sensor Task */
    run_sensor_task_tests();

    /* 1.6 Brake Light Task */
    run_brake_light_task_tests();

    /* 1.7 CAN RX Task */
    run_can_rx_task_tests();

    /* 1.8 CAN TX Task */
    run_can_tx_task_tests();

    /* 1.9 App Initialization */
    run_app_init_tests();

    /* ================================================================
     * PART 2: INTEGRATION TESTS
     * ================================================================ */
    printf("\n\n");
    printf("================================================================\n");
    printf("  PART 2: INTEGRATION TESTS\n");
    printf("================================================================\n");

    run_integration_tests();

    /* ================================================================
     * PART 3: ACCEPTANCE TESTS
     * ================================================================ */
    printf("\n\n");
    printf("================================================================\n");
    printf("  PART 3: ACCEPTANCE TESTS\n");
    printf("================================================================\n");

    run_acceptance_tests();

    /* ================================================================
     * FINAL SUMMARY
     * ================================================================ */
    int result = unity_print_summary();
    
    if (result == 0) {
        printf("\n  >>> HARDWARE READY: All tests passed! <<<\n");
        printf("  >>> System simulation complete - safe to deploy to STM32 <<<\n\n");
    } else {
        printf("\n  >>> WARNING: %d test(s) failed! Fix before hardware deployment <<<\n\n", result);
    }

    return result;
}
