#include "sensors.h"

static uint16_t rpm_left_filter[WS_FILTER_SIZE] = {0};
static int rpm_left_counter = 0;
static uint16_t rpm_right_filter[WS_FILTER_SIZE] = {0};
static int rpm_right_counter = 0;

uint16_t Sensor_CalculatePressure(uint16_t adc_value) 
{
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

int16_t Sensor_CalculateSteeringAngle(uint16_t adc_value) 
{
    int16_t steering = adc_value;
    steering = (1800.0f / (STEERING_RAW_RIGHT - STEERING_RAW_LEFT)) * (steering - STEERING_RAW_LEFT) - 900;
    return steering;
}

uint16_t Sensor_FilterRPM_Left(uint16_t rpm_value)
{
    rpm_left_filter[rpm_left_counter++] = rpm_value;
    if(rpm_left_counter >= WS_FILTER_SIZE) 
        rpm_left_counter = 0;
    
    uint16_t max_value = rpm_value;
    for(int i = 0; i < WS_FILTER_SIZE; i++){
        if(rpm_left_filter[i] > max_value) 
            max_value = rpm_left_filter[i];
    }
    return max_value;
}

uint16_t Sensor_FilterRPM_Right(uint16_t rpm_value)
{
    rpm_right_filter[rpm_right_counter++] = rpm_value;
    if(rpm_right_counter >= WS_FILTER_SIZE) 
        rpm_right_counter = 0;
    
    uint16_t max_value = rpm_value;
    for(int i = 0; i < WS_FILTER_SIZE; i++){
        if(rpm_right_filter[i] > max_value) 
            max_value = rpm_right_filter[i];
    }
    return max_value;
}
