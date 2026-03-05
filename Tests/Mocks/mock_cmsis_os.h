/* ============================================================================
 * Mock CMSIS-OS2 / FreeRTOS - Simulates RTOS APIs for PC-based testing
 * ============================================================================
 * Provides message queues, mutexes, threads, and delays that work on PC.
 * Queues use simple circular buffers; mutexes use flags.
 * ============================================================================ */
#ifndef MOCK_CMSIS_OS_H
#define MOCK_CMSIS_OS_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ---- OS Status ---- */
typedef enum {
    osOK                    = 0,
    osError                 = -1,
    osErrorTimeout          = -2,
    osErrorResource         = -3,
    osErrorParameter        = -4,
    osErrorNoMemory         = -5,
    osErrorISR              = -6,
    osStatusReserved        = 0x7FFFFFFF
} osStatus_t;

/* ---- OS Priority ---- */
typedef enum {
    osPriorityNone          = 0,
    osPriorityIdle          = 1,
    osPriorityLow           = 8,
    osPriorityBelowNormal   = 16,
    osPriorityNormal        = 24,
    osPriorityAboveNormal   = 32,
    osPriorityHigh          = 40,
    osPriorityRealtime      = 48
} osPriority_t;

/* ---- Wait defines ---- */
#define osWaitForever  0xFFFFFFFFU

/* ---- Thread ---- */
typedef void (*osThreadFunc_t)(void *argument);

typedef struct {
    const char *name;
    uint32_t attr_bits;
    void *cb_mem;
    uint32_t cb_size;
    void *stack_mem;
    uint32_t stack_size;
    osPriority_t priority;
} osThreadAttr_t;

typedef void* osThreadId_t;

/* ---- Message Queue (simulated with circular buffer) ---- */
#define MOCK_QUEUE_MAX_SIZE    32
#define MOCK_QUEUE_MAX_MSG_SIZE 64

typedef struct MockMessageQueue {
    uint8_t buffer[MOCK_QUEUE_MAX_SIZE][MOCK_QUEUE_MAX_MSG_SIZE];
    uint32_t msg_size;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} MockMessageQueue_t;

typedef struct {
    const char *name;
    uint32_t attr_bits;
    void *cb_mem;
    uint32_t cb_size;
    void *mq_mem;
    uint32_t mq_size;
} osMessageQueueAttr_t;

typedef MockMessageQueue_t* osMessageQueueId_t;

/* ---- Mutex ---- */
typedef struct MockMutex {
    uint8_t locked;
    const char *name;
} MockMutex_t;

typedef struct {
    const char *name;
    uint32_t attr_bits;
    void *cb_mem;
    uint32_t cb_size;
} osMutexAttr_t;

typedef MockMutex_t* osMutexId_t;

/* ---- Mock RTOS Tracking ---- */
#define MOCK_MAX_THREADS    16
#define MOCK_MAX_QUEUES     16
#define MOCK_MAX_MUTEXES    16

typedef struct {
    /* Thread tracking */
    struct {
        osThreadFunc_t func;
        void *argument;
        const char *name;
    } threads[MOCK_MAX_THREADS];
    int thread_count;
    int thread_create_fail; /* Set to 1 to make osThreadNew return NULL */
    
    /* Queue tracking */
    MockMessageQueue_t queues[MOCK_MAX_QUEUES];
    int queue_count;
    int queue_create_fail;
    
    /* Mutex tracking */
    MockMutex_t mutexes[MOCK_MAX_MUTEXES];
    int mutex_count;
    int mutex_create_fail;
    int mutex_acquire_fail;
    
    /* Delay tracking */
    uint32_t total_delay_ms;
    int delay_call_count;
    
    /* Kernel */
    int kernel_initialized;
    int kernel_started;
} MockRTOS_State_t;

extern MockRTOS_State_t mock_rtos;

/* ---- Mock initialization ---- */
static inline void MockRTOS_Reset(void) {
    memset(&mock_rtos, 0, sizeof(MockRTOS_State_t));
}

/* ============================================================================
 * RTOS Function Declarations
 * ============================================================================ */

/* Kernel */
osStatus_t osKernelInitialize(void);
osStatus_t osKernelStart(void);

/* Thread */
osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr);

/* Message Queue */
osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size,
                                      const osMessageQueueAttr_t *attr);
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr,
                              uint8_t msg_prio, uint32_t timeout);
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr,
                              uint8_t *msg_prio, uint32_t timeout);
uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id);

/* Mutex */
osMutexId_t osMutexNew(const osMutexAttr_t *attr);
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
osStatus_t osMutexRelease(osMutexId_t mutex_id);

/* Delay */
osStatus_t osDelay(uint32_t ticks);

#endif /* MOCK_CMSIS_OS_H */
