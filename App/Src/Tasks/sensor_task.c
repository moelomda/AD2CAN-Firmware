#include "sensor_task.h"
#include "adc_hal.h"
#include "sensors.h"
#include "app_config.h"
#include "main.h"
#include "debug_printf.h"
#include <string.h>

static ProcessedSensorData_t sensorData;
static osMutexId_t sensorDataMutexHandle;

void SensorTask_Init(void)
{
    const osMutexAttr_t mutex_attributes = {
        .name = "SensorDataMutex"
    };
    sensorDataMutexHandle = osMutexNew(&mutex_attributes);
    if (sensorDataMutexHandle == NULL) { 
        Error_Handler(); 
    }

    const osThreadAttr_t task_attributes = {
        .name = "SensorTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 512 * 4
    };
    if(osThreadNew(SensorTask_Run, NULL, &task_attributes) == NULL) { 
        Error_Handler(); 
    }
}

ProcessedSensorData_t* SensorTask_GetDataPointer(void)
{
    return &sensorData;
}

osMutexId_t SensorTask_GetMutexHandle(void)
{
    return sensorDataMutexHandle;
}

void SensorTask_Run(void *argument)
{
    uint16_t received_adc_data[ADC_NUMBER_OF_CHANNELS];
    ProcessedSensorData_t local_data;

    osMessageQueueId_t adcQueue = ADC_HAL_GetQueueHandle();

    extern volatile uint32_t dbg_sensor_count;

    for(;;)
    {
        if (osMessageQueueGet(adcQueue, received_adc_data, NULL, osWaitForever) == osOK)
        {
            dbg_sensor_count++;

            local_data.brake_pressure_front = Sensor_CalculatePressure(received_adc_data[0]);
            local_data.brake_pressure_rear  = Sensor_CalculatePressure(received_adc_data[1]);
            local_data.steering_angle       = Sensor_CalculateSteeringAngle(received_adc_data[2]);
            local_data.apps1                = received_adc_data[3];
            local_data.apps2                = received_adc_data[4];

            if (osMutexAcquire(sensorDataMutexHandle, osWaitForever) == osOK)
            {
                memcpy(&sensorData, &local_data, sizeof(ProcessedSensorData_t));
                osMutexRelease(sensorDataMutexHandle);
            }

            /* Trigger next ADC conversion */
            ADC_HAL_StartOnce();
        }
    }
}
