#ifndef ADC_HAL_H_
#define ADC_HAL_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"

void ADC_HAL_Init(void);
void ADC_HAL_Start(void);
void ADC_HAL_StartOnce(void);
osMessageQueueId_t ADC_HAL_GetQueueHandle(void);
uint16_t* ADC_HAL_GetBuffer(void);

#endif /* ADC_HAL_H_ */
