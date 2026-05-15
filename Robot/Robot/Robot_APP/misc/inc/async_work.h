/******************************************************************************
 * @brief        异步作业
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-22     Morro        Initial version.
 ******************************************************************************/
#ifndef _ASYNC_WORK_H_
#define _ASYNC_WORK_H_

#include <stdbool.h>
#include "qlink.h"
#include "sys_util.h"

/*异步作业管理器 -------------------------------------------------------------*/
typedef struct {
    struct qlink idle;                             /*空闲队列*/
    struct qlink ready;                            /*就绪队列*/
} work_async_t;

typedef void (*async_work_handle)(work_async_t *w, void *obj, void *params);

/*异步作业结点 ---------------------------------------------------------------*/
typedef struct {
    void *object;                                  /*作业对象*/
    void *params;                                  /*作业参数*/
    async_work_handle work;                        /*作业入口*/
    struct qlink_node node;                        /*链表节点*/
} work_node_t;

/*
 * @brief       异步作业处理
 * @param[in]   w - 作业管理器
 */
char async_work_add(work_async_t *w, void *obj, void *params, async_work_handle work);

/**
 * @brief       作业入队
 * @param[in]   qlink     - 工作链
 * @param[in]   new_item -  工作项
 * @return      none
 */
void async_work_init(work_async_t *w, work_node_t *nodes, int max_nodes);

/*
 * @brief       异步作业处理
 * @param[in]   w - 作业管理器
 */
void async_work_process(work_async_t *w);

#endif
