#ifndef TIM_HAL_H_
#define TIM_HAL_H_

#include "main.h"
#include "app_config.h"

void TIM_HAL_Init(void);
uint16_t TIM_HAL_GetRPM_Left(void);
uint16_t TIM_HAL_GetRPM_Right(void);

#endif /* TIM_HAL_H_ */
