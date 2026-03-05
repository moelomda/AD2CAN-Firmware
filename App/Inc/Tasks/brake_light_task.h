#ifndef BRAKE_LIGHT_TASK_H_
#define BRAKE_LIGHT_TASK_H_

#include "cmsis_os.h"

void BrakeLightTask_Init(void);
void BrakeLightTask_Run(void *argument);

#endif /* BRAKE_LIGHT_TASK_H_ */
