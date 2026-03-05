# Project Structure

## File Organization

```
High_VoltageTeam/
+-- App/
|   +-- Inc/                    All header files
|   |   +-- HAL/               Hardware Abstraction Layer headers
|   |   |   +-- adc_hal.h      ADC DMA queue interface
|   |   |   +-- can_hal.h      CAN TX/RX and brake queue interface
|   |   |   +-- tim_hal.h      RPM measurement interface
|   |   +-- Tasks/             RTOS task headers
|   |   |   +-- sensor_task.h  Sensor processing task
|   |   |   +-- can_tx_task.h  CAN transmit task
|   |   |   +-- can_rx_task.h  Heartbeat LED task
|   |   |   +-- brake_light_task.h  Brake light control task
|   |   +-- Drivers/           Driver layer headers
|   |   |   +-- sensors.h      Pressure/steering/RPM filter math
|   |   +-- app_config.h       CAN IDs, sensor constants, timing
|   |   +-- app_init.h         App_Init() / App_Start() interface
|   |   +-- data_types.h       ProcessedSensorData_t struct
|   |
|   +-- Src/                    All source files
|       +-- HAL/
|       |   +-- adc_hal.c      ADC DMA + ConvCpltCallback + queue
|       |   +-- can_hal.c      CAN TX/RX + RxFifo0Callback
|       |   +-- tim_hal.c      Input capture + IC_CaptureCallback + RPM calc
|       +-- Tasks/
|       |   +-- sensor_task.c  ADC data -> processed sensors (mutex-protected)
|       |   +-- can_tx_task.c  Pack + transmit CAN frames at 100 Hz
|       |   +-- can_rx_task.c  Heartbeat LED toggle/control
|       |   +-- brake_light_task.c  Brake light GPIO control
|       +-- Drivers/
|       |   +-- sensors.c      Pressure, steering, RPM moving-max filter
|       +-- app_init.c         Central init: creates all tasks, starts HW
|
+-- Core/                       STM32 generated code
|   +-- Inc/                   main.h, FreeRTOSConfig.h, stm32g4xx_it.h
|   +-- Src/                   main.c, stm32g4xx_it.c, stm32g4xx_hal_msp.c
|
+-- Drivers/                    ST HAL library (do not modify)
+-- Middlewares/                 FreeRTOS (do not modify)
+-- High_VoltageTeam.ioc        CubeMX project file
```

## Architecture Layers

```
+---------------------------------------------+
|  Tasks (RTOS threads)                       |  Application logic
|  sensor_task, can_tx_task, can_rx_task,     |
|  brake_light_task                           |
+---------------------+-----------------------+
                      |
+---------------------v-----------------------+
|  Drivers (Business Logic)                   |  Pure calculations
|  sensors.c - pressure, steering, RPM filter |
+---------------------+-----------------------+
                      |
+---------------------v-----------------------+
|  HAL (Hardware Abstraction)                 |  Hardware access
|  adc_hal, can_hal, tim_hal                  |
+---------------------+-----------------------+
                      |
+---------------------v-----------------------+
|  STM32 HAL + FreeRTOS                       |  Vendor code
+---------------------------------------------+
```

## Data Flow

```
ADC Hardware --DMA--> adc_hal --queue--> sensor_task --mutex--> shared data
                                                                    |
                                                              can_tx_task
                                                                    |
                                                         can_hal --> CAN Bus

CAN Bus --> can_hal --ISR--> brake queue --> brake_light_task --> GPIO (PA8)
                   --ISR--> heartbeat flag --> can_rx_task --> LED (PB6)
```

## Thread Safety

| Shared Resource | Protection | Producer | Consumer |
|----------------|------------|----------|----------|
| ADC sensor data | osMessageQueue (4 slots) | DMA ISR (adc_hal) | SensorTask |
| Processed sensors | osMutex | SensorTask | CanTxTask |
| Brake command | osMessageQueue (1 slot) | CAN RX ISR (can_hal) | BrakeLightTask |
| heartbeatActive flag | volatile bool | CAN RX ISR | CanRxTask |

## Build Configuration

### STM32CubeIDE Include Paths
```
${workspace_loc:/${ProjName}/App/Inc}
${workspace_loc:/${ProjName}/App/Inc/HAL}
${workspace_loc:/${ProjName}/App/Inc/Tasks}
${workspace_loc:/${ProjName}/App/Inc/Drivers}
```

### Source Entries in .cproject
```
App
Core
Drivers
Middlewares
```

## Adding New Code

### New Sensor
1. Add ADC channel in CubeMX / main.c MX_ADC1_Init
2. Update `ADC_NUMBER_OF_CHANNELS` in app_config.h
3. Add processing in sensor_task.c
4. Add field to ProcessedSensorData_t in data_types.h
5. Pack into CAN frame in can_tx_task.c

### New CAN Message
1. Define CAN ID in app_config.h
2. Add TX logic in can_tx_task.c
3. Or add RX handler in HAL_FDCAN_RxFifo0Callback (can_hal.c)
4. Update FDCAN filter in main.c MX_FDCAN1_Init if needed

### New RTOS Task
1. Create header in App/Inc/Tasks/
2. Create source in App/Src/Tasks/
3. Add XxxTask_Init() call in app_init.c App_Init()
4. Verify heap has room (check FreeRTOSConfig.h configTOTAL_HEAP_SIZE)

---

**Version:** 2.1.0
**Date:** February 7, 2026
