#include "can_rx_task.h"
#include "can_hal.h"
#include "main.h"
#include "debug_printf.h"

void CanRxTask_Init(void)
{
    const osThreadAttr_t task_attributes = {
        .name = "CanRxTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 256 * 4
    };
    if(osThreadNew(CanRxTask_Run, NULL, &task_attributes) == NULL) {
        Error_Handler();
    }
}

void CanRxTask_Run(void *argument)
{
    /* Heartbeat LED task: mirrors taskLED from AD2CAN_F1
     * If heartbeat CAN message was received, LED is controlled by network master.
     * Otherwise, toggle green LED every 1s as local heartbeat indicator. */
    extern volatile uint32_t dbg_canrx_count;

    for(;;)
    {
        osDelay(1000);
        dbg_canrx_count++;

        if (CAN_HAL_GetHeartbeatActive())
        {
            CAN_HAL_ClearHeartbeatActive();
        }
        else
        {
            HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        }
    }
}
