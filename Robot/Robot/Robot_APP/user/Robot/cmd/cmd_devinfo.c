/******************************************************************************
 * @brief    设备相关命令
 *
 * Copyright (c) 2020, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020/07/11     worker
 ******************************************************************************/
#include <stdlib.h>
#include "platform.h"
#include "sys_cfg.h"
#include "cli.h"
#include "sfud.h"
#include "lift_task.h"
#include "charge_task.h"
#include "bsp.h"

/*
 * @brief   show system information
 */
static int do_sysinfo_handler(void *pobj, int argc, char *argv[])
{	
    debug_printf("\r\n|*************************************************************|\r\n");
    debug_printf("|                            \\\\\\|///                          |\r\n");
    debug_printf("|                            \\- - -/                          |\r\n");
    debug_printf("|                           (  @ @  )                         |\r\n");
    debug_printf("|  +----------------------o00o-(_)-o00o---------------------+ |\r\n");
    debug_printf("|  |                                                        | |\r\n");
    debug_printf("|  | Project Name :        Robot-STM                        | |\r\n");
    debug_printf("|  |--------------------------------------------------------| |\r\n");
    debug_printf("|  | Author      :  worker                                  | |\r\n");
    debug_printf("|  | Contact     :  <worker@junlei.com>                     | |\r\n");
    debug_printf("|  |                                                        | |\r\n");
    debug_printf("|  |                               Oooo                     | |\r\n");
    debug_printf("|  +---------------------- oooO---(   )---------------------+ |\r\n");
    debug_printf("|                          (   )   ) /                        |\r\n");
    debug_printf("|                           \\ (   (_/                         |\r\n");
    debug_printf("|                            \\_)                              |\r\n");
    debug_printf("|*************************************************************|\r\n\r\n");

	debug_printf("Build in %s %s\r\n",__DATE__,__TIME__);
    debug_printf("Software version: %s\r\n", SW_VER);
    debug_printf("System clock: %d hz\r\n", SystemCoreClock);
    if (true) {
        RCC_ClocksTypeDef rcc;

        RCC_GetClocksFreq(&rcc);
        debug_printf("HCLK:%d Hz, PCLK1:%d Hz, PCLK2:%d Hz, SYSCLK:%d Hz\r\n",
               rcc.HCLK_Frequency, rcc.PCLK1_Frequency, rcc.PCLK2_Frequency, rcc.SYSCLK_Frequency);
        debug_printf("Reset type:%08x\r\n", RCC->CSR);
    }
    return 1;
}cmd_register("sysinfo", do_sysinfo_handler, "show system infomation.");

/*
 * @brief       reset system
 */
int do_cmd_reset(void *pobj, int argc, char *argv[])
{
    NVIC_SystemReset();
    return 0;
}cmd_register("reset", do_cmd_reset, "reset system");

/*
 * @brief Mode setting
 * example:
 *      SetMode:0
 */
int do_cmd_setMode(void *pobj, int argc, char *argv[])
{
    if (1 == argc && NULL != argv[0]) {
        uint8_t mode = (uint8_t)atoi(argv[0]);

		if((mode < 2) && (gWorkWorker.WorkSta == 0) && (IS_WALK_STOP() ||IS_WALK_ERR()))
		{	//最大三种模式 && 机器人不在巡检状态 && 行走电机不在运行状态
			if(!IS_Auto_Mode() && (mode == 0))
				lift_high_pos(0);
			if(gWorkWorker.WorkMode != mode)
				set_cloud_push_flag(PROP_MODE);
			if(mode != 0)
			{
				gulUnWorkTick = sys_get_ms();
				if(gucPmMode == 0)
					end_pm_ctl(__func__, __LINE__);
			}
			if(!IS_Auto_Mode() && (mode == 0) && gucIsHome)
			{
				gWorkWorker.StayHomeFlag = 1;
				gWorkWorker.ulStartTick = sys_get_ms();
				gucFrontCtrTime = 0;
				gucBackCtrTime = 0;
			}
			gWorkWorker.WorkMode = mode;
			debug_printf("Set mode to %u\r\n", mode);
		}
    }
    return -1;
} cmd_register("SetMode", do_cmd_setMode, "Mode setting");

void led_control(uint8_t ucLedNum, uint8_t ucLedSta)
{
	if(ucLedNum == 0)
	{
		gLedDevInfo.ucLedSta1 = ucLedSta;
		if(ucLedSta == 0)
			gpio_dev_set_status(&gLed_front_dev, ENABLE);
		else
			gpio_dev_set_status(&gLed_front_dev, DISABLE);
	}
	else if(ucLedNum == 1)
	{
		gLedDevInfo.ucLedSta2 = ucLedSta;
		if(ucLedSta == 0)
			gpio_dev_set_status(&gLed_back_dev, ENABLE);
		else
			gpio_dev_set_status(&gLed_back_dev, DISABLE);
	}
	else if(ucLedNum == 2)
	{
		if(ucLedSta == 0)
		{
			gpio_dev_set_status(&gLed_front_dev, ENABLE);
			gpio_dev_set_status(&gLed_back_dev, ENABLE);
		}
		else
		{
			gpio_dev_set_status(&gLed_front_dev, DISABLE);
			gpio_dev_set_status(&gLed_back_dev, DISABLE);
		}
	}
}

/*
 * @brief Led setting
 * example:
 *      SetLed:0;0
 */
int do_cmd_setLed(void *pobj, int argc, char *argv[])
{	
    if (2 == argc && NULL != argv[0] && NULL != argv[1]) {
		gulUnWorkTick = sys_get_ms();
        if(*argv[0] >= '0' && *argv[0] <= '2')
        {
			if(*argv[1] >= '0' && *argv[1] <= '1')
			{
				led_control(*argv[0] - '0', *argv[1] - '0');
			}
		}
    }
    return -1;
}
cmd_register("SetLed", do_cmd_setLed, "Led setting");

/*
 * @brief Place setting
 * example:
 *      SetPlace:9.5;10.0
 */
int do_cmd_setPlace(void *pobj, int argc, char *argv[])
{
    if (2 == argc && NULL != argv[0] && NULL != argv[1]) {
        float fold, fnew;
		gulUnWorkTick = sys_get_ms();

        if (0 <= sys_str2float(argv[0], &fold)
            && 0 <= sys_str2float(argv[1], &fnew)) {
            fold *= 100;
            fnew *= 100;
            return cfg_data_update(CFG_INSPECT_POS, fold, fnew);
        }
    }
    return -1;
}
cmd_register("SetPlace", do_cmd_setPlace, "Place setting");

/*
 * @brief set inspect plan
 * example:
 *      SetPlan:0;ID00001;0;240928120000;250928120000;0 0 8 * * ? *
 */
int do_cmd_setPlan(void *pobj, int argc, char *argv[])
{
	unsigned char ucDataBuf[64] = {0}, ucIndex = 1, i, opt;

	//if(IS_Auto_Mode()) return -1;

	if((6 == argc) ||(4 == argc))
	{
		gulUnWorkTick = sys_get_ms();
		if((*argv[2] != '0') && (*argv[2] != '1'))	//非单次或
			return -1;
		if(strlen(argv[1]) != 6) return -1;			//巡检计划非6位
		if(*argv[2] == '0')
		{			//单次模式超时
			if(check_time_out((unsigned char *)argv[3])) return -1;
		}
		else
		{
			if(check_time_out((unsigned char *)argv[4])) return -1;	//重复模式超时
		}
			
		for(i = 1; i < argc; i++)
			ucIndex += sprintf((char *)&ucDataBuf[ucIndex], "%s", argv[i]);
		opt = sys_atoi(argv[0]);
		if(opt == 3) ucDataBuf[0] = '1';	//暂停计划
		else	ucDataBuf[0] = '0';			//正常执行计划
		switch (opt)
		{
		case 0:					//新增计划
			add_work_plan(ucDataBuf);
			break;
		case 1:					//修改计划
			change_work_plan(ucDataBuf);
			break;
		case 2:					//删除计划
			del_work_plan(ucDataBuf, 0);
		case 3:					//暂停计划
			pause_resume_work_plan(ucDataBuf, 0);
			break;
		case 4:					//启用计划
			pause_resume_work_plan(ucDataBuf, 1);
			break;
		default:	break;
		}
	}
#if 0
    for (; 6 == argc && NULL != argv[0]; argc = 0) {
        st_inspect_plan *pplan = NULL;
        st_inspect_plan plan;
        st_inspect_plan data;
        uint8_t opt = sys_atoi(argv[0]); // 0 new, 1 update 2 delete 3 disable 4 enable
        const char *perror = NULL;
        int index;
		
        index = cfg_data_match((void *)argv[1], CFG_INSPECT_ID, sys_strlen(argv[1]));
        if (0 > index) {
            if (0 != opt || !argv[3] || !argv[4] || !argv[5]) {
                return -1;
            }

            sys_memset((void *)&plan, 0, sizeof(plan));
        }
        else {
            cfg_item_read((void *)&data, CFG_INSPECT_PLAN, index, sizeof(data));
            sys_memcpy((void *)&plan, (void *)pplan, sizeof(plan));
            if (4 == opt) { // enable
                pplan->enable = 1;
                cfg_item_write((void *)&data, CFG_INSPECT_PLAN, index, sizeof(data));
                break;
            }
            else if (3 == opt) { // disable
                pplan->enable = 0;
                cfg_item_write((void *)&data, CFG_INSPECT_PLAN, index, sizeof(data));
                break;
            }
            else if (2 == opt) { // delete
                if (0 > cfg_item_clear(CFG_INSPECT_PLAN, index)) {
                    return -1;
                }
                break;
            }
            else if (1 != opt) {
                return -1;
            }
        }

        if (NULL != argv[5]) {
            cron_parse_expr(argv[5], &plan.cycle, &perror);
            if (NULL != perror) {
                return -1;
            }
        }

        if (NULL != argv[4] && 0 > strtotime(argv[4], &plan.expire_time)) {
            return -1;
        }

        if (NULL != argv[3] && 0 > strtotime(argv[3], &plan.start_time)) {
            return -1;
        }

        plan.repeat = atoi(argv[2]);
        sys_memcpy(plan.id, (void *)argv[1], sys_strlen(argv[1]) + 1);
        cfg_item_write((void *)&plan, CFG_INSPECT_PLAN, index, sizeof(plan));
    }
#endif
    return 0;
}
cmd_register("SetPlan", do_cmd_setPlan, "set inspect plan");
