#ifndef INC_AD2CAN_H_
#define INC_AD2CAN_H_

#include "main.h"
#include "cmsis_os.h"
#include "app_config.h"

// هيكل (Struct) لتخزين كل بيانات الحساسات المُعالجة
typedef struct {
    uint16_t brake_pressure_front;
    uint16_t brake_pressure_rear;
    int16_t  steering_angle;
    uint16_t apps1;
    uint16_t apps2;
    // (الحساس السادس غير مستخدم في كودهم، سنتركه فارغاً)
} ProcessedADC_t;


/* --- Shared Global Variables (Externs) --- */
// هذه المتغيرات تُعرّف في ad2can.c وتُستخدم في ad2can_can.c
extern ProcessedADC_t g_sensorData; // <--- هيكل جديد بدلاً من g_latestTemperatures
extern osMutexId_t g_sensorDataMutexHandle; // <--- اسم جديد للـ Mutex

/* --- Public Function Prototypes --- */
void App_Init_RTOS(void);
void SensorProcessingTask(void *argument);

#endif /* INC_AD2CAN_H_ */
