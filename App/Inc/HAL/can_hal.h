#ifndef CAN_HAL_H_
#define CAN_HAL_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"
#include <stdbool.h>

void CAN_HAL_Init(void);
osMessageQueueId_t CAN_HAL_GetBrakeQueueHandle(void);
HAL_StatusTypeDef CAN_HAL_Transmit(uint32_t id, uint8_t* data, uint32_t length);
bool CAN_HAL_GetHeartbeatActive(void);
void CAN_HAL_ClearHeartbeatActive(void);

#endif /* CAN_HAL_H_ */
