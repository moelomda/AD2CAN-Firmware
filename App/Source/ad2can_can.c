#include "ad2can_can.h"
#include "ad2can.h" // To access the shared temperature data
#include <string.h>

/* --- File-Scope Variables --- */
// HAL Handles (defined in main.c, accessed via extern)
extern FDCAN_HandleTypeDef hfdcan1;

// RTOS Objects
osMessageQueueId_t g_brakeCommandQueueHandle; //Queue for brake commands

// Buffers
FDCAN_TxHeaderTypeDef g_canTxHeader;
uint8_t g_canTxData[8];
FDCAN_RxHeaderTypeDef g_canRxHeader;
uint8_t g_canRxData[8];

/* --- RTOS Object Initialization --- */

void CAN_Init_RTOS(void)
{
    // Create Brake Command Queue
    const osMessageQueueAttr_t brakeCommandQueue_attributes = {
        .name = "BrakeCmdQueue"
    };
    g_brakeCommandQueueHandle = osMessageQueueNew(1, sizeof(uint8_t), &brakeCommandQueue_attributes);
    if (g_brakeCommandQueueHandle == NULL) { Error_Handler(); }

    // Create CAN Tasks
    const osThreadAttr_t canTxTask_attributes = {
        .name = "CanTxTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 192 * 4
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

void CanTxTask(void *argument)
{
  // --- Configure CAN Transmission Header (once) ---
  g_canTxHeader.Identifier = CAN_ID_TX_TEMPS;
  g_canTxHeader.IdType = FDCAN_STANDARD_ID;
  g_canTxHeader.TxFrameType = FDCAN_DATA_FRAME;
  g_canTxHeader.DataLength = FDCAN_DLC_BYTES_8;
  g_canTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  g_canTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  g_canTxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  g_canTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  g_canTxHeader.MessageMarker = 0;

  // --- Start the FDCAN Peripheral ---
  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
  { Error_Handler(); }

  float local_temps[8]; // Local buffer

  for(;;)
  {
    osDelay(CAN_TX_PERIOD_MS);

    if (osMutexAcquire(g_temperatureMutexHandle, 10) == osOK)
    {
        memcpy(local_temps, g_latestTemperatures, sizeof(local_temps));
        osMutexRelease(g_temperatureMutexHandle);

        // --- Prepare CAN Tx Data (Example Format) ---
        for(int i = 0; i < 4; i++) // Send first 4 sensors
        {
            int16_t temp_to_send = (int16_t)(local_temps[i] * 10.0f);
            g_canTxData[i*2]   = (uint8_t)(temp_to_send >> 8);    // MSB
            g_canTxData[i*2+1] = (uint8_t)(temp_to_send & 0xFF); // LSB
        }
        g_canTxHeader.DataLength = FDCAN_DLC_BYTES_8;

        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData) != HAL_OK)
        {
             HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin); // Tx Error
        }
    } else {
         HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin); // Mutex Error
    }
  }
}

void CanRxTask(void *argument)
{
  for(;;)
  {
    osDelay(1000); // Idle task, logic is in callback
  }
}

void BrakeLightTask(void *argument)
{
  uint8_t received_command;
  for(;;)
  {
    if (osMessageQueueGet(g_brakeCommandQueueHandle, &received_command, NULL, osWaitForever) == osOK)
    {
      if (received_command == 1) // Command for ON
      {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
      }
      else // Command for OFF
      {
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
      }
    }
  }
}

/* --- HAL Callback Functions --- */

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
