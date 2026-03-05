/* Redirect: maps #include "main.h" to mock HAL for testing */
#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H

#include "mock_stm32_hal.h"

/* GPIO port externs - simulated hardware */
extern GPIO_TypeDef mock_gpioa;
extern GPIO_TypeDef mock_gpiob;
extern GPIO_TypeDef mock_gpioc;

/* Map port defines used in application code to mock ports */
#define GPIOA (&mock_gpioa)
#define GPIOB (&mock_gpiob)
#define GPIOC (&mock_gpioc)

/* Pin assignments from main.h */
#define BL_ENABLE_Pin       GPIO_PIN_8
#define BL_ENABLE_GPIO_Port GPIOA
#define LED_GREEN_Pin       GPIO_PIN_6
#define LED_GREEN_GPIO_Port GPIOB
#define LED_RED_Pin         GPIO_PIN_7
#define LED_RED_GPIO_Port   GPIOB
#define WS_LEFT_Pin         GPIO_PIN_2
#define WS_LEFT_GPIO_Port   GPIOA
#define WS_RIGHT_Pin        GPIO_PIN_0
#define WS_RIGHT_GPIO_Port  GPIOB

/* Peripheral handle externs - defined in mock_stm32_hal.c */
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern FDCAN_HandleTypeDef hfdcan1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim15;

/* ADC1 Instance identifier for comparison */
#define ADC1 (hadc1.Instance)

/* TIM Instance identifiers for comparison */
#define TIM3  (htim3.Instance)
#define TIM15 (htim15.Instance)

/* Debug counters */
extern volatile uint32_t dbg_init_stage;
extern volatile uint32_t dbg_free_heap;
extern volatile uint32_t dbg_sensor_count;
extern volatile uint32_t dbg_canrx_count;
extern volatile uint32_t dbg_cantx_count;
extern volatile uint32_t dbg_adc_callback_count;
extern volatile uint32_t dbg_adc_queue_fail;

#endif /* MOCK_MAIN_H */
