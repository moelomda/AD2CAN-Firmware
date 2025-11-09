#ifndef INC_AD2CAN_HW_H_
#define INC_AD2CAN_HW_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"

/* --- Public Function Prototypes --- */

/** @brief Initializes ADC-related RTOS objects (Queues) */
void HW_Init_ADC_RTOS(void);

/** @brief Starts the ADC DMA hardware */
void HW_StartADC(void);

/** @brief Gets the handle for the ADC data queue */
osMessageQueueId_t HW_GetAdcQueueHandle(void);

/** @brief Converts raw ADC value (0-4095) to voltage */
float AdcValue_To_Voltage(int adc_value);

/** @brief Converts voltage from voltage divider to resistance */
float Voltage_To_Resistance(float voltage);

/** @brief Converts NTC resistance (Ohms) to Temperature (Celsius) */
float Resistance_To_Temperature(float resistance);

#endif /* INC_AD2CAN_HW_H_ */
