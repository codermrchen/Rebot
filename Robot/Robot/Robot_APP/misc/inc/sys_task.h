/******************************************************************************
 * @brief    任务管理器
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-03     Morro        Initial version
 ******************************************************************************/
#ifndef _TASK_MGR_H_
#define _TASK_MGR_H_

#include <stdbool.h>
#include "sys_util.h"
#include "comdef.h"

//#pragma section = "HEAP"
#define HEAP_BEGIN      (__section_begin("HEAP"))
#define HEAP_END        (__section_end("HEAP"))

#define __module_initialize(name,func,level)           \
    USED ANONY_TYPE(const init_item_t, init_tbl_##func)\
    SECTION("init.item."level) = {name,func}

/*
 * @brief       模块初始化注册(优先级system_init > driver_init > module_init)
 * @param[in]   name    - 模块名称
 * @param[in]   func    - 初始化入口函数(void func(void){...})
 */
#define system_init(name,func)  __module_initialize(name,func,"1")
#define driver_init(name,func)  __module_initialize(name,func,"2")
#define module_init(name,func)  __module_initialize(name,func,"3")

/*模块初始化项*/
typedef struct {
    const char *name;               //模块名称
    void (*init)(void);             //初始化接口
} init_item_t;

#ifdef SYS_OS_TYPE
#include "os_port.h"

/*
 * @brief       任务定义
 * @param[in]  name        - 任务名称
 * @param[in]  handle       - 任务入口(void func(void *){...})
 * @param[in]  stack_size  - 栈大小
 * @param[in]  prority     - 任务优先级
 */
#define task_define(name, init, handle, interval, stack_size, prority)  \
    static void name##_task(void *params) \
    {                               \
        init();                     \
        while (1) {                 \
            handle();               \
            sys_delay_ms(interval);     \
        }                                               \
    }                                                   \
    USED ANONY_TYPE(const task_item_t, task_item_##name)\
    SECTION("task.item.1") =                            \
        {#name, (void (*)(void *))name##_task, stack_size, prority}

/*任务处理项*/
typedef struct {
    const char *name;               //模组名称
    void (*entry)(void *param);     //任务入口
    unsigned int stack_size;        //栈大小
    int          prority;           //任务优先级
} task_item_t;

#else

/*
 * @brief       任务注册
 * @param[in]   name    - 任务名称
 * @param[in]   handle  - 初始化处理(void func(void){...})
  * @param[in]  interval- 轮询间隔(ms)
 */
#define task_define(name, init, handle, interval, stack_size, prority)                \
    static unsigned int __task_timer_##name;              \
    USED ANONY_TYPE(const task_item_t, task_item_##name)  \
    SECTION("task.item.1") =                              \
    {#name, handle, interval, &__task_timer_##name}

/*任务处理项*/
typedef struct {
    const char *name;               //模块名称
    void (*handle)(void);           //初始化接口
    unsigned int interval;          //轮询间隔
    unsigned int *timer;            //指向定时器指针
} task_item_t;

bool is_timeout(unsigned int start, unsigned int timeout);
void module_task_init(void);
void module_task_process(void);

void sys_delay_ms(unsigned int ms);
#endif

void os_run(void);
#endif
