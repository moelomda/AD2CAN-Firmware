# 🔄 MIGRATION GUIDE - Old to New Structure

## 📁 File Mapping

### Old Structure → New Structure

| Old File | New File(s) | Description |
|----------|-------------|-------------|
| `App/Inc/app_config.h` | `App/Config/app_config.h` | Configuration constants |
| `App/Inc/ad2can.h` | `App/Utils/data_types.h` + `App/Tasks/sensor_task.h` | Data types + task interface |
| `App/Inc/ad2can_hw.h` | `App/HAL/adc_hal.h` + `App/HAL/tim_hal.h` | Hardware abstraction |
| `App/Inc/ad2can_can.h` | `App/HAL/can_hal.h` + `App/Tasks/can_tx_task.h` | CAN interface |
| `App/Source/ad2can.c` | `App/Tasks/sensor_task.c` | Sensor processing task |
| `App/Source/ad2can_hw.c` | `App/HAL/adc_hal.c` + `App/HAL/tim_hal.c` + `App/Drivers/sensors.c` | ADC/TIM HAL + sensor math |
| `App/Source/ad2can_can.c` | `App/HAL/can_hal.c` + `App/Tasks/can_tx_task.c` + `App/Tasks/can_rx_task.c` + `App/Tasks/brake_light_task.c` | CAN HAL + all CAN tasks |

---

## 🔧 Function Mapping

### ADC Functions (ad2can_hw.c → adc_hal.c + sensors.c)

| Old Function | New Function | Location |
|--------------|--------------|----------|
| `HW_Init_ADC_RTOS()` | `ADC_HAL_Init()` | `HAL/adc_hal.c` |
| `HW_StartADC()` | `ADC_HAL_Start()` | `HAL/adc_hal.c` |
| `HW_GetAdcQueueHandle()` | `ADC_HAL_GetQueueHandle()` | `HAL/adc_hal.c` |
| `HW_GetPressure()` | `Sensor_CalculatePressure()` | `Drivers/sensors.c` |
| `HW_GetSteeringAngle()` | `Sensor_CalculateSteeringAngle()` | `Drivers/sensors.c` |
| `HW_MovingMaxL()` | `Sensor_FilterRPM_Left()` | `Drivers/sensors.c` |
| `HW_MovingMaxR()` | `Sensor_FilterRPM_Right()` | `Drivers/sensors.c` |

### Timer Functions (ad2can_hw.c → tim_hal.c)

| Old Function | New Function | Location |
|--------------|--------------|----------|
| `HW_Init_RPM()` | `TIM_HAL_Init()` | `HAL/tim_hal.c` |
| `HW_Get_RPM_Left()` | `TIM_HAL_GetRPM_Left()` | `HAL/tim_hal.c` |
| `HW_Get_RPM_Right()` | `TIM_HAL_GetRPM_Right()` | `HAL/tim_hal.c` |

### CAN Functions (ad2can_can.c → can_hal.c + tasks)

| Old Function | New Function | Location |
|--------------|--------------|----------|
| `CAN_Init_RTOS()` | `CAN_HAL_Init()` + `CanTxTask_Init()` + `CanRxTask_Init()` + `BrakeLightTask_Init()` | Multiple files |
| `HAL_FDCAN_AddMessageToTxFifoQ()` | `CAN_HAL_Transmit()` | `HAL/can_hal.c` |

### Sensor Processing (ad2can.c → sensor_task.c)

| Old Function | New Function | Location |
|--------------|--------------|----------|
| `App_Init_RTOS()` | `SensorTask_Init()` | `Tasks/sensor_task.c` |
| `SensorProcessingTask()` | `SensorTask_Run()` | `Tasks/sensor_task.c` |

### Global Variables

| Old Variable | New Access Method | Location |
|--------------|-------------------|----------|
| `g_sensorData` | `SensorTask_GetDataPointer()` | `Tasks/sensor_task.c` |
| `g_sensorDataMutexHandle` | `SensorTask_GetMutexHandle()` | `Tasks/sensor_task.c` |
| `g_adcDataQueueHandle` | `ADC_HAL_GetQueueHandle()` | `HAL/adc_hal.c` |
| `g_brakeCommandQueueHandle` | `CAN_HAL_GetBrakeQueueHandle()` | `HAL/can_hal.c` |

---

## 🎯 Code Example Migration

### Old Code (ad2can_can.c):
```c
// Old way
extern ProcessedADC_t g_sensorData;
extern osMutexId_t g_sensorDataMutexHandle;

void CanTxTask(void *argument)
{
    ProcessedADC_t local_data;
    
    if (osMutexAcquire(g_sensorDataMutexHandle, 10) == osOK)
    {
        memcpy(&local_data, &g_sensorData, sizeof(ProcessedADC_t));
        osMutexRelease(g_sensorDataMutexHandle);
    }
}
```

### New Code (can_tx_task.c):
```c
// New way - cleaner interface!
#include "sensor_task.h"

void CanTxTask_Run(void *argument)
{
    ProcessedSensorData_t local_data;
    
    if (osMutexAcquire(SensorTask_GetMutexHandle(), 10) == osOK)
    {
        memcpy(&local_data, SensorTask_GetDataPointer(), sizeof(ProcessedSensorData_t));
        osMutexRelease(SensorTask_GetMutexHandle());
    }
}
```

**Benefits**:
- No `extern` declarations needed
- Clear module ownership
- Type name is more descriptive
- Easier to understand data flow

---

## 📋 Include Changes

### Old main.c:
```c
#include "app_config.h"
#include "ad2can_hw.h"
#include "ad2can_can.h"
#include "ad2can.h"
```

### New main.c:
```c
#include "adc_hal.h"
#include "can_hal.h"
#include "tim_hal.h"
#include "sensor_task.h"
#include "can_tx_task.h"
#include "can_rx_task.h"
#include "brake_light_task.h"
```

**Benefits**:
- More descriptive names
- Clear separation of concerns
- Each include represents one responsibility

---

## ⚠️ Breaking Changes

### 1. Data Type Renamed
- **Old**: `ProcessedADC_t`
- **New**: `ProcessedSensorData_t`
- **Reason**: More descriptive, not tied to ADC

### 2. Function Prefix Changes
- **Old**: `HW_`, `CAN_`, `App_`
- **New**: `ADC_HAL_`, `CAN_HAL_`, `TIM_HAL_`, `Sensor_`, `SensorTask_`, etc.
- **Reason**: Clearer module ownership

### 3. No Direct Global Access
- **Old**: Direct `extern` access to `g_sensorData`
- **New**: Use getter functions like `SensorTask_GetDataPointer()`
- **Reason**: Encapsulation, easier to maintain

---

## ✅ Benefits of New Structure

1. **Modularity**: Each module has single responsibility
2. **Testability**: Can test HAL layers independently
3. **Maintainability**: Easy to find and modify code
4. **Scalability**: Easy to add new sensors/tasks
5. **Readability**: Clear naming and organization
6. **Portability**: HAL can be reused in other projects

---

## 🔄 Step-by-Step Migration

If you have custom code in the old structure:

### Step 1: Identify Your Changes
Look for your custom code in:
- `App/Source/ad2can_hw.c`
- `App/Source/ad2can_can.c`
- `App/Source/ad2can.c`

### Step 2: Map to New Files
Use the function mapping table above to find where your code should go

### Step 3: Update Function Calls
Replace old function names with new ones:
- `HW_Init_ADC_RTOS()` → `ADC_HAL_Init()`
- `g_sensorData` → `SensorTask_GetDataPointer()`
- etc.

### Step 4: Update Includes
Replace old includes with new modular includes

### Step 5: Test Build
Follow the **BUILD_GUIDE.md** to configure and build

---

## 🆘 Need Help?

If you're stuck migrating code:

1. Check the function mapping tables above
2. Look at similar code in the new structure
3. Read the module headers for available functions
4. Check **MODULAR_STRUCTURE.md** for architecture overview

---

**Remember**: The new structure does EXACTLY the same thing as the old code, just organized better! 🎯
