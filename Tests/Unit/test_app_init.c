/* ============================================================================
 * Unit Tests: app_init.c - Application Initialization
 * ============================================================================
 * Tests:
 *   - App_Init(): Proper initialization sequence and ordering
 *   - App_Start(): FDCAN start and ADC start
 *   - Debug stage tracking
 *   - Error handling on peripheral failures
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include "../../App/Inc/data_types.h"
#include <string.h>

/* ---- Tracking for init calls ---- */
static int stub_adc_hal_init_count = 0;
static int stub_can_hal_init_count = 0;
static int stub_tim_hal_init_count = 0;
static int stub_sensor_task_init_count = 0;
static int stub_can_tx_task_init_count = 0;
static int stub_can_rx_task_init_count = 0;
static int stub_brake_light_task_init_count = 0;
static int stub_adc_hal_start_count = 0;
static int stub_init_order[10];
static int stub_init_order_idx = 0;

/* Override all Init functions with tracking stubs */
#define ADC_HAL_Init        stub_ADC_HAL_Init_Track
#define CAN_HAL_Init        stub_CAN_HAL_Init_Track
#define TIM_HAL_Init        stub_TIM_HAL_Init_Track
#define SensorTask_Init     stub_SensorTask_Init_Track
#define CanTxTask_Init      stub_CanTxTask_Init_Track
#define CanRxTask_Init      stub_CanRxTask_Init_Track
#define BrakeLightTask_Init stub_BrakeLightTask_Init_Track
#define ADC_HAL_Start       stub_ADC_HAL_Start_Track

void stub_ADC_HAL_Init_Track(void) { 
    stub_adc_hal_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 1;
}
void stub_CAN_HAL_Init_Track(void) { 
    stub_can_hal_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 2;
}
void stub_TIM_HAL_Init_Track(void) { 
    stub_tim_hal_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 3;
}
void stub_SensorTask_Init_Track(void) { 
    stub_sensor_task_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 4;
}
void stub_CanTxTask_Init_Track(void) { 
    stub_can_tx_task_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 5;
}
void stub_CanRxTask_Init_Track(void) { 
    stub_can_rx_task_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 6;
}
void stub_BrakeLightTask_Init_Track(void) { 
    stub_brake_light_task_init_count++; 
    if (stub_init_order_idx < 10) stub_init_order[stub_init_order_idx++] = 7;
}
void stub_ADC_HAL_Start_Track(void) { 
    stub_adc_hal_start_count++; 
}

#include "../../App/Src/app_init.c"

#undef ADC_HAL_Init
#undef CAN_HAL_Init
#undef TIM_HAL_Init
#undef SensorTask_Init
#undef CanTxTask_Init
#undef CanRxTask_Init
#undef BrakeLightTask_Init
#undef ADC_HAL_Start

static void app_init_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    
    stub_adc_hal_init_count = 0;
    stub_can_hal_init_count = 0;
    stub_tim_hal_init_count = 0;
    stub_sensor_task_init_count = 0;
    stub_can_tx_task_init_count = 0;
    stub_can_rx_task_init_count = 0;
    stub_brake_light_task_init_count = 0;
    stub_adc_hal_start_count = 0;
    stub_init_order_idx = 0;
    memset(stub_init_order, 0, sizeof(stub_init_order));
    
    dbg_init_stage = 0;
    dbg_free_heap = 0;
}

/* ============================================================================
 * App_Init Tests
 * ============================================================================ */

void test_app_init_calls_all_inits(void)
{
    app_init_test_setup();
    App_Init();
    
    TEST_ASSERT_EQUAL(1, stub_adc_hal_init_count);
    TEST_ASSERT_EQUAL(1, stub_can_hal_init_count);
    TEST_ASSERT_EQUAL(1, stub_tim_hal_init_count);
    TEST_ASSERT_EQUAL(1, stub_sensor_task_init_count);
    TEST_ASSERT_EQUAL(1, stub_can_tx_task_init_count);
    TEST_ASSERT_EQUAL(1, stub_can_rx_task_init_count);
    TEST_ASSERT_EQUAL(1, stub_brake_light_task_init_count);
}

void test_app_init_correct_order(void)
{
    app_init_test_setup();
    App_Init();
    
    /* Order: ADC(1), CAN(2), TIM(3), Sensor(4), CanTx(5), CanRx(6), BrakeLight(7) */
    TEST_ASSERT_EQUAL(7, stub_init_order_idx);
    TEST_ASSERT_EQUAL(1, stub_init_order[0]); /* ADC first */
    TEST_ASSERT_EQUAL(2, stub_init_order[1]); /* CAN second */
    TEST_ASSERT_EQUAL(3, stub_init_order[2]); /* TIM third */
    TEST_ASSERT_EQUAL(4, stub_init_order[3]); /* SensorTask fourth */
    TEST_ASSERT_EQUAL(5, stub_init_order[4]); /* CanTxTask fifth */
    TEST_ASSERT_EQUAL(6, stub_init_order[5]); /* CanRxTask sixth */
    TEST_ASSERT_EQUAL(7, stub_init_order[6]); /* BrakeLightTask seventh */
}

void test_app_init_hal_before_tasks(void)
{
    app_init_test_setup();
    App_Init();
    
    /* HAL init (1,2,3) must come before task init (4,5,6,7) */
    for (int i = 0; i < 3; i++) {
        for (int j = 3; j < 7; j++) {
            TEST_ASSERT_TRUE(stub_init_order[i] < stub_init_order[j]);
        }
    }
}

void test_app_init_debug_stage_tracking(void)
{
    app_init_test_setup();
    App_Init();
    
    /* Final stage should be 9 (all init complete) */
    TEST_ASSERT_EQUAL(9, dbg_init_stage);
}

void test_app_init_reports_free_heap(void)
{
    app_init_test_setup();
    App_Init();
    
    /* xPortGetFreeHeapSize mock returns 8192 */
    TEST_ASSERT_EQUAL(8192, dbg_free_heap);
}

/* ============================================================================
 * App_Start Tests
 * ============================================================================ */

void test_app_start_starts_fdcan(void)
{
    app_init_test_setup();
    App_Start();
    
    TEST_ASSERT_EQUAL(1, mock_hal.fdcan_start_count);
}

void test_app_start_starts_adc(void)
{
    app_init_test_setup();
    App_Start();
    
    TEST_ASSERT_EQUAL(1, stub_adc_hal_start_count);
}

void test_app_start_fdcan_fail_calls_error(void)
{
    app_init_test_setup();
    mock_hal.fdcan_start_return = HAL_ERROR;
    
    App_Start();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_app_init_tests();
    return unity_print_summary();
}

void run_app_init_tests(void)
{
    UNITY_BEGIN_SUITE("App Init - Initialization Sequence");
    RUN_TEST(test_app_init_calls_all_inits);
    RUN_TEST(test_app_init_correct_order);
    RUN_TEST(test_app_init_hal_before_tasks);
    RUN_TEST(test_app_init_debug_stage_tracking);
    RUN_TEST(test_app_init_reports_free_heap);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("App Init - App Start");
    RUN_TEST(test_app_start_starts_fdcan);
    RUN_TEST(test_app_start_starts_adc);
    RUN_TEST(test_app_start_fdcan_fail_calls_error);
    UNITY_END_SUITE();
}
