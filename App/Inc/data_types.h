#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include <stdint.h>

typedef struct {
    uint16_t brake_pressure_front;
    uint16_t brake_pressure_rear;
    int16_t  steering_angle;
    uint16_t apps1;
    uint16_t apps2;
} ProcessedSensorData_t;

#endif /* DATA_TYPES_H_ */
