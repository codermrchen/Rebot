/******************************************************************************
 * @brief    ultrasonic control
 *
 * Copyright (c) 2020, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-20     worker    inited
 ******************************************************************************/
#include <stdlib.h>
#include "sys_task.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"
#include "blink.h"
#include "bsp.h"
#include "ultras_task.h"

static void _ultras_front_pm_before(void);
#if (NEW_BOARD == 0)
static void _ultras_back_pm_before(void);
#endif
static void _ultras_ctl_pm_before(void);

typedef enum {
    _CMD_SET = 1,
    _CMD_MODE = 3,
    _CMD_SWITCH,
    _CMD_TEMP
} e_DEV_cmd;

static st_blink_dev _ultras_front_dev = {	\
		.gpio = TGT_ULTRAS_PWR,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE,				\
		.pm_doing_befor = _ultras_front_pm_before,	\
	};

static st_blink_dev _ultras_set_front_dev = {	\
			.gpio = TGT_ULTRAS_SET,			\
			.BlinkSta = DISABLE,			\
			.NeedAble = DISABLE, 			\
		};

#if (NEW_BOARD == 0)
static st_blink_dev _ultras_back_dev = {	\
			.gpio = TGT_BULTRAS_PWR,			\
			.BlinkSta = DISABLE,			\
			.NeedAble = ENABLE, 			\
			.pm_doing_befor = _ultras_back_pm_before,	\
		};
#endif

static st_blink_dev _ultras_set_back_dev = {	\
			.gpio = TGT_BULTRAS_SET, 		\
			.BlinkSta = DISABLE,			\
			.NeedAble = DISABLE, 			\
		};


static st_blink_dev _ultras_ctl_dev = {	\
		.gpio = TGT_ULTRAS_CTL, 		\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE, 			\
		.pm_doing_befor = _ultras_ctl_pm_before,	\
	};

static void _ultras_front_pm_before(void)
{
	_ultras_front_dev.ReadyToPm = 0;
}

#if (NEW_BOARD == 0)
static void _ultras_back_pm_before(void)
{
	_ultras_back_dev.ReadyToPm = 0;
}
#endif

static void _ultras_ctl_pm_before(void)
{
	_ultras_ctl_dev.ReadyToPm = 0;
}

char ultras1_status_judge(uint8_t ucUltrasSta, uint16_t usUltrasErr)
{
	if((gUltrasDevInfo.ucUltrasSta1 != ucUltrasSta) || (gUltrasDevInfo.ucUltrasErrCode1 != usUltrasErr))	//运行状态
	{
		gUltrasDevInfo.ucUltrasSta1 = ucUltrasSta;
		gUltrasDevInfo.ucUltrasErrCode1 = usUltrasErr;
		return 1;
	}	
	return 0;
}

char ultras2_status_judge(uint8_t ucUltrasSta, uint16_t usUltrasErr)
{
	if((gUltrasDevInfo.ucUltrasSta2 != ucUltrasSta) || (gUltrasDevInfo.ucUltrasErrCode2 != usUltrasErr))	//运行状态
	{
		gUltrasDevInfo.ucUltrasSta2 = ucUltrasSta;
		gUltrasDevInfo.ucUltrasErrCode2 = usUltrasErr;
		return 1;
	}	
	return 0;
}



/* Private function prototypes -----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static int _ultra_data_urc(unsigned short cmd, unsigned char *pdata, unsigned short num)
{
    static st_dev_state  _ultra_state[TGT_ULTRA_ADC_CHNS] = {
                                [0] = {.state = 1},
                                [1] = {.state = 1},
                            };
    st_cfg_cmd *pcmd = (st_cfg_cmd *)&cmd;

    if (TGT_ULTRA_ADC_CHNS > pcmd->index && NULL != pdata && !num) {
        switch (pcmd->type) {
        case CFG_ULTRA_ADC:
//            *(void **)pdata = (uint8_t *)&_ultra_adc[pcmd->index];
//            return sizeof(_ultra_adc[pcmd->index]);
			break;
        case CFG_ULTRA_STATE:
            *(void **)pdata = &_ultra_state[pcmd->index].state;
            return sizeof(_ultra_state[pcmd->index].state);
        case CFG_ULTRA_ERR:
            *(void **)pdata = _ultra_state[pcmd->index].error;
            return sizeof(_ultra_state[pcmd->index].error);
        default: break;
        }
    }

    *(void **)pdata = NULL;
    return 0;
}
sys_urc_register(SYS_MODULE_ULTRA, _ultra_data_urc);

//float adc_value_transfor_distance(uint32_t usAdcValue)
//{
//	float fData, fDistance, fStandard = 3515;
//	fData = (float)usAdcValue;
//	if(fData > fStandard) fData = fStandard;
//#if (ROBOT_VER == 0)
//	fDistance = 10.0f + 90.0f * fData / fStandard;
//#else
//	fDistance = 10.0f + 90.0f * (fStandard - fData) / fStandard;
//#endif
//	return fDistance;
//}

void ultras_status_check(void)
{
	static unsigned char sucSta[2] = {0}, sucErrSta[2] = {0};

	if((sucSta[0] != gUltrasDevInfo.ucUltrasSta1) ||(sucSta[1] != gUltrasDevInfo.ucUltrasSta2) ||
		(sucErrSta[0] != gUltrasDevInfo.ucUltrasErrCode1) || (sucErrSta[1] != gUltrasDevInfo.ucUltrasErrCode2))
	{
		sucSta[0] = gUltrasDevInfo.ucUltrasSta1;
		sucSta[1] = gUltrasDevInfo.ucUltrasSta2;
		sucErrSta[0] = gUltrasDevInfo.ucUltrasErrCode1;
		sucErrSta[1] = gUltrasDevInfo.ucUltrasErrCode2;
		set_cloud_push_flag(PROP_ULTRASONIC);
	}
}

void led_status_check(void)
{
	static uint8_t sucLedSta[2] = {0}, sucLedErr[2] = {0};

	if((sucLedSta[0] != gLedDevInfo.ucLedSta1) ||(sucLedSta[1] != gLedDevInfo.ucLedSta2) ||
		(sucLedErr[0] != gLedDevInfo.ucLedErrCode1) ||(sucLedErr[1] != gLedDevInfo.ucLedErrCode2))
	{
		sucLedSta[0] = gLedDevInfo.ucLedSta1;
		sucLedSta[1] = gLedDevInfo.ucLedSta2;
		sucLedErr[0] = gLedDevInfo.ucLedErrCode1;
		sucLedErr[1] = gLedDevInfo.ucLedErrCode2;
		set_cloud_push_flag(PROP_LED);
	}
}

void led1_status_judge(uint8_t ucLedSta, uint8_t ucLedErr)
{
	if((gLedDevInfo.ucLedSta1 != ucLedSta) ||(gLedDevInfo.ucLedErrCode1 != ucLedErr))
	{	//LED故障
		gLedDevInfo.ucLedSta1 = ucLedSta;
		gLedDevInfo.ucLedErrCode1 = ucLedErr;
	}
}

void led2_status_judge(uint8_t ucLedSta, uint8_t ucLedErr)
{
	if((gLedDevInfo.ucLedSta2 != ucLedSta) ||(gLedDevInfo.ucLedErrCode2 != ucLedErr))
	{	//LED故障
		gLedDevInfo.ucLedSta2 = ucLedSta;
		gLedDevInfo.ucLedErrCode2 = ucLedErr;
	}
}

void led_status_deteck_change(void)
{
	if((gLed_front_dev.enable != gLed_front_dev.NeedAble) && sys_over_time(gLed_front_dev.tick, 1000)) {
		led1_status_judge(9, 1);
	}
	else if(gLed_front_dev.enable == gLed_front_dev.NeedAble)
	{
		if(gLed_front_dev.enable == ENABLE) {
			led1_status_judge(0, 0);
		}
		else if(gLed_front_dev.enable == DISABLE) {
			led1_status_judge(1, 0);
		}
	}
	if((gLed_back_dev.enable != gLed_back_dev.NeedAble) && sys_over_time(gLed_back_dev.tick, 1000)) {
		led2_status_judge(9, 1);
	}
	else if(gLed_back_dev.enable == gLed_back_dev.NeedAble)
	{
		if(gLed_back_dev.enable == ENABLE) {
			led2_status_judge(0, 0);
		}
		else if(gLed_back_dev.enable == DISABLE) {
			led2_status_judge(1, 0);
		}
	}
}

static void _module_process(void)
{
  	//static uint32_t sulTick;
	float fDetectDis;
	static unsigned short susCheckDis;

	if(sys_over_time(gulTimeTick, 1000))
	{
		gulTimeTick += 1000;
		time_inc_auto();
	}

	if(!JUDGE_WORK(WORK_BLOCK_FRONT) && !JUDGE_WORK(WORK_BLOCK_BACK))
	{
		fDetectDis = 50.0f + (abs(gWalkDevInfo.lSpeed) * 0.0006f + (abs(gWalkDevInfo.lSpeed) - 1000) * 0.00032f);
		if(fDetectDis > 240.0f)
			fDetectDis = 240.0f;
		susCheckDis = fDetectDis * 10;
	}
	if(gUltrasDevInfo.ucUltrasErrCode1 != 2)
	{	//超声波可以正常通讯
		if(gusFrontDis < susCheckDis)
		{	//前脸超声波小于60cm
			if(ultras1_status_judge(9, 1))
				debug_printf("front block, cur:%u, detect:%u!\r\n", gusFrontDis, susCheckDis);
		}
		else
		{
			if(ultras1_status_judge(0, 0)) {
				debug_printf("front release, cur:%u, detect:%u!\r\n", gusFrontDis, susCheckDis);
				gucFrontCtrTime = 0;
			}
		}
	}

	if(gUltrasDevInfo.ucUltrasErrCode2 != 2)
	{
		if(gusBackDis < susCheckDis)
		{	//后脸超声波小于60cm
			if(ultras2_status_judge(9, 1)) 
				debug_printf("back block,cur:%u, detect:%u\r\n", gusBackDis, susCheckDis);				
		}
		else
		{
			if(ultras2_status_judge(0, 0))
			{
				debug_printf("back release, cur:%u, detect:%u!\r\n", gusBackDis, susCheckDis);
				gucBackCtrTime = 0;
			}			
		}
	}

	ultras_status_check();
	led_status_deteck_change();	//前后照明灯状态判断&变更
	led_status_check();
}

static void _module_init(void)
{
	gpio_dev_register(&_ultras_front_dev);	//测距工具已变更为激光485，无需开关	
#if (NEW_BOARD == 0)
	gpio_dev_register(&_ultras_back_dev);
#endif
	gpio_dev_register(&_ultras_ctl_dev);
	gpio_dev_register(&_ultras_set_front_dev);	
	gpio_dev_register(&_ultras_set_back_dev);
	
//	pm_dev_register(&_ultras_front_dev);	
//	pm_dev_register(&_ultras_back_dev);
//	pm_dev_register(&_ultras_ctl_dev);
	gulRegisterFlag &= ~BIT(5);
	ultras1_status_judge(0, 0);
	ultras2_status_judge(0, 0);
	led1_status_judge(1, 0);
	led2_status_judge(1, 0);
	set_cloud_push_flag(PROP_ULTRASONIC);
	set_cloud_push_flag(PROP_LED);
	gulTimeTick = sys_get_ms();
}

/*
 * @brief    module task(500ms loop one time)
 */
task_define(ultrasonic, _module_init, _module_process, 100, 256, 9);
