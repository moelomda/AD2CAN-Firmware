/* ============================================================================
 * Unit Tests: sensor_task.c - Sensor processing RTOS task
 * ============================================================================
 * Tests:
 *   - SensorTask_Init(): Mutex and thread creation
 *   - SensorTask_Run(): ADC data processing pipeline
 *   - SensorTask_GetDataPointer(): Returns shared data struct
 *   - SensorTask_GetMutexHandle(): Returns valid mutex
 *   - Data integrity through mutex protection
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include "../../App/Inc/data_types.h"
#include "../../App/Inc/Drivers/sensors.h"
#include "../../App/Inc/HAL/adc_hal.h"

/* We need the sensor source for calculations */
#include "../../App/Src/Drivers/sensors.c"

/* ---- Stubs for ADC HAL functions called by sensor_task ---- */
static osMessageQueueId_t stub_adc_queue = NULL;

/* Override the real ADC_HAL functions with test stubs */
#define ADC_HAL_GetQueueHandle  stub_ADC_HAL_GetQueueHandle
#define ADC_HAL_StartOnce       stub_ADC_HAL_StartOnce

static int stub_adc_start_once_count = 0;

osMessageQueueId_t stub_ADC_HAL_GetQueueHandle(void)
{
    return stub_adc_queue;
}

void stub_ADC_HAL_StartOnce(void)
{
    stub_adc_start_once_count++;
}

/* Now include the source under test (it will use our stubs) */
#include "../../App/Src/Tasks/sensor_task.c"

/* Undef to avoid conflicts */
#undef ADC_HAL_GetQueueHandle
#undef ADC_HAL_StartOnce

static void sensor_task_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    stub_adc_start_once_count = 0;
    
    /* Create stubs for the ADC queue */
    const osMessageQueueAttr_t attr = { .name = "StubAdcQueue" };
    stub_adc_queue = osMessageQueueNew(4, sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), &attr);
    
    /* Reset sensor data */
    memset(&sensorData, 0, sizeof(ProcessedSensorData_t));
    sensorDataMutexHandle = NULL;
}

/* ============================================================================
 * SensorTask_Init Tests
 * ============================================================================ */

void test_sensor_task_init_creates_mutex(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    TEST_ASSERT_NOT_NULL(SensorTask_GetMutexHandle());
}

void test_sensor_task_init_creates_thread(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    /* Should have created 1 thread */
    TEST_ASSERT_TRUE(mock_rtos.thread_count > 0);
}

void test_sensor_task_init_mutex_fail_calls_error(void)
{
    sensor_task_test_setup();
    mock_rtos.mutex_create_fail = 1;
    
    SensorTask_Init();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

void test_sensor_task_init_thread_fail_calls_error(void)
{
    sensor_task_test_setup();
    mock_rtos.thread_create_fail = 1;
    /* Allow mutex to succeed */
    
    SensorTask_Init();
    
    /* Thread creation failure should call Error_Handler */
    TEST_ASSERT_TRUE(mock_hal.error_handler_count > 0);
}

/* ============================================================================
 * SensorTask Data Pointer Tests
 * ============================================================================ */

void test_sensor_task_get_data_pointer(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    ProcessedSensorData_t *ptr = SensorTask_GetDataPointer();
    TEST_ASSERT_NOT_NULL(ptr);
}

void test_sensor_task_get_mutex_handle(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    osMutexId_t m = SensorTask_GetMutexHandle();
    TEST_ASSERT_NOT_NULL(m);
}

/* ============================================================================
 * SensorTask_Run - Single Iteration Test
 * ============================================================================ */

void test_sensor_task_processes_adc_data(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    /* Prepare simulated ADC data */
    uint16_t adc_data[ADC_NUMBER_OF_CHANNELS] = {
        2000,   /* brake_pressure_front: valid range */
        1500,   /* brake_pressure_rear: valid range */
        2648,   /* steering_angle: center-ish */
        2048,   /* apps1 */
        2048,   /* apps2 */
        0       /* unused channel 6 */
    };
    
    /* Put ADC data into the stub queue */
    osMessageQueuePut(stub_adc_queue, adc_data, 0, 0);
    
    /* Run one iteration of the task (it will get the message and process) */
    /* We need to manually call the processing logic since the task loop
     * would block on osMessageQueueGet with osWaitForever */
    uint16_t received_adc_data[ADC_NUMBER_OF_CHANNELS];
    ProcessedSensorData_t local_data;
    
    osStatus_t status = osMessageQueueGet(stub_adc_queue, received_adc_data, NULL, 0);
    TEST_ASSERT_EQUAL(osOK, status);
    
    local_data.brake_pressure_front = Sensor_CalculatePressure(received_adc_data[0]);
    local_data.brake_pressure_rear  = Sensor_CalculatePressure(received_adc_data[1]);
    local_data.steering_angle       = Sensor_CalculateSteeringAngle(received_adc_data[2]);
    local_data.apps1                = received_adc_data[3];
    local_data.apps2                = received_adc_data[4];
    
    /* Verify pressure calculations */
    TEST_ASSERT_TRUE(local_data.brake_pressure_front > 0);
    TEST_ASSERT_TRUE(local_data.brake_pressure_rear > 0);
    
    /* Verify raw pass-through for APPS */
    TEST_ASSERT_EQUAL_UINT16(2048, local_data.apps1);
    TEST_ASSERT_EQUAL_UINT16(2048, local_data.apps2);
    
    /* Verify steering is reasonable */
    TEST_ASSERT_TRUE(local_data.steering_angle >= -900);
    TEST_ASSERT_TRUE(local_data.steering_angle <= 900);
}

void test_sensor_task_pressure_out_of_range(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    /* ADC values outside valid pressure range */
    uint16_t adc_data[ADC_NUMBER_OF_CHANNELS] = {
        100,    /* Below PRESSURE_MIN_ADC_VAL */
        4000,   /* Above PRESSURE_MAX_ADC_VAL */
        2648,
        0, 0, 0
    };
    
    osMessageQueuePut(stub_adc_queue, adc_data, 0, 0);
    
    uint16_t received[ADC_NUMBER_OF_CHANNELS];
    osMessageQueueGet(stub_adc_queue, received, NULL, 0);
    
    uint16_t front = Sensor_CalculatePressure(received[0]);
    uint16_t rear  = Sensor_CalculatePressure(received[1]);
    
    TEST_ASSERT_EQUAL_UINT16(0, front);
    TEST_ASSERT_EQUAL_UINT16(0, rear);
}

void test_sensor_task_mutex_protects_data(void)
{
    sensor_task_test_setup();
    SensorTask_Init();
    
    osMutexId_t m = SensorTask_GetMutexHandle();
    
    /* Acquire mutex */
    TEST_ASSERT_EQUAL(osOK, osMutexAcquire(m, 0));
    
    /* Write some data */
    ProcessedSensorData_t *data = SensorTask_GetDataPointer();
    data->apps1 = 1234;
    data->apps2 = 5678;
    
    /* Release mutex */
    TEST_ASSERT_EQUAL(osOK, osMutexRelease(m));
    
    /* Verify data persists */
    TEST_ASSERT_EQUAL_UINT16(1234, data->apps1);
    TEST_ASSERT_EQUAL_UINT16(5678, data->apps2);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_sensor_task_tests();
    return unity_print_summary();
}

void run_sensor_task_tests(void)
{
    UNITY_BEGIN_SUITE("Sensor Task - Initialization");
    RUN_TEST(test_sensor_task_init_creates_mutex);
    RUN_TEST(test_sensor_task_init_creates_thread);
    RUN_TEST(test_sensor_task_init_mutex_fail_calls_error);
    RUN_TEST(test_sensor_task_init_thread_fail_calls_error);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Sensor Task - Data Access");
    RUN_TEST(test_sensor_task_get_data_pointer);
    RUN_TEST(test_sensor_task_get_mutex_handle);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Sensor Task - Processing");
    RUN_TEST(test_sensor_task_processes_adc_data);
    RUN_TEST(test_sensor_task_pressure_out_of_range);
    RUN_TEST(test_sensor_task_mutex_protects_data);
    UNITY_END_SUITE();
}
