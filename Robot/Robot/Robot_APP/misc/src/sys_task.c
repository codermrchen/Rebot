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
#include <stddef.h>
#include "sys_task.h"
#include "platform.h"

#ifdef SYS_OS_TYPE

/*
 * @brief       空处理,用于定位段入口
 */
static void nop_process(void) {}


const init_item_t init_tbl_start SECTION("init.item.0") = {
    "", nop_process
};

const init_item_t init_tbl_end SECTION("init.item.4") = {
    "", nop_process
};


const task_item_t task_tbl_start SECTION("task.item.0") = {
    0
};

const task_item_t task_tbl_end SECTION("task.item.2") = {
    0
};

/*
 * @brief       模组初始化
 * @param[in]   none
 * @return      none
 */
static void init_items(void)
{
    const init_item_t *it = &init_tbl_start;
    while (it < &init_tbl_end) {
        it++->init();
    }
}

/*
 * @brief       创建任务
 * @param[in]   none
 * @return      none
 */
static void create_tasks(void)
{
	uint8_t i = 0;
    const task_item_t *t;
    for (t = &task_tbl_start + 1; t < &task_tbl_end; t++) {
        os_task_create( t->entry, t->name, t->stack_size, t->prority, &gTaskHandle[i++], NULL);
    }
}

/*
 * @brief      运行任务
 *              1. 模组初始化优化级 system_init > driver_init > module_init
 *              2. 创建任务
 *              3. 启动任务
 * @param[in]   none
 * @return      none
 */
void os_run(void)
{
    init_items();
    create_tasks();
    os_start_kernel();
}

#else
/*
 * @brief       超时判断
 * @param[in]   start   - 起始时间
 * @param[in]   timeout - 超时时间(ms)
 */
bool is_timeout(unsigned int start, unsigned int timeout)
{
    return sys_get_tick() - start > timeout;
}

/*
 * @brief       任务休眠
 * @param[in]   ms
 * @return      none
 */
void sys_delay_ms(unsigned int ms)
{
    unsigned int i = 0;

    for (i = 0; ms > i; i++) {
        i = i;
    }
}

/*
 * @brief       空处理,用于定位段入口
 */
static void nop_process(void) {}

//第一个初始化项
const init_item_t init_tbl_start SECTION("init.item.0") = {
    "", nop_process
};
//最后个初始化项
const init_item_t init_tbl_end SECTION("init.item.4") = {
    "", nop_process
};

//第一个任务项
const task_item_t task_tbl_start SECTION("task.item.0") = {
    "", nop_process
};
//最后个任务项
const task_item_t task_tbl_end SECTION("task.item.2") = {
    "", nop_process
};

/*
 * @brief       模块初始处理
 *              初始化模块优化级 system_init > driver_init > module_init
 * @param[in]   none
 * @return      none
 */
void module_task_init(void)
{
    const init_item_t *it = &init_tbl_start;

    while (it < &init_tbl_end) {
        it++->init();
    }
}

/*
 * @brief       任务轮询处理
 * @param[in]   none
 * @return      none
 */
void module_task_process(void)
{
    const task_item_t *t;

    for (t = &task_tbl_start + 1; t < &task_tbl_end; t++) {
        if  ((get_tick() - *t->timer) >= t->interval) {
            *t->timer = get_tick();
            t->handle();
        }
    }
}

void os_run(void)
{
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, APP_ADDRESS);
    module_task_init();       /*模块初始化*/
    while (1) {
        module_task_process();/*任务轮询*/
    }
}

#endif

/*
 * @brief       显示任务信息
 */
void os_show_task_info(void)
{
}
