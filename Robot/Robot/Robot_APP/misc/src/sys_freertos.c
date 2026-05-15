/******************************************************************************
 * @brief    sys Free-RTOS 移植接口
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-23     Morro        Initial version.
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "sys_util.h"
#include "platform.h"

extern __weak void vPortSetupTimerInterrupt( void );

static  uint8_t s_is_inited = 0;

void sys_init_set(uint8_t isInited)
{
    s_is_inited = isInited;
}

/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void sys_delay_ms(unsigned int ms)
{
    if (0 != s_is_inited) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
    else {
        unsigned int i = 0;

        for (i = 0; ms > i; i++) {
            i = i;
        }
    }
}

// SystemCoreClock为系统时钟(system_stmf4xx.c中)，通常选择该时钟作为
// systick定时器时钟，根据具体情况更改
void sys_delay_us(uint32_t us)
{
    u32 told, tnow, reload, tcnt = 0;
    u32 ticks;

    if (0 == (0x0001 & (SysTick->CTRL))) { //定时器未工作
        vPortSetupTimerInterrupt(); //初始化定时器
    }

    told = SysTick->VAL;                        //获取当前数值寄存器值（开始时数值）
    reload = SysTick->LOAD;                     //获取重装载寄存器值
    ticks = us * (SystemCoreClock / 1000000);  //计数时间值
    while (1) {
      tnow = SysTick->VAL;     //获取当前数值寄存器值
      if (tnow != told) {      //当前值不等于开始值说明已在计数
         if( tnow < told) {    //当前值小于开始数值，说明未计到0
            tcnt += told - tnow; //计数值=开始值-当前值
         }
         else { //当前值大于开始数值，说明已计到0并重新计数
            tcnt += reload - tnow + told; //计数值=重装载值-当前值+开始值  （已从开始值计到0）
         }

         told = tnow;         //更新开始值
         if (tcnt >= ticks) { //时间超过/等于要延迟的时间,则退出.
            break;
         }
      }
    }
}

extern void time_inc_auto(void);

/**
 * @brief	   获取当前系统毫秒数
 */
unsigned int sys_get_ms(void)
{
	unsigned int ulTick;
    ulTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
	return ulTick;
}

unsigned int sys_get_tick(void)
{
    return xTaskGetTickCount();
}

/**
 * @brief	   创建信号量
 * @params[in] value    - 初始值
 * @return     指向一新信号量的指针
 */
sys_sem_t sys_sem_new(int value)
{
    return xQueueCreateCountingSemaphore(100, value);
}

/**
 * @brief	   等待信号量
 * @params[in] s       - 信号量
 * @params[in] timeout - 等待超时时间(ms为单位)
 * @return     true - 成功获取到信号量, false - 等待超时
 * @note       如果"timeout"参数非零，线程应该仅在指定的时间内阻塞(以单位为毫秒)。
 *             如果"timeout"参数为零,则任务应该一直阻塞,直到信号量发出信号。
 */
bool sys_sem_wait(sys_sem_t s, unsigned int timeout)
{
    return xQueueSemaphoreTake(s, 0 == timeout ? portMAX_DELAY : timeout) == pdTRUE;
}

/**
 * @brief	   发送信号量
 * @params[in] s      - 信号量
 * @return     none
 */
void sys_sem_post(sys_sem_t s)
{
    xQueueGenericSend(s, NULL, 0, queueSEND_TO_BACK );
}

///**
// * @brief	   释放信号量
// * @return     none
// */
void sys_sem_free(sys_sem_t s)
{
    //vSemaphoreDelete(s);
}

/**
 * @brief	   内存分配
 * @params[in] nbytes - 分配字节数
 */
void* sys_malloc(int nbytes)
{
    return pvPortMalloc(nbytes);
}

/**
 * @brief	   内存释放
 * @params[in] p - 待释放指针
 */
void sys_free(void *p)
{
    vPortFree(p);
}

/**
 * @brief	   进入临界区
 * @return     none
 */
void sys_enter_critical(void)
{
    portTICK_TYPE_ENTER_CRITICAL();
}

/*
 * @brief	   退入临界区
 * @return     none
 */
void sys_exit_critical(void)
{
   portTICK_TYPE_EXIT_CRITICAL();
}
