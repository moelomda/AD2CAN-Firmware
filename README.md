# High Voltage Team - AD2CAN Sensor Node

**FAU Formula Student Race Car - Sensor Acquisition and CAN Communication**

- **MCU:** STM32G473CE (Cortex-M4F, LQFP48)
- **RTOS:** FreeRTOS (CMSIS-RTOS V2)
- **SYSCLK:** 64 MHz (HSE 16 MHz, PLL M=1 N=8 R=2)
- **CAN:** FDCAN1 @ 1 Mbps (Classic CAN)

---

## What This Firmware Does

Reads 5 analog sensors + 2 wheel speed sensors, packs the data into CAN messages, and transmits at 100 Hz. Also receives brake light commands and a heartbeat from the CAN master.

---

## Program Execution Flow

### Phase 1: Hardware Initialization (before RTOS)

```
main()
  |
  +-- HAL_Init()                      Reset peripherals, init SysTick
  +-- SystemClock_Config()            HSE 16MHz -> PLL -> 64 MHz SYSCLK
  |
  +-- MX_GPIO_Init()                  PA8=BrakeLight, PB6=LED_GREEN, PB7=LED_RED
  +-- MX_DMA_Init()                   DMA1_CH1 for ADC (circular, halfword)
  +-- MX_ADC1_Init()                  6-channel scan, continuous, DMA, 12-bit
  +-- MX_FDCAN1_Init()                1 Mbps, filter 0x200+0x100, RX FIFO0 notify
  |     +-- NVIC enable FDCAN1_IT0 (priority 6)
  |     +-- Filter: dual ID (0x200 brake, 0x100 heartbeat)
  |     +-- Global filter: reject all others
  |     +-- Activate RX FIFO0 new message notification
  +-- MX_TIM3_Init()                  Input capture CH3 (PB0, right WS), NVIC pri 6
  +-- MX_TIM15_Init()                 Input capture CH1 (PA2, left WS), NVIC pri 6
```

### Phase 2: RTOS Object Creation

```
  +-- osKernelInitialize()
  +-- Create defaultTask (required by CubeMX, idle loop)
  |
  +-- App_Init()                      (app_init.c - creates all RTOS objects)
  |     +-- ADC_HAL_Init()            Create ADC data queue (4 slots x 12 bytes)
  |     +-- CAN_HAL_Init()            Create brake command queue (1 slot x 1 byte)
  |     |                             Initialize CAN TX header (classic CAN, std ID)
  |     +-- TIM_HAL_Init()            Start TIM15 CH1 + TIM3 CH3 input capture + IT
  |     |                             Start TIM15 + TIM3 base timers for overflow count
  |     +-- SensorTask_Init()         Create mutex + SensorTask thread (2048B stack)
  |     +-- CanTxTask_Init()          Create CanTxTask thread (2048B stack)
  |     +-- CanRxTask_Init()          Create CanRxTask thread (1024B stack)
  |     +-- BrakeLightTask_Init()     Create BrakeLightTask thread (1024B stack)
  |
  +-- App_Start()                     (app_init.c - starts hardware)
  |     +-- HAL_FDCAN_Start()         Enable CAN peripheral (must be after filters)
  |     +-- ADC_HAL_Start()           Calibrate ADC + start DMA continuous scan
```

### Phase 3: Scheduler Running (6 tasks)

```
  +-- osKernelStart()                 <<< RTOS takes over, never returns >>>
```

Once the scheduler starts, these tasks and interrupts run concurrently:

---

## Runtime Task and Interrupt Flow

### ISR: DMA Transfer Complete (ADC)

```
DMA1_Channel1_IRQHandler
  -> HAL_ADC_ConvCpltCallback()       (adc_hal.c)
       |
       +-- Skip first read (ADC settling)
       +-- Post adcBuffer[6] to ADC queue (zero-copy from DMA buffer)
       +-- If queue full: toggle RED LED
```

Fires continuously. DMA circular mode fills `adcBuffer[0..5]` with all 6 ADC channels, then triggers callback.

### ISR: Timer Input Capture (Wheel Speed)

```
TIM1_BRK_TIM15_IRQHandler / TIM3_IRQHandler
  -> HAL_TIM_IC_CaptureCallback()     (tim_hal.c)
       |
       +-- Read captured timer value
       +-- Calculate period = (overflows * 65536) + capture - last_capture
       +-- Store period + timestamp for RPM calculation
```

### ISR: Timer Overflow (RPM timeout support)

```
TIM6_DAC_IRQHandler (also TIM3/TIM15 overflow)
  -> HAL_TIM_PeriodElapsedCallback()  (main.c)
       |
       +-- TIM6:  HAL_IncTick() (RTOS timebase)
       +-- TIM15: left_overflow_count++
       +-- TIM3:  right_overflow_count++
```

### ISR: CAN RX

```
FDCAN1_IT0_IRQHandler
  -> HAL_FDCAN_RxFifo0Callback()      (can_hal.c)
       |
       +-- ID 0x200 (brake): post byte[0] to brake command queue
       +-- ID 0x100 (heartbeat): set LED_GREEN to byte[0], flag active
```

### Task: SensorTask (osPriorityNormal, 2048B stack)

```
SensorTask_Run()                      (sensor_task.c)
  loop forever:
    +-- Wait for ADC data from queue (blocks until available)
    +-- Sensor_CalculatePressure(adc[0])     -> brake_pressure_front
    +-- Sensor_CalculatePressure(adc[1])     -> brake_pressure_rear
    +-- Sensor_CalculateSteeringAngle(adc[2]) -> steering_angle
    +-- apps1 = adc[3] (raw)
    +-- apps2 = adc[4] (raw)
    +-- Lock mutex -> copy to shared sensorData -> release mutex
```

### Task: CanTxTask (osPriorityNormal, 2048B stack)

```
CanTxTask_Run()                       (can_tx_task.c)
  +-- Send startup message 0x700 [0xAA] (once)
  loop forever:
    +-- osDelay(10ms)                        -> 100 Hz cycle
    +-- Lock sensor mutex -> copy sensorData -> release
    +-- Read RPM left/right from TIM_HAL -> apply moving-max filter
    |
    +-- TX 0x510 (8 bytes): APPS1, APPS2, Steering, BrakeFront
    +-- TX 0x511 (7 bytes): RPM_L, RPM_R, BrakeRear, WS_Status
    +-- Every 100ms: TX 0x512 (4 bytes): BrakeRear, BrakeFront
```

### Task: CanRxTask (osPriorityBelowNormal, 1024B stack)

```
CanRxTask_Run()                       (can_rx_task.c)
  loop forever:
    +-- osDelay(1000ms)
    +-- If heartbeat was received this cycle: clear flag (LED set by ISR)
    +-- Else: toggle GREEN LED (local heartbeat indicator)
```

### Task: BrakeLightTask (osPriorityNormal, 1024B stack)

```
BrakeLightTask_Run()                  (brake_light_task.c)
  loop forever:
    +-- Wait for brake command from queue (blocks until received)
    +-- command == 1 ? PA8 HIGH (brake light ON) : PA8 LOW (OFF)
```

### Task: defaultTask (osPriorityNormal, 512B stack)

```
StartDefaultTask()                    (main.c)
  loop forever:
    +-- osDelay(1)                    <- CubeMX required idle task
```

---

## Pin Mapping

| Pin  | Function         | Peripheral   | Description              |
|------|------------------|--------------|--------------------------|
| PF0  | HSE OSC_IN       | RCC          | 16 MHz crystal           |
| PF1  | HSE OSC_OUT      | RCC          | 16 MHz crystal           |
| PA0  | ADC1_IN1 (Rank1) | ADC          | Brake Pressure Front     |
| PA1  | ADC1_IN2 (Rank2) | ADC          | Brake Pressure Rear      |
| PB14 | ADC1_IN5 (Rank3) | ADC          | Steering Angle           |
| PB12 | ADC1_IN11 (Rank4)| ADC          | APPS1                    |
| PB1  | ADC1_IN12 (Rank5)| ADC          | APPS2                    |
| PB11 | ADC1_IN14 (Rank6)| ADC          | Unused (reserved)        |
| PA2  | TIM15_CH1 (AF9)  | Timer IC     | Left Wheel Speed Sensor  |
| PB0  | TIM3_CH3 (AF2)   | Timer IC     | Right Wheel Speed Sensor |
| PA11 | FDCAN1_RX (AF9)  | CAN          | CAN Bus Receive          |
| PA12 | FDCAN1_TX (AF9)  | CAN          | CAN Bus Transmit         |
| PA8  | GPIO Output      | GPIO         | BL_ENABLE (Brake Light)  |
| PB6  | GPIO Output      | GPIO         | LED_GREEN                |
| PB7  | GPIO Output      | GPIO         | LED_RED                  |
| PA13 | SWDIO            | SWD          | Debug                    |
| PA14 | SWCLK            | SWD          | Debug                    |

---

## CAN Messages

### TX Messages (sent by this node)

**0x510** - Sensor Data Frame 1 (8 bytes, every 10ms)
| Byte | Content                    | Type     |
|------|----------------------------|----------|
| 0-1  | APPS1 (raw ADC)            | uint16_t LE |
| 2-3  | APPS2 (raw ADC)            | uint16_t LE |
| 4-5  | Steering Angle (0.1 deg)   | int16_t LE  |
| 6-7  | Brake Pressure Front       | uint16_t LE |

**0x511** - Sensor Data Frame 2 (7 bytes, every 10ms)
| Byte | Content                    | Type     |
|------|----------------------------|----------|
| 0-1  | RPM Left                   | uint16_t LE |
| 2-3  | RPM Right                  | uint16_t LE |
| 4-5  | Brake Pressure Rear        | uint16_t LE |
| 6    | WS Status (bit0=L, bit1=R) | uint8_t  |

**0x512** - DV Sensors Front (4 bytes, every 100ms)
| Byte | Content                    | Type     |
|------|----------------------------|----------|
| 0-1  | Brake Pressure Rear        | uint16_t LE |
| 2-3  | Brake Pressure Front       | uint16_t LE |

**0x700** - Startup Message (1 byte, once at boot)
| Byte | Content | Value |
|------|---------|-------|
| 0    | Magic   | 0xAA  |

### RX Messages (received by this node)

**0x200** - Brake Light Command
| Byte | Content         | Values          |
|------|-----------------|-----------------|
| 0    | Brake command   | 1=ON, 0=OFF     |

**0x100** - Heartbeat from Master
| Byte | Content       | Values            |
|------|---------------|-------------------|
| 0    | LED state     | 1=ON, 0=OFF       |

---

## Sensor Calculations

### Brake Pressure
```
ADC range: 400 - 3610 counts
Output: 0 - 20000 (represents 0.0 - 20.0 bar)
Formula: pressure = (adc - 400) * 20000 / (3610 - 400)
Below 400 or above 3610: returns 0
```

### Steering Angle
```
ADC range: 1736 (full left) - 3561 (full right)
Output: -900 to +900 (represents -90.0 to +90.0 degrees)
Formula: angle = (1800 / (3561 - 1736)) * (adc - 1736) - 900
```

### RPM
```
Timer clock: 64 MHz (prescaler = 0)
Teeth per revolution: 25
Formula: RPM = (64000000 * 60) / (period * 25)
Timeout: 500ms with no pulse -> RPM = 0
Filter: moving-max over 10 samples
```

---

## Clock Tree

```
HSE 16 MHz --> PLL (M=1, N=8, R=2) --> SYSCLK = 64 MHz
                                          |
                                    AHB div1 = 64 MHz
                                     /           \
                               APB1 div1        APB2 div1
                               = 64 MHz         = 64 MHz
                                  |                |
                            FDCAN, TIM3        TIM15
                            ADC (div2=32MHz)
```

## FDCAN Timing (1 Mbps)

```
FDCAN clock = PCLK1 = 64 MHz
Prescaler = 4 -> TQ = 62.5 ns
Sync + BS1 + BS2 = 1 + 12 + 3 = 16 TQ
Bit time = 16 * 62.5 ns = 1000 ns -> 1 Mbps
Sample point = (1 + 12) / 16 = 81.25%
```

---

## FreeRTOS Configuration

| Parameter               | Value  |
|-------------------------|--------|
| Total Heap              | 32768 bytes |
| Tick Rate               | 1000 Hz |
| Max Priorities          | 56     |
| Minimal Stack           | 128 words |
| Timer Task Stack        | 256 words |
| Timebase                | TIM6 (not SysTick) |
| configENABLE_FPU        | 0      |
| MAX_SYSCALL_INT_PRIORITY| 5      |

### Task Stack Allocation

| Task           | Stack  | Priority         |
|----------------|--------|------------------|
| defaultTask    | 512B   | Normal           |
| SensorTask     | 2048B  | Normal           |
| CanTxTask      | 2048B  | Normal           |
| CanRxTask      | 1024B  | BelowNormal      |
| BrakeLightTask | 1024B  | Normal           |
| TimerTask      | 1024B  | (FreeRTOS internal)|
| **Total**      | **7680B** |               |

---

## NVIC Interrupt Priorities

| IRQ                    | Priority | Purpose                |
|------------------------|----------|------------------------|
| DMA1_Channel1          | 6        | ADC DMA transfer       |
| FDCAN1_IT0             | 6        | CAN RX notification    |
| TIM3                   | 6        | Right wheel speed IC   |
| TIM1_BRK_TIM15         | 6        | Left wheel speed IC    |
| TIM6_DAC               | 15       | RTOS timebase (HAL tick)|
| PendSV                 | 15       | RTOS context switch    |
| SysTick                | 15       | RTOS tick              |

All peripheral interrupts use priority 6 (above `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`), meaning they can safely call FreeRTOS `FromISR` API functions.

---

## Project File Structure

```
High_VoltageTeam/
|
+-- App/
|   +-- Inc/
|   |   +-- app_config.h            CAN IDs, sensor constants, timing
|   |   +-- app_init.h              App_Init() / App_Start() prototypes
|   |   +-- data_types.h            ProcessedSensorData_t struct
|   |   +-- HAL/
|   |   |   +-- adc_hal.h           ADC queue, DMA start
|   |   |   +-- can_hal.h           CAN TX/RX, brake queue, heartbeat
|   |   |   +-- tim_hal.h           RPM measurement
|   |   +-- Tasks/
|   |   |   +-- sensor_task.h       Sensor processing task
|   |   |   +-- can_tx_task.h       CAN transmit task
|   |   |   +-- can_rx_task.h       Heartbeat LED task
|   |   |   +-- brake_light_task.h  Brake light control task
|   |   +-- Drivers/
|   |       +-- sensors.h           Pressure/steering/RPM calculations
|   |
|   +-- Src/
|       +-- app_init.c              Central initialization
|       +-- HAL/
|       |   +-- adc_hal.c           ADC DMA + callback + queue
|       |   +-- can_hal.c           CAN TX/RX + callback
|       |   +-- tim_hal.c           Input capture + RPM calc
|       +-- Tasks/
|       |   +-- sensor_task.c       ADC data -> processed sensors
|       |   +-- can_tx_task.c       Pack + transmit CAN frames
|       |   +-- can_rx_task.c       Heartbeat LED logic
|       |   +-- brake_light_task.c  Brake light GPIO control
|       +-- Drivers/
|           +-- sensors.c           Pressure, steering, RPM filter math
|
+-- Core/
|   +-- Inc/
|   |   +-- main.h                  Pin defines, Error_Handler
|   |   +-- stm32g4xx_it.h          IRQ handler prototypes
|   |   +-- FreeRTOSConfig.h        RTOS configuration
|   |   +-- stm32g4xx_hal_conf.h    HAL module enables
|   +-- Src/
|       +-- main.c                  Entry point, peripheral init, App_Init/App_Start
|       +-- stm32g4xx_it.c          IRQ handlers (DMA, TIM3, TIM15, FDCAN1)
|       +-- stm32g4xx_hal_msp.c     GPIO/clock init for peripherals
|       +-- system_stm32g4xx.c      System init
|
+-- Drivers/                        ST HAL library (do not modify)
+-- Middlewares/                     FreeRTOS (do not modify)
+-- High_VoltageTeam.ioc            CubeMX project file
```

---

## Important Warnings

1. **DO NOT regenerate code from CubeMX** without verifying:
   - FDCAN timing (prescaler=4, BS1=12, BS2=3) and AutoRetransmission=ENABLE
   - NVIC for FDCAN1_IT0, TIM3, TIM1_BRK_TIM15 (manually enabled in USER CODE)
   - IRQ handlers in `stm32g4xx_it.c` USER CODE section
   - Timer prescaler = 0 for both TIM3 and TIM15

2. **WS Status pins** (PA2, PB0) are in timer AF mode. Pin state is read via IDR register, not `HAL_GPIO_ReadPin`.

3. **ADC channel 6** (PB11, ADC1_IN14) is scanned but not used. Reserved for future expansion.

4. **FreeRTOS heap** = 32768 bytes. Current usage is ~8400 bytes. If adding tasks, verify heap is sufficient.

---

## Software Test Suite

A full offline test suite validates all firmware logic on PC using mocked STM32 HAL and FreeRTOS.

**Result: 120 / 120 tests PASSED** (March 5, 2026)

| Category          | Tests | Status     |
|-------------------|-------|------------|
| Unit Tests        | 95    | 95 PASS    |
| Integration Tests | 7     | 7 PASS     |
| Acceptance Tests  | 12    | 12 PASS    |

### What's Tested

- **Unit (9 modules):** sensors, adc_hal, can_hal, tim_hal, sensor_task, brake_light_task, can_rx_task, can_tx_task, app_init
- **Integration (6 suites):** ADC→Sensor→CAN pipeline, CAN RX→Brake Light, Heartbeat LED, Full TX cycle, RPM pipeline, Pressure range sweep
- **Acceptance (12 scenarios):** Startup, normal driving, hard braking, full steering lock, high speed, sensor failure, CAN bus failure/recovery, heartbeat loss, rapid brake toggle (100x), race lap simulation (130 cycles), APPS plausibility, wheel speed pin states

### How to Run

```bash
cd Tests
run_tests.bat
```

Full results: see `Tests/TEST_RESULTS.md`

---

## Documentation

| File | Purpose |
|------|---------|
| `README.md` | This file — full project reference |
| `PROJECT_STRUCTURE.md` | Architecture layers, data flow, how to add new code |
| `IDE_CONFIGURATION.md` | STM32CubeIDE include paths and source folder setup |
| `TESTING_GUIDE.md` | Hardware testing guide (on-board with debugger) |
| `Tests/TEST_RESULTS.md` | Software test results (120/120 pass) |

---

## Version History

| Version | Date       | Changes                                    |
|---------|------------|--------------------------------------------|
| 2.2.0   | 2026-03-05 | Added full test suite (120 tests), cleaned up docs |
| 2.1.0   | 2026-02-07 | Fixed .ioc sync, heap size, WS pin read, cleanup |
| 2.0.0   | 2026-01-24 | Complete restructure to modular architecture|
| 1.0.0   | 2025       | Initial monolithic implementation           |

---

**Team:** High Voltage Team - FAU Racing
