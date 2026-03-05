#include "tim_hal.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim15;

/* Timer clock = 64 MHz (prescaler=0, APB1 timer clock = PCLK1 = 64 MHz), teeth per revolution */
#define TIM_CLOCK_FREQ      64000000UL
#define TEETH_PER_REV       25

/* Timeout: if no pulse for this many ms, RPM = 0 */
#define RPM_TIMEOUT_MS      500

/* Left wheel (TIM15 CH1) */
static volatile uint32_t left_last_capture = 0;
static volatile uint32_t left_period = 0;
volatile uint32_t left_overflow_count = 0;  /* accessed from main.c PeriodElapsedCallback */
static volatile uint32_t left_last_tick = 0;

/* Right wheel (TIM3 CH3) */
static volatile uint32_t right_last_capture = 0;
static volatile uint32_t right_period = 0;
volatile uint32_t right_overflow_count = 0;  /* accessed from main.c PeriodElapsedCallback */
static volatile uint32_t right_last_tick = 0;

void TIM_HAL_Init(void)
{
    /* Start input capture with interrupts */
    HAL_TIM_IC_Start_IT(&htim15, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);

    /* Start base timers for overflow counting */
    HAL_TIM_Base_Start_IT(&htim15);
    HAL_TIM_Base_Start_IT(&htim3);
}

uint16_t TIM_HAL_GetRPM_Left(void)
{
    uint32_t period = left_period;
    uint32_t last = left_last_tick;
    uint32_t now = HAL_GetTick();

    /* Timeout check: if no pulse received recently, RPM = 0 */
    if (period == 0 || (now - last) > RPM_TIMEOUT_MS)
    {
        return 0;
    }

    /* RPM = (timer_freq / period) * 60 / teeth_per_rev */
    uint32_t rpm = (uint32_t)((uint64_t)TIM_CLOCK_FREQ * 60ULL / ((uint64_t)period * TEETH_PER_REV));

    if (rpm > 0xFFFF) return 0xFFFF;
    return (uint16_t)rpm;
}

uint16_t TIM_HAL_GetRPM_Right(void)
{
    uint32_t period = right_period;
    uint32_t last = right_last_tick;
    uint32_t now = HAL_GetTick();

    if (period == 0 || (now - last) > RPM_TIMEOUT_MS)
    {
        return 0;
    }

    uint32_t rpm = (uint32_t)((uint64_t)TIM_CLOCK_FREQ * 60ULL / ((uint64_t)period * TEETH_PER_REV));

    if (rpm > 0xFFFF) return 0xFFFF;
    return (uint16_t)rpm;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        uint32_t overflows = left_overflow_count;
        left_overflow_count = 0;

        /* Calculate period including overflows (16-bit timer, period=65535) */
        left_period = (overflows * 65536UL) + capture - left_last_capture;
        if (capture < left_last_capture && overflows == 0)
        {
            left_period += 65536UL;
        }
        left_last_capture = capture;
        left_last_tick = HAL_GetTick();
    }
    else if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
        uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
        uint32_t overflows = right_overflow_count;
        right_overflow_count = 0;

        right_period = (overflows * 65536UL) + capture - right_last_capture;
        if (capture < right_last_capture && overflows == 0)
        {
            right_period += 65536UL;
        }
        right_last_capture = capture;
        right_last_tick = HAL_GetTick();
    }
}
