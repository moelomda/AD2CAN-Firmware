/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
FDCAN_HandleTypeDef hfdcan1;

/* Definitions for defaultTask */

/* USER CODE BEGIN PV */
volatile uint16_t g_adcBuffer[8]; // Buffer used by DMA to store ADC values
osMessageQueueId_t g_adcDataQueueHandle; //Queue to process ADC values
// --- CAN Variables ---
FDCAN_TxHeaderTypeDef g_canTxHeader; // To describe Tx msq
uint8_t g_canTxData[8];             // To Store the data sent
FDCAN_RxHeaderTypeDef g_canRxHeader; // To describe Rx msg
uint8_t g_canRxData[8];             // To store Data Received
osMessageQueueId_t g_brakeCommandQueueHandle; //Queue for brake commands
// --- Shared Temperature Data ---
float g_latestTemperatures[8]; //To store last 8 degrees
osMutexId_t g_temperatureMutexHandle; // Mutex to protect the variable
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_FDCAN1_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void SensorProcessingTask(void *argument);
float Resistance_To_Temperature(float resistance);
void CanTxTask(void *argument);
void CanRxTask(void *argument);
void BrakeLightTask(void *argument);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  const osMutexAttr_t temperatureMutex_attributes = {
    .name = "TempMutex"
  };
  g_temperatureMutexHandle = osMutexNew(&temperatureMutex_attributes);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  const osMessageQueueAttr_t g_adcDataQueue_attributes = {
    .name = "AdcDataQueue"
  };
  g_adcDataQueueHandle = osMessageQueueNew(4, sizeof(uint16_t[8]), &g_adcDataQueue_attributes);
  const osMessageQueueAttr_t brakeCommandQueue_attributes = {
    .name = "BrakeCmdQueue"
  };
  g_brakeCommandQueueHandle = osMessageQueueNew(1, sizeof(uint8_t), &brakeCommandQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  const osThreadAttr_t sensorProcessingTask_attributes = {
    .name = "SensorProcessingTask",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
  };
  osThreadNew(SensorProcessingTask, NULL, &sensorProcessingTask_attributes);
  const osThreadAttr_t canTxTask_attributes = {
    .name = "CanTxTask",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 128 * 4
  };
  osThreadNew(CanTxTask, NULL, &canTxTask_attributes);
  const osThreadAttr_t canRxTask_attributes = {
    .name = "CanRxTask",
    .priority = (osPriority_t) osPriorityHigh,
    .stack_size = 128 * 4
  };
  osThreadNew(CanRxTask, NULL, &canRxTask_attributes);
  const osThreadAttr_t brakeLightTask_attributes = {
    .name = "BrakeLightTask",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 128 * 4
  };
  osThreadNew(BrakeLightTask, NULL, &brakeLightTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adcBuffer, 8) != HAL_OK)
    {
        Error_Handler(); // خطأ في بدء الـ ADC DMA
    }
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 8;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = ADC_REGULAR_RANK_7;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_8;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 4;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 13;
  hfdcan1.Init.NominalTimeSeg2 = 2;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.StdFiltersNbr = 0;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */
    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType = FDCAN_STANDARD_ID;         // Standard ID filter
    sFilterConfig.FilterIndex = 0;                    // Use filter index 0
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;     // Use ID + Mask type
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // Store accepted messages in FIFO 0
    sFilterConfig.FilterID1 = 0x200;                  // The ID we want to receive (Brake command)
    sFilterConfig.FilterID2 = 0x7FF;                  // Mask: 0x7FF means all bits must match for Standard ID

    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
      Error_Handler(); // Filter configuration error
    }

    // --- Activate Reception Interrupt Notification ---
    // Tell HAL to call HAL_FDCAN_RxFifo0Callback when a message arrives in FIFO 0
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
      Error_Handler(); // Notification activation error
    }
  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_GREEN_Pin|LED_RED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BL_ENABLE_Pin */
  GPIO_InitStruct.Pin = BL_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BL_ENABLE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_GREEN_Pin LED_RED_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin|LED_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief يحول قراءة الـ ADC الأولية (0-4095) إلى جهد.
 */
float AdcValue_To_Voltage(int adc_value) {
    const float VREF = 3.3;
    const float MAX_ADC_VALUE = 4095.0;

    if (adc_value < 0) adc_value = 0;
    if (adc_value > 4095) adc_value = 4095;

    return (adc_value / MAX_ADC_VALUE) * VREF;
}

/**
 * @brief يحول الجهد إلى مقاومة (بافتراض R15 = 47k)
 */
float Voltage_To_Resistance(float voltage) {
    const float V_SOURCE = 3.3;
    const float R_FIXED = 47000.0;

    if (voltage >= V_SOURCE) {
        return INFINITY;
    }
    if (voltage <= 0) {
        return 0.0;
    }

    // معادلة مقسم الجهد
    return (voltage * R_FIXED) / (V_SOURCE - voltage);
}
/**
  * @brief مهمة معالجة بيانات الحساسات
  */
void SensorProcessingTask(void *argument)
{
  uint16_t received_data[8];
  float voltage;
  float resistance;
  float current_temperatures[8];

  for(;;)
  {
    if (osMessageQueueGet(g_adcDataQueueHandle, &received_data, NULL, osWaitForever) == osOK)
    {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);

        // حساب كل الـ 8 درجات حرارة وتخزينها مؤقتاً
        // !!! انتبه لترتيب الـ Ranks الصحيح !!!
        // Rank 1 -> CH1 -> received_data[0] -> Temp[0] (AN_IN_1)
        // Rank 2 -> CH2 -> received_data[1] -> Temp[1] (AN_IN_2)
        // Rank 3 -> CH3 -> received_data[2] -> Temp[2] (AN_IN_3)
        // Rank 4 -> CH5 -> received_data[3] -> Temp[4] (AN_IN_5) // لاحظ الفهرس!
        // Rank 5 -> CH11-> received_data[4] -> Temp[5] (AN_IN_6) // لاحظ الفهرس!
        // Rank 6 -> CH12-> received_data[5] -> Temp[6] (AN_IN_7) // لاحظ الفهرس!
        // Rank 7 -> CH14-> received_data[6] -> Temp[7] (AN_IN_8) // لاحظ الفهرس!
        // Rank 8 -> CH15-> received_data[7] -> Temp[3] (AN_IN_4) // لاحظ الفهرس!

        // (مثال لتبسيط الكود، يجب تطبيق هذا على كل الـ 8)
        for (int i = 0; i < 8; i++) {
            voltage = AdcValue_To_Voltage(received_data[i]);
            resistance = Voltage_To_Resistance(voltage);
            // (مؤقتاً سنضع قيمة وهمية، يجب استبدالها بدالة الحرارة الصحيحة وترتيبها الصحيح)
            current_temperatures[i] = Resistance_To_Temperature(resistance);
        }

        // إعادة ترتيب النتائج لتطابق AN_IN_1..8
        float ordered_temps[8];
        ordered_temps[0] = current_temperatures[0]; // AN_IN_1 from Rank 1
        ordered_temps[1] = current_temperatures[1]; // AN_IN_2 from Rank 2
        ordered_temps[2] = current_temperatures[2]; // AN_IN_3 from Rank 3
        ordered_temps[3] = current_temperatures[7]; // AN_IN_4 from Rank 8
        ordered_temps[4] = current_temperatures[3]; // AN_IN_5 from Rank 4
        ordered_temps[5] = current_temperatures[4]; // AN_IN_6 from Rank 5
        ordered_temps[6] = current_temperatures[5]; // AN_IN_7 from Rank 6
        ordered_temps[7] = current_temperatures[6]; // AN_IN_8 from Rank 7


        // تحديث المتغير العام بأمان
        if (osMutexAcquire(g_temperatureMutexHandle, osWaitForever) == osOK) {
            memcpy(g_latestTemperatures, ordered_temps, sizeof(g_latestTemperatures));
            osMutexRelease(g_temperatureMutexHandle);
        }
    }
  }
}

/**
  * @brief  Conversion complete callback in non-blocking mode
  * @param  hadc: ADC handle
  * @retval None
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

	    uint16_t snapshot_buffer[8];
	    memcpy(snapshot_buffer, (const void*)g_adcBuffer, sizeof(snapshot_buffer)); // (أضفنا const void* للأمان مع volatile)

	    // التحقق من حالة الـ Queue
	    osStatus_t queue_status = osMessageQueuePut(g_adcDataQueueHandle, &snapshot_buffer, 0, 0);

	    if (queue_status != osOK)
	    {
	        // حدث خطأ (الـ Queue ممتلئ)
	        // (لأغراض الاختبار، سنقوم بتبديل الليد الأحمر)
	        HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
	    }
	}
}
/**
 * @brief يحول مقاومة الـ NTC (بالأوم) إلى درجة حرارة (بالدرجة المئوية).
 * يستخدم معادلة Steinhart-Hart بمعاملات افتراضية لحساس 10k NTC.
 * !!! يجب تحديث المعاملات A, B, C لتطابق الحساس الفعلي !!!
 */
float Resistance_To_Temperature(float resistance) {
    // معاملات Steinhart-Hart (افتراضية لحساس 10k NTC شائع)
    const float A = 0.001129148f;
    const float B = 0.000234125f;
    const float C = 0.0000000876741f;

    // حساب لوغاريتم المقاومة
    float log_resistance = logf(resistance); // logf هي log لـ float

    // تطبيق معادلة Steinhart-Hart لحساب مقلوب الحرارة بالكلفن
    float inv_kelvin = A + (B * log_resistance) + (C * powf(log_resistance, 3)); // powf لرفع الأس 3

    // حساب الحرارة بالكلفن
    float kelvin = 1.0f / inv_kelvin;

    // تحويل من كلفن إلى درجة مئوية
    float celsius = kelvin - 273.15f;

    return celsius;
}
void CanTxTask(void *argument)
{
  // --- تهيئة رأس رسالة الإرسال (مرة واحدة) ---
  g_canTxHeader.Identifier = 0x100; // ID الإرسال (افتراضنا)
  g_canTxHeader.IdType = FDCAN_STANDARD_ID;
  g_canTxHeader.TxFrameType = FDCAN_DATA_FRAME;
  g_canTxHeader.DataLength = FDCAN_DLC_BYTES_8; // سنرسل 8 بايت
  g_canTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  g_canTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  g_canTxHeader.FDFormat = FDCAN_CLASSIC_CAN; // ليس CAN FD
  g_canTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  g_canTxHeader.MessageMarker = 0;

  // --- بدء تشغيل طرفية الـ CAN ---
  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
      Error_Handler(); // خطأ في بدء الـ CAN
  }

  float local_temps[8]; // متغير محلي لنسخ الحرارة

  /* Infinite loop */
  for(;;)
  {
    // الانتظار لمدة 100 ميللي ثانية
    osDelay(100);

    // 1. قراءة آخر درجات حرارة بأمان
    if (osMutexAcquire(g_temperatureMutexHandle, 10) == osOK) { // ننتظر 10ms فقط
        memcpy(local_temps, g_latestTemperatures, sizeof(local_temps));
        osMutexRelease(g_temperatureMutexHandle);

        // 2. تجهيز بيانات الإرسال (g_canTxData)
        //    (هنا تحتاج لتحديد كيفية تحويل الـ float إلى uint8_t)
        //    مثال بسيط: إرسال أول 4 درجات كـ float (كل واحدة 2 بايت)
        //    !!! هذا يعتمد على البروتوكول المتفق عليه !!!
        //    (للتوضيح، سنرسل قيم وهمية الآن)
        for(int i=0; i<8; i++) {
            // تحويل درجة الحرارة (مثلاً 25.5) إلى قيمة مناسبة للإرسال
            // طريقة شائعة: ضرب في 10 وإرسال كـ integer (255)
            int16_t temp_to_send = (int16_t)(local_temps[i] * 10.0f);
            // وضع البايتين في مصفوفة الإرسال
            // (هذا مثال، ترتيب البايتات (Endianness) مهم)
            if (i < 4) { // نرسل أول 4 حساسات فقط كمثال
               g_canTxData[i*2]   = (uint8_t)(temp_to_send >> 8); // MSB
               g_canTxData[i*2+1] = (uint8_t)(temp_to_send & 0xFF); // LSB
            }
        }
        g_canTxHeader.DataLength = FDCAN_DLC_BYTES_8; // نرسل 8 بايت (4 درجات * 2 بايت)

        // 3. إرسال رسالة الـ CAN
        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_canTxHeader, g_canTxData) != HAL_OK)
        {
            // خطأ في الإرسال (قد يكون الـ FIFO ممتلئ)
            // (يمكن تبديل الليد الأحمر هنا كإشارة خطأ)
             HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
        }
    } else {
        // لم نتمكن من الحصول على الـ Mutex (قد تكون مهمة المعالجة مشغولة)
        // تجاهل الإرسال هذه المرة
    }
  }
}
/**
  * @brief  Rx FIFO 0 New Message callback.
  * @param  hfdcan: FDCAN handle
  * @param  RxFifo0ITs: FIFO 0 Interrupt sources enabled.
  * @retval None
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    // Retrieve the received message from RX FIFO 0
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &g_canRxHeader, g_canRxData) == HAL_OK)
    {
      // Check if the received ID matches our expected brake command ID
      if (g_canRxHeader.Identifier == 0x200)
      {
        // Assuming the command (0=OFF, 1=ON) is in the first data byte
        uint8_t brake_command = g_canRxData[0];

        // Send the command to the BrakeLightTask via queue (from ISR context)
        osMessageQueuePut(g_brakeCommandQueueHandle, &brake_command, 0, 0);
      }
    }
    /* Clear the RX FIFO 0 new message flag */
    // Note: HAL_FDCAN_GetRxMessage should clear the flag, but double-check HAL version specifics if issues arise.
  }
}
/**
  * @brief Task for receiving and routing CAN messages.
  */
void CanRxTask(void *argument)
{
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000); // Placeholder delay - tasks shouldn't block like this normally
  }
}

/**
  * @brief Task for controlling the brake light GPIO.
  */
void BrakeLightTask(void *argument)
{
  uint8_t received_command; // Variable to store the command from the queue

  /* Infinite loop */
  for(;;)
  {
    // Wait forever (block) until a command arrives in the brake command queue
    if (osMessageQueueGet(g_brakeCommandQueueHandle, &received_command, NULL, osWaitForever) == osOK)
    {
      // Check the command value
      if (received_command == 1) // Assuming 1 means ON
      {
        // Turn the brake light ON (Set PA8 High)
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_SET);
      }
      else // Assuming any other value (like 0) means OFF
      {
        // Turn the brake light OFF (Set PA8 Low)
        HAL_GPIO_WritePin(BL_ENABLE_GPIO_Port, BL_ENABLE_Pin, GPIO_PIN_RESET);
      }
    }
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
