/******************************************************************************
 * @brief    独立按键管理
 *
 * Copyright (c) 2017~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-08-10     Morro        Initial version
 ******************************************************************************/
#include "key.h"

static st_KEY_dev *keyhead = NULL; /*按键链表头结点*/

/*******************************************************************************
 * @brief       创建一个按键
 * @param[in]   key     - 按键管理器
 * @param[in]   readkey - 按键触发测试函数指针
 * @param[in]   event   - 按键事件处理函数指针
 * @return      true    - 创建失败, false - 创建成功
 ******************************************************************************/
char key_dev_create(st_KEY_dev *pkey, unsigned int gpio, key_evt_handle event)
{
    st_KEY_dev *keytail = keyhead;

    if (NULL == pkey || 0 == gpio || NULL == event) {
        return 0;
    }

    pkey->event = event;
    pkey->gpio  = gpio;
    pkey->next  = NULL;
    if (NULL == keyhead) {
        keyhead = pkey;
        return 1;
    }

    while (NULL != keytail->next) { /*转至链尾*/
        keytail = keytail->next;
    }

    keytail->next = pkey;
    return 1;
}

/*******************************************************************************
 * @brief       忙判断
 * @return      none
 ******************************************************************************/
char key_dev_busy(st_KEY_dev *pkey)
{
    return (0 != pkey->tick);
}


/*******************************************************************************
 * @brief       按键扫描处理
 * @return      none
 ******************************************************************************/
void key_scan_process(void)
{
}
