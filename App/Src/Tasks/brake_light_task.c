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
    
    extern volatile uint32_t dbg_brake_on_count;
    extern volatile uint32_t dbg_brake_off_count;

    for(;;)
    {
        if (osMessageQueueGet(brakeQueue, &received_command, NULL, osWaitForever) == osOK)
        {
            if (received_command == 1)
            {
                HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
                dbg_brake_on_count++;
            }
            else
            {
                HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
                dbg_brake_off_count++;
            }
        }
    }
}
