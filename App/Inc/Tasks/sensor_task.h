#ifndef SENSOR_TASK_H_
#define SENSOR_TASK_H_

#include "cmsis_os.h"
#include "data_types.h"

void SensorTask_Init(void);
void SensorTask_Run(void *argument);
ProcessedSensorData_t* SensorTask_GetDataPointer(void);
osMutexId_t SensorTask_GetMutexHandle(void);

#endif /* SENSOR_TASK_H_ */
