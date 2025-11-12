#include "ad2can_can.h"
#include "main.h"
#include "ad2can.h"
#include "ad2can_hw.h"
#include <string.h>

/* --- File-Scope Variables --- */
extern FDCAN_HandleTypeDef hfdcan1;
osMessageQueueId_t g_brakeCommandQueueHandle;
FDCAN_TxHeaderTypeDef g_canTxHeader;
uint8_t g_canTxData[8];
FDCAN_RxHeaderTypeDef g_canRxHeader;
uint8_t g_canRxData[8];

/* --- RTOS Object Initialization --- */

void CAN_Init_RTOS(void)
{

    const osMessageQueueAttr_t brakeCommandQueue_attributes = {
        .name = "BrakeCmdQueue"
    };
    g_brakeCommandQueueHandle = osMessageQueueNew(1, sizeof(uint8_t), &brakeCommandQueue_attributes);
    if (g_brakeCommandQueueHandle == NULL) { Error_Handler(); }


    const osThreadAttr_t canTxTask_attributes = {
        .name = "CanTxTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 256 * 4 // زيادة الـ Stack قليلاً
    };
    if(osThreadNew(CanTxTask, NULL, &canTxTask_attributes) == NULL) { Error_Handler(); }

    const osThreadAttr_t canRxTask_attributes = {
        .name = "CanRxTask",
        .priority = (osPriority_t) osPriorityBelowNormal,
        .stack_size = 128 * 4
    };
    if(osThreadNew(CanRxTask, NULL, &canRxTask_attributes) == NULL) { Error_Handler(); }

    const osThreadAttr_t brakeLightTask_attributes = {
        .name = "BrakeLightTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 128 * 4
    };
    if(osThreadNew(BrakeLightTask, NULL, &brakeLightTask_attributes) == NULL) { Error_Handler(); }
}

/* --- Task Definitions --- */

/**
 * @brief
 */
void CanTxTask(void *argument)
{
  ProcessedADC_t local_sensor_data;
  uint16_t rpmLeft, rpmRight;

  g_canTxHeader.IdType = FDCAN_STANDARD_ID;
  g_canTxHeader.TxFrameType = FDCAN_DATA_FRAME;
  g_canTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  g_canTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  g_canTxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  g_canTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  g_canTxHeader.MessageMarker = 0;

  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
  { Error_Handler(); }

  g_canTxHeader.Identifier = CAN_ID_AD2CAN_STARTUP;
  g_canTxHeader.DataLength = FDCAN_DLC_BYTES_1;
  g_canTxData[0] = 0xAA;
  HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData);
  uint8_t dv_counter = 0;

  for(;;)
  {


    osDelay(10);
    if (osMutexAcquire(g_sensorDataMutexHandle, 10) == osOK)
    {
        memcpy(&local_sensor_data, &g_sensorData, sizeof(ProcessedADC_t));
        osMutexRelease(g_sensorDataMutexHandle);
    } else {
        HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
        continue;
    }


    rpmLeft  = HW_MovingMaxL(HW_Get_RPM_Left());
    rpmRight = HW_MovingMaxR(HW_Get_RPM_Right());


    g_canTxHeader.Identifier = CAN_ID_AD2CAN_F1_1;
    g_canTxHeader.DataLength = FDCAN_DLC_BYTES_8;
    g_canTxData[0] = local_sensor_data.apps1 & 0xFF;
    g_canTxData[1] = local_sensor_data.apps1 >> 8;
    g_canTxData[2] = local_sensor_data.apps2 & 0xFF;
    g_canTxData[3] = local_sensor_data.apps2 >> 8;
    g_canTxData[4] = local_sensor_data.steering_angle & 0xFF;
    g_canTxData[5] = local_sensor_data.steering_angle >> 8;
    g_canTxData[6] = local_sensor_data.brake_pressure_front & 0xFF;
    g_canTxData[7] = local_sensor_data.brake_pressure_front >> 8;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData);

    g_canTxHeader.Identifier = CAN_ID_AD2CAN_F1_2;
    g_canTxHeader.DataLength = FDCAN_DLC_BYTES_7;
    g_canTxData[0] = rpmLeft & 0xFF;
    g_canTxData[1] = rpmLeft >> 8;
    g_canTxData[2] = rpmRight & 0xFF;
    g_canTxData[3] = rpmRight >> 8;
    g_canTxData[4] = local_sensor_data.brake_pressure_rear & 0xFF;
    g_canTxData[5] = local_sensor_data.brake_pressure_rear >> 8;
    g_canTxData[6] = 0; // (كودهم كان يقرأ GPIOs، سنضع 0 مؤقتاً)
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData);

    // 5. إرسال الرسالة الثالثة (كل 10 دورات)
    if(dv_counter == 0){
        g_canTxHeader.Identifier = CAN_ID_DV_SENSORS_FRONT;
        g_canTxHeader.DataLength = FDCAN_DLC_BYTES_4;
        g_canTxData[0] = local_sensor_data.brake_pressure_rear & 0xFF;
        g_canTxData[1] = local_sensor_data.brake_pressure_rear >> 8;
        g_canTxData[2] = local_sensor_data.brake_pressure_front & 0xFF;
        g_canTxData[3] = local_sensor_data.brake_pressure_front >> 8;
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData);
    }
    dv_counter++;
    dv_counter %= 10;
  }
}

void CanRxTask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
  }
}

void BrakeLightTask(void *argument)
{
  uint8_t received_command;
  for(;;)
  {
    if (osMessageQueueGet(g_brakeCommandQueueHandle, &received_command, NULL, osWaitForever) == osOK)
    {
      if (received_command == 1)
      {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
      }
      else
      {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
      }
    }
  }
}


void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &g_canRxHeader, g_canRxData) == HAL_OK)
    {

      if (g_canRxHeader.Identifier == CAN_ID_RX_BRAKE)
      {
        if (g_canRxHeader.DataLength >= FDCAN_DLC_BYTES_1)
        {
            uint8_t brake_command = g_canRxData[0];
            osMessageQueuePut(g_brakeCommandQueueHandle, &brake_command, 0, 0);
        }
      }

    }
  }
}
