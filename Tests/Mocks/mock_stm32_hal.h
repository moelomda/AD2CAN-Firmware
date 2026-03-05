/* ============================================================================
 * Mock STM32 HAL - Simulates STM32G4xx HAL for PC-based testing
 * ============================================================================
 * This mock provides all the types, defines, and function stubs needed to
 * compile and test the application code on a PC without real hardware.
 * ============================================================================ */
#ifndef MOCK_STM32_HAL_H
#define MOCK_STM32_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ---- HAL Status ---- */
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/* ---- GPIO Definitions ---- */
typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET   = 1U
} GPIO_PinState;

typedef struct {
    uint32_t IDR;   /* Input Data Register */
    uint32_t ODR;   /* Output Data Register */
    uint32_t BSRR;  /* Bit Set/Reset Register */
} GPIO_TypeDef;

#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_2   ((uint16_t)0x0004)
#define GPIO_PIN_3   ((uint16_t)0x0008)
#define GPIO_PIN_4   ((uint16_t)0x0010)
#define GPIO_PIN_5   ((uint16_t)0x0020)
#define GPIO_PIN_6   ((uint16_t)0x0040)
#define GPIO_PIN_7   ((uint16_t)0x0080)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_11  ((uint16_t)0x0800)
#define GPIO_PIN_12  ((uint16_t)0x1000)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)
#define GPIO_PIN_15  ((uint16_t)0x8000)

/* ---- ADC Definitions ---- */
typedef struct {
    uint32_t Instance_id;
} ADC_TypeDef;

typedef enum {
    HAL_TIM_ACTIVE_CHANNEL_1 = 0x01U,
    HAL_TIM_ACTIVE_CHANNEL_2 = 0x02U,
    HAL_TIM_ACTIVE_CHANNEL_3 = 0x04U,
    HAL_TIM_ACTIVE_CHANNEL_4 = 0x08U,
    HAL_TIM_ACTIVE_CHANNEL_CLEARED = 0x00U
} HAL_TIM_ActiveChannel;

typedef struct {
    ADC_TypeDef *Instance;
    HAL_TIM_ActiveChannel Channel; /* reuse for tracking active channel if needed */
} ADC_HandleTypeDef;

#define ADC_SINGLE_ENDED 0x00000000U
#define ADC1_INSTANCE_ID 1

/* ---- DMA Definitions ---- */
typedef struct {
    uint32_t dummy;
} DMA_HandleTypeDef;

/* ---- TIM Definitions ---- */
typedef struct {
    uint32_t Instance_id;
} TIM_TypeDef;

typedef struct {
    TIM_TypeDef *Instance;
    HAL_TIM_ActiveChannel Channel;
} TIM_HandleTypeDef;

#define TIM_CHANNEL_1 0x00000000U
#define TIM_CHANNEL_2 0x00000004U
#define TIM_CHANNEL_3 0x00000008U
#define TIM_CHANNEL_4 0x0000000CU

/* ---- FDCAN Definitions ---- */
#define FDCAN_STANDARD_ID           0x00000000U
#define FDCAN_DATA_FRAME            0x00000000U
#define FDCAN_ESI_ACTIVE            0x00000000U
#define FDCAN_BRS_OFF               0x00000000U
#define FDCAN_CLASSIC_CAN           0x00000000U
#define FDCAN_NO_TX_EVENTS          0x00000000U
#define FDCAN_DLC_BYTES_0           0x00000000U
#define FDCAN_DLC_BYTES_1           0x00010000U
#define FDCAN_DLC_BYTES_2           0x00020000U
#define FDCAN_DLC_BYTES_3           0x00030000U
#define FDCAN_DLC_BYTES_4           0x00040000U
#define FDCAN_DLC_BYTES_5           0x00050000U
#define FDCAN_DLC_BYTES_6           0x00060000U
#define FDCAN_DLC_BYTES_7           0x00070000U
#define FDCAN_DLC_BYTES_8           0x00080000U
#define FDCAN_RX_FIFO0              0x00000040U
#define FDCAN_IT_RX_FIFO0_NEW_MESSAGE  0x00000001U
#define RESET                       0U

typedef struct {
    uint32_t dummy;
} FDCAN_TypeDef;

typedef struct {
    FDCAN_TypeDef *Instance;
} FDCAN_HandleTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t RxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t FilterIndex;
    uint32_t IsFilterMatchingFrame;
} FDCAN_RxHeaderTypeDef;

/* ============================================================================
 * Mock Tracking Structures - Record all HAL calls for verification
 * ============================================================================ */
#define MOCK_MAX_CAN_MESSAGES  64
#define MOCK_MAX_GPIO_CALLS    128

typedef struct {
    uint32_t id;
    uint8_t  data[8];
    uint32_t length;
} MockCanMessage_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState state;
} MockGpioCall_t;

/* Global mock state */
typedef struct {
    /* CAN TX tracking */
    MockCanMessage_t can_tx_messages[MOCK_MAX_CAN_MESSAGES];
    int can_tx_count;
    HAL_StatusTypeDef can_tx_return_status;
    
    /* CAN RX injection */
    FDCAN_RxHeaderTypeDef can_rx_inject_header;
    uint8_t can_rx_inject_data[8];
    HAL_StatusTypeDef can_rx_return_status;
    
    /* GPIO tracking */
    MockGpioCall_t gpio_write_calls[MOCK_MAX_GPIO_CALLS];
    int gpio_write_count;
    MockGpioCall_t gpio_toggle_calls[MOCK_MAX_GPIO_CALLS];
    int gpio_toggle_count;
    
    /* ADC tracking */
    uint16_t adc_dma_buffer[6];
    int adc_start_dma_count;
    int adc_stop_dma_count;
    int adc_calibration_count;
    HAL_StatusTypeDef adc_calibration_return;
    HAL_StatusTypeDef adc_start_dma_return;
    
    /* TIM tracking */
    int tim_ic_start_count;
    int tim_base_start_count;
    uint32_t tim_capture_value;
    
    /* HAL tick for timeout simulation */
    uint32_t hal_tick;
    
    /* FDCAN Start tracking */
    int fdcan_start_count;
    HAL_StatusTypeDef fdcan_start_return;
    
    /* Error handler tracking */
    int error_handler_count;
    
} MockHAL_State_t;

extern MockHAL_State_t mock_hal;

/* ---- Mock initialization ---- */
static inline void MockHAL_Reset(void) {
    memset(&mock_hal, 0, sizeof(MockHAL_State_t));
    mock_hal.can_tx_return_status = HAL_OK;
    mock_hal.can_rx_return_status = HAL_OK;
    mock_hal.adc_calibration_return = HAL_OK;
    mock_hal.adc_start_dma_return = HAL_OK;
    mock_hal.fdcan_start_return = HAL_OK;
}

/* ============================================================================
 * Mock HAL Function Declarations
 * ============================================================================ */

/* GPIO */
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ADC */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *hadc, uint32_t SingleDiff);
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *hadc, uint32_t *pData, uint32_t Length);
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef *hadc);

/* TIM */
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim);
uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *htim, uint32_t Channel);

/* FDCAN */
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                  FDCAN_TxHeaderTypeDef *pTxHeader,
                                                  uint8_t *pTxData);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                          FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData);

/* System */
HAL_StatusTypeDef HAL_Init(void);
uint32_t HAL_GetTick(void);
void Error_Handler(void);

/* ITM stub */
static inline uint32_t ITM_SendChar(uint32_t ch) { return ch; }

#endif /* MOCK_STM32_HAL_H */
