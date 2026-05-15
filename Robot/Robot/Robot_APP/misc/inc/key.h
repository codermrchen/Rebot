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
 * 2021-03-07     Morro        增加忙判断接口
 ******************************************************************************/
#ifndef _KEY_H_
#define _KEY_H_

#include "sys_util.h"

#define LONG_PRESS_TIME         (1000) /*长按确认时间 ------------*/
#define KEY_DEBOUNCE_TIME       (50)   /*按键消抖时间 ------------*/

typedef enum {
    KEY_PRESS = 0,  /*短按 --------------------*/
    KEY_LONG_DOWN,  /*长按按下 ----------------*/
    KEY_LONG_UP     /*长按释放 ----------------*/
} e_KEY_event;

/*
 *@brief     按键事件触发处理
 *@param[in] event - 事件类型(KEY_SHORT_PRESS - 短按, KEY_LONG_PRESS - 长按)
 *@param[in] duration 持续时间,长按有效
 *@return    none
 */
typedef void (*key_evt_handle)(void *pkey, e_KEY_event event, unsigned int duration);

/*按键管理器 -----------------------------------------------------------------*/
typedef struct KEY_dev {
    key_evt_handle event;   /*按键事件处理 ------------*/
    unsigned int tick;      /*滴答计时器 --------------*/
    unsigned int gpio;
    void  *next;            /*连接下一个按键并构成链表 */
} st_KEY_dev;

#ifdef __cplusplus
extern "C" {
#endif

char key_dev_create(st_KEY_dev *pkey, unsigned int gpio, key_evt_handle handle); /*创建按键*/

char key_dev_busy(st_KEY_dev *pkey); /*忙判断*/

void key_scan_process(void); /*按键扫描处理*/

#ifdef __cplusplus
}
#endif

#endif
