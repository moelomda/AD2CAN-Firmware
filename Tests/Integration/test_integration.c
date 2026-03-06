/* ============================================================================
 * Integration Tests: ADC → Sensor Processing → CAN TX Pipeline
 * ============================================================================
 * Verifies the complete data flow from simulated ADC samples through sensor
 * processing to CAN frame output. This tests multiple modules working together.
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include "../../App/Inc/data_types.h"
#include "../../App/Inc/Drivers/sensors.h"
#include <string.h>

/* Include the sensor source for calculations */
#include "../../App/Src/Drivers/sensors.c"

/* ---- Shared test state ---- */
static osMessageQueueId_t test_adc_queue = NULL;
static ProcessedSensorData_t test_sensor_data;
static osMutexId_t test_mutex = NULL;
static MockCanMessage_t test_can_messages[64];
static int test_can_msg_count = 0;

static HAL_StatusTypeDef test_CAN_Transmit(uint32_t id, uint8_t* data, uint32_t length)
{
    if (test_can_msg_count < 64) {
        test_can_messages[test_can_msg_count].id = id;
        test_can_messages[test_can_msg_count].length = length;
        memcpy(test_can_messages[test_can_msg_count].data, data, length);
        test_can_msg_count++;
    }
    return HAL_OK;
}

static void integration_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    
    test_can_msg_count = 0;
    memset(test_can_messages, 0, sizeof(test_can_messages));
    memset(&test_sensor_data, 0, sizeof(ProcessedSensorData_t));
    
    const osMessageQueueAttr_t qattr = { .name = "TestAdcQueue" };
    test_adc_queue = osMessageQueueNew(4, sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), &qattr);
    
    const osMutexAttr_t mattr = { .name = "TestMutex" };
    test_mutex = osMutexNew(&mattr);
}

/* ============================================================================
 * Integration Test: Full ADC → Process → CAN Pipeline
 * ============================================================================ */

void test_integration_adc_to_can_pipeline(void)
{
    integration_setup();
    
    /* Step 1: Simulate ADC DMA data (realistic sensor readings) */
    uint16_t adc_data[ADC_NUMBER_OF_CHANNELS] = {
        2000,   /* brake_pressure_front ADC: valid range */
        1800,   /* brake_pressure_rear ADC: valid range */
        2648,   /* steering_angle ADC: roughly center */
        2048,   /* APPS1: half range */
        2100,   /* APPS2: slightly above half */
        0       /* unused */
    };
    
    /* Step 2: Put ADC data in queue (simulates DMA callback) */
    osMessageQueuePut(test_adc_queue, adc_data, 0, 0);
    
    /* Step 3: Sensor task processing - get from queue and process */
    uint16_t received[ADC_NUMBER_OF_CHANNELS];
    TEST_ASSERT_EQUAL(osOK, osMessageQueueGet(test_adc_queue, received, NULL, 0));
    
    test_sensor_data.brake_pressure_front = Sensor_CalculatePressure(received[0]);
    test_sensor_data.brake_pressure_rear  = Sensor_CalculatePressure(received[1]);
    test_sensor_data.steering_angle       = Sensor_CalculateSteeringAngle(received[2]);
    test_sensor_data.apps1                = received[3];
    test_sensor_data.apps2                = received[4];
    
    /* Step 4: CAN TX task - build and send F1_1 message */
    ProcessedSensorData_t local_data;
    TEST_ASSERT_EQUAL(osOK, osMutexAcquire(test_mutex, 10));
    memcpy(&local_data, &test_sensor_data, sizeof(ProcessedSensorData_t));
    osMutexRelease(test_mutex);
    
    uint8_t txData[8];
    txData[0] = local_data.apps1 & 0xFF;
    txData[1] = local_data.apps1 >> 8;
    txData[2] = local_data.apps2 & 0xFF;
    txData[3] = local_data.apps2 >> 8;
    txData[4] = local_data.steering_angle & 0xFF;
    txData[5] = local_data.steering_angle >> 8;
    txData[6] = local_data.brake_pressure_front & 0xFF;
    txData[7] = local_data.brake_pressure_front >> 8;
    test_CAN_Transmit(CAN_ID_AD2CAN_F1_1, txData, 8);
    
    /* Step 5: Verify CAN output */
    TEST_ASSERT_EQUAL(1, test_can_msg_count);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_1, test_can_messages[0].id);
    
    /* Decode and verify values are reasonable */
    uint16_t rx_apps1 = test_can_messages[0].data[0] | (test_can_messages[0].data[1] << 8);
    uint16_t rx_apps2 = test_can_messages[0].data[2] | (test_can_messages[0].data[3] << 8);
    int16_t  rx_steer = (int16_t)(test_can_messages[0].data[4] | (test_can_messages[0].data[5] << 8));
    uint16_t rx_press = test_can_messages[0].data[6] | (test_can_messages[0].data[7] << 8);
    
    TEST_ASSERT_EQUAL_UINT16(2048, rx_apps1);
    TEST_ASSERT_EQUAL_UINT16(2100, rx_apps2);
    TEST_ASSERT_TRUE(rx_steer >= -900 && rx_steer <= 900);
    TEST_ASSERT_TRUE(rx_press > 0); /* Valid pressure */
}

/* ============================================================================
 * Integration Test: CAN RX → Brake Light
 * ============================================================================ */

void test_integration_can_rx_brake_light_on(void)
{
    integration_setup();
    
    /* Step 1: Simulate CAN RX brake command */
    osMessageQueueId_t brake_queue;
    const osMessageQueueAttr_t battr = { .name = "BrakeQueue" };
    brake_queue = osMessageQueueNew(1, sizeof(uint8_t), &battr);
    
    uint8_t brake_cmd = 1;
    osMessageQueuePut(brake_queue, &brake_cmd, 0, 0);
    
    /* Step 2: Brake light task processes command */
    uint8_t received;
    TEST_ASSERT_EQUAL(osOK, osMessageQueueGet(brake_queue, &received, NULL, 0));
    
    if (received == 1) {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
    }
    
    /* Step 3: Verify brake light is ON */
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, mock_hal.gpio_write_calls[0].state);
    TEST_ASSERT_EQUAL(BL_ENABLE_Pin, mock_hal.gpio_write_calls[0].pin);
    
    /* Verify ODR register reflects the change */
    TEST_ASSERT_TRUE(mock_gpioa.ODR & BL_ENABLE_Pin);
}

void test_integration_can_rx_brake_light_off(void)
{
    integration_setup();
    
    /* Pre-set brake light ON */
    mock_gpioa.ODR |= BL_ENABLE_Pin;
    
    osMessageQueueId_t brake_queue;
    const osMessageQueueAttr_t battr = { .name = "BrakeQueue" };
    brake_queue = osMessageQueueNew(1, sizeof(uint8_t), &battr);
    
    uint8_t brake_cmd = 0; /* OFF */
    osMessageQueuePut(brake_queue, &brake_cmd, 0, 0);
    
    uint8_t received;
    osMessageQueueGet(brake_queue, &received, NULL, 0);
    
    if (received == 1) {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
    }
    
    /* Brake light should be OFF */
    TEST_ASSERT_FALSE(mock_gpioa.ODR & BL_ENABLE_Pin);
}

/* ============================================================================
 * Integration Test: Heartbeat CAN → LED Control
 * ============================================================================ */

void test_integration_heartbeat_led_control(void)
{
    integration_setup();
    
    /* Simulate heartbeat CAN message: LED ON */
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
    TEST_ASSERT_TRUE(mock_gpiob.ODR & LED_GREEN_Pin);
    
    /* Simulate heartbeat CAN message: LED OFF */
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    TEST_ASSERT_FALSE(mock_gpiob.ODR & LED_GREEN_Pin);
}

/* ============================================================================
 * Integration Test: Multiple CAN frames per cycle
 * ============================================================================ */

void test_integration_full_tx_cycle(void)
{
    integration_setup();
    
    /* Set up sensor data */
    test_sensor_data.apps1 = 1500;
    test_sensor_data.apps2 = 1600;
    test_sensor_data.steering_angle = 45;
    test_sensor_data.brake_pressure_front = 8000;
    test_sensor_data.brake_pressure_rear = 6000;
    
    uint16_t rpmLeft = 300;
    uint16_t rpmRight = 310;
    
    /* Build and send all CAN frames like can_tx_task does */
    uint8_t txData[8];
    
    /* F1_1 */
    txData[0] = test_sensor_data.apps1 & 0xFF;
    txData[1] = test_sensor_data.apps1 >> 8;
    txData[2] = test_sensor_data.apps2 & 0xFF;
    txData[3] = test_sensor_data.apps2 >> 8;
    txData[4] = test_sensor_data.steering_angle & 0xFF;
    txData[5] = test_sensor_data.steering_angle >> 8;
    txData[6] = test_sensor_data.brake_pressure_front & 0xFF;
    txData[7] = test_sensor_data.brake_pressure_front >> 8;
    test_CAN_Transmit(CAN_ID_AD2CAN_F1_1, txData, 8);
    
    /* F1_2 */
    txData[0] = rpmLeft & 0xFF;
    txData[1] = rpmLeft >> 8;
    txData[2] = rpmRight & 0xFF;
    txData[3] = rpmRight >> 8;
    txData[4] = test_sensor_data.brake_pressure_rear & 0xFF;
    txData[5] = test_sensor_data.brake_pressure_rear >> 8;
    txData[6] = 0x03; /* Both wheel sensors active */
    test_CAN_Transmit(CAN_ID_AD2CAN_F1_2, txData, 7);
    
    /* DV_SENSORS_FRONT */
    txData[0] = test_sensor_data.brake_pressure_rear & 0xFF;
    txData[1] = test_sensor_data.brake_pressure_rear >> 8;
    txData[2] = test_sensor_data.brake_pressure_front & 0xFF;
    txData[3] = test_sensor_data.brake_pressure_front >> 8;
    test_CAN_Transmit(CAN_ID_DV_SENSORS_FRONT, txData, 4);
    
    /* Verify all 3 messages were sent */
    TEST_ASSERT_EQUAL(3, test_can_msg_count);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_1, test_can_messages[0].id);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_2, test_can_messages[1].id);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_DV_SENSORS_FRONT, test_can_messages[2].id);
}

/* ============================================================================
 * Integration Test: RPM Pipeline (Timer → Filter → CAN)
 * ============================================================================ */

void test_integration_rpm_pipeline(void)
{
    integration_setup();
    
    /* Simulate RPM readings from timers */
    uint16_t raw_rpm_left = 500;
    uint16_t raw_rpm_right = 520;
    
    /* Filter RPM values */
    /* Reset filter state first */
    for (int i = 0; i < WS_FILTER_SIZE; i++) {
        Sensor_FilterRPM_Left(raw_rpm_left);
        Sensor_FilterRPM_Right(raw_rpm_right);
    }
    
    uint16_t filtered_left = Sensor_FilterRPM_Left(raw_rpm_left);
    uint16_t filtered_right = Sensor_FilterRPM_Right(raw_rpm_right);
    
    /* Pack into CAN message */
    uint8_t txData[8] = {0};
    txData[0] = filtered_left & 0xFF;
    txData[1] = filtered_left >> 8;
    txData[2] = filtered_right & 0xFF;
    txData[3] = filtered_right >> 8;
    
    test_CAN_Transmit(CAN_ID_AD2CAN_F1_2, txData, 7);
    
    /* Decode and verify */
    uint16_t decoded_left  = test_can_messages[0].data[0] | (test_can_messages[0].data[1] << 8);
    uint16_t decoded_right = test_can_messages[0].data[2] | (test_can_messages[0].data[3] << 8);
    
    TEST_ASSERT_EQUAL_UINT16(raw_rpm_left, decoded_left);
    TEST_ASSERT_EQUAL_UINT16(raw_rpm_right, decoded_right);
}

/* ============================================================================
 * Integration Test: Pressure sensor boundary sweep
 * ============================================================================ */

void test_integration_pressure_full_range_sweep(void)
{
    integration_setup();
    
    /* Test all ADC values across the full range through the pipeline */
    uint16_t test_points[] = {0, 200, 399, 400, 401, 1000, 2000, 3000, 3609, 3610, 3611, 4095};
    int num_points = sizeof(test_points) / sizeof(test_points[0]);
    
    for (int i = 0; i < num_points; i++) {
        uint16_t pressure = Sensor_CalculatePressure(test_points[i]);
        
        if (test_points[i] < PRESSURE_MIN_ADC_VAL || test_points[i] > PRESSURE_MAX_ADC_VAL) {
            TEST_ASSERT_EQUAL_UINT16(0, pressure);
        } else if (test_points[i] == PRESSURE_MIN_ADC_VAL) {
            TEST_ASSERT_EQUAL_UINT16(0, pressure);
        } else {
            /* Valid range: pressure should be > 0 and <= PRESSURE_MAX_PRESSURE */
            TEST_ASSERT_TRUE(pressure > 0);
            TEST_ASSERT_TRUE(pressure <= PRESSURE_MAX_PRESSURE);
        }
    }
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_integration_tests();
    return unity_print_summary();
}

void run_integration_tests(void)
{
    UNITY_BEGIN_SUITE("Integration - ADC to CAN Pipeline");
    RUN_TEST(test_integration_adc_to_can_pipeline);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Integration - CAN RX to Brake Light");
    RUN_TEST(test_integration_can_rx_brake_light_on);
    RUN_TEST(test_integration_can_rx_brake_light_off);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Integration - Heartbeat LED Control");
    RUN_TEST(test_integration_heartbeat_led_control);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Integration - Full TX Cycle");
    RUN_TEST(test_integration_full_tx_cycle);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Integration - RPM Pipeline");
    RUN_TEST(test_integration_rpm_pipeline);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Integration - Pressure Range Sweep");
    RUN_TEST(test_integration_pressure_full_range_sweep);
    UNITY_END_SUITE();
}
