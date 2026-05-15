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
#include <stdbool.h>
#include "comdef.h"
#include "async_work.h"

static work_async_t  *_work_queue;

/*******************************************************************************
 * @brief       作业入队
 * @param[in]   qlink     - 工作链
 * @param[in]   new_item -  工作项
 * @return      none
 ******************************************************************************/
static inline void workqueue_put(struct qlink *q, work_node_t *n)
{
	qlink_put(q, &n->node);                                  /*加入到就绪链表*/
}

/*******************************************************************************
 * @brief       作业出队
 * @param[in]   qlink     - 工作链
 * @return      工作项
 ******************************************************************************/
static inline work_node_t* workqueue_get(struct qlink *q)
{
	struct qlink_node *n = qlink_get(q);

    if (NULL != n) {
	    return container_of(n, work_node_t, node);
    }
    return NULL;
}

#if 0
/*******************************************************************************
 * @brief       作业预出队
 * @param[in]   qlink     - 工作链
 * @return      工作项
 ******************************************************************************/
static inline st_WORK_node* _workqueue_peek(struct qlink *q)
{
	struct qlink_node *n = qlink_peek(q);

	return n ? container_of(n, st_WORK_node, node) : NULL;
}
#endif

/*
 * @brief       异常作业初始化
 * @param[in]   w        - 作业管理器
 * @param[in]   node_tbl - 作业节点表
 * @param[in]   count    - node_tbl个数
 */
void async_work_init(work_async_t *w, work_node_t *node_tbl, int count)
{
    if (!w || !node_tbl || !count) {
        return;
    }

    if (!_work_queue) _work_queue = w;
    qlink_init(&w->idle);
    qlink_init(&w->ready);
    while (count--) {
        qlink_put(&w->idle, &node_tbl->node);
        node_tbl++;
    }
}

/*
 * @brief       增加作业到队列
 * @param[in]   w        - 作业管理器
 * @param[in]   params   - 作业参数
 * @param[in]   work     - 作业入口
 */
char async_work_add(work_async_t *w, void *object, void *params, async_work_handle work)
{
    work_node_t *n;

    if (!w) w = _work_queue;
    n = workqueue_get(&w->idle);
    if (NULL != n) {
        n->object = object;
        n->params = params;
        n->work   = work;
        sys_enter_critical();
        workqueue_put(&w->ready, n); /*加入到就绪链表*/
        sys_exit_critical();
        return 1;
    }
    return 0;
}

/*
 * @brief       异步作业处理
 * @param[in]   w         - 作业管理器
 */
void async_work_process(work_async_t *w)
{
    work_node_t *n;

    if (!w) w = _work_queue;
    n = workqueue_get(&w->ready);
    if (NULL != n) {
        n->work(w, n->object, n->params);
        workqueue_put(&w->idle, n);
    }
}
