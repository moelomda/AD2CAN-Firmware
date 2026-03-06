/* ============================================================================
 * Unit Tests: can_hal.c - CAN Hardware Abstraction Layer
 * ============================================================================
 * Tests:
 *   - CAN_HAL_Init(): Queue creation, TX header setup
 *   - CAN_HAL_Transmit(): Message transmission with various DLC
 *   - CAN_HAL_GetBrakeQueueHandle(): Returns valid queue
 *   - CAN_HAL_GetHeartbeatActive() / CAN_HAL_ClearHeartbeatActive()
 *   - HAL_FDCAN_RxFifo0Callback(): CAN RX interrupt handler
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include <stdbool.h>

/* Include the source under test */
#include "../../App/Src/HAL/can_hal.c"

static void can_test_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    heartbeatActive = false;
}

/* ============================================================================
 * CAN_HAL_Init Tests
 * ============================================================================ */

void test_can_init_creates_brake_queue(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    osMessageQueueId_t q = CAN_HAL_GetBrakeQueueHandle();
    TEST_ASSERT_NOT_NULL(q);
}

void test_can_init_brake_queue_capacity(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    osMessageQueueId_t q = CAN_HAL_GetBrakeQueueHandle();
    MockMessageQueue_t *mq = (MockMessageQueue_t *)q;
    TEST_ASSERT_EQUAL(1, mq->capacity);
    TEST_ASSERT_EQUAL(sizeof(uint8_t), mq->msg_size);
}

void test_can_init_queue_fail_calls_error(void)
{
    can_test_setup();
    mock_rtos.queue_create_fail = 1;
    
    CAN_HAL_Init();
    
    TEST_ASSERT_EQUAL(1, mock_hal.error_handler_count);
}

void test_can_init_sets_tx_header(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    TEST_ASSERT_EQUAL(FDCAN_STANDARD_ID, canTxHeader.IdType);
    TEST_ASSERT_EQUAL(FDCAN_DATA_FRAME, canTxHeader.TxFrameType);
    TEST_ASSERT_EQUAL(FDCAN_CLASSIC_CAN, canTxHeader.FDFormat);
    TEST_ASSERT_EQUAL(FDCAN_BRS_OFF, canTxHeader.BitRateSwitch);
}

/* ============================================================================
 * CAN_HAL_Transmit Tests
 * ============================================================================ */

void test_can_transmit_1_byte(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    uint8_t data[1] = {0xAA};
    HAL_StatusTypeDef status = CAN_HAL_Transmit(0x510, data, 1);
    
    TEST_ASSERT_EQUAL(HAL_OK, status);
    TEST_ASSERT_EQUAL(1, mock_hal.can_tx_count);
    TEST_ASSERT_EQUAL_HEX32(0x510, mock_hal.can_tx_messages[0].id);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mock_hal.can_tx_messages[0].data[0]);
}

void test_can_transmit_8_bytes(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    CAN_HAL_Transmit(0x511, data, 8);
    
    TEST_ASSERT_EQUAL(1, mock_hal.can_tx_count);
    TEST_ASSERT_EQUAL_HEX32(0x511, mock_hal.can_tx_messages[0].id);
    TEST_ASSERT_EQUAL_MEMORY(data, mock_hal.can_tx_messages[0].data, 8);
}

void test_can_transmit_all_dlc_lengths(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    uint8_t data[8] = {0};
    
    for (uint32_t len = 1; len <= 8; len++) {
        CAN_HAL_Transmit(0x100 + len, data, len);
    }
    
    TEST_ASSERT_EQUAL(8, mock_hal.can_tx_count);
}

void test_can_transmit_failure(void)
{
    can_test_setup();
    CAN_HAL_Init();
    mock_hal.can_tx_return_status = HAL_ERROR;
    
    uint8_t data[1] = {0};
    HAL_StatusTypeDef status = CAN_HAL_Transmit(0x100, data, 1);
    
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
}

void test_can_transmit_correct_id(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    uint8_t data[1] = {0};
    CAN_HAL_Transmit(CAN_ID_AD2CAN_F1_1, data, 1);
    CAN_HAL_Transmit(CAN_ID_AD2CAN_F1_2, data, 1);
    CAN_HAL_Transmit(CAN_ID_DV_SENSORS_FRONT, data, 1);
    CAN_HAL_Transmit(CAN_ID_AD2CAN_STARTUP, data, 1);
    
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_1, mock_hal.can_tx_messages[0].id);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_2, mock_hal.can_tx_messages[1].id);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_DV_SENSORS_FRONT, mock_hal.can_tx_messages[2].id);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_STARTUP, mock_hal.can_tx_messages[3].id);
}

/* ============================================================================
 * Heartbeat Tests
 * ============================================================================ */

void test_heartbeat_initially_false(void)
{
    can_test_setup();
    TEST_ASSERT_FALSE(CAN_HAL_GetHeartbeatActive());
}

void test_heartbeat_clear(void)
{
    can_test_setup();
    heartbeatActive = true;
    
    TEST_ASSERT_TRUE(CAN_HAL_GetHeartbeatActive());
    CAN_HAL_ClearHeartbeatActive();
    TEST_ASSERT_FALSE(CAN_HAL_GetHeartbeatActive());
}

/* ============================================================================
 * CAN RX Callback Tests
 * ============================================================================ */

void test_can_rx_brake_command(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    /* Inject brake command message */
    mock_hal.can_rx_inject_header.Identifier = CAN_ID_RX_BRAKE;
    mock_hal.can_rx_inject_header.DataLength = FDCAN_DLC_BYTES_1;
    mock_hal.can_rx_inject_data[0] = 1; /* Brake ON */
    
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    
    /* Verify brake command was queued */
    osMessageQueueId_t q = CAN_HAL_GetBrakeQueueHandle();
    uint8_t received;
    TEST_ASSERT_EQUAL(osOK, osMessageQueueGet(q, &received, NULL, 0));
    TEST_ASSERT_EQUAL_UINT8(1, received);
}

void test_can_rx_brake_command_off(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    mock_hal.can_rx_inject_header.Identifier = CAN_ID_RX_BRAKE;
    mock_hal.can_rx_inject_header.DataLength = FDCAN_DLC_BYTES_1;
    mock_hal.can_rx_inject_data[0] = 0; /* Brake OFF */
    
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    
    osMessageQueueId_t q = CAN_HAL_GetBrakeQueueHandle();
    uint8_t received;
    TEST_ASSERT_EQUAL(osOK, osMessageQueueGet(q, &received, NULL, 0));
    TEST_ASSERT_EQUAL_UINT8(0, received);
}

void test_can_rx_heartbeat_sets_active(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    mock_hal.can_rx_inject_header.Identifier = CAN_ID_HEARTBEAT;
    mock_hal.can_rx_inject_header.DataLength = FDCAN_DLC_BYTES_1;
    mock_hal.can_rx_inject_data[0] = 1; /* LED ON */
    
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    
    TEST_ASSERT_TRUE(CAN_HAL_GetHeartbeatActive());
}

void test_can_rx_heartbeat_controls_led(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    /* Heartbeat with LED=1 */
    mock_hal.can_rx_inject_header.Identifier = CAN_ID_HEARTBEAT;
    mock_hal.can_rx_inject_header.DataLength = FDCAN_DLC_BYTES_1;
    mock_hal.can_rx_inject_data[0] = 1;
    
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    
    /* Green LED should be set */
    TEST_ASSERT_EQUAL(1, mock_hal.gpio_write_count);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, mock_hal.gpio_write_calls[0].state);
}

void test_can_rx_unknown_id_ignored(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    mock_hal.can_rx_inject_header.Identifier = 0x999; /* Unknown ID */
    mock_hal.can_rx_inject_header.DataLength = FDCAN_DLC_BYTES_1;
    
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    
    /* No brake command queued, no heartbeat */
    osMessageQueueId_t q = CAN_HAL_GetBrakeQueueHandle();
    TEST_ASSERT_EQUAL(0, osMessageQueueGetCount(q));
    TEST_ASSERT_FALSE(CAN_HAL_GetHeartbeatActive());
}

void test_can_rx_no_new_message_flag(void)
{
    can_test_setup();
    CAN_HAL_Init();
    
    /* Call without NEW_MESSAGE flag */
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, 0);
    
    /* Nothing should change */
    osMessageQueueId_t q = CAN_HAL_GetBrakeQueueHandle();
    TEST_ASSERT_EQUAL(0, osMessageQueueGetCount(q));
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_can_hal_tests();
    return unity_print_summary();
}

void run_can_hal_tests(void)
{
    UNITY_BEGIN_SUITE("CAN HAL - Initialization");
    RUN_TEST(test_can_init_creates_brake_queue);
    RUN_TEST(test_can_init_brake_queue_capacity);
    RUN_TEST(test_can_init_queue_fail_calls_error);
    RUN_TEST(test_can_init_sets_tx_header);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN HAL - Transmit");
    RUN_TEST(test_can_transmit_1_byte);
    RUN_TEST(test_can_transmit_8_bytes);
    RUN_TEST(test_can_transmit_all_dlc_lengths);
    RUN_TEST(test_can_transmit_failure);
    RUN_TEST(test_can_transmit_correct_id);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN HAL - Heartbeat");
    RUN_TEST(test_heartbeat_initially_false);
    RUN_TEST(test_heartbeat_clear);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("CAN HAL - RX Callback");
    RUN_TEST(test_can_rx_brake_command);
    RUN_TEST(test_can_rx_brake_command_off);
    RUN_TEST(test_can_rx_heartbeat_sets_active);
    RUN_TEST(test_can_rx_heartbeat_controls_led);
    RUN_TEST(test_can_rx_unknown_id_ignored);
    RUN_TEST(test_can_rx_no_new_message_flag);
    UNITY_END_SUITE();
}
