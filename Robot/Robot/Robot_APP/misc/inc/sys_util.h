/******************************************************************************
 * @brief     sys操作系统相关移植接口
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-21     Morro        Initial version.
 ******************************************************************************/

#ifndef _SYS_PORT_H_
#define _SYS_PORT_H_

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "bsp.h"

/**
 * 错误码定义
 */
#define SYS_OK                           0                   /* 正确无误 */
#define SYS_ERROR                       -1                   /* 通用错误 */
#define SYS_TIMEOUT                     -2                   /* 执行超时 */
#define SYS_FAILED                      -3                   /* 执行失败 */
#define SYS_NOIMPL                      -4                   /* 接口未实现 */
#define SYS_ABORT                       -5                   /* 已终止 */
#define SYS_NOMEM                       -6                   /* 内存不足 */
#define SYS_REJECT                      -7                   /* 操作被拒*/
#define SYS_INVALID                     -8                   /* 无效参数*/
#define SYS_ONGOING                     -9                   /* 进行中*/
#define SYS_FILE_NOT_FOUND              -10                  /* 未找到文件*/

/* log等级定义 ---------------------------------------------------------------*/
#define SYS_LOG_DBG                     0   /* debug信息 */
#define SYS_LOG_INFO                    1   /* 正常状态信息 */
#define SYS_LOG_WARN                    2   /* 警告信息 */
#define SYS_LOG_ERR                     3   /* 异常信息 */

#define _KEY_isEnable(gpio)             BSP_gpio_isEnable(gpio)
#define _BLINK_isEnable(gpio)           BSP_gpio_isEnable(gpio)
#define _BLINK_io_set(gpio, state)      BSP_gpio_set(gpio, state)

#define SYS_NET_BYTE                    (0x8000)
#define sys_strlen                      strlen
#define sys_memset                      memset
#define SYS_NOP()

typedef void *sys_sem_t; /* 信号量*/

/**
 * @brief	   获取当前系统毫秒数
 */
unsigned int sys_get_ms(void);
unsigned int sys_get_tick(void);

int strtotime(const char *str, time_t *t);

/**
 * @brief	   超时判断
 * @params[in] start_time - 起始时间
 * @params[in] timeout    - 超时时间(ms)
 * @return     true | false
 */
static inline bool sys_istimeout(unsigned int start_time, unsigned int timeout)
{
	return ((sys_get_ms() - start_time) > timeout);
}

/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void sys_init_set(uint8_t isInited);

/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void sys_delay_ms(unsigned int ms);
void sys_delay_us(unsigned int us);

/**
 * @brief	   新建信号量
 * @params[in] value    - 初始值
 * @return     指向一新信号量的指针
 */
sys_sem_t sys_sem_new(int value);

/**
 * @brief	   等待信号量
 * @params[in] s       - 信号量
 * @params[in] timeout - 等待超时时间
 * @return     true - 成功获取到信号量, false - 等待超时
 */
bool sys_sem_wait(sys_sem_t s, unsigned int timeout);

/**
 * @brief	   发送信号量
 * @params[in] s      - 信号量
 * @return     none
 */
void sys_sem_post(sys_sem_t s);

/**
 * @brief	   释放信号量(暂时未用)
 * @return     none
 */
void sys_sem_free(sys_sem_t s);


/**
 * @brief	   进入临界区
 * @return     none
 */
void sys_enter_critical(void);

/**
 * @brief	   退入临界区
 * @return     none
 */
void sys_exit_critical(void);

/**
 * @brief	   内存分配
 * @params[in] nbytes - 分配字节数
 */
void* sys_malloc(int nbytes);

/**
 * @brief	   内存释放
 * @params[in] p - 待释放指针
 */
void sys_free(void *p);

#endif
