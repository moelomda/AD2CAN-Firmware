#ifndef CAN_TX_TASK_H_
#define CAN_TX_TASK_H_

#include "cmsis_os.h"

void CanTxTask_Init(void);
void CanTxTask_Run(void *argument);

#endif /* CAN_TX_TASK_H_ */
