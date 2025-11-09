#include "ad2can.h"
#include "ad2can_hw.h" // To get data from ADC queue and use calc functions
#include <string.h>

/* --- Shared Global Variables (Definitions) --- */
float g_latestTemperatures[ADC_NUMBER_OF_CHANNELS];
osMutexId_t g_temperatureMutexHandle;

/* --- RTOS Object Initialization --- */

void App_Init_RTOS(void)
{
    // Create the mutex for protecting temperature data
    const osMutexAttr_t temperatureMutex_attributes = {
        .name = "TempMutex"
    };
    g_temperatureMutexHandle = osMutexNew(&temperatureMutex_attributes);
    if (g_temperatureMutexHandle == NULL) { Error_Handler(); }

    // Create Sensor Processing Task
    const osThreadAttr_t sensorProcessingTask_attributes = {
        .name = "SensorProcessingTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 256 * 4 // Larger stack for float calculations
    };
    if(osThreadNew(SensorProcessingTask, NULL, &sensorProcessingTask_attributes) == NULL) { Error_Handler(); }
}

/* --- Task Definitions --- */

void SensorProcessingTask(void *argument)
{
  uint16_t received_data[ADC_NUMBER_OF_CHANNELS];
  float voltage;
  float resistance;
  float current_temperatures[ADC_NUMBER_OF_CHANNELS]; // Unordered temps
  float ordered_temps[ADC_NUMBER_OF_CHANNELS];      // Ordered temps

  // Get the ADC queue handle from the HW module
  osMessageQueueId_t adcQueue = HW_GetAdcQueueHandle();

  for(;;)
  {
    if (osMessageQueueGet(adcQueue, &received_data, NULL, osWaitForever) == osOK)
    {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);

        // Calculate all 8 temperatures (in Rank order)
        for (int i = 0; i < ADC_NUMBER_OF_CHANNELS; i++) {
            voltage = AdcValue_To_Voltage(received_data[i]);
            resistance = Voltage_To_Resistance(voltage);
            current_temperatures[i] = Resistance_To_Temperature(resistance);
        }

        // Reorder results to match logical AN_IN_1..8
        ordered_temps[ADC_RANK_TO_AN_IN_1] = current_temperatures[0];
        ordered_temps[ADC_RANK_TO_AN_IN_2] = current_temperatures[1];
        ordered_temps[ADC_RANK_TO_AN_IN_3] = current_temperatures[2];
        ordered_temps[ADC_RANK_TO_AN_IN_4] = current_temperatures[7];
        ordered_temps[ADC_RANK_TO_AN_IN_5] = current_temperatures[3];
        ordered_temps[ADC_RANK_TO_AN_IN_6] = current_temperatures[4];
        ordered_temps[ADC_RANK_TO_AN_IN_7] = current_temperatures[5];
        ordered_temps[ADC_RANK_TO_AN_IN_8] = current_temperatures[6];

        // Safely update the global temperature array
        if (osMutexAcquire(g_temperatureMutexHandle, osWaitForever) == osOK) {
            memcpy(g_latestTemperatures, ordered_temps, sizeof(g_latestTemperatures));
            osMutexRelease(g_temperatureMutexHandle);
        }
    }
  }
}
