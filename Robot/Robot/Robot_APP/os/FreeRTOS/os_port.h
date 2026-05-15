/******************************************************************************
 * @brief    通用os接口
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-16     Morro        Initial version. 
 ******************************************************************************/
#ifndef _OS_PORT_H_
#define _OS_PORT_H_

#include "freertos.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>

typedef QueueHandle_t os_sem_t;                           /*信号量定义*/

typedef TaskHandle_t  os_task_t;

/*
 * @brief       创建任务
 * @param[in]   entry       - 入口函数
 * @param[in]   name        - 任务名称
 * @param[in]   stack_size  - 栈大小
 * @param[in]   prority     - 优先级
 * @param[out]  t           - 用于存储任务句柄,如果不需要填NULL
 * @param[out]  task_params - 任务参数
 */
static inline bool os_task_create(void (*entry)(void *), const char *name, 
                                  int stack_size, int prority, os_task_t *t, 
                                  void *task_params)
{
    return pdPASS == xTaskCreate(entry, name, stack_size, task_params, prority, t);   
}



/*
 * @brief 启动内核运行
 */
static inline void os_start_kernel(void)
{
    /* Start the scheduler. */
	vTaskStartScheduler();    
}

/*
 * @brief	   获取当前系统毫秒数
 */
static inline unsigned int os_get_ms(void)
{
    return xTaskGetTickCount();
}

/*
 * @brief	   超时判断
 * @retval     true | false
 */
static inline bool os_istimeout(unsigned int start_time, unsigned int timeout)
{
    return os_get_ms() - start_time > timeout;
}

/*
 * @brief	   毫秒延时
 * @retval     none
 */
static inline void os_delay(unsigned int ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
/*
 * @brief	   初始化信号量
 * @retval     none
 */
static inline os_sem_t os_sem_new(int value)
{
    return xQueueCreateCountingSemaphore(100, value);
}
/*
 * @brief	   获取信号量
 * @retval     none
 */
static inline bool os_sem_wait(os_sem_t s, unsigned int timeout)
{
    return xQueueSemaphoreTake(s, timeout) == pdTRUE;
}

/*
 * @brief	   释放信号量
 * @retval     none
 */  
static inline void os_sem_post(os_sem_t s)
{
    xQueueGenericSend(s, NULL, 0, queueSEND_TO_BACK );
}

/*
 * @brief	   释放信号量
 * @retval     none
 */  
static inline void os_sem_free(os_sem_t s)
{
    //vSemaphoreDelete(s);
}

/*
 * @brief	   进入临界区
 * @retval     none
 */  
static inline void os_enter_critical(void)
{
    portTICK_TYPE_ENTER_CRITICAL();
}

/*
 * @brief	   退入临界区
 * @retval     none
 */  
static inline void os_exit_critical(void)
{
   portTICK_TYPE_EXIT_CRITICAL();
}

/*
 * @brief	   内存分配
 */  
static inline void *os_mem_malloc(int nbytes)
{
    return pvPortMalloc(nbytes);
}
/*
 * @brief	   内存释放
 */  
static inline void os_mem_free(void *p)
{
    vPortFree(p);
}

#endif
