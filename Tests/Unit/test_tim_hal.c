/* ============================================================================
 * Unit Tests: tim_hal.c - Timer Hardware Abstraction Layer
 * ============================================================================
 * Tests:
 *   - TIM_HAL_Init(): Timer start verification
 *   - TIM_HAL_GetRPM_Left(): Left wheel RPM calculation
 *   - TIM_HAL_GetRPM_Right(): Right wheel RPM calculation
 *   - HAL_TIM_IC_CaptureCallback(): Input capture interrupt handler
 *   - Timeout behavior when no pulses received
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"

/* Include the source under test */
#include "../../App/Src/HAL/tim_hal.c"

static void tim_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    
    /* Reset internal state */
    left_last_capture  = 0;
    left_period        = 0;
    left_overflow_count = 0;
    left_last_tick     = 0;
    right_last_capture = 0;
    right_period       = 0;
    right_overflow_count = 0;
    right_last_tick    = 0;
}

/* ============================================================================
 * TIM_HAL_Init Tests
 * ============================================================================ */

void test_tim_init_starts_input_capture(void)
{
    tim_test_setup();
    TIM_HAL_Init();
    
    /* Should start 2 input capture channels */
    TEST_ASSERT_EQUAL(2, mock_hal.tim_ic_start_count);
}

void test_tim_init_starts_base_timers(void)
{
    tim_test_setup();
    TIM_HAL_Init();
    
    /* Should start 2 base timers for overflow counting */
    TEST_ASSERT_EQUAL(2, mock_hal.tim_base_start_count);
}

/* ============================================================================
 * RPM Calculation Tests
 * ============================================================================ */

void test_rpm_left_zero_when_no_period(void)
{
    tim_test_setup();
    mock_hal.hal_tick = 100;
    
    TEST_ASSERT_EQUAL_UINT16(0, TIM_HAL_GetRPM_Left());
}

void test_rpm_right_zero_when_no_period(void)
{
    tim_test_setup();
    mock_hal.hal_tick = 100;
    
    TEST_ASSERT_EQUAL_UINT16(0, TIM_HAL_GetRPM_Right());
}

void test_rpm_left_timeout(void)
{
    tim_test_setup();
    
    /* Set a valid period but make tick far in the future */
    left_period = 100000;
    left_last_tick = 0;
    mock_hal.hal_tick = 600; /* > RPM_TIMEOUT_MS (500) */
    
    TEST_ASSERT_EQUAL_UINT16(0, TIM_HAL_GetRPM_Left());
}

void test_rpm_right_timeout(void)
{
    tim_test_setup();
    
    right_period = 100000;
    right_last_tick = 0;
    mock_hal.hal_tick = 600;
    
    TEST_ASSERT_EQUAL_UINT16(0, TIM_HAL_GetRPM_Right());
}

void test_rpm_left_valid_calculation(void)
{
    tim_test_setup();
    
    /* Simulate a period that gives known RPM
     * RPM = (64000000 * 60) / (period * 25)
     * For period = 153600: RPM = 3840000000000 / 3840000 = 1000 RPM */
    left_period = 153600;
    left_last_tick = 100;
    mock_hal.hal_tick = 100; /* No timeout */
    
    uint16_t rpm = TIM_HAL_GetRPM_Left();
    /* RPM = 64000000 * 60 / (153600 * 25) = 3840000000 / 3840000 = 1000 */
    TEST_ASSERT_EQUAL_UINT16(1000, rpm);
}

void test_rpm_right_valid_calculation(void)
{
    tim_test_setup();
    
    right_period = 153600;
    right_last_tick = 100;
    mock_hal.hal_tick = 100;
    
    uint16_t rpm = TIM_HAL_GetRPM_Right();
    TEST_ASSERT_EQUAL_UINT16(1000, rpm);
}

void test_rpm_left_just_before_timeout(void)
{
    tim_test_setup();
    
    left_period = 153600;
    left_last_tick = 0;
    mock_hal.hal_tick = 499; /* Just within timeout window */
    
    uint16_t rpm = TIM_HAL_GetRPM_Left();
    TEST_ASSERT_TRUE(rpm > 0);
}

void test_rpm_left_at_timeout_boundary(void)
{
    tim_test_setup();
    
    left_period = 153600;
    left_last_tick = 0;
    mock_hal.hal_tick = 500; /* Exactly at boundary */
    
    uint16_t rpm = TIM_HAL_GetRPM_Left();
    TEST_ASSERT_TRUE(rpm > 0); /* 500 - 0 = 500, NOT > 500, so still valid */
}

void test_rpm_high_speed_clamp(void)
{
    tim_test_setup();
    
    /* Very small period = very high RPM, should clamp to 0xFFFF */
    left_period = 1;
    left_last_tick = 100;
    mock_hal.hal_tick = 100;
    
    uint16_t rpm = TIM_HAL_GetRPM_Left();
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, rpm);
}

void test_rpm_low_speed(void)
{
    tim_test_setup();
    
    /* Large period = low RPM */
    /* For RPM=100: period = 64000000 * 60 / (100 * 25) = 1536000 */
    left_period = 1536000;
    left_last_tick = 100;
    mock_hal.hal_tick = 100;
    
    uint16_t rpm = TIM_HAL_GetRPM_Left();
    TEST_ASSERT_EQUAL_UINT16(100, rpm);
}

/* ============================================================================
 * Capture Callback Tests
 * ============================================================================ */

void test_capture_callback_left_channel(void)
{
    tim_test_setup();
    
    /* Simulate TIM15 CH1 capture */
    TIM_HandleTypeDef test_tim = htim15;
    test_tim.Channel = HAL_TIM_ACTIVE_CHANNEL_1;
    mock_hal.tim_capture_value = 10000;
    
    HAL_TIM_IC_CaptureCallback(&test_tim);
    
    /* Period should be updated (10000 - 0 = 10000) */
    TEST_ASSERT_TRUE(left_period > 0);
    TEST_ASSERT_EQUAL(10000, left_last_capture);
}

void test_capture_callback_right_channel(void)
{
    tim_test_setup();
    
    TIM_HandleTypeDef test_tim = htim3;
    test_tim.Channel = HAL_TIM_ACTIVE_CHANNEL_3;
    mock_hal.tim_capture_value = 20000;
    
    HAL_TIM_IC_CaptureCallback(&test_tim);
    
    TEST_ASSERT_TRUE(right_period > 0);
    TEST_ASSERT_EQUAL(20000, right_last_capture);
}

void test_capture_callback_overflow_handling(void)
{
    tim_test_setup();
    
    /* Simulate capture with overflows */
    left_last_capture = 60000;
    left_overflow_count = 2;
    
    TIM_HandleTypeDef test_tim = htim15;
    test_tim.Channel = HAL_TIM_ACTIVE_CHANNEL_1;
    mock_hal.tim_capture_value = 5000;
    
    HAL_TIM_IC_CaptureCallback(&test_tim);
    
    /* period = (2 * 65536) + 5000 - 60000 = 131072 + 5000 - 60000 = 76072 */
    TEST_ASSERT_EQUAL_UINT32(76072, left_period);
    TEST_ASSERT_EQUAL(0, left_overflow_count); /* Should be cleared */
}

void test_capture_callback_wrap_without_overflow(void)
{
    tim_test_setup();
    
    /* Counter wrapped but no overflow counted */
    left_last_capture = 60000;
    left_overflow_count = 0;
    
    TIM_HandleTypeDef test_tim = htim15;
    test_tim.Channel = HAL_TIM_ACTIVE_CHANNEL_1;
    mock_hal.tim_capture_value = 5000;
    
    HAL_TIM_IC_CaptureCallback(&test_tim);
    
    /* Should detect wrap: period = 0 + 5000 - 60000 + 65536 = 10536 */
    TEST_ASSERT_EQUAL_UINT32(10536, left_period);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_tim_hal_tests();
    return unity_print_summary();
}

void run_tim_hal_tests(void)
{
    UNITY_BEGIN_SUITE("TIM HAL - Initialization");
    RUN_TEST(test_tim_init_starts_input_capture);
    RUN_TEST(test_tim_init_starts_base_timers);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("TIM HAL - RPM Calculation");
    RUN_TEST(test_rpm_left_zero_when_no_period);
    RUN_TEST(test_rpm_right_zero_when_no_period);
    RUN_TEST(test_rpm_left_timeout);
    RUN_TEST(test_rpm_right_timeout);
    RUN_TEST(test_rpm_left_valid_calculation);
    RUN_TEST(test_rpm_right_valid_calculation);
    RUN_TEST(test_rpm_left_just_before_timeout);
    RUN_TEST(test_rpm_left_at_timeout_boundary);
    RUN_TEST(test_rpm_high_speed_clamp);
    RUN_TEST(test_rpm_low_speed);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("TIM HAL - Capture Callback");
    RUN_TEST(test_capture_callback_left_channel);
    RUN_TEST(test_capture_callback_right_channel);
    RUN_TEST(test_capture_callback_overflow_handling);
    RUN_TEST(test_capture_callback_wrap_without_overflow);
    UNITY_END_SUITE();
}
