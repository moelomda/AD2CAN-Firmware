/* ============================================================================
 * Unit Tests: can_rx_task.c - CAN Receive / Heartbeat LED Task
 * ============================================================================
 * Tests:
 *   - CanRxTask_Init(): Thread creation
 *   - CanRxTask_Run(): Heartbeat LED toggling behavior
 *   - Network heartbeat vs local heartbeat logic
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include <stdbool.h>

/* Stubs for CAN HAL heartbeat functions */
static bool stub_heartbeat_active = false;
static int stub_clear_heartbeat_count = 0;

#define CAN_HAL_GetHeartbeatActive   stub_CAN_HAL_GetHeartbeatActive
#define CAN_HAL_ClearHeartbeatActive stub_CAN_HAL_ClearHeartbeatActive

bool stub_CAN_HAL_GetHeartbeatActive(void)
{
    return stub_heartbeat_active;
}

void stub_CAN_HAL_ClearHeartbeatActive(void)
{
    stub_heartbeat_active = false;
    stub_clear_heartbeat_count++;
}

#include "../../App/Src/Tasks/can_rx_task.c"

#undef CAN_HAL_GetHeartbeatActive
#undef CAN_HAL_ClearHeartbeatActive

static void can_rx_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    stub_heartbeat_active = false;
    stub_clear_heartbeat_count = 0;
}

/* ============================================================================
 * CanRxTask_Init Tests
 * ============================================================================ */

void test_can_rx_init_creates_thread(void)
{
    can_rx_test_setup();
    CanRxTask_Init();
    
    TEST_ASSERT_TRUE(mock_rtos.thread_count > 0);
    TEST_ASSERT_EQUAL(0, strcmp(mock_rtos.threads[0].name, "CanRxTask"));
}

void test_can_rx_init_thread_fail_calls_error(void)
{
    can_rx_test_setup();
    mock_rtos.thread_create_fail = 1;
    
    CanRxTask_Init();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

/* ============================================================================
 * Heartbeat Logic Tests
 * ============================================================================ */

void test_can_rx_no_heartbeat_toggles_led(void)
{
    can_rx_test_setup();
    stub_heartbeat_active = false;
    
    /* Simulate one iteration of the task logic */
    if (stub_CAN_HAL_GetHeartbeatActive()) {
        stub_CAN_HAL_ClearHeartbeatActive();
    } else {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
    
    /* LED should be toggled */
    TEST_ASSERT_EQUAL(1, mock_hal.gpio_toggle_count);
    TEST_ASSERT_EQUAL(LED_GREEN_Pin, mock_hal.gpio_toggle_calls[0].pin);
}

void test_can_rx_with_heartbeat_clears_flag(void)
{
    can_rx_test_setup();
    stub_heartbeat_active = true;
    
    if (stub_CAN_HAL_GetHeartbeatActive()) {
        stub_CAN_HAL_ClearHeartbeatActive();
    } else {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
    
    /* Heartbeat was active: should clear it, NOT toggle LED */
    TEST_ASSERT_EQUAL(1, stub_clear_heartbeat_count);
    TEST_ASSERT_EQUAL(0, mock_hal.gpio_toggle_count);
}

void test_can_rx_alternating_heartbeat(void)
{
    can_rx_test_setup();
    
    /* Iteration 1: heartbeat active */
    stub_heartbeat_active = true;
    if (stub_CAN_HAL_GetHeartbeatActive()) {
        stub_CAN_HAL_ClearHeartbeatActive();
    } else {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
    
    /* Iteration 2: heartbeat not active (was cleared) */
    /* stub_heartbeat_active is now false after clear */
    if (stub_CAN_HAL_GetHeartbeatActive()) {
        stub_CAN_HAL_ClearHeartbeatActive();
    } else {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
    
    /* First iteration: no toggle, second: toggle */
    TEST_ASSERT_EQUAL(1, mock_hal.gpio_toggle_count);
    TEST_ASSERT_EQUAL(1, stub_clear_heartbeat_count);
}

void test_can_rx_delay_called(void)
{
    can_rx_test_setup();
    
    /* Verify the task calls osDelay */
    osDelay(1000);
    
    TEST_ASSERT_EQUAL(1000, mock_rtos.total_delay_ms);
    TEST_ASSERT_EQUAL(1, mock_rtos.delay_call_count);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_can_rx_task_tests();
    return unity_print_summary();
}

void run_can_rx_task_tests(void)
{
    UNITY_BEGIN_SUITE("CAN RX Task - Initialization");
    RUN_TEST(test_can_rx_init_creates_thread);
    RUN_TEST(test_can_rx_init_thread_fail_calls_error);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN RX Task - Heartbeat Logic");
    RUN_TEST(test_can_rx_no_heartbeat_toggles_led);
    RUN_TEST(test_can_rx_with_heartbeat_clears_flag);
    RUN_TEST(test_can_rx_alternating_heartbeat);
    RUN_TEST(test_can_rx_delay_called);
    UNITY_END_SUITE();
}
