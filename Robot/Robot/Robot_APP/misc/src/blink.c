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
 ******************************************************************************/
#include "blink.h"
#include "sys_task.h"
#include "platform.h"

static st_blink_dev *head = NULL; /* 头结点 -*/
static st_blink_dev *pm_head = NULL; /*功耗管理节点*/

unsigned char gpio_register_lock = 0;
unsigned char pm_register_lock = 0;

/*
 * @brief       创建blink设备
 * @param[in]   dev    - 设备
 * @return      none
 */
void gpio_dev_create(st_blink_dev *dev, unsigned int gpio)
{
	while(gpio_register_lock == 1)
		sys_delay_ms(5);
	gpio_register_lock = 1;
	
    st_blink_dev *tail = head;

    _BLINK_io_set(gpio, DISABLE);
    dev->gpio = gpio;
    dev->next = NULL;
    if (NULL == head) {
        head = dev;
		gpio_register_lock = 0;
        return;
    }

    while (NULL != tail->next) {
        tail = tail->next;
    }

    tail->next = dev;
	gpio_register_lock = 0;
}

/*
 * @brief       创建blink设备
 * @param[in]   dev    - 设备
 * @return      none
 */
void pm_dev_create(st_blink_dev *dev, unsigned int gpio)
{
	while(pm_register_lock == 1)
		sys_delay_ms(5);
	pm_register_lock = 1;
	
    st_blink_dev *tail = pm_head;

    dev->gpio = gpio;
    dev->pm_next = NULL;
    if (NULL == pm_head) {
        pm_head = dev;
		pm_register_lock = 0;
        return;
    }

    while (NULL != tail->pm_next) {
        tail = tail->pm_next;
    }

    tail->pm_next = dev;
	pm_register_lock = 0;
}


e_BLINK_state gpio_dev_state(st_blink_dev *dev)
{
//    if (_BLINK_isEnable(dev->gpio)) {
//        return (!dev->enable ? BLINK_STATE_CLOSE : BLINK_STATE_OPENED);
//    }
//    return (!dev->enable ? BLINK_STATE_CLOSED : BLINK_STATE_OPEN);
	return (e_BLINK_state)0;
}

/*
 * @brief	   忙判断
 */
int gpio_dev_busy(st_blink_dev *dev)
{
    return (0 != dev->repeats);
}


//注册为任务巡检设备
void gpio_dev_register(st_blink_dev * dev)
{
	gpio_dev_create(dev, dev->gpio);
}

//注册为低功耗设备
void pm_dev_register(st_blink_dev * dev)
{
	pm_dev_create(dev, dev->gpio);
}

void gpio_dev_set_status(st_blink_dev * dev, unsigned char Status)
{
	if(dev != NULL)
	{
		if(Status == DISABLE)
		{
			dev->NeedAble = DISABLE;
			dev->BlinkSta = DISABLE;
		}
		else
		{
			dev->NeedAble = ENABLE;
			dev->BlinkSta = DISABLE;
		}
		dev->tick = sys_get_ms();	//获取最新的一次操作时间
	}
}

void start_gpio_blink(st_blink_dev * dev, int ontime, int closetime, int repeats)
{
	if(dev != NULL)
	{
		dev->NeedAble = ENABLE;
		dev->BlinkSta = ENABLE;
		dev->openTime = ontime;
		dev->closetime = closetime;
		dev->repeats = repeats;
		if(repeats <= 0)
			dev->BlinkContinue = ENABLE;
		dev->tick = sys_get_ms() - closetime;
	}
}

void stop_gpio_blink(st_blink_dev * dev)
{
	if(dev != NULL)
	{
		dev->NeedAble = DISABLE;
		dev->BlinkSta = DISABLE;
		dev->BlinkContinue = DISABLE;
	}
}


bool is_dev_enable(st_blink_dev * dev)
{
	if(dev != NULL)
		return (dev->enable == ENABLE);
	else
		return false;
}

//进入低功耗模式
void start_to_pm_ctl(const char * func, int line)
{
	st_blink_dev *pdev;
	unsigned char ucSta = 1;

	if(IS_SLEEP_MODE()) return;	//已经是休眠状态无需休眠
	if(IS_WALK_WORK()) return;	//移动过程中禁止休眠

	debug_printf("%s:%d:Start to sleep!\r\n", func, line);

	for(pdev = pm_head; NULL != pdev; pdev = pdev->pm_next)
		pdev->ReadyToPm = 1;
	while(ucSta)
	{
		ucSta = 0;
		for(pdev = pm_head; NULL != pdev; pdev = pdev->pm_next)
		{
			pdev->pm_doing_befor();
			if(pdev->ReadyToPm && ucSta == 0)
				ucSta = 1;
		}
		sys_delay_ms(10);
	}
	gpio_dev_set_status(&gLed_front_dev, DISABLE);
	gpio_dev_set_status(&gLed_back_dev, DISABLE);
	for(pdev = pm_head; NULL != pdev; pdev = pdev->pm_next)
		gpio_dev_set_status(pdev, DISABLE);
	SET_SLEEP_MODE();
}

//推出低功耗模式
void end_pm_ctl(const char * func, int line)
{
	st_blink_dev *pdev;

	if(IS_WAKE_UP_MODE()) return;

	debug_printf("%s:%d:end sleep!\r\n", func, line);

	for(pdev = pm_head; NULL != pdev; pdev = pdev->pm_next)
		gpio_dev_set_status(pdev, ENABLE);
	QUIT_SLEEP_MODE();
}

void key_to_control_pm_handle(void)
{	//按键检测开关机
	static unsigned char ucPmsta = 0;
	static unsigned long sulPmTick;
	
	if(ucPmsta == 0)
	{	//检测按键按下
		if(GPIO_ReadInputDataBit(GPIOI, GPIO_Pin_5) == 0)
		{
			sulPmTick = sys_get_ms();
			ucPmsta = 1;
		}
	}
	else if(ucPmsta == 1)
	{	//消抖按键
		if(GPIO_ReadInputDataBit(GPIOI, GPIO_Pin_5) == 1)
			ucPmsta = 0;
		if((sys_get_ms() - sulPmTick) > 100)	//消抖100ms
			ucPmsta = 2;
	}
	else if(ucPmsta == 2)
	{	//检测按键时间并进行开关机处理
		if(GPIO_ReadInputDataBit(GPIOI, GPIO_Pin_5) == 1)
			ucPmsta = 0;

		if(IS_SLEEP_MODE() && ((sys_get_ms() - sulPmTick) > 1000))	//正常运行模式并且按键按下大于1s
		{
			start_to_pm_ctl(__func__, __LINE__);
			gucPmMode = 1;
			ucPmsta = 3;
			sulPmTick = sys_get_ms();
		}
		if(IS_WAKE_UP_MODE() && sys_over_time(sulPmTick, 3000))	//正常休眠模式并且按键按下大于3s
		{
			end_pm_ctl(__func__, __LINE__);
			ucPmsta = 3;
			sulPmTick = sys_get_ms();
			gWorkWorker.StayHomeFlag = 0;
		}
	}
	else if(ucPmsta == 3)
	{	//等待按键松开
		if(GPIO_ReadInputDataBit(GPIOI, GPIO_Pin_5) == 0)
		{
			sulPmTick = sys_get_ms();
		}
		if(sys_over_time(sulPmTick, 100))	//消抖100ms
			ucPmsta = 0;
	}
}

/*
 * @brief       blink设备管理
 * @param[in]   none
 * @return      none
 */
void gpio_dev_process(st_blink_dev *pdev)
{
	key_to_control_pm_handle();
	
	if(pdev->BlinkSta)
	{	//闪烁状态
		if((pdev->BlinkContinue == ENABLE) || (pdev->repeats > 0))
		{
	        if ((pdev->enable != pdev->NeedAble) && sys_istimeout(pdev->tick, ((pdev->NeedAble == ENABLE) ? pdev->closetime : pdev->openTime))) {
				//当前状态不等于需要的状态且时间达到设置的时间后
	            _BLINK_io_set(pdev->gpio, pdev->NeedAble);
	            pdev->tick = sys_get_ms();//tick 复用led事件处理句柄
	            pdev->enable = pdev->NeedAble;
				if(pdev->NeedAble == DISABLE)
				{
					if(pdev->repeats > 0)
						pdev->repeats--;
					if((pdev->BlinkContinue == DISABLE) && (pdev->repeats == 0))	//非持续闪烁模式
						pdev->BlinkSta = 0;
				}
				if(pdev->BlinkSta)
					pdev->NeedAble = !pdev->NeedAble;
	        }
		}
	}
	else if(pdev->enable != pdev->NeedAble)
	{	//非闪烁状态且状态不匹配
		if(_BLINK_io_set(pdev->gpio, pdev->NeedAble) > 0)	//GPIO设置成功则改变状态
			pdev->enable = pdev->NeedAble;
	}
}

void gpio_ctl_task(void)
{
	st_blink_dev *pdev;

	if(gulRegisterFlag == 0)
	{	//等待注册完成
		for(pdev = head; NULL != pdev; pdev = pdev->next)
			gpio_dev_process(pdev);
	}

	sys_delay_ms(30);
}


task_define(led, SYS_NOP, gpio_ctl_task, 50, 256, 8);


