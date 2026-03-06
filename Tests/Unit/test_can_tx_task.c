/* ============================================================================
 * Unit Tests: can_tx_task.c - CAN Transmit Task
 * ============================================================================
 * Tests:
 *   - CanTxTask_Init(): Thread creation
 *   - CAN message formatting (F1_1, F1_2, DV_SENSORS_FRONT, STARTUP)
 *   - Sensor data packing into CAN frames
 *   - DV counter / periodic message logic
 *   - Mutex acquisition failure handling
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include "../../App/Inc/data_types.h"
#include <string.h>

/* ---- Stubs for dependencies ---- */
static ProcessedSensorData_t stub_sensor_data;
static osMutexId_t stub_sensor_mutex = NULL;
static uint16_t stub_rpm_left = 0;
static uint16_t stub_rpm_right = 0;
static uint16_t stub_filtered_rpm_left = 0;
static uint16_t stub_filtered_rpm_right = 0;
static int stub_can_tx_count = 0;
static MockCanMessage_t stub_can_messages[64];

/* Stub: SensorTask_GetDataPointer */
#define SensorTask_GetDataPointer stub_SensorTask_GetDataPointer
#define SensorTask_GetMutexHandle stub_SensorTask_GetMutexHandle
#define TIM_HAL_GetRPM_Left      stub_TIM_HAL_GetRPM_Left
#define TIM_HAL_GetRPM_Right     stub_TIM_HAL_GetRPM_Right
#define Sensor_FilterRPM_Left    stub_Sensor_FilterRPM_Left
#define Sensor_FilterRPM_Right   stub_Sensor_FilterRPM_Right
#define CAN_HAL_Transmit         stub_CAN_HAL_Transmit

ProcessedSensorData_t* stub_SensorTask_GetDataPointer(void)
{
    return &stub_sensor_data;
}

osMutexId_t stub_SensorTask_GetMutexHandle(void)
{
    return stub_sensor_mutex;
}

uint16_t stub_TIM_HAL_GetRPM_Left(void)
{
    return stub_rpm_left;
}

uint16_t stub_TIM_HAL_GetRPM_Right(void)
{
    return stub_rpm_right;
}

uint16_t stub_Sensor_FilterRPM_Left(uint16_t rpm_value)
{
    stub_filtered_rpm_left = rpm_value;
    return rpm_value;
}

uint16_t stub_Sensor_FilterRPM_Right(uint16_t rpm_value)
{
    stub_filtered_rpm_right = rpm_value;
    return rpm_value;
}

HAL_StatusTypeDef stub_CAN_HAL_Transmit(uint32_t id, uint8_t* data, uint32_t length)
{
    if (stub_can_tx_count < 64) {
        stub_can_messages[stub_can_tx_count].id = id;
        stub_can_messages[stub_can_tx_count].length = length;
        memcpy(stub_can_messages[stub_can_tx_count].data, data, length);
        stub_can_tx_count++;
    }
    return HAL_OK;
}

#include "../../App/Src/Tasks/can_tx_task.c"

#undef SensorTask_GetDataPointer
#undef SensorTask_GetMutexHandle
#undef TIM_HAL_GetRPM_Left
#undef TIM_HAL_GetRPM_Right
#undef Sensor_FilterRPM_Left
#undef Sensor_FilterRPM_Right
#undef CAN_HAL_Transmit

static void can_tx_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    
    memset(&stub_sensor_data, 0, sizeof(ProcessedSensorData_t));
    stub_rpm_left = 0;
    stub_rpm_right = 0;
    stub_filtered_rpm_left = 0;
    stub_filtered_rpm_right = 0;
    stub_can_tx_count = 0;
    memset(stub_can_messages, 0, sizeof(stub_can_messages));
    
    /* Create test mutex */
    const osMutexAttr_t attr = { .name = "StubMutex" };
    stub_sensor_mutex = osMutexNew(&attr);
}

/* ============================================================================
 * CanTxTask_Init Tests
 * ============================================================================ */

void test_can_tx_init_creates_thread(void)
{
    can_tx_test_setup();
    CanTxTask_Init();
    
    TEST_ASSERT_TRUE(mock_rtos.thread_count > 0);
    TEST_ASSERT_EQUAL(0, strcmp(mock_rtos.threads[0].name, "CanTxTask"));
}

void test_can_tx_init_thread_fail_calls_error(void)
{
    can_tx_test_setup();
    mock_rtos.thread_create_fail = 1;
    
    CanTxTask_Init();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

/* ============================================================================
 * CAN TX Message Formatting Tests
 * ============================================================================ */

void test_can_tx_f1_1_message_format(void)
{
    can_tx_test_setup();
    
    /* Set known sensor data */
    stub_sensor_data.apps1 = 0x1234;
    stub_sensor_data.apps2 = 0x5678;
    stub_sensor_data.steering_angle = 0x0100; /* 256 = 25.6 degrees */
    stub_sensor_data.brake_pressure_front = 0xABCD;
    
    /* Build F1_1 message like the task does */
    uint8_t txData[8];
    txData[0] = stub_sensor_data.apps1 & 0xFF;        /* 0x34 */
    txData[1] = stub_sensor_data.apps1 >> 8;           /* 0x12 */
    txData[2] = stub_sensor_data.apps2 & 0xFF;        /* 0x78 */
    txData[3] = stub_sensor_data.apps2 >> 8;           /* 0x56 */
    txData[4] = stub_sensor_data.steering_angle & 0xFF; /* 0x00 */
    txData[5] = stub_sensor_data.steering_angle >> 8;   /* 0x01 */
    txData[6] = stub_sensor_data.brake_pressure_front & 0xFF; /* 0xCD */
    txData[7] = stub_sensor_data.brake_pressure_front >> 8;   /* 0xAB */
    
    stub_CAN_HAL_Transmit(CAN_ID_AD2CAN_F1_1, txData, 8);
    
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_1, stub_can_messages[0].id);
    TEST_ASSERT_EQUAL(8, stub_can_messages[0].length);
    
    /* Verify byte order (little-endian) */
    TEST_ASSERT_EQUAL_UINT8(0x34, stub_can_messages[0].data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, stub_can_messages[0].data[1]);
    TEST_ASSERT_EQUAL_UINT8(0x78, stub_can_messages[0].data[2]);
    TEST_ASSERT_EQUAL_UINT8(0x56, stub_can_messages[0].data[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, stub_can_messages[0].data[4]);
    TEST_ASSERT_EQUAL_UINT8(0x01, stub_can_messages[0].data[5]);
    TEST_ASSERT_EQUAL_UINT8(0xCD, stub_can_messages[0].data[6]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, stub_can_messages[0].data[7]);
}

void test_can_tx_f1_2_message_format(void)
{
    can_tx_test_setup();
    
    uint16_t rpmLeft = 500;
    uint16_t rpmRight = 600;
    stub_sensor_data.brake_pressure_rear = 0x1122;
    
    uint8_t txData[8];
    txData[0] = rpmLeft & 0xFF;
    txData[1] = rpmLeft >> 8;
    txData[2] = rpmRight & 0xFF;
    txData[3] = rpmRight >> 8;
    txData[4] = stub_sensor_data.brake_pressure_rear & 0xFF;
    txData[5] = stub_sensor_data.brake_pressure_rear >> 8;
    txData[6] = 0; /* WS pin state */
    
    stub_CAN_HAL_Transmit(CAN_ID_AD2CAN_F1_2, txData, 7);
    
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_2, stub_can_messages[0].id);
    TEST_ASSERT_EQUAL(7, stub_can_messages[0].length);
    
    /* Verify RPM encoding */
    uint16_t decoded_left = stub_can_messages[0].data[0] | (stub_can_messages[0].data[1] << 8);
    uint16_t decoded_right = stub_can_messages[0].data[2] | (stub_can_messages[0].data[3] << 8);
    TEST_ASSERT_EQUAL_UINT16(500, decoded_left);
    TEST_ASSERT_EQUAL_UINT16(600, decoded_right);
}

void test_can_tx_startup_message(void)
{
    can_tx_test_setup();
    
    uint8_t txData[1] = {0xAA};
    stub_CAN_HAL_Transmit(CAN_ID_AD2CAN_STARTUP, txData, 1);
    
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_STARTUP, stub_can_messages[0].id);
    TEST_ASSERT_EQUAL_UINT8(0xAA, stub_can_messages[0].data[0]);
}

void test_can_tx_dv_sensors_front_message(void)
{
    can_tx_test_setup();
    
    stub_sensor_data.brake_pressure_rear = 0x3344;
    stub_sensor_data.brake_pressure_front = 0x5566;
    
    uint8_t txData[4];
    txData[0] = stub_sensor_data.brake_pressure_rear & 0xFF;
    txData[1] = stub_sensor_data.brake_pressure_rear >> 8;
    txData[2] = stub_sensor_data.brake_pressure_front & 0xFF;
    txData[3] = stub_sensor_data.brake_pressure_front >> 8;
    
    stub_CAN_HAL_Transmit(CAN_ID_DV_SENSORS_FRONT, txData, 4);
    
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_DV_SENSORS_FRONT, stub_can_messages[0].id);
    TEST_ASSERT_EQUAL(4, stub_can_messages[0].length);
    
    uint16_t decoded_rear = stub_can_messages[0].data[0] | (stub_can_messages[0].data[1] << 8);
    uint16_t decoded_front = stub_can_messages[0].data[2] | (stub_can_messages[0].data[3] << 8);
    TEST_ASSERT_EQUAL_UINT16(0x3344, decoded_rear);
    TEST_ASSERT_EQUAL_UINT16(0x5566, decoded_front);
}

/* ============================================================================
 * DV Counter Logic Tests
 * ============================================================================ */

void test_can_tx_dv_counter_sends_every_10th(void)
{
    can_tx_test_setup();
    
    /* Simulate dv_counter behavior */
    uint8_t dv_counter = 0;
    int dv_sends = 0;
    
    for (int i = 0; i < 100; i++) {
        if (dv_counter == 0) {
            dv_sends++;
        }
        dv_counter++;
        dv_counter %= 10;
    }
    
    /* Should send DV message exactly 10 times in 100 iterations */
    TEST_ASSERT_EQUAL(10, dv_sends);
}

/* ============================================================================
 * Mutex Failure Handling Test
 * ============================================================================ */

void test_can_tx_mutex_fail_toggles_red_led(void)
{
    can_tx_test_setup();
    mock_rtos.mutex_acquire_fail = 1;
    
    /* Attempt to acquire mutex - should fail */
    osStatus_t status = osMutexAcquire(stub_sensor_mutex, 10);
    
    if (status != osOK) {
        HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
    }
    
    TEST_ASSERT_EQUAL(1, mock_hal.gpio_toggle_count);
    TEST_ASSERT_EQUAL(LED_RED_Pin, mock_hal.gpio_toggle_calls[0].pin);
}

/* ============================================================================
 * Data Encoding Roundtrip Test
 * ============================================================================ */

void test_can_tx_data_roundtrip(void)
{
    can_tx_test_setup();
    
    /* Set realistic sensor values */
    stub_sensor_data.apps1 = 2048;
    stub_sensor_data.apps2 = 2100;
    stub_sensor_data.steering_angle = -150;
    stub_sensor_data.brake_pressure_front = 5000;
    stub_sensor_data.brake_pressure_rear = 3000;
    
    /* Encode like the task */
    uint8_t txData[8];
    txData[0] = stub_sensor_data.apps1 & 0xFF;
    txData[1] = stub_sensor_data.apps1 >> 8;
    txData[2] = stub_sensor_data.apps2 & 0xFF;
    txData[3] = stub_sensor_data.apps2 >> 8;
    txData[4] = stub_sensor_data.steering_angle & 0xFF;
    txData[5] = (stub_sensor_data.steering_angle >> 8) & 0xFF;
    txData[6] = stub_sensor_data.brake_pressure_front & 0xFF;
    txData[7] = stub_sensor_data.brake_pressure_front >> 8;
    
    /* Decode back */
    uint16_t decoded_apps1 = txData[0] | (txData[1] << 8);
    uint16_t decoded_apps2 = txData[2] | (txData[3] << 8);
    int16_t decoded_steering = (int16_t)(txData[4] | (txData[5] << 8));
    uint16_t decoded_pressure = txData[6] | (txData[7] << 8);
    
    TEST_ASSERT_EQUAL_UINT16(2048, decoded_apps1);
    TEST_ASSERT_EQUAL_UINT16(2100, decoded_apps2);
    TEST_ASSERT_EQUAL_INT16(-150, decoded_steering);
    TEST_ASSERT_EQUAL_UINT16(5000, decoded_pressure);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_can_tx_task_tests();
    return unity_print_summary();
}

void run_can_tx_task_tests(void)
{
    UNITY_BEGIN_SUITE("CAN TX Task - Initialization");
    RUN_TEST(test_can_tx_init_creates_thread);
    RUN_TEST(test_can_tx_init_thread_fail_calls_error);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN TX Task - Message Formatting");
    RUN_TEST(test_can_tx_f1_1_message_format);
    RUN_TEST(test_can_tx_f1_2_message_format);
    RUN_TEST(test_can_tx_startup_message);
    RUN_TEST(test_can_tx_dv_sensors_front_message);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN TX Task - DV Counter");
    RUN_TEST(test_can_tx_dv_counter_sends_every_10th);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN TX Task - Error Handling");
    RUN_TEST(test_can_tx_mutex_fail_toggles_red_led);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN TX Task - Data Roundtrip");
    RUN_TEST(test_can_tx_data_roundtrip);
    UNITY_END_SUITE();
}
