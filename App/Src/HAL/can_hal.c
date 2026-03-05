#include "can_hal.h"
#include "main.h"
#include <stdbool.h>

extern FDCAN_HandleTypeDef hfdcan1;

static osMessageQueueId_t brakeCommandQueueHandle;
static FDCAN_TxHeaderTypeDef canTxHeader;
static FDCAN_RxHeaderTypeDef canRxHeader;
static uint8_t canRxData[8];

/* Heartbeat state: controlled by CAN master */
volatile bool heartbeatActive = false;

void CAN_HAL_Init(void)
{
    const osMessageQueueAttr_t brakeQueue_attributes = {
        .name = "BrakeCmdQueue"
    };
    brakeCommandQueueHandle = osMessageQueueNew(1, sizeof(uint8_t), &brakeQueue_attributes);
    if (brakeCommandQueueHandle == NULL) {
        Error_Handler();
    }

    canTxHeader.IdType = FDCAN_STANDARD_ID;
    canTxHeader.TxFrameType = FDCAN_DATA_FRAME;
    canTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    canTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    canTxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    canTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    canTxHeader.MessageMarker = 0;
}

osMessageQueueId_t CAN_HAL_GetBrakeQueueHandle(void)
{
    return brakeCommandQueueHandle;
}

bool CAN_HAL_GetHeartbeatActive(void)
{
    return heartbeatActive;
}

void CAN_HAL_ClearHeartbeatActive(void)
{
    heartbeatActive = false;
}

HAL_StatusTypeDef CAN_HAL_Transmit(uint32_t id, uint8_t* data, uint32_t length)
{
    canTxHeader.Identifier = id;

    switch(length) {
        case 1: canTxHeader.DataLength = FDCAN_DLC_BYTES_1; break;
        case 2: canTxHeader.DataLength = FDCAN_DLC_BYTES_2; break;
        case 3: canTxHeader.DataLength = FDCAN_DLC_BYTES_3; break;
        case 4: canTxHeader.DataLength = FDCAN_DLC_BYTES_4; break;
        case 5: canTxHeader.DataLength = FDCAN_DLC_BYTES_5; break;
        case 6: canTxHeader.DataLength = FDCAN_DLC_BYTES_6; break;
        case 7: canTxHeader.DataLength = FDCAN_DLC_BYTES_7; break;
        case 8: canTxHeader.DataLength = FDCAN_DLC_BYTES_8; break;
        default: canTxHeader.DataLength = FDCAN_DLC_BYTES_8; break;
    }

    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &canTxHeader, data);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &canRxHeader, canRxData) == HAL_OK)
        {
            switch (canRxHeader.Identifier)
            {
            case CAN_ID_RX_BRAKE:
                if (canRxHeader.DataLength >= FDCAN_DLC_BYTES_1)
                {
                    uint8_t brake_command = canRxData[0];
                    osMessageQueuePut(brakeCommandQueueHandle, &brake_command, 0, 0);
                }
                break;

            case CAN_ID_HEARTBEAT:
                /* Heartbeat from network master: control green LED directly */
                HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, (GPIO_PinState)canRxData[0]);
                heartbeatActive = true;
                break;

            default:
                break;
            }
        }
    }
}
