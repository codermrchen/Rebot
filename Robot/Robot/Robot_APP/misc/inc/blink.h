/******************************************************************************
 * @brief    具有闪烁特性(dev, motor, buzzer)的设备(dev, motor, buzzer)管理
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-01     Morro        Initial version
 * 2021-03-07     Morro        增加忙判断接口
 ******************************************************************************/
#ifndef _BLINK_H_
#define _BLINK_H_

#include "sys_util.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLINK_STATE_OPEN,
    BLINK_STATE_OPENED,
    BLINK_STATE_CLOSE,
    BLINK_STATE_CLOSED
} e_BLINK_state;

/*Blink 设备定义*/
typedef struct {
    unsigned int   gpio;
    unsigned int   tick;
    unsigned short closetime;   	//关闭时间
    unsigned short openTime;		//开启时间
    unsigned short repeats; 		//闪烁次数
    unsigned char  enable : 1;		//设备当前状态 0:失能 1:使能
	unsigned char  NeedAble : 1;	//设备要求状态 0:使能 1:使能
	unsigned char  BlinkSta : 1;	//当前是否为闪烁状态
	unsigned char  BlinkContinue : 1;  //是否为持续闪烁，持续闪烁则不检测repeats,否则repeats为0时则停止闪烁
	unsigned char  ReadyToPm : 2;	//准备好进入低功耗模式
	unsigned char  Resv : 2;		//备用
    void *next;
	void *pm_next;
	void (*pm_doing_befor)(void);	//进入低功耗模式前先处理数据
} st_blink_dev;

void gpio_dev_create(st_blink_dev *pdev, unsigned int gpio);

void gpio_dev_ctrl(st_blink_dev *pdev, int ontime, int cycle, int repeat);

e_BLINK_state gpio_dev_state(st_blink_dev *dev);

int gpio_dev_busy(st_blink_dev *pdev);

void gpio_dev_process(st_blink_dev *pdev);

void gpio_ctl_task(void);

void gpio_dev_register(st_blink_dev * dev);

void pm_dev_register(st_blink_dev * dev);

void gpio_dev_set_status(st_blink_dev * dev, unsigned char Status);

void start_gpio_blink(st_blink_dev * dev, int ontime, int closetime, int repeats);

void stop_gpio_blink(st_blink_dev * dev);

bool is_dev_enable(st_blink_dev * dev);

void start_to_pm_ctl(const char * func, int line);

void end_pm_ctl(const char * func, int line);


#ifdef __cplusplus
}
#endif

#endif
