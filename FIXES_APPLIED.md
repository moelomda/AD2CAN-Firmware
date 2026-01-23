# 🔧 CRITICAL FIXES APPLIED - ADC Queue & Task Issues

## Date: January 23, 2026
## Author: Claude (Debugging Assistant)

---

## 📋 SUMMARY OF PROBLEMS FOUND

### **Problem 1: ADC Callback Triggering Too Early**
- **Issue**: In continuous mode with DMA, the callback was firing after EACH conversion instead of after ALL 6 channels
- **Symptom**: Green LED blinking rapidly (4-5 times), queue getting partial/garbage data
- **Root Cause**: Missing conversion counter in `HAL_ADC_ConvCpltCallback()`

### **Problem 2: Race Condition in Initialization**
- **Issue**: ADC was started INSIDE the sensor task, causing timing issues
- **Symptom**: Task waiting on empty queue forever
- **Root Cause**: `HW_StartADC()` called after scheduler started

### **Problem 3: FDCAN Not Started Early Enough**
- **Issue**: FDCAN peripheral started inside task instead of during init
- **Symptom**: CAN RX interrupts not working properly
- **Root Cause**: `HAL_FDCAN_Start()` called too late

---

## ✅ CHANGES MADE

### **File 1: `App/Source/ad2can_hw.c`**

#### Change: Fixed ADC Callback to Wait for All 6 Channels

```c
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    static uint8_t first_read_skipped = 0;
    static uint8_t conversion_count = 0;  // ← ADDED
    
    if (hadc->Instance == ADC1)
    {
        if (first_read_skipped == 0) {
            first_read_skipped = 1;
            conversion_count = 0;  // ← ADDED
            return;
        }

        // CRITICAL FIX: Only process after ALL 6 channels complete
        conversion_count++;  // ← ADDED
        
        if (conversion_count < ADC_NUMBER_OF_CHANNELS) {  // ← ADDED
            return;  // Wait for next callback
        }
        
        conversion_count = 0;  // ← ADDED: Reset for next sequence
        
        // Now queue the complete data...
        osMessageQueuePut(g_adcDataQueueHandle, (const void*)g_adcBuffer, 0, 0);
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
}
```

**What this does**: Ensures the callback only queues data after ALL 6 ADC channels have been read, preventing partial/garbage data from entering the queue.

---

### **File 2: `App/Source/ad2can.c`**

#### Change: Removed HW_StartADC() from Task

```c
void SensorProcessingTask(void *argument)
{
    uint16_t received_adc_data[ADC_NUMBER_OF_CHANNELS];
    ProcessedADC_t local_sensor_data;
    osMessageQueueId_t adcQueue = HW_GetAdcQueueHandle();

    // REMOVED: HW_StartADC();  ← This was causing the race condition
    
    for(;;)
    {
        if (osMessageQueueGet(adcQueue, received_adc_data, NULL, osWaitForever) == osOK)
        {
            // Process data...
        }
    }
}
```

**What this does**: Prevents race condition where task starts before ADC is initialized.

---

### **File 3: `Core/Src/main.c`**

#### Change: Start Hardware BEFORE Scheduler

```c
/* USER CODE BEGIN RTOS_EVENTS */

// 1. Create all RTOS objects first
HW_Init_ADC_RTOS();    // Creates ADC Queue
CAN_Init_RTOS();       // Creates CAN Tasks & Queue
App_Init_RTOS();       // Creates App Task & Mutex

// 2. Start hardware peripherals (CRITICAL ORDER!)
if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {  // ← ADDED
    Error_Handler();
}

HW_StartADC();  // ← ADDED: Start ADC+DMA before scheduler

/* USER CODE END RTOS_EVENTS */

// 3. Now start scheduler
osKernelStart();
```

**What this does**: Ensures ADC is running and filling the queue BEFORE tasks start waiting on it.

---

### **File 4: `App/Source/ad2can_can.c`**

#### Change: Removed Duplicate FDCAN Start

```c
void CanTxTask(void *argument)
{
    // Setup TX header...
    
    // REMOVED: HAL_FDCAN_Start(&hfdcan1);  ← Duplicate removed
    
    // Send startup message
    g_canTxHeader.Identifier = CAN_ID_AD2CAN_STARTUP;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData);
    
    for(;;) {
        // CAN TX loop...
    }
}
```

**What this does**: Removes duplicate FDCAN start (now done in main.c).

---

## 🎯 BREAKPOINT LOCATIONS FOR DEBUGGING

Place breakpoints at these exact locations to verify the fixes:

### **Breakpoint #1: ADC Callback Entry**
- **File**: `App/Source/ad2can_hw.c`
- **Function**: `HAL_ADC_ConvCpltCallback`
- **Line**: `if (hadc->Instance == ADC1)` (first line inside function)
- **Purpose**: Verify callback is being called
- **Expected**: Should hit 6 times, then queue data on the 6th hit

### **Breakpoint #2: Conversion Counter Check**
- **File**: `App/Source/ad2can_hw.c`
- **Function**: `HAL_ADC_ConvCpltCallback`
- **Line**: `if (conversion_count < ADC_NUMBER_OF_CHANNELS)`
- **Purpose**: Watch the counter increment from 1 to 6
- **Expected**: `conversion_count` should be 1, 2, 3, 4, 5, then 6 (triggers queue)

### **Breakpoint #3: Queue Put Operation**
- **File**: `App/Source/ad2can_hw.c`
- **Function**: `HAL_ADC_ConvCpltCallback`
- **Line**: `osStatus_t queue_status = osMessageQueuePut(...)`
- **Purpose**: Verify data is being queued
- **Expected**: Should hit once every 6 callbacks, `queue_status` should be `osOK`
- **Watch Variables**: `g_adcBuffer[0]` through `g_adcBuffer[5]` (should contain valid ADC readings ~400-4095)

### **Breakpoint #4: Sensor Task Unblocks**
- **File**: `App/Source/ad2can.c`
- **Function**: `SensorProcessingTask`
- **Line**: `if (osMessageQueueGet(adcQueue, received_adc_data, NULL, osWaitForever) == osOK)`
- **Purpose**: Verify task receives data from queue
- **Expected**: Should unblock shortly after ADC callback queues data
- **Watch Variables**: `received_adc_data[0]` through `received_adc_data[5]`

### **Breakpoint #5: Mutex Acquire in CAN Task**
- **File**: `App/Source/ad2can_can.c`
- **Function**: `CanTxTask`
- **Line**: `if (osMutexAcquire(g_sensorDataMutexHandle, 10) == osOK)`
- **Purpose**: Verify processed data reaches CAN transmission
- **Expected**: Should acquire mutex and copy `g_sensorData` to `local_sensor_data`
- **Watch Variables**: `local_sensor_data.brake_pressure_front`, `local_sensor_data.apps1`, etc.

---

## 🧪 TESTING PROCEDURE

### **Step 1: Verify ADC Callback Frequency**
1. Set Breakpoint #2
2. Run program
3. Count how many times it hits before queuing data
4. **Expected**: Should hit exactly 6 times before data is queued

### **Step 2: Verify Queue Data Quality**
1. Set Breakpoint #3
2. When it hits, inspect `g_adcBuffer[]` in Watch window
3. **Expected**: All 6 values should be in range 0-4095 (not zeros, not 0xFFFF)

### **Step 3: Verify Task Execution**
1. Set Breakpoint #4
2. **Expected**: Task should unblock within ~10ms after ADC callback
3. Inspect `received_adc_data[]` - should match `g_adcBuffer[]` from callback

### **Step 4: Verify End-to-End Flow**
1. Set Breakpoint #5
2. **Expected**: CAN task should receive processed sensor data
3. Verify `local_sensor_data` contains calculated values (not raw ADC)

---

## 📊 EXPECTED LED BEHAVIOR AFTER FIXES

### **Green LED**
- **Before Fix**: Blinking rapidly (4-5 times per sequence)
- **After Fix**: Blinking at regular intervals (~10ms or slower)
- **Meaning**: One blink = One complete 6-channel ADC conversion queued successfully

### **Red LED**
- **Before Fix**: Blinking (queue full errors)
- **After Fix**: Should NOT blink during normal operation
- **Meaning**: If blinking, queue is full (increase queue size or processing speed)

---

## ⚠️ IMPORTANT NOTES

### **ADC Configuration Requirements**
Your ADC is configured correctly in CubeMX with:
- `EOCSelection = ADC_EOC_SEQ_CONV` ✅
- `ContinuousConvMode = ENABLE` ✅
- `DMAContinuousRequests = ENABLE` ✅
- `NbrOfConversion = 6` ✅

**DO NOT CHANGE THESE** - the fix compensates for the callback behavior in software.

### **Queue Size**
Current queue size is **4 items**, each holding 6x uint16_t values. If you see red LED blinking:
```c
// In ad2can_hw.c - HW_Init_ADC_RTOS()
g_adcDataQueueHandle = osMessageQueueNew(4, ...);  // Try increasing to 8 or 16
```

### **Why Continuous Mode?**
Keeping continuous mode means:
- ADC runs automatically without software triggers
- Lower CPU overhead
- More consistent sampling rate
- The conversion counter fix handles the callback correctly

---

## 🔍 DEBUGGING CHECKLIST

- [ ] Green LED blinks at consistent rate (~10ms intervals)
- [ ] Red LED does NOT blink
- [ ] Breakpoint #1 hits continuously
- [ ] Breakpoint #2 shows counter going 1→2→3→4→5→6
- [ ] Breakpoint #3 shows valid ADC data (not zeros)
- [ ] Breakpoint #4 shows task unblocking and receiving data
- [ ] Breakpoint #5 shows CAN task getting processed data
- [ ] `g_sensorData.brake_pressure_front` has realistic values
- [ ] `g_sensorData.apps1` and `apps2` change when you move sensors

---

## 📞 NEXT STEPS IF STILL NOT WORKING

If the queue still has no data after these fixes:

1. **Check DMA Configuration**:
   - Open CubeMX → ADC1 → DMA Settings
   - Verify DMA1 Channel 1 is enabled
   - Direction: Peripheral to Memory
   - Mode: Circular

2. **Check Clock Configuration**:
   - Verify ADC clock is enabled
   - Check if ADC prescaler is too slow

3. **Add Debug Counters**:
   ```c
   // In HAL_ADC_ConvCpltCallback
   static uint32_t total_callbacks = 0;
   static uint32_t total_queued = 0;
   
   total_callbacks++;
   if (conversion_count == 0) {
       total_queued++;
   }
   // Set breakpoint here and inspect counters
   ```

---

## ✅ SUCCESS CRITERIA

You'll know the fixes worked when:
1. **Green LED** blinks steadily (not rapidly)
2. **Sensor Processing Task** enters its main loop
3. **CAN Task** transmits sensor data on the bus
4. **No Red LED** blinking (no queue errors)
5. **Breakpoint #4** hits repeatedly (task receiving data)

---

**Good luck! Your life depends on this working, and now it should! 💪**

---

*Generated by: Claude (Anthropic)*  
*Date: January 23, 2026*
