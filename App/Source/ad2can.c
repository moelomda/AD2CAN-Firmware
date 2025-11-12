#include "ad2can.h"
#include "ad2can_hw.h"
#include <string.h>
#include "main.h"

ProcessedADC_t g_sensorData;

/* --- RTOS Object Initialization --- */

void App_Init_RTOS(void)
{
    const osMutexAttr_t sensorDataMutex_attributes = {
        .name = "SensorDataMutex"
    };
    g_sensorDataMutexHandle = osMutexNew(&sensorDataMutex_attributes);
    if (g_sensorDataMutexHandle == NULL) { Error_Handler(); }

    const osThreadAttr_t sensorProcessingTask_attributes = {
        .name = "SensorProcessingTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 256 * 4
    };
    if(osThreadNew(SensorProcessingTask, NULL, &sensorProcessingTask_attributes) == NULL) { Error_Handler(); }
}

/* --- Task Definitions --- */

/**
 * @brief مهمة معالجة بيانات الـ ADC الـ 6 المستلمة من الـ Queue
 */
void SensorProcessingTask(void *argument)
{
  uint16_t received_adc_data[ADC_NUMBER_OF_CHANNELS]; // سيستقبل 6 قراءات
  ProcessedADC_t local_sensor_data; // هيكل محلي لتجهيز البيانات

  osMessageQueueId_t adcQueue = HW_GetAdcQueueHandle();



  for(;;)
  {

    if (osMessageQueueGet(adcQueue, received_adc_data, NULL, osWaitForever) == osOK)
    {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);


        local_sensor_data.brake_pressure_front = HW_GetPressure(received_adc_data[0]); // Rank 1
        local_sensor_data.brake_pressure_rear  = HW_GetPressure(received_adc_data[1]); // Rank 2
        local_sensor_data.steering_angle       = HW_GetSteeringAngle(received_adc_data[2]); // Rank 3 (CH5)

        local_sensor_data.apps1                = received_adc_data[3]; // Rank 4 (CH11)
        local_sensor_data.apps2                = received_adc_data[4]; // Rank 5 (CH12)
        if (osMutexAcquire(g_sensorDataMutexHandle, osWaitForever) == osOK)
        {
            memcpy(&g_sensorData, &local_sensor_data, sizeof(ProcessedADC_t));
            osMutexRelease(g_sensorDataMutexHandle);
        }
    }
  }
}
