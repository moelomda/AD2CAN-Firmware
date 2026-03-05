#include "adc_hal.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

static osMessageQueueId_t adcDataQueueHandle;
static volatile uint16_t adcBuffer[ADC_NUMBER_OF_CHANNELS];

void ADC_HAL_Init(void)
{
    const osMessageQueueAttr_t adcQueue_attributes = {
        .name = "AdcDataQueue"
    };
    adcDataQueueHandle = osMessageQueueNew(4, sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), &adcQueue_attributes);
    if (adcDataQueueHandle == NULL) { 
        Error_Handler(); 
    }
}

void ADC_HAL_Start(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, ADC_NUMBER_OF_CHANNELS) != HAL_OK)
    {
        Error_Handler();
    }
}

osMessageQueueId_t ADC_HAL_GetQueueHandle(void)
{
    return adcDataQueueHandle;
}

uint16_t* ADC_HAL_GetBuffer(void)
{
    return (uint16_t*)adcBuffer;
}

void ADC_HAL_StartOnce(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, ADC_NUMBER_OF_CHANNELS);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    extern volatile uint32_t dbg_adc_callback_count;
    extern volatile uint32_t dbg_adc_queue_fail;
    static uint8_t first_read_skipped = 0;

    if (hadc->Instance == ADC1)
    {
        dbg_adc_callback_count++;

        /* Stop DMA immediately to prevent continuous flooding */
        HAL_ADC_Stop_DMA(&hadc1);

        /* Skip first DMA completion (ADC settling) */
        if (first_read_skipped == 0) {
            first_read_skipped = 1;
            /* Restart for the real first read */
            HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, ADC_NUMBER_OF_CHANNELS);
            return;
        }

        /* Post data to queue. SensorTask will call ADC_HAL_StartOnce() to trigger next read */
        osStatus_t queue_status = osMessageQueuePut(adcDataQueueHandle,
                                                     (const void*)adcBuffer,
                                                     0,
                                                     0);

        if (queue_status != osOK) {
            dbg_adc_queue_fail++;
            HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
        }
    }
}
