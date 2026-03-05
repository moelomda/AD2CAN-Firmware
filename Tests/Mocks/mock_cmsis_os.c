/* ============================================================================
 * Mock CMSIS-OS2 / FreeRTOS - Implementation
 * ============================================================================ */
#include "mock_cmsis_os.h"
#include <string.h>

MockRTOS_State_t mock_rtos;

/* ============================================================================
 * Kernel
 * ============================================================================ */
osStatus_t osKernelInitialize(void)
{
    mock_rtos.kernel_initialized = 1;
    return osOK;
}

osStatus_t osKernelStart(void)
{
    mock_rtos.kernel_started = 1;
    return osOK;
}

/* ============================================================================
 * Thread
 * ============================================================================ */
osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
    if (mock_rtos.thread_create_fail) {
        return NULL;
    }
    
    if (mock_rtos.thread_count >= MOCK_MAX_THREADS) {
        return NULL;
    }
    
    int idx = mock_rtos.thread_count++;
    mock_rtos.threads[idx].func = func;
    mock_rtos.threads[idx].argument = argument;
    mock_rtos.threads[idx].name = (attr && attr->name) ? attr->name : "unnamed";
    
    /* Return a non-NULL thread ID (just the index + 1 cast to pointer) */
    return (osThreadId_t)(uintptr_t)(idx + 1);
}

/* ============================================================================
 * Message Queue 
 * ============================================================================ */
osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size,
                                      const osMessageQueueAttr_t *attr)
{
    if (mock_rtos.queue_create_fail) {
        return NULL;
    }
    
    if (mock_rtos.queue_count >= MOCK_MAX_QUEUES) {
        return NULL;
    }
    
    if (msg_size > MOCK_QUEUE_MAX_MSG_SIZE) {
        return NULL;
    }
    
    MockMessageQueue_t *q = &mock_rtos.queues[mock_rtos.queue_count++];
    memset(q, 0, sizeof(MockMessageQueue_t));
    q->msg_size = msg_size;
    q->capacity = (msg_count < MOCK_QUEUE_MAX_SIZE) ? msg_count : MOCK_QUEUE_MAX_SIZE;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    return q;
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr,
                              uint8_t msg_prio, uint32_t timeout)
{
    if (mq_id == NULL || msg_ptr == NULL) {
        return osErrorParameter;
    }
    
    MockMessageQueue_t *q = (MockMessageQueue_t *)mq_id;
    
    if (q->count >= q->capacity) {
        return osErrorResource;
    }
    
    memcpy(q->buffer[q->tail], msg_ptr, q->msg_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    
    return osOK;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr,
                              uint8_t *msg_prio, uint32_t timeout)
{
    if (mq_id == NULL || msg_ptr == NULL) {
        return osErrorParameter;
    }
    
    MockMessageQueue_t *q = (MockMessageQueue_t *)mq_id;
    
    if (q->count == 0) {
        return osErrorResource; /* Would block, but in test we return error */
    }
    
    memcpy(msg_ptr, q->buffer[q->head], q->msg_size);
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    
    return osOK;
}

uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id)
{
    if (mq_id == NULL) return 0;
    return ((MockMessageQueue_t *)mq_id)->count;
}

/* ============================================================================
 * Mutex
 * ============================================================================ */
osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
    if (mock_rtos.mutex_create_fail) {
        return NULL;
    }
    
    if (mock_rtos.mutex_count >= MOCK_MAX_MUTEXES) {
        return NULL;
    }
    
    MockMutex_t *m = &mock_rtos.mutexes[mock_rtos.mutex_count++];
    m->locked = 0;
    m->name = (attr && attr->name) ? attr->name : "unnamed";
    
    return m;
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
    if (mutex_id == NULL) {
        return osErrorParameter;
    }
    
    if (mock_rtos.mutex_acquire_fail) {
        return osErrorTimeout;
    }
    
    MockMutex_t *m = (MockMutex_t *)mutex_id;
    m->locked = 1;
    return osOK;
}

osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
    if (mutex_id == NULL) {
        return osErrorParameter;
    }
    
    MockMutex_t *m = (MockMutex_t *)mutex_id;
    m->locked = 0;
    return osOK;
}

/* ============================================================================
 * Delay
 * ============================================================================ */
osStatus_t osDelay(uint32_t ticks)
{
    mock_rtos.total_delay_ms += ticks;
    mock_rtos.delay_call_count++;
    return osOK;
}
