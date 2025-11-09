#ifndef INC_AD2CAN_H_
#define INC_AD2CAN_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"

/* --- Shared Global Variables (Externs) --- */
// These are defined in ad2can.c and used by ad2can_can.c
extern float g_latestTemperatures[8];
extern osMutexId_t g_temperatureMutexHandle;

/* --- Public Function Prototypes --- */

/** @brief Initializes Application-layer RTOS objects (Mutexes, Tasks) */
void App_Init_RTOS(void);

/** @brief Task for processing sensor data */
void SensorProcessingTask(void *argument);

#endif /* INC_AD2CAN_H_ */
