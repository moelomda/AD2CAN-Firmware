#ifndef INC_AD2CAN_CAN_H_
#define INC_AD2CAN_CAN_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"

/* --- Public Function Prototypes --- */

/** @brief Initializes CAN-related RTOS objects (Queues, Tasks) */
void CAN_Init_RTOS(void);

/** @brief Task for transmitting CAN messages. */
void CanTxTask(void *argument);

/** @brief Task for receiving CAN messages (placeholder). */
void CanRxTask(void *argument);

/** @brief Task for controlling the brake light GPIO. */
void BrakeLightTask(void *argument);

#endif /* INC_AD2CAN_CAN_H_ */
