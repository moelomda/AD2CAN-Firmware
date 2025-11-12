#ifndef INC_AD2CAN_HW_H_
#define INC_AD2CAN_HW_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"

/* --- Public Function Prototypes --- */

// --- ADC Functions ---
void HW_Init_ADC_RTOS(void);
void HW_StartADC(void);
osMessageQueueId_t HW_GetAdcQueueHandle(void);

// --- RPM Functions ---
void HW_Init_RPM(void);
uint16_t HW_Get_RPM_Left(void);
uint16_t HW_Get_RPM_Right(void);
uint16_t HW_MovingMaxL(uint16_t ws_left);
uint16_t HW_MovingMaxR(uint16_t ws_right);

// --- Sensor Calculation Functions
uint16_t HW_GetPressure(uint16_t adc_value);
int16_t HW_GetSteeringAngle(uint16_t adc_value);


#endif /* INC_AD2CAN_HW_H_ */
