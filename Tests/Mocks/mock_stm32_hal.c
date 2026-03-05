/* ============================================================================
 * Mock STM32 HAL - Implementation
 * ============================================================================ */
#include "mock_stm32_hal.h"
#include <string.h>

/* ---- Global mock state ---- */
MockHAL_State_t mock_hal;

/* ---- Mock GPIO Ports (simulated hardware registers) ---- */
GPIO_TypeDef mock_gpioa = {0};
GPIO_TypeDef mock_gpiob = {0};
GPIO_TypeDef mock_gpioc = {0};

/* ---- Mock ADC/TIM/FDCAN instances ---- */
static ADC_TypeDef  mock_adc1_instance  = { .Instance_id = 1 };
static TIM_TypeDef  mock_tim3_instance  = { .Instance_id = 3 };
static TIM_TypeDef  mock_tim15_instance = { .Instance_id = 15 };
static FDCAN_TypeDef mock_fdcan1_instance = {0};

/* ---- Global peripheral handles (extern'd by application code) ---- */
ADC_HandleTypeDef hadc1 = { .Instance = &mock_adc1_instance };
DMA_HandleTypeDef hdma_adc1 = {0};
FDCAN_HandleTypeDef hfdcan1 = { .Instance = &mock_fdcan1_instance };
TIM_HandleTypeDef htim3 = { .Instance = &mock_tim3_instance };
TIM_HandleTypeDef htim15 = { .Instance = &mock_tim15_instance };

/* ---- Debug trace variables (extern'd by application code) ---- */
volatile uint32_t dbg_init_stage = 0;
volatile uint32_t dbg_free_heap = 0;
volatile uint32_t dbg_sensor_count = 0;
volatile uint32_t dbg_canrx_count = 0;
volatile uint32_t dbg_cantx_count = 0;
volatile uint32_t dbg_adc_callback_count = 0;
volatile uint32_t dbg_adc_queue_fail = 0;

/* ============================================================================
 * GPIO Mock Implementation
 * ============================================================================ */
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    if (mock_hal.gpio_write_count < MOCK_MAX_GPIO_CALLS) {
        mock_hal.gpio_write_calls[mock_hal.gpio_write_count].port = GPIOx;
        mock_hal.gpio_write_calls[mock_hal.gpio_write_count].pin = GPIO_Pin;
        mock_hal.gpio_write_calls[mock_hal.gpio_write_count].state = PinState;
        mock_hal.gpio_write_count++;
    }
    
    /* Update simulated ODR register */
    if (PinState == GPIO_PIN_SET) {
        GPIOx->ODR |= GPIO_Pin;
    } else {
        GPIOx->ODR &= ~((uint32_t)GPIO_Pin);
    }
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if (mock_hal.gpio_toggle_count < MOCK_MAX_GPIO_CALLS) {
        mock_hal.gpio_toggle_calls[mock_hal.gpio_toggle_count].port = GPIOx;
        mock_hal.gpio_toggle_calls[mock_hal.gpio_toggle_count].pin = GPIO_Pin;
        mock_hal.gpio_toggle_count++;
    }
    
    GPIOx->ODR ^= GPIO_Pin;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    return (GPIOx->IDR & GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/* ============================================================================
 * ADC Mock Implementation
 * ============================================================================ */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *hadc, uint32_t SingleDiff)
{
    mock_hal.adc_calibration_count++;
    return mock_hal.adc_calibration_return;
}

HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *hadc, uint32_t *pData, uint32_t Length)
{
    mock_hal.adc_start_dma_count++;
    return mock_hal.adc_start_dma_return;
}

HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef *hadc)
{
    mock_hal.adc_stop_dma_count++;
    return HAL_OK;
}

/* ============================================================================
 * TIM Mock Implementation
 * ============================================================================ */
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    mock_hal.tim_ic_start_count++;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim)
{
    mock_hal.tim_base_start_count++;
    return HAL_OK;
}

uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    return mock_hal.tim_capture_value;
}

/* ============================================================================
 * FDCAN Mock Implementation
 * ============================================================================ */
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan)
{
    mock_hal.fdcan_start_count++;
    return mock_hal.fdcan_start_return;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                  FDCAN_TxHeaderTypeDef *pTxHeader,
                                                  uint8_t *pTxData)
{
    if (mock_hal.can_tx_count < MOCK_MAX_CAN_MESSAGES) {
        MockCanMessage_t *msg = &mock_hal.can_tx_messages[mock_hal.can_tx_count];
        msg->id = pTxHeader->Identifier;
        
        /* Decode DLC to byte count */
        uint32_t dlc = pTxHeader->DataLength >> 16;
        msg->length = (dlc <= 8) ? dlc : 8;
        
        memcpy(msg->data, pTxData, msg->length);
        mock_hal.can_tx_count++;
    }
    return mock_hal.can_tx_return_status;
}

HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                          FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData)
{
    if (mock_hal.can_rx_return_status == HAL_OK) {
        memcpy(pRxHeader, &mock_hal.can_rx_inject_header, sizeof(FDCAN_RxHeaderTypeDef));
        memcpy(pRxData, mock_hal.can_rx_inject_data, 8);
    }
    return mock_hal.can_rx_return_status;
}

/* ============================================================================
 * System Mock Implementation
 * ============================================================================ */
HAL_StatusTypeDef HAL_Init(void)
{
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return mock_hal.hal_tick;
}

void Error_Handler(void)
{
    mock_hal.error_handler_count++;
}

/* ---- FreeRTOS heap stub ---- */
uint32_t xPortGetFreeHeapSize(void)
{
    return 8192; /* Simulated free heap */
}
