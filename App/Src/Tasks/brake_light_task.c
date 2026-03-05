#include "brake_light_task.h"
#include "can_hal.h"
#include "main.h"

void BrakeLightTask_Init(void)
{
    const osThreadAttr_t task_attributes = {
        .name = "BrakeLightTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 256 * 4
    };
    if(osThreadNew(BrakeLightTask_Run, NULL, &task_attributes) == NULL) { 
        Error_Handler(); 
    }
}

void BrakeLightTask_Run(void *argument)
{
    uint8_t received_command;
    osMessageQueueId_t brakeQueue = CAN_HAL_GetBrakeQueueHandle();
    
    for(;;)
    {
        if (osMessageQueueGet(brakeQueue, &received_command, NULL, osWaitForever) == osOK)
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
