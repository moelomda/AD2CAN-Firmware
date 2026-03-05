/* ============================================================================
 * Unit Tests: sensors.c - Sensor calculation and filtering functions
 * ============================================================================
 * Tests all pure computation functions:
 *   - Sensor_CalculatePressure()
 *   - Sensor_CalculateSteeringAngle()
 *   - Sensor_FilterRPM_Left()
 *   - Sensor_FilterRPM_Right()
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../../App/Inc/Drivers/sensors.h"
#include "../../App/Src/Drivers/sensors.c"  /* Include source for white-box testing */

/* ============================================================================
 * Sensor_CalculatePressure Tests
 * ============================================================================ */

/* Test: Nominal middle value */
void test_pressure_nominal_middle(void)
{
    /* Middle of range: (2005 - 400) * 20000 / (3610 - 400) = ~9984 */
    uint16_t result = Sensor_CalculatePressure(2005);
    TEST_ASSERT_TRUE(result > 9900 && result < 10100);
}

/* Test: Minimum valid ADC value */
void test_pressure_at_minimum_valid(void)
{
    uint16_t result = Sensor_CalculatePressure(PRESSURE_MIN_ADC_VAL);
    TEST_ASSERT_EQUAL_UINT16(0, result);
}

/* Test: Just above minimum */
void test_pressure_just_above_minimum(void)
{
    uint16_t result = Sensor_CalculatePressure(PRESSURE_MIN_ADC_VAL + 1);
    TEST_ASSERT_TRUE(result > 0);
    TEST_ASSERT_TRUE(result < 100);
}

/* Test: Maximum valid ADC value */
void test_pressure_at_maximum_valid(void)
{
    uint16_t result = Sensor_CalculatePressure(PRESSURE_MAX_ADC_VAL);
    TEST_ASSERT_EQUAL_UINT16(PRESSURE_MAX_PRESSURE, result);
}

/* Test: Below range returns 0 */
void test_pressure_below_range(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_CalculatePressure(0));
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_CalculatePressure(100));
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_CalculatePressure(PRESSURE_MIN_ADC_VAL - 1));
}

/* Test: Above range returns 0 */
void test_pressure_above_range(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_CalculatePressure(PRESSURE_MAX_ADC_VAL + 1));
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_CalculatePressure(4095));
}

/* Test: Zero ADC value */
void test_pressure_zero(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_CalculatePressure(0));
}

/* Test: Pressure linearity - quarter points */
void test_pressure_linearity(void)
{
    uint16_t range = PRESSURE_MAX_ADC_VAL - PRESSURE_MIN_ADC_VAL;
    
    uint16_t quarter   = Sensor_CalculatePressure(PRESSURE_MIN_ADC_VAL + range / 4);
    uint16_t half      = Sensor_CalculatePressure(PRESSURE_MIN_ADC_VAL + range / 2);
    uint16_t three_qtr = Sensor_CalculatePressure(PRESSURE_MIN_ADC_VAL + 3 * range / 4);
    
    /* Should be roughly 5000, 10000, 15000 */
    TEST_ASSERT_TRUE(quarter > 4500 && quarter < 5500);
    TEST_ASSERT_TRUE(half > 9500 && half < 10500);
    TEST_ASSERT_TRUE(three_qtr > 14500 && three_qtr < 15500);
    
    /* Verify monotonically increasing */
    TEST_ASSERT_TRUE(quarter < half);
    TEST_ASSERT_TRUE(half < three_qtr);
}

/* ============================================================================
 * Sensor_CalculateSteeringAngle Tests
 * ============================================================================ */

/* Test: Center position */
void test_steering_center(void)
{
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    int16_t result = Sensor_CalculateSteeringAngle(center);
    /* Should be approximately 0 degrees */
    TEST_ASSERT_TRUE(result >= -20 && result <= 20);
}

/* Test: Full left position */
void test_steering_full_left(void)
{
    int16_t result = Sensor_CalculateSteeringAngle(STEERING_RAW_LEFT);
    /* Should be -900 (representing -90.0 degrees) */
    TEST_ASSERT_EQUAL_INT16(-900, result);
}

/* Test: Full right position */
void test_steering_full_right(void)
{
    int16_t result = Sensor_CalculateSteeringAngle(STEERING_RAW_RIGHT);
    /* Should be +900 (representing +90.0 degrees) */
    TEST_ASSERT_EQUAL_INT16(900, result);
}

/* Test: Steering monotonicity */
void test_steering_monotonicity(void)
{
    int16_t prev = Sensor_CalculateSteeringAngle(STEERING_RAW_LEFT);
    for (uint16_t adc = STEERING_RAW_LEFT + 50; adc <= STEERING_RAW_RIGHT; adc += 50)
    {
        int16_t curr = Sensor_CalculateSteeringAngle(adc);
        TEST_ASSERT_TRUE(curr >= prev);
        prev = curr;
    }
}

/* Test: Steering symmetry */
void test_steering_symmetry(void)
{
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    uint16_t offset = 200;
    
    int16_t left_of_center  = Sensor_CalculateSteeringAngle(center - offset);
    int16_t right_of_center = Sensor_CalculateSteeringAngle(center + offset);
    
    /* Should be roughly equal magnitude, opposite sign */
    int16_t sum = left_of_center + right_of_center;
    TEST_ASSERT_TRUE(sum >= -30 && sum <= 30); /* Allow small rounding error */
}

/* ============================================================================
 * Sensor_FilterRPM Tests
 * ============================================================================ */

/* Reset filter state between tests (reinitializes the static arrays) */
static void reset_rpm_filters(void)
{
    /* Reset internal static state by filling with zeros */
    for (int i = 0; i < WS_FILTER_SIZE; i++) {
        rpm_left_filter[i] = 0;
        rpm_right_filter[i] = 0;
    }
    rpm_left_counter = 0;
    rpm_right_counter = 0;
}

/* Test: Filter returns max value from window (left) */
void test_filter_rpm_left_max_value(void)
{
    reset_rpm_filters();
    
    /* Feed increasing values */
    Sensor_FilterRPM_Left(100);
    Sensor_FilterRPM_Left(200);
    Sensor_FilterRPM_Left(300);
    uint16_t result = Sensor_FilterRPM_Left(150);
    
    /* Max in buffer should be 300 */
    TEST_ASSERT_EQUAL_UINT16(300, result);
}

/* Test: Filter returns max value from window (right) */
void test_filter_rpm_right_max_value(void)
{
    reset_rpm_filters();
    
    Sensor_FilterRPM_Right(500);
    Sensor_FilterRPM_Right(100);
    Sensor_FilterRPM_Right(800);
    uint16_t result = Sensor_FilterRPM_Right(200);
    
    TEST_ASSERT_EQUAL_UINT16(800, result);
}

/* Test: Filter wrap-around after WS_FILTER_SIZE samples */
void test_filter_rpm_wraparound(void)
{
    reset_rpm_filters();
    
    /* Fill the entire window with a high value */
    for (int i = 0; i < WS_FILTER_SIZE; i++) {
        Sensor_FilterRPM_Left(1000);
    }
    
    /* Now overwrite all with lower values */
    for (int i = 0; i < WS_FILTER_SIZE; i++) {
        Sensor_FilterRPM_Left(50);
    }
    
    /* After full overwrite, max should be 50 */
    uint16_t result = Sensor_FilterRPM_Left(50);
    TEST_ASSERT_EQUAL_UINT16(50, result);
}

/* Test: Filter with all zeros */
void test_filter_rpm_all_zeros(void)
{
    reset_rpm_filters();
    
    for (int i = 0; i < WS_FILTER_SIZE; i++) {
        Sensor_FilterRPM_Left(0);
    }
    
    TEST_ASSERT_EQUAL_UINT16(0, Sensor_FilterRPM_Left(0));
}

/* Test: Filter with single spike */
void test_filter_rpm_spike_retention(void)
{
    reset_rpm_filters();
    
    /* Send a spike */
    Sensor_FilterRPM_Left(5000);
    
    /* Follow with normal values */
    uint16_t result = 0;
    for (int i = 0; i < WS_FILTER_SIZE - 1; i++) {
        result = Sensor_FilterRPM_Left(100);
    }
    
    /* Spike should still be in the window */
    TEST_ASSERT_EQUAL_UINT16(5000, result);
    
    /* One more sample pushes spike out */
    result = Sensor_FilterRPM_Left(100);
    TEST_ASSERT_EQUAL_UINT16(100, result);
}

/* Test: Left and right filters are independent */
void test_filter_rpm_independence(void)
{
    reset_rpm_filters();
    
    Sensor_FilterRPM_Left(1000);
    Sensor_FilterRPM_Right(500);
    
    uint16_t left = Sensor_FilterRPM_Left(200);
    uint16_t right = Sensor_FilterRPM_Right(100);
    
    TEST_ASSERT_EQUAL_UINT16(1000, left);
    TEST_ASSERT_EQUAL_UINT16(500, right);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
void run_sensor_tests(void)
{
    UNITY_BEGIN_SUITE("Sensors - Pressure Calculation");
    RUN_TEST(test_pressure_nominal_middle);
    RUN_TEST(test_pressure_at_minimum_valid);
    RUN_TEST(test_pressure_just_above_minimum);
    RUN_TEST(test_pressure_at_maximum_valid);
    RUN_TEST(test_pressure_below_range);
    RUN_TEST(test_pressure_above_range);
    RUN_TEST(test_pressure_zero);
    RUN_TEST(test_pressure_linearity);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Sensors - Steering Angle Calculation");
    RUN_TEST(test_steering_center);
    RUN_TEST(test_steering_full_left);
    RUN_TEST(test_steering_full_right);
    RUN_TEST(test_steering_monotonicity);
    RUN_TEST(test_steering_symmetry);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Sensors - RPM Filter");
    RUN_TEST(test_filter_rpm_left_max_value);
    RUN_TEST(test_filter_rpm_right_max_value);
    RUN_TEST(test_filter_rpm_wraparound);
    RUN_TEST(test_filter_rpm_all_zeros);
    RUN_TEST(test_filter_rpm_spike_retention);
    RUN_TEST(test_filter_rpm_independence);
    UNITY_END_SUITE();
}
