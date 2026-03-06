/* ============================================================================
 * Unit Tests: adc_hal.c - ADC Hardware Abstraction Layer
 * ============================================================================
 * Tests:
 *   - ADC_HAL_Init(): Queue creation
 *   - ADC_HAL_Start(): Calibration + DMA start
 *   - ADC_HAL_StartOnce(): Single DMA trigger
 *   - ADC_HAL_GetQueueHandle(): Returns valid queue
 *   - ADC_HAL_GetBuffer(): Returns valid buffer
 *   - HAL_ADC_ConvCpltCallback(): DMA callback behavior
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"

/* Include the source under test */
#include "../../App/Src/HAL/adc_hal.c"

/* ---- Setup/Teardown ---- */
static void adc_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
}

/* ============================================================================
 * ADC_HAL_Init Tests
 * ============================================================================ */

void test_adc_init_creates_queue(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    osMessageQueueId_t q = ADC_HAL_GetQueueHandle();
    TEST_ASSERT_NOT_NULL(q);
}

void test_adc_init_queue_capacity(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    osMessageQueueId_t q = ADC_HAL_GetQueueHandle();
    MockMessageQueue_t *mq = (MockMessageQueue_t *)q;
    TEST_ASSERT_EQUAL(4, mq->capacity);
    TEST_ASSERT_EQUAL(sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), mq->msg_size);
}

void test_adc_init_queue_fail_calls_error(void)
{
    adc_test_setup();
    mock_rtos.queue_create_fail = 1;
    
    ADC_HAL_Init();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

/* ============================================================================
 * ADC_HAL_Start Tests
 * ============================================================================ */

void test_adc_start_calibrates(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    ADC_HAL_Start();
    
    TEST_ASSERT_EQUAL(1, mock_hal.adc_calibration_count);
}

void test_adc_start_starts_dma(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    ADC_HAL_Start();
    
    TEST_ASSERT_EQUAL(1, mock_hal.adc_start_dma_count);
}

void test_adc_start_calibration_fail_calls_error(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    mock_hal.adc_calibration_return = HAL_ERROR;
    
    ADC_HAL_Start();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

void test_adc_start_dma_fail_calls_error(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    mock_hal.adc_start_dma_return = HAL_ERROR;
    
    ADC_HAL_Start();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

/* ============================================================================
 * ADC_HAL_StartOnce Tests
 * ============================================================================ */

void test_adc_start_once_triggers_dma(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    int before = mock_hal.adc_start_dma_count;
    ADC_HAL_StartOnce();
    
    TEST_ASSERT_EQUAL(before + 1, mock_hal.adc_start_dma_count);
}

/* ============================================================================
 * ADC_HAL_GetBuffer Tests
 * ============================================================================ */

void test_adc_get_buffer_not_null(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    uint16_t *buf = ADC_HAL_GetBuffer();
    TEST_ASSERT_NOT_NULL(buf);
}

/* ============================================================================
 * HAL_ADC_ConvCpltCallback Tests
 * ============================================================================ */

void test_adc_callback_skips_first_read(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    /* Simulate first DMA completion - should be skipped */
    HAL_ADC_ConvCpltCallback(&hadc1);
    
    osMessageQueueId_t q = ADC_HAL_GetQueueHandle();
    TEST_ASSERT_EQUAL(0, osMessageQueueGetCount(q));
    
    /* DMA should be restarted after skip */
    TEST_ASSERT_TRUE(mock_hal.adc_start_dma_count > 0);
}

void test_adc_callback_posts_data_on_second_read(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    /* Note: first_read_skipped is a persistent static local in the callback.
     * After a prior test set it to 1, subsequent calls always post data.
     * We test that a callback posts data to the queue. */
    osMessageQueueId_t q = ADC_HAL_GetQueueHandle();
    uint32_t count_before = osMessageQueueGetCount(q);
    
    HAL_ADC_ConvCpltCallback(&hadc1);
    
    uint32_t count_after = osMessageQueueGetCount(q);
    /* At least one new message should be posted (once first-skip has passed) */
    TEST_ASSERT_TRUE(count_after > count_before);
}

void test_adc_callback_stops_dma(void)
{
    adc_test_setup();
    ADC_HAL_Init();
    
    HAL_ADC_ConvCpltCallback(&hadc1);
    
    TEST_ASSERT_TRUE(mock_hal.adc_stop_dma_count > 0);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_adc_hal_tests();
    return unity_print_summary();
}

void run_adc_hal_tests(void)
{
    UNITY_BEGIN_SUITE("ADC HAL - Initialization");
    RUN_TEST(test_adc_init_creates_queue);
    RUN_TEST(test_adc_init_queue_capacity);
    RUN_TEST(test_adc_init_queue_fail_calls_error);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("ADC HAL - Start");
    RUN_TEST(test_adc_start_calibrates);
    RUN_TEST(test_adc_start_starts_dma);
    RUN_TEST(test_adc_start_calibration_fail_calls_error);
    RUN_TEST(test_adc_start_dma_fail_calls_error);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("ADC HAL - Runtime");
    RUN_TEST(test_adc_start_once_triggers_dma);
    RUN_TEST(test_adc_get_buffer_not_null);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("ADC HAL - DMA Callback");
    RUN_TEST(test_adc_callback_skips_first_read);
    RUN_TEST(test_adc_callback_posts_data_on_second_read);
    RUN_TEST(test_adc_callback_stops_dma);
    UNITY_END_SUITE();
}
