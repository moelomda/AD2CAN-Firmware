#ifndef SENSORS_H_
#define SENSORS_H_

#include <stdint.h>
#include "app_config.h"

uint16_t Sensor_CalculatePressure(uint16_t adc_value);
int16_t Sensor_CalculateSteeringAngle(uint16_t adc_value);
uint16_t Sensor_FilterRPM_Left(uint16_t rpm_value);
uint16_t Sensor_FilterRPM_Right(uint16_t rpm_value);

#endif /* SENSORS_H_ */
