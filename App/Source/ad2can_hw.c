#include "ad2can_hw.h"
#include "main.h"
#include <math.h>
#include <string.h>

/* --- File-Scope Variables --- */
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;  // افترضنا أن CubeIDE سمّاها هكذا
extern TIM_HandleTypeDef htim15; // افترضنا أن CubeIDE سمّاها هكذا

osMessageQueueId_t g_adcDataQueueHandle;
volatile uint16_t g_adcBuffer[ADC_NUMBER_OF_CHANNELS]; // سيستخدم "6" من ADC_config.h

// متغيرات لحساب الـ RPM (مبسطة)
volatile uint32_t g_rpm_left_ticks = 0;
volatile uint32_t g_rpm_right_ticks = 0;
// (منطق حساب الـ RPM الفعلي معقد، سنستخدم قيم وهمية مؤقتاً)

// فلاتر RPM (منسوخة من ad2can.c الخاص بهم)
uint16_t ws_left_values[WS_FILTER_SIZE] = {0};
int ws_left_counter = 0;
uint16_t ws_right_values[WS_FILTER_SIZE] = {0};
int ws_right_counter = 0;

/* --- RTOS Object Initialization --- */

void HW_Init_ADC_RTOS(void)
{
    const osMessageQueueAttr_t g_adcDataQueue_attributes = {
        .name = "AdcDataQueue"
    };
    // إنشاء طابور بحجم 6
    g_adcDataQueueHandle = osMessageQueueNew(4, sizeof(uint16_t[ADC_NUMBER_OF_CHANNELS]), &g_adcDataQueue_attributes);
    if (g_adcDataQueueHandle == NULL) { Error_Handler(); }
}

void HW_StartADC(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adcBuffer, ADC_NUMBER_OF_CHANNELS) != HAL_OK)
    {
        Error_Handler();
    }
}

void HW_Init_RPM(void)
{
    HAL_TIM_IC_Start_IT(&htim15, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);
}

osMessageQueueId_t HW_GetAdcQueueHandle(void)
{
    return g_adcDataQueueHandle;
}

/* --- Calculation Helper Functions (منسوخة من كودهم) --- */

uint16_t HW_Get_RPM_Left(void)
{
    // (منطق وهمي مؤقتاً)
    g_rpm_left_ticks++;
    return (uint16_t)g_rpm_left_ticks;
}

uint16_t HW_Get_RPM_Right(void)
{
    // (منطق وهمي مؤقتاً)
    g_rpm_right_ticks++;
    return (uint16_t)g_rpm_right_ticks;
}

// دالة فلتر RPM (منسوخة من ad2can.c الخاص بهم)
uint16_t HW_MovingMaxL(uint16_t ws_left){
	ws_left_values[ws_left_counter++] = ws_left;
	if(ws_left_counter >= WS_FILTER_SIZE) ws_left_counter = 0;
	for(int i = 0; i < WS_FILTER_SIZE; i++){
	    if(ws_left_values[i] > ws_left) ws_left = ws_left_values[i];
	}
	return ws_left;
}
uint16_t HW_MovingMaxR(uint16_t ws_right){
	ws_right_values[ws_right_counter++] = ws_right;
	if(ws_right_counter >= WS_FILTER_SIZE) ws_right_counter = 0;
	for(int i = 0; i < WS_FILTER_SIZE; i++){
	    if(ws_right_values[i] > ws_right) ws_right = ws_right_values[i];
	}
	return ws_right;
}

// دالة حساب الضغط (منسوخة من ad2can_can.c الخاص بهم)
uint16_t HW_GetPressure(uint16_t adc_value) {
	uint32_t pressure = adc_value;
	if (pressure < PRESSURE_MIN_ADC_VAL || pressure > PRESSURE_MAX_ADC_VAL) {
		return 0;
	}
	pressure = (pressure - PRESSURE_MIN_ADC_VAL) * PRESSURE_MAX_PRESSURE
			/ (PRESSURE_MAX_ADC_VAL - PRESSURE_MIN_ADC_VAL);
	if(pressure > 0xFFFF){
			return 0xFFFF;
		}
	return (uint16_t)pressure;
}

int16_t HW_GetSteeringAngle(uint16_t adc_value) {
    int16_t steering = adc_value;
    steering = (1800.0f / (STEERING_RAW_RIGHT - STEERING_RAW_LEFT)) * (steering - STEERING_RAW_LEFT) - 900;
    return steering;
}


/* --- HAL Callback Functions --- */

// Temporary test buffer to verify DMA is writing
uint16_t test_copy[ADC_NUMBER_OF_CHANNELS];

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    static uint8_t first_read_skipped = 0;
    
    if (hadc->Instance == ADC1)
    {
        // Skip first conversion (sometimes contains garbage)
        if (first_read_skipped == 0) {
            first_read_skipped = 1;
            return;
        }

        // Copy buffer IMMEDIATELY to test if DMA is working
        for(int i = 0; i < ADC_NUMBER_OF_CHANNELS; i++) {
            test_copy[i] = g_adcBuffer[i];
        }

        // At this point: DMA has finished writing all 6 channels to g_adcBuffer
        // The callback is triggered by DMA TC (Transfer Complete)
        // Queue will make a copy of the buffer
        osStatus_t queue_status = osMessageQueuePut(g_adcDataQueueHandle, 
                                                     (const void*)g_adcBuffer, 
                                                     0, 
                                                     0);
        
        if (queue_status != osOK) {
            // Queue full - data loss
            HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
        } else {
            // Successfully queued - toggle green LED to show activity
            HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        }
    }
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15)
    {
        // (هنا نضع منطق حساب RPM الحقيقي بناءً على التردد الملتقط)
        // g_rpm_left_ticks = ...
    }
    else if (htim->Instance == TIM3) // افترضنا PB0
    {
        // (هنا نضع منطق حساب RPM الحقيقي بناءً على التردد الملتقط)
        // g_rpm_right_ticks = ...
    }
}
