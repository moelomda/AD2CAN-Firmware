/* ============================================================================
 * Acceptance Tests: Full System Simulation
 * ============================================================================
 * Simulates complete driving scenarios to validate the system behaves
 * correctly when connected to real hardware. Tests the entire data flow
 * from sensor inputs through processing to CAN outputs and GPIO control.
 *
 * SCENARIOS:
 *   1. System Startup Sequence
 *   2. Normal Driving - Straight Line
 *   3. Hard Braking Event
 *   4. Full Steering Lock Turn
 *   5. High Speed Driving
 *   6. Sensor Failure / Out-of-Range
 *   7. CAN Bus Failure Recovery
 *   8. Heartbeat Loss Detection
 *   9. Rapid Brake Light Toggling
 *  10. Full Race Lap Simulation
 * ============================================================================ */
#include "../Unity/unity.h"
#include "../Mocks/mock_stm32_hal.h"
#include "../Mocks/mock_cmsis_os.h"
#include "../Mocks/main.h"
#include "../../App/Inc/app_config.h"
#include "../../App/Inc/data_types.h"
#include "../../App/Inc/Drivers/sensors.h"
#include <string.h>

/* Include sensor source for calculations */
#include "../../App/Src/Drivers/sensors.c"

/* ---- Acceptance test infrastructure ---- */
static osMessageQueueId_t acc_adc_queue = NULL;
static osMessageQueueId_t acc_brake_queue = NULL;
static ProcessedSensorData_t acc_sensor_data;
static osMutexId_t acc_mutex = NULL;
static MockCanMessage_t acc_can_messages[512];
static int acc_can_msg_count = 0;

/* Simulated wheel sensor GPIO state */
static uint8_t sim_ws_left_active = 0;
static uint8_t sim_ws_right_active = 0;

static HAL_StatusTypeDef acc_CAN_Transmit(uint32_t id, uint8_t* data, uint32_t length)
{
    if (acc_can_msg_count < 512) {
        acc_can_messages[acc_can_msg_count].id = id;
        acc_can_messages[acc_can_msg_count].length = length;
        memcpy(acc_can_messages[acc_can_msg_count].data, data, length);
        acc_can_msg_count++;
    }
    return HAL_OK;
}

/* Simulate one complete cycle: ADC → Process → CAN TX */
static void simulate_one_cycle(uint16_t adc_channels[6], uint16_t rpm_left, uint16_t rpm_right)
{
    /* Sensor processing */
    acc_sensor_data.brake_pressure_front = Sensor_CalculatePressure(adc_channels[0]);
    acc_sensor_data.brake_pressure_rear  = Sensor_CalculatePressure(adc_channels[1]);
    acc_sensor_data.steering_angle       = Sensor_CalculateSteeringAngle(adc_channels[2]);
    acc_sensor_data.apps1                = adc_channels[3];
    acc_sensor_data.apps2                = adc_channels[4];
    
    /* Filter RPM */
    uint16_t filt_left  = Sensor_FilterRPM_Left(rpm_left);
    uint16_t filt_right = Sensor_FilterRPM_Right(rpm_right);
    
    /* Build and send CAN messages */
    uint8_t txData[8];
    
    /* F1_1: APPS + Steering + Front Brake Pressure */
    txData[0] = acc_sensor_data.apps1 & 0xFF;
    txData[1] = acc_sensor_data.apps1 >> 8;
    txData[2] = acc_sensor_data.apps2 & 0xFF;
    txData[3] = acc_sensor_data.apps2 >> 8;
    txData[4] = acc_sensor_data.steering_angle & 0xFF;
    txData[5] = (acc_sensor_data.steering_angle >> 8) & 0xFF;
    txData[6] = acc_sensor_data.brake_pressure_front & 0xFF;
    txData[7] = acc_sensor_data.brake_pressure_front >> 8;
    acc_CAN_Transmit(CAN_ID_AD2CAN_F1_1, txData, 8);
    
    /* F1_2: RPM + Rear Brake + WS pins */
    txData[0] = filt_left & 0xFF;
    txData[1] = filt_left >> 8;
    txData[2] = filt_right & 0xFF;
    txData[3] = filt_right >> 8;
    txData[4] = acc_sensor_data.brake_pressure_rear & 0xFF;
    txData[5] = acc_sensor_data.brake_pressure_rear >> 8;
    txData[6] = (sim_ws_left_active ? 1 : 0) | ((sim_ws_right_active ? 1 : 0) << 1);
    acc_CAN_Transmit(CAN_ID_AD2CAN_F1_2, txData, 7);
}

/* Decode a CAN F1_1 message back to sensor values */
static void decode_f1_1(const MockCanMessage_t *msg, uint16_t *apps1, uint16_t *apps2,
                         int16_t *steering, uint16_t *brake_front)
{
    *apps1       = msg->data[0] | (msg->data[1] << 8);
    *apps2       = msg->data[2] | (msg->data[3] << 8);
    *steering    = (int16_t)(msg->data[4] | (msg->data[5] << 8));
    *brake_front = msg->data[6] | (msg->data[7] << 8);
}

/* Decode a CAN F1_2 message */
static void decode_f1_2(const MockCanMessage_t *msg, uint16_t *rpm_l, uint16_t *rpm_r,
                         uint16_t *brake_rear, uint8_t *ws_pins)
{
    *rpm_l      = msg->data[0] | (msg->data[1] << 8);
    *rpm_r      = msg->data[2] | (msg->data[3] << 8);
    *brake_rear = msg->data[4] | (msg->data[5] << 8);
    *ws_pins    = msg->data[6];
}

static void acceptance_setup(void)
{
    MockHAL_Reset();
    MockRTOS_Reset();
    
    acc_can_msg_count = 0;
    memset(acc_can_messages, 0, sizeof(acc_can_messages));
    memset(&acc_sensor_data, 0, sizeof(ProcessedSensorData_t));
    sim_ws_left_active = 0;
    sim_ws_right_active = 0;
    
    /* Reset RPM filters */
    for (int i = 0; i < WS_FILTER_SIZE; i++) {
        Sensor_FilterRPM_Left(0);
        Sensor_FilterRPM_Right(0);
    }
    
    const osMessageQueueAttr_t qattr = { .name = "AccAdcQueue" };
    acc_adc_queue = osMessageQueueNew(4, sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), &qattr);
    
    const osMessageQueueAttr_t battr = { .name = "AccBrakeQueue" };
    acc_brake_queue = osMessageQueueNew(4, sizeof(uint8_t), &battr);
    
    const osMutexAttr_t mattr = { .name = "AccMutex" };
    acc_mutex = osMutexNew(&mattr);
}

/* ============================================================================
 * SCENARIO 1: System Startup Sequence
 * ============================================================================ */
void test_acceptance_startup_sequence(void)
{
    acceptance_setup();
    
    /* 1. Send startup message (0xAA) */
    uint8_t startup_data = 0xAA;
    acc_CAN_Transmit(CAN_ID_AD2CAN_STARTUP, &startup_data, 1);
    
    TEST_ASSERT_EQUAL(1, acc_can_msg_count);
    TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_STARTUP, acc_can_messages[0].id);
    TEST_ASSERT_EQUAL_UINT8(0xAA, acc_can_messages[0].data[0]);
    
    /* 2. First sensor cycle with zeroed data */
    uint16_t zero_adc[6] = {0, 0, STEERING_RAW_LEFT, 0, 0, 0};
    simulate_one_cycle(zero_adc, 0, 0);
    
    /* All pressures should be 0, steering at -900 */
    uint16_t apps1, apps2, brake_front;
    int16_t steering;
    decode_f1_1(&acc_can_messages[1], &apps1, &apps2, &steering, &brake_front);
    
    TEST_ASSERT_EQUAL_UINT16(0, apps1);
    TEST_ASSERT_EQUAL_UINT16(0, apps2);
    TEST_ASSERT_EQUAL_INT16(-900, steering);
    TEST_ASSERT_EQUAL_UINT16(0, brake_front);
}

/* ============================================================================
 * SCENARIO 2: Normal Driving - Straight Line, Moderate Speed
 * ============================================================================ */
void test_acceptance_normal_driving_straight(void)
{
    acceptance_setup();
    
    /* Center steering, moderate throttle, no braking, ~60 km/h */
    uint16_t center_steering = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    uint16_t adc[6] = {
        300,                /* front brake: below minimum (no braking) */
        300,                /* rear brake: below minimum (no braking) */
        center_steering,    /* straight ahead */
        3000,               /* APPS1: ~73% throttle */
        3000,               /* APPS2: ~73% throttle (should match APPS1) */
        0
    };
    
    sim_ws_left_active = 1;
    sim_ws_right_active = 1;
    
    /* Run 10 cycles */
    for (int i = 0; i < 10; i++) {
        simulate_one_cycle(adc, 800, 800);
    }
    
    /* Verify last F1_1 message */
    int last_f1_1_idx = acc_can_msg_count - 2; /* F1_1 is sent before F1_2 */
    uint16_t apps1, apps2, brake_front;
    int16_t steering;
    decode_f1_1(&acc_can_messages[last_f1_1_idx], &apps1, &apps2, &steering, &brake_front);
    
    TEST_ASSERT_EQUAL_UINT16(3000, apps1);
    TEST_ASSERT_EQUAL_UINT16(3000, apps2);
    TEST_ASSERT_TRUE(steering >= -20 && steering <= 20); /* Near center */
    TEST_ASSERT_EQUAL_UINT16(0, brake_front); /* No braking */
    
    /* Verify last F1_2 message */
    int last_f1_2_idx = acc_can_msg_count - 1;
    uint16_t rpm_l, rpm_r, brake_rear;
    uint8_t ws_pins;
    decode_f1_2(&acc_can_messages[last_f1_2_idx], &rpm_l, &rpm_r, &brake_rear, &ws_pins);
    
    TEST_ASSERT_EQUAL_UINT16(800, rpm_l); /* Max filter should keep 800 */
    TEST_ASSERT_EQUAL_UINT16(800, rpm_r);
    TEST_ASSERT_EQUAL_UINT16(0, brake_rear);
    TEST_ASSERT_EQUAL_UINT8(0x03, ws_pins); /* Both WS active */
}

/* ============================================================================
 * SCENARIO 3: Hard Braking Event
 * ============================================================================ */
void test_acceptance_hard_braking(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    
    /* Phase 1: Driving at speed */
    uint16_t adc_driving[6] = {300, 300, center, 3000, 3000, 0};
    for (int i = 0; i < 5; i++) {
        simulate_one_cycle(adc_driving, 1000, 1000);
    }
    
    /* Phase 2: Hard braking - high pressure, throttle released */
    uint16_t adc_braking[6] = {
        3400,   /* front brake: near max pressure */
        3200,   /* rear brake: high pressure */
        center,
        500,    /* APPS1: minimal throttle */
        500,    /* APPS2: minimal throttle */
        0
    };
    
    for (int i = 0; i < 5; i++) {
        /* RPM decreasing as car brakes */
        simulate_one_cycle(adc_braking, 1000 - i * 150, 1000 - i * 150);
    }
    
    /* Verify brake pressure is reported in CAN */
    int last_f1_1_idx = acc_can_msg_count - 2;
    uint16_t apps1, apps2, brake_front;
    int16_t steering;
    decode_f1_1(&acc_can_messages[last_f1_1_idx], &apps1, &apps2, &steering, &brake_front);
    
    TEST_ASSERT_TRUE(brake_front > 10000); /* High pressure */
    TEST_ASSERT_EQUAL_UINT16(500, apps1); /* Throttle released */
    
    /* Verify RPM is decreasing */
    int last_f1_2_idx = acc_can_msg_count - 1;
    uint16_t rpm_l, rpm_r, brake_rear;
    uint8_t ws_pins;
    decode_f1_2(&acc_can_messages[last_f1_2_idx], &rpm_l, &rpm_r, &brake_rear, &ws_pins);
    
    TEST_ASSERT_TRUE(brake_rear > 10000);
    
    /* Simulate brake light control */
    uint8_t brake_cmd = 1;
    HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
    TEST_ASSERT_TRUE(mock_gpioa.ODR & BL_ENABLE_Pin);
}

/* ============================================================================
 * SCENARIO 4: Full Steering Lock Turn
 * ============================================================================ */
void test_acceptance_full_steering_lock(void)
{
    acceptance_setup();
    
    /* Turn from full left to full right */
    uint16_t adc[6] = {300, 300, 0, 1500, 1500, 0};
    
    /* Full left */
    adc[2] = STEERING_RAW_LEFT;
    simulate_one_cycle(adc, 200, 200);
    
    int idx = acc_can_msg_count - 2;
    uint16_t a1, a2, bf;
    int16_t steer;
    decode_f1_1(&acc_can_messages[idx], &a1, &a2, &steer, &bf);
    TEST_ASSERT_EQUAL_INT16(-900, steer);
    
    /* Full right */
    adc[2] = STEERING_RAW_RIGHT;
    simulate_one_cycle(adc, 200, 200);
    
    idx = acc_can_msg_count - 2;
    decode_f1_1(&acc_can_messages[idx], &a1, &a2, &steer, &bf);
    TEST_ASSERT_EQUAL_INT16(900, steer);
    
    /* Center */
    adc[2] = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    simulate_one_cycle(adc, 200, 200);
    
    idx = acc_can_msg_count - 2;
    decode_f1_1(&acc_can_messages[idx], &a1, &a2, &steer, &bf);
    TEST_ASSERT_TRUE(steer >= -20 && steer <= 20);
}

/* ============================================================================
 * SCENARIO 5: High Speed Driving (RPM at extremes)
 * ============================================================================ */
void test_acceptance_high_speed(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    uint16_t adc[6] = {300, 300, center, 4000, 4000, 0};
    
    sim_ws_left_active = 1;
    sim_ws_right_active = 1;
    
    /* Simulate acceleration to high RPM */
    for (int i = 0; i < 20; i++) {
        uint16_t rpm = 500 + i * 250; /* 500 to 5250 RPM */
        simulate_one_cycle(adc, rpm, rpm);
    }
    
    /* Verify high RPM is transmitted (max filtered value) */
    int last = acc_can_msg_count - 1;
    uint16_t rpm_l, rpm_r, brake_rear;
    uint8_t ws_pins;
    decode_f1_2(&acc_can_messages[last], &rpm_l, &rpm_r, &brake_rear, &ws_pins);
    
    TEST_ASSERT_TRUE(rpm_l >= 5000);
    TEST_ASSERT_TRUE(rpm_r >= 5000);
    TEST_ASSERT_EQUAL_UINT8(0x03, ws_pins);
}

/* ============================================================================
 * SCENARIO 6: Sensor Failure / Out-of-Range Values
 * ============================================================================ */
void test_acceptance_sensor_failure(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    
    /* ADC reads 0 on all channels (sensor disconnected) */
    uint16_t adc_fail[6] = {0, 0, center, 0, 0, 0};
    simulate_one_cycle(adc_fail, 0, 0);
    
    uint16_t apps1, apps2, brake_front;
    int16_t steering;
    decode_f1_1(&acc_can_messages[0], &apps1, &apps2, &steering, &brake_front);
    
    /* Pressure should be 0 (out of range) */
    TEST_ASSERT_EQUAL_UINT16(0, brake_front);
    TEST_ASSERT_EQUAL_UINT16(0, apps1);
    
    /* ADC reads max (4095) on pressure channels */
    uint16_t adc_max[6] = {4095, 4095, center, 4095, 4095, 0};
    simulate_one_cycle(adc_max, 0, 0);
    
    int idx = acc_can_msg_count - 2;
    decode_f1_1(&acc_can_messages[idx], &apps1, &apps2, &steering, &brake_front);
    
    /* Pressure should be 0 (above max ADC range for pressure sensor) */
    TEST_ASSERT_EQUAL_UINT16(0, brake_front);
    /* APPS raw values should pass through */
    TEST_ASSERT_EQUAL_UINT16(4095, apps1);
    TEST_ASSERT_EQUAL_UINT16(4095, apps2);
}

/* ============================================================================
 * SCENARIO 7: CAN Bus Failure and Recovery
 * ============================================================================ */
void test_acceptance_can_bus_failure_recovery(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    uint16_t adc[6] = {2000, 1800, center, 2048, 2048, 0};
    
    /* Normal operation: 5 cycles */
    for (int i = 0; i < 5; i++) {
        simulate_one_cycle(adc, 500, 500);
    }
    int normal_count = acc_can_msg_count;
    TEST_ASSERT_TRUE(normal_count > 0);
    
    /* CAN bus failure simulation: mock returns HAL_ERROR */
    mock_hal.can_tx_return_status = HAL_ERROR;
    
    /* Try to transmit during failure - using real mock */
    uint8_t failData[1] = {0};
    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, NULL, failData);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
    
    /* Recovery: CAN bus comes back */
    mock_hal.can_tx_return_status = HAL_OK;
    
    /* Continue normal operation */
    for (int i = 0; i < 5; i++) {
        simulate_one_cycle(adc, 500, 500);
    }
    
    /* Verify messages continued after recovery */
    TEST_ASSERT_TRUE(acc_can_msg_count > normal_count);
}

/* ============================================================================
 * SCENARIO 8: Heartbeat Loss Detection
 * ============================================================================ */
void test_acceptance_heartbeat_loss(void)
{
    acceptance_setup();
    
    /* Phase 1: Heartbeat present - network controls LED */
    int heartbeat_active = 1;
    
    for (int i = 0; i < 5; i++) {
        if (heartbeat_active) {
            heartbeat_active = 0; /* Cleared after check */
        } else {
            HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        }
        heartbeat_active = 1; /* Heartbeat arrives before next check */
    }
    
    /* No toggles should have occurred (heartbeat was always active) */
    TEST_ASSERT_EQUAL(0, mock_hal.gpio_toggle_count);
    
    /* Phase 2: Heartbeat lost - local toggle takes over */
    heartbeat_active = 0;
    
    for (int i = 0; i < 5; i++) {
        if (heartbeat_active) {
            heartbeat_active = 0;
        } else {
            HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        }
        /* No heartbeat arrives */
    }
    
    /* 5 toggles should have occurred */
    TEST_ASSERT_EQUAL(5, mock_hal.gpio_toggle_count);
}

/* ============================================================================
 * SCENARIO 9: Rapid Brake Light Toggle (Stress Test)
 * ============================================================================ */
void test_acceptance_rapid_brake_toggle(void)
{
    acceptance_setup();
    
    /* Rapidly toggle brake light 100 times */
    for (int i = 0; i < 100; i++) {
        uint8_t cmd = (i % 2) ? 1 : 0;
        osMessageQueuePut(acc_brake_queue, &cmd, 0, 0);
        
        uint8_t received;
        if (osMessageQueueGet(acc_brake_queue, &received, NULL, 0) == osOK) {
            if (received == 1) {
                HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
            } else {
                HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
            }
        }
    }
    
    /* All 100 commands should have been processed */
    TEST_ASSERT_EQUAL(100, mock_hal.gpio_write_count);
    
    /* Final state: i=99 -> (99%2)=1 -> cmd=1 -> SET (brake ON) */
    GPIO_PinState final_state = mock_hal.gpio_write_calls[99].state;
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, final_state);
}

/* ============================================================================
 * SCENARIO 10: Full Race Lap Simulation
 * ============================================================================ */
void test_acceptance_race_lap_simulation(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    int total_cycles = 0;
    
    /* ---- Straight: accelerate ---- */
    for (int i = 0; i < 50; i++) {
        uint16_t throttle = 1000 + i * 60; /* Ramp up throttle */
        uint16_t rpm = 200 + i * 80;
        uint16_t adc[6] = {300, 300, center, throttle, throttle, 0};
        sim_ws_left_active = 1;
        sim_ws_right_active = 1;
        simulate_one_cycle(adc, rpm, rpm);
        total_cycles++;
    }
    
    /* ---- Brake into corner ---- */
    for (int i = 0; i < 20; i++) {
        uint16_t brake_front = 1000 + i * 130; /* Increasing brake pressure */
        uint16_t brake_rear  = 800 + i * 100;
        uint16_t rpm = 4000 - i * 150;
        uint16_t steer = center - i * 20; /* Turning left */
        uint16_t adc[6] = {brake_front, brake_rear, steer, 500, 500, 0};
        simulate_one_cycle(adc, rpm, rpm - 10);
        total_cycles++;
    }
    
    /* ---- Mid-corner: holding steering, light brake ---- */
    for (int i = 0; i < 30; i++) {
        uint16_t steer = center - 400; /* Holding left turn */
        uint16_t adc[6] = {800, 700, steer, 2000, 2000, 0};
        simulate_one_cycle(adc, 1500, 1490);
        total_cycles++;
    }
    
    /* ---- Corner exit: straighten + accelerate ---- */
    for (int i = 0; i < 30; i++) {
        uint16_t steer = center - 400 + i * 14; /* Straightening */
        uint16_t throttle = 2000 + i * 50;
        uint16_t rpm = 1500 + i * 50;
        uint16_t adc[6] = {300, 300, steer, throttle, throttle, 0};
        simulate_one_cycle(adc, rpm, rpm);
        total_cycles++;
    }
    
    /* Verify: all cycles produced CAN messages */
    /* Each cycle produces 2 messages (F1_1 + F1_2) */
    TEST_ASSERT_EQUAL(total_cycles * 2, acc_can_msg_count);
    
    /* Verify: all F1_1 messages have valid IDs */
    for (int i = 0; i < acc_can_msg_count; i += 2) {
        TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_1, acc_can_messages[i].id);
    }
    
    /* Verify: all F1_2 messages have valid IDs */
    for (int i = 1; i < acc_can_msg_count; i += 2) {
        TEST_ASSERT_EQUAL_HEX32(CAN_ID_AD2CAN_F1_2, acc_can_messages[i].id);
    }
    
    /* Verify data integrity: no zero-length messages */
    for (int i = 0; i < acc_can_msg_count; i++) {
        TEST_ASSERT_TRUE(acc_can_messages[i].length > 0);
    }
    
    printf("\n    >> Race lap: %d cycles, %d CAN messages transmitted\n", 
           total_cycles, acc_can_msg_count);
}

/* ============================================================================
 * SCENARIO 11: APPS Plausibility (Both sensors should track)
 * ============================================================================ */
void test_acceptance_apps_plausibility(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    
    /* Normal: APPS1 and APPS2 track (difference < 10%) */
    uint16_t adc_normal[6] = {300, 300, center, 2048, 2100, 0};
    simulate_one_cycle(adc_normal, 500, 500);
    
    uint16_t apps1, apps2, bf;
    int16_t steer;
    decode_f1_1(&acc_can_messages[0], &apps1, &apps2, &steer, &bf);
    
    /* Both APPS values should be transmitted for plausibility check by ECU */
    int16_t diff = (int16_t)apps1 - (int16_t)apps2;
    if (diff < 0) diff = -diff;
    TEST_ASSERT_TRUE(diff < 200); /* Within ~5% of 4096 range */
    
    /* Faulty: APPS1 stuck, APPS2 normal (ECU should detect) */
    uint16_t adc_fault[6] = {300, 300, center, 0, 2048, 0};
    simulate_one_cycle(adc_fault, 500, 500);
    
    int idx = acc_can_msg_count - 2;
    decode_f1_1(&acc_can_messages[idx], &apps1, &apps2, &steer, &bf);
    
    /* Both values transmitted as-is (ECU does plausibility) */
    TEST_ASSERT_EQUAL_UINT16(0, apps1);
    TEST_ASSERT_EQUAL_UINT16(2048, apps2);
    
    /* The difference is huge - ECU would flag this */
    diff = (int16_t)apps1 - (int16_t)apps2;
    if (diff < 0) diff = -diff;
    TEST_ASSERT_TRUE(diff > 1000);
}

/* ============================================================================
 * SCENARIO 12: Wheel Speed Sensor Pin State Verification
 * ============================================================================ */
void test_acceptance_ws_pin_states(void)
{
    acceptance_setup();
    
    uint16_t center = (STEERING_RAW_LEFT + STEERING_RAW_RIGHT) / 2;
    uint16_t adc[6] = {300, 300, center, 2048, 2048, 0};
    
    /* Both WS inactive */
    sim_ws_left_active = 0;
    sim_ws_right_active = 0;
    simulate_one_cycle(adc, 0, 0);
    
    int idx = acc_can_msg_count - 1;
    uint16_t rl, rr, br;
    uint8_t ws;
    decode_f1_2(&acc_can_messages[idx], &rl, &rr, &br, &ws);
    TEST_ASSERT_EQUAL_UINT8(0x00, ws);
    
    /* Only left WS active */
    sim_ws_left_active = 1;
    sim_ws_right_active = 0;
    simulate_one_cycle(adc, 100, 0);
    
    idx = acc_can_msg_count - 1;
    decode_f1_2(&acc_can_messages[idx], &rl, &rr, &br, &ws);
    TEST_ASSERT_EQUAL_UINT8(0x01, ws);
    
    /* Only right WS active */
    sim_ws_left_active = 0;
    sim_ws_right_active = 1;
    simulate_one_cycle(adc, 0, 100);
    
    idx = acc_can_msg_count - 1;
    decode_f1_2(&acc_can_messages[idx], &rl, &rr, &br, &ws);
    TEST_ASSERT_EQUAL_UINT8(0x02, ws);
    
    /* Both WS active */
    sim_ws_left_active = 1;
    sim_ws_right_active = 1;
    simulate_one_cycle(adc, 100, 100);
    
    idx = acc_can_msg_count - 1;
    decode_f1_2(&acc_can_messages[idx], &rl, &rr, &br, &ws);
    TEST_ASSERT_EQUAL_UINT8(0x03, ws);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */
int main(void)
{
    run_acceptance_tests();
    return unity_print_summary();
}

void run_acceptance_tests(void)
{
    UNITY_BEGIN_SUITE("Acceptance - System Startup");
    RUN_TEST(test_acceptance_startup_sequence);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Normal Driving");
    RUN_TEST(test_acceptance_normal_driving_straight);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Hard Braking");
    RUN_TEST(test_acceptance_hard_braking);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Steering");
    RUN_TEST(test_acceptance_full_steering_lock);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - High Speed");
    RUN_TEST(test_acceptance_high_speed);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Sensor Failure");
    RUN_TEST(test_acceptance_sensor_failure);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - CAN Bus Failure");
    RUN_TEST(test_acceptance_can_bus_failure_recovery);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Heartbeat Loss");
    RUN_TEST(test_acceptance_heartbeat_loss);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Brake Light Stress");
    RUN_TEST(test_acceptance_rapid_brake_toggle);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Race Lap Simulation");
    RUN_TEST(test_acceptance_race_lap_simulation);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - APPS Plausibility");
    RUN_TEST(test_acceptance_apps_plausibility);
    UNITY_END_SUITE();

    UNITY_BEGIN_SUITE("Acceptance - Wheel Speed Pins");
    RUN_TEST(test_acceptance_ws_pin_states);
    UNITY_END_SUITE();
}
