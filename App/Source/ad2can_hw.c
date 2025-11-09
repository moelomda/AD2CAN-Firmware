#include "ad2can_hw.h"
#include <math.h>
#include <string.h>

/* --- File-Scope Variables --- */
// HAL Handles (defined in main.c, accessed via extern)
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

// RTOS Objects
osMessageQueueId_t g_adcDataQueueHandle; //Queue to process ADC values

// Buffers
volatile uint16_t g_adcBuffer[ADC_NUMBER_OF_CHANNELS]; // Buffer used by DMA

/* --- RTOS Object Initialization --- */

void HW_Init_ADC_RTOS(void)
{
    const osMessageQueueAttr_t g_adcDataQueue_attributes = {
        .name = "AdcDataQueue"
    };
    g_adcDataQueueHandle = osMessageQueueNew(4, sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), &g_adcDataQueue_attributes);
    if (g_adcDataQueueHandle == NULL) { Error_Handler(); }
}

void HW_StartADC(void)
{
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adcBuffer, ADC_NUMBER_OF_CHANNELS) != HAL_OK)
    {
        Error_Handler(); // Error starting ADC DMA
    }
}

/* --- Public Getter Functions --- */

osMessageQueueId_t HW_GetAdcQueueHandle(void)
{
    return g_adcDataQueueHandle;
}

/* --- Calculation Helper Functions --- */

float AdcValue_To_Voltage(int adc_value) {
    if (adc_value < 0) adc_value = 0;
    if (adc_value > (int)ADC_MAX_VALUE) adc_value = (int)ADC_MAX_VALUE;
    return ((float)adc_value / ADC_MAX_VALUE) * ADC_VREF;
}

float Voltage_To_Resistance(float voltage) {
    if (voltage >= NTC_DIVIDER_VOLTAGE) {
        return INFINITY;
    }
    if (voltage <= 0.0f) {
        return 0.0f;
    }
    return (voltage * NTC_DIVIDER_RESISTANCE) / (NTC_DIVIDER_VOLTAGE - voltage);
}

float Resistance_To_Temperature(float resistance) {
    if (resistance <= 0.0f) { return -999.0f; } // Invalid resistance

    float log_resistance = logf(resistance);
    float inv_kelvin = NTC_COEFF_A + (NTC_COEFF_B * log_resistance) + (NTC_COEFF_C * powf(log_resistance, 3));

    if (inv_kelvin == 0.0f) { return -999.0f; } // Invalid calculation

    float kelvin = 1.0f / inv_kelvin;
    float celsius = kelvin - 273.15f;
    return celsius;
}

/* --- HAL Callback Functions --- */

/**
  * @brief  Conversion complete callback in non-blocking mode
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    static uint8_t first_read_skipped = 0;

    if (hadc->Instance == ADC1)
    {
        if (first_read_skipped == 0)
        {
            first_read_skipped = 1;
            return;
        }

        uint16_t snapshot_buffer[ADC_NUMBER_OF_CHANNELS];
        memcpy(snapshot_buffer, (const void*)g_adcBuffer, sizeof(snapshot_buffer));

        osStatus_t queue_status = osMessageQueuePut(g_adcDataQueueHandle, &snapshot_buffer, 0, 0);

        if (queue_status != osOK)
        {
            // Queue is full, data was dropped. Toggle red LED.
            HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
        }
    }
}
