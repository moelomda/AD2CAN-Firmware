/**
 * @file app_init.c
 * @brief Application initialization implementation
 */

#include "app_init.h"
#include "adc_hal.h"
#include "can_hal.h"
#include "tim_hal.h"
#include "sensor_task.h"
#include "can_tx_task.h"
#include "can_rx_task.h"
#include "brake_light_task.h"
#include "main.h"
#include "debug_printf.h"

extern FDCAN_HandleTypeDef hfdcan1;
extern volatile uint32_t dbg_init_stage;
extern volatile uint32_t dbg_free_heap;

void App_Init(void)
{
    dbg_init_stage = 1;  /* Entering App_Init */

    ADC_HAL_Init();
    dbg_init_stage = 2;  /* ADC HAL done */

    CAN_HAL_Init();
    dbg_init_stage = 3;  /* CAN HAL done */

    TIM_HAL_Init();
    dbg_init_stage = 4;  /* TIM HAL done */

    SensorTask_Init();
    dbg_init_stage = 5;  /* SensorTask done */

    CanTxTask_Init();
    dbg_init_stage = 6;  /* CanTxTask done */

    CanRxTask_Init();
    dbg_init_stage = 7;  /* CanRxTask done */

    BrakeLightTask_Init();
    dbg_init_stage = 8;  /* BrakeLightTask done */

    dbg_free_heap = (uint32_t)xPortGetFreeHeapSize();
    dbg_init_stage = 9;  /* All init complete */
}

void App_Start(void)
{
    DBG_PRINT("[START] App_Start...\r\n");

    /* Start FDCAN peripheral (must be after filter config in MX_FDCAN1_Init) */
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        DBG_PRINT("[ERROR] FDCAN Start failed!\r\n");
        Error_Handler();
    }
    DBG_PRINT("[START] FDCAN started OK\r\n");

    /* Start ADC with DMA */
    ADC_HAL_Start();
    DBG_PRINT("[START] ADC started OK\r\n");
}
