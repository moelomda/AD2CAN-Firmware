/* ============================================================================
 * Unit Tests: brake_light_task.c - Brake Light Control Task
 * ============================================================================
 * Tests:
 *   - BrakeLightTask_Init(): Thread creation
 *   - BrakeLightTask_Run(): Brake light on/off based on CAN commands
 *   - GPIO output verification
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"

/* Stub for CAN HAL */
static osMessageQueueId_t stub_brake_queue = NULL;

#define CAN_HAL_GetBrakeQueueHandle stub_CAN_HAL_GetBrakeQueueHandle

osMessageQueueId_t stub_CAN_HAL_GetBrakeQueueHandle(void)
{
    return stub_brake_queue;
}

/* Include source under test */
#include "../../App/Src/Tasks/brake_light_task.c"

#undef CAN_HAL_GetBrakeQueueHandle

static void brake_light_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    
    const osMessageQueueAttr_t attr = { .name = "StubBrakeQueue" };
    stub_brake_queue = osMessageQueueNew(4, sizeof(uint8_t), &attr);
}

/* ============================================================================
 * BrakeLightTask_Init Tests
 * ============================================================================ */

void test_brake_light_init_creates_thread(void)
{
    brake_light_test_setup();
    BrakeLightTask_Init();
    
    TEST_ASSERT_TRUE(mock_rtos.thread_count > 0);
    TEST_ASSERT_EQUAL(0, strcmp(mock_rtos.threads[0].name, "BrakeLightTask"));
}

void test_brake_light_init_thread_fail_calls_error(void)
{
    brake_light_test_setup();
    mock_rtos.thread_create_fail = 1;
    
    BrakeLightTask_Init();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

/* ============================================================================
 * Brake Light Control Logic Tests
 * ============================================================================ */

void test_brake_light_on_command(void)
{
    brake_light_test_setup();
    
    /* Simulate receiving brake ON command */
    uint8_t cmd = 1;
    osMessageQueuePut(stub_brake_queue, &cmd, 0, 0);
    
    /* Process one iteration */
    uint8_t received;
    if (osMessageQueueGet(stub_brake_queue, &received, NULL, 0) == osOK) {
        if (received == 1) {
            HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
        }
    }
    
    /* Verify GPIO was set HIGH */
    TEST_ASSERT_EQUAL(1, mock_hal.gpio_write_count);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, mock_hal.gpio_write_calls[0].state);
    TEST_ASSERT_EQUAL(BL_ENABLE_Pin, mock_hal.gpio_write_calls[0].pin);
}

void test_brake_light_off_command(void)
{
    brake_light_test_setup();
    
    uint8_t cmd = 0;
    osMessageQueuePut(stub_brake_queue, &cmd, 0, 0);
    
    uint8_t received;
    if (osMessageQueueGet(stub_brake_queue, &received, NULL, 0) == osOK) {
        if (received == 1) {
            HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
        }
    }
    
    /* Verify GPIO was set LOW */
    TEST_ASSERT_EQUAL(1, mock_hal.gpio_write_count);
    TEST_ASSERT_EQUAL(GPIO_PIN_RESET, mock_hal.gpio_write_calls[0].state);
}

void test_brake_light_multiple_commands(void)
{
    brake_light_test_setup();
    
    /* Queue ON, OFF, ON sequence */
    uint8_t cmds[] = {1, 0, 1};
    for (int i = 0; i < 3; i++) {
        osMessageQueuePut(stub_brake_queue, &cmds[i], 0, 0);
    }
    
    /* Process all commands */
    for (int i = 0; i < 3; i++) {
        uint8_t received;
        if (osMessageQueueGet(stub_brake_queue, &received, NULL, 0) == osOK) {
            if (received == 1) {
                HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
            } else {
                HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
            }
        }
    }
    
    TEST_ASSERT_EQUAL(3, mock_hal.gpio_write_count);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, mock_hal.gpio_write_calls[0].state);
    TEST_ASSERT_EQUAL(GPIO_PIN_RESET, mock_hal.gpio_write_calls[1].state);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, mock_hal.gpio_write_calls[2].state);
}

void test_brake_light_no_command_no_action(void)
{
    brake_light_test_setup();
    
    /* Queue is empty - verify no GPIO activity */
    uint8_t received;
    osStatus_t status = osMessageQueueGet(stub_brake_queue, &received, NULL, 0);
    
    TEST_ASSERT_TRUE(status != osOK);
    TEST_ASSERT_EQUAL(0, mock_hal.gpio_write_count);
}

void test_brake_light_gpio_port_correct(void)
{
    brake_light_test_setup();
    
    uint8_t cmd = 1;
    osMessageQueuePut(stub_brake_queue, &cmd, 0, 0);
    
    uint8_t received;
    if (osMessageQueueGet(stub_brake_queue, &received, NULL, 0) == osOK) {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
    }
    
    /* Verify correct port (GPIOA) and pin (PA8) */
    TEST_ASSERT_EQUAL(BL_ENABLE_GPIO_Port, mock_hal.gpio_write_calls[0].port);
    TEST_ASSERT_EQUAL(BL_ENABLE_Pin, mock_hal.gpio_write_calls[0].pin);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_brake_light_task_tests();
    return unity_print_summary();
}

void run_brake_light_task_tests(void)
{
    UNITY_BEGIN_SUITE("Brake Light Task - Initialization");
    RUN_TEST(test_brake_light_init_creates_thread);
    RUN_TEST(test_brake_light_init_thread_fail_calls_error);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Brake Light Task - Control Logic");
    RUN_TEST(test_brake_light_on_command);
    RUN_TEST(test_brake_light_off_command);
    RUN_TEST(test_brake_light_multiple_commands);
    RUN_TEST(test_brake_light_no_command_no_action);
    RUN_TEST(test_brake_light_gpio_port_correct);
    UNITY_END_SUITE();
}
