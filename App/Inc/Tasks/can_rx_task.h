#ifndef CAN_RX_TASK_H_
#define CAN_RX_TASK_H_

#include "cmsis_os.h"

void CanRxTask_Init(void);
void CanRxTask_Run(void *argument);

#endif /* CAN_RX_TASK_H_ */
