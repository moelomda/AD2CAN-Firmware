#include "can_tx_task.h"
#include "can_hal.h"
#include "tim_hal.h"
#include "sensors.h"
#include "sensor_task.h"
#include "app_config.h"
#include "main.h"
#include <string.h>

void CanTxTask_Init(void)
{
    const osThreadAttr_t task_attributes = {
        .name = "CanTxTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 512 * 4
    };
    if(osThreadNew(CanTxTask_Run, NULL, &task_attributes) == NULL) { 
        Error_Handler(); 
    }
}

void CanTxTask_Run(void *argument)
{
    ProcessedSensorData_t local_data;
    uint16_t rpmLeft, rpmRight;
    uint8_t txData[8];
    uint8_t dv_counter = 0;
    
    txData[0] = 0xAA;
    CAN_HAL_Transmit(CAN_ID_AD2CAN_STARTUP, txData, 1);

    extern volatile uint32_t dbg_cantx_count;

    for(;;)
    {
        osDelay(CAN_TX_PERIOD_MS);
        dbg_cantx_count++;

        if (osMutexAcquire(SensorTask_GetMutexHandle(), 10) == osOK)
        {
            memcpy(&local_data, SensorTask_GetDataPointer(), sizeof(ProcessedSensorData_t));
            osMutexRelease(SensorTask_GetMutexHandle());
        } 
        else 
        {
            HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
            continue;
        }

        rpmLeft  = Sensor_FilterRPM_Left(TIM_HAL_GetRPM_Left());
        rpmRight = Sensor_FilterRPM_Right(TIM_HAL_GetRPM_Right());

        txData[0] = local_data.apps1 & 0xFF;
        txData[1] = local_data.apps1 >> 8;
        txData[2] = local_data.apps2 & 0xFF;
        txData[3] = local_data.apps2 >> 8;
        txData[4] = local_data.steering_angle & 0xFF;
        txData[5] = local_data.steering_angle >> 8;
        txData[6] = local_data.brake_pressure_front & 0xFF;
        txData[7] = local_data.brake_pressure_front >> 8;
        CAN_HAL_Transmit(CAN_ID_AD2CAN_F1_1, txData, 8);

        txData[0] = rpmLeft & 0xFF;
        txData[1] = rpmLeft >> 8;
        txData[2] = rpmRight & 0xFF;
        txData[3] = rpmRight >> 8;
        txData[4] = local_data.brake_pressure_rear & 0xFF;
        txData[5] = local_data.brake_pressure_rear >> 8;
        /* Read pin state directly from IDR register because PA2/PB0 are in
         * alternate function mode (timer input capture). HAL_GPIO_ReadPin is
         * unreliable on AF pins, but the IDR still reflects the physical level. */
        txData[6] = ((WS_LEFT_GPIO_Port->IDR & WS_LEFT_Pin) ? 1 : 0)
                   | (((WS_RIGHT_GPIO_Port->IDR & WS_RIGHT_Pin) ? 1 : 0) << 1);
        CAN_HAL_Transmit(CAN_ID_AD2CAN_F1_2, txData, 7);

        if(dv_counter == 0)
        {
            txData[0] = local_data.brake_pressure_rear & 0xFF;
            txData[1] = local_data.brake_pressure_rear >> 8;
            txData[2] = local_data.brake_pressure_front & 0xFF;
            txData[3] = local_data.brake_pressure_front >> 8;
            CAN_HAL_Transmit(CAN_ID_DV_SENSORS_FRONT, txData, 4);
        }
        dv_counter++;
        dv_counter %= 10;
    }
}
