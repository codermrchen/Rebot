/*******************************************************************************
					巡检功能实现任务
*******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "async_work.h"
#include "sys_util.h"
#include "sys_task.h"
#include "sys_pm.h"
#include "platform.h"
#include "blink.h"
#include "cli.h"
#include "key.h"
#include "bsp.h"
#include "platform.h"
#include "walk_task.h"
#include "lift_task.h"
#include "lfs.h"
#include "sfud.h"
#include "charge_task.h"
#include "rfid_task.h"
#include "net_task_uart.h"
#include "work_task.h"

#define RFID_LAST_POS   7
#define RFID_FIRST_POS  1

void set_work_sta(float fSpeed, uint32_t ulCheckPos, uint8_t ucWorkSta)
{
	gWorkWorker.fSpeed = fSpeed;
	SET_CHECK_POS(ulCheckPos);
	if(ucWorkSta != NULL)
		SET_WORK_STA(ucWorkSta);
}

uint8_t wait_module_start(void)
{
	/*非休眠模式 && 前后超声波处于运行状态 && 行走电机正常工作中*/
	if(IS_WAKE_UP_MODE() && !IS_WALK_ERR() && IS_UNUPDATING())
		return 1;
	return 0;
}

float fWorkSpeed;

unsigned char work_all_start(float fParam)
{
	if(gucHomeFlag == 0) return 0;
	if(!BAT_VOL_OVER_ALLOW()) 	//电量过低， 无法启动运行
	{
		debug_printf("电量低于允许巡检电压,无法巡检!\r\n");
		return 1;
	}
	if(gWorkWorker.WorkSta == 0)
	{
        SET_CHECK_POS(BIT(5) | BIT(6));
		if(gWorkWorker.PosDoorFlag)
			SET_WORK_STA(WORK_WAIT_DISPEAR);
		else
			SET_WORK_STA(WORK_WALKING);
		gWorkWorker.WorkStart = 1;
		if(gWorkWorker.CurrentPos < 6)	//当前位置为巡检点1直接进行巡检任务
			gWorkWorker.fSpeed = -fParam;
		else if(gWorkWorker.CurrentPos > 6)
			gWorkWorker.fSpeed = fParam;
		else if(gWorkWorker.CurrentPos == 6)
		{
			if(gucPosLRFlag == 0)	//设备在左边
				gWorkWorker.fSpeed = -fParam;
			else
				gWorkWorker.fSpeed = fParam;
		}
		
		gWorkWorker.IsNeedToHome = 1;
		gWorkWorker.IsNeedPhoto = 1;
		debug_printf("start inspect all\r\n");
		fWorkSpeed = fParam;
		return 1;
	}
	else
		debug_printf("Robot is working,please operate after stop it!\r\n");
	return 0;
}

void work_next(float fParam)
{
	if(gucHomeFlag == 0) return;	//未回过原点不允许动作
	uint8_t ucStart = 0;
	if(((gWorkWorker.CurrentPos < RFID_LAST_POS) && (gWorkWorker.CurrentPos >= RFID_FIRST_POS)) && (gWorkWorker.WorkSta == 0))	//最后一个巡检点
	{
		if(gWorkWorker.CurrentPos <= 5) {	//第二个rfid卡只用于回原,在1或2时需直接跑到第六张RFID卡
			SET_CHECK_POS(BIT(5));
		} else {
			SET_CHECK_POS((gucPosLRFlag == 1) ? BIT(gWorkWorker.CurrentPos) : BIT(gWorkWorker.CurrentPos - 1));
		}
		SET_WORK_STA(gucIsHome ? WORK_WAIT_DISPEAR : WORK_WALKING);
		ucStart = 1;
	}
	else if((gWorkWorker.CurrentPos == RFID_LAST_POS) && (gucPosLRFlag == 0))
	{
		SET_CHECK_POS(BIT(RFID_LAST_POS - 1));
		SET_WORK_STA(WORK_WALKING);
		ucStart = 1;
	}
	if(ucStart)
	{
		gWorkWorker.WorkStart = 1;
		gWorkWorker.fSpeed = -fParam;
		gWorkWorker.IsNeedToHome = 0;
		gWorkWorker.IsNeedPhoto = 0;
		debug_printf("start go to next pos\r\n");
	}
}

void work_last(float fParam)
{
	if(gucHomeFlag == 0) return;
	if(gWorkWorker.WorkSta == 0)	//上一个巡检点
	{
		if(gWorkWorker.CurrentPos <= 5)
			work_home(fParam, 0);
		else
		{
			if(gWorkWorker.CurrentPos == 6)
				work_home(fParam, 0);
			else
			{
				SET_CHECK_POS(BIT(gWorkWorker.CurrentPos - 2));
				SET_WORK_STA(WORK_WALKING);
			}
			gucPosLRFlag = 0;
			gWorkWorker.WorkStart = 1;
			gWorkWorker.fSpeed = ((gWorkWorker.CurrentPos == 1) && (gucPosLRFlag == 0)) ? -fParam : fParam;
			gWorkWorker.IsNeedToHome = 0;
			gWorkWorker.IsNeedPhoto = 0;
		}
		debug_printf("start go to next pos\r\n");
	}
}

void work_home(float fParam, unsigned char ucType)
{
	if(gWorkWorker.WorkSta == 0)
	{
		WalkMotor.IsPosControl = 0;
		SET_WORK_STA(WORK_GO_HOME);
		gWorkWorker.WorkStart = 1;
		gWorkWorker.isNeedCharge = ucType;
		if(gWorkWorker.PosKnownFlag == 0)	//设备位置未知
		{	//设备状态未知，全检
			LOCK_WALK();
			set_work_sta(10.0f, BIT(0) |BIT(1) |BIT(2) |BIT(3) | BIT(4) |BIT(5) | BIT(6), NULL);
		}
		else if(gWorkWorker.CurrentPos == 3)
		{
			LOCK_WALK();
			set_work_sta(-10.0f, BIT(0) |BIT(4), NULL);
		}
		else if(gWorkWorker.CurrentPos == 4)
		{
			if(get_magnet_all_sta())
			{
				set_work_sta(0.0f, BIT(0), WORK_WAIT_BACK_OPEN);
				gulDetectTick = sys_get_ms();
			}
			else
			{
				if((gucPosLRFlag == 0) ||(gucPosLRFlag == 2))
				{
					LOCK_WALK();
					set_work_sta(-10.0f, BIT(0) |BIT(4), NULL);
				}
				else 
				{
					LOCK_WALK();
					set_work_sta(10.0f, BIT(0), WORK_DETECT_BACK);
				}
			}
		}
		else if(gWorkWorker.CurrentPos == 5)
		{
			if(gucPosLRFlag == 0)
			{
				LOCK_WALK();
				set_work_sta(10.0f, BIT(0), WORK_DETECT_BACK);
			}
			else
			{
				set_work_sta(fParam, BIT(0) |BIT(2) |BIT(3) |BIT(4), NULL);
			}
		}
		else if(gWorkWorker.CurrentPos > 5)
		{
			set_work_sta(fParam, BIT(0) | BIT(2)| BIT(3) |BIT(4), NULL);
		}
		else
		{
			set_work_sta(10.0f, BIT(0), NULL);
		}

		gWorkWorker.IsNeedToHome = 0;
		debug_printf("start go to home\r\n");
	}
	else
		debug_printf("Robot is working,please operate after stop it!\r\n");
}

void work_test(float fParam)
{
	if(gWorkWorker.WorkSta == 0)
	{
		//set_work_sta(fParam, BIT(0), WORK_WALKING);
		SET_WORK_STA(WORK_ASK_PHOTO);	
		gWorkWorker.WorkSta = 1;
		gWorkWorker.IsNeedPhoto = 1;
		debug_printf("work test\r\n");
	}
	else
		debug_printf("Robot is working,please operate after stop it!\r\n");
}

void work_stop(void)
{
	if((gWorkWorker.IsNeedToHome == 1) && (gWorkWorker.WorkSta == 1))
		push_plan_status(gWorkWorker.CurrentPlanID, PLAN_ERR, 0, gWorkWorker.usTaskID);
	gWorkWorker.WorkStart = 0;
	gWorkWorker.WorkSta = 0;
	sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_END_INSPECT, NULL, 9);
	walk_stop();
	lift_high_pos(0);
	SET_WORK_STA(WORK_NONE);
	gWorkWorker.IsNeedToHome = 0;
	debug_printf("stop work\r\n");
}

void work_pause(unsigned char * ucSta)
{
	if((gWorkWorker.WorkSta == 1) && (JUDGE_WORK(WORK_WALKING) || (gWorkWorker.CurrentWork == WORK_GO_HOME)))
	{
		*ucSta =gWorkWorker.CurrentWork;
		walk_stop();
		SET_WORK_STA(WORK_PAUSE);
		gulPauseTime = sys_get_ms();
	}
}

void work_resume(unsigned char ucSta)
{
	if((gWorkWorker.WorkSta == 1) && JUDGE_WORK(WORK_PAUSE))
	{
		debug_printf("resume, sta:%u, check:%x\r\n", ucSta, gWorkWorker.CheckPos);
		SET_WORK_STA(ucSta);
		set_walk_speed(gWorkWorker.fSpeed, __func__, __LINE__);
	}	
}

void work_select(float fParam)
{
	uint8_t ucNum, i;
	
	gWorkWorker.CheckPos &= 0x00000007; 	//避免原点及不存在的巡检点进行巡检
	if(gWorkWorker.CheckPos == 1)
		work_home(fParam, 0);
	else if(gWorkWorker.CheckPos > 1)
	{
		ucNum = 0;
		for(i = 0; i < 3; i++)
		{
			if(CHECK_DETECT(1))
			{
				ucNum++;
				break;
			}
			gWorkWorker.CheckPos >>= 1;
			ucNum++;
		}
		ucNum += 4;
		if(gWorkWorker.WorkSta == 0)
		{
			SET_WORK_STA(gucIsHome ? WORK_WAIT_DISPEAR : WORK_WALKING);
			gWorkWorker.WorkStart = 1;
			if(gWorkWorker.CurrentPos == ucNum)
			{
				if(gucPosLRFlag == 0)	//在左边，往右边走
					gWorkWorker.fSpeed = -fParam;
				else
					gWorkWorker.fSpeed = fParam;
			}
			else if(gWorkWorker.CurrentPos < ucNum)
				gWorkWorker.fSpeed = -fParam;
			else if(gWorkWorker.CurrentPos > ucNum)
				gWorkWorker.fSpeed = fParam;
					
			gWorkWorker.IsNeedToHome = 0;
			SET_CHECK_POS(BIT(ucNum - 1));
			debug_printf("work station is %u\r\n", gWorkWorker.CheckPos);
		}
		else
			debug_printf("Robot is working,please operate after stop it!\r\n");
	}
}

static int _do_cmd_workctl(void *pobj, int argc, char *argv[])
{
	float fParam = gSysParameter.fAutoSpeed;
	gulUnWorkTick = sys_get_ms();
	
    if (0 < argc) {
		if(argc == 2)
			fParam = atof(argv[1]);
		if(NULL != sys_strstr(argv[0], "home"))
		{		//复位
			work_home(fParam, 0);
		}
		else if(NULL != sys_strstr(argv[0], "charge"))
		{	//返回充电
			work_home(fParam, 1);
		}
		//if(gucHomeFlag == 0) return -1;
        else if (NULL != sys_strstr(argv[0], "all")) {	//全检
        	if(!IS_Auto_Mode()) return -1;	//巡检任务仅在自动模式下生效
        	if(gucHomeFlag == 0) return -1;
			if(work_all_start(fParam))
			{
				memcpy(gWorkWorker.CurrentPlanID, "000000", 6);
				gWorkWorker.usTaskID = 0;
			}
			end_pm_ctl(__func__, __LINE__);
        }
		else if (NULL != sys_strstr(argv[0], "next")) {	//下一个巡检点
			if(IS_Auto_Mode() || (gucHomeFlag == 0)) return -1;	//自动模式下及未复位时
			work_next(fParam);
		}
		else if (NULL != sys_strstr(argv[0], "last")) {	//上一个巡检点
			if(IS_Auto_Mode() || (gucHomeFlag == 0)) return -1;
			work_last(fParam);
		}
		else if(NULL != sys_strstr(argv[0], "test"))
		{	//测试专用
			debug_printf("mode:%u WorkSta:%u WorkStart:%u Update:%u Reflash:%u GetRealTime:%u\r\n", gWorkWorker.WorkMode, \
				gWorkWorker.WorkSta, gWorkWorker.WorkStart, gOTAFlag.ucUpdateFlag, gWorkWorker.ReflashWorkTime, gWorkWorker.GetRealWorkTime);
		}
		else if(NULL != sys_strstr(argv[0], "stop"))
		{	//停止巡检
//			if(WALK_STOP_STA())
//			{
//				if(IS_WALK_FORWARD())
//					WORK_STOP_WAIT();
//			}
//			else
				work_stop();
			gucManualStop = 1;
			gulManualStopTick = sys_get_ms();
		}
		else if(NULL != sys_strstr(argv[0], "pause"))
		{	//暂停
			work_pause(&gucWorkBack);
		}
		else if(NULL != sys_strstr(argv[0], "resume"))
		{	//恢复运行
			work_resume(gucWorkBack);
		}
		else 
		{	//选择巡检点
			SET_CHECK_POS(sys_atoi(argv[0]));
			if(gWorkWorker.CheckPos > 4)
				return -1;
			work_select(fParam);
		}
    }

	if(gWorkWorker.WorkStart == 1)
		fWorkSpeed = fParam;

    return 0;
}
cmd_register("Work", _do_cmd_workctl, "ALL/NUM/HOME");

int do_cmd_setstatus(void *pobj, int argc, char *argv[])
{
	unsigned char num; 
	gulUnWorkTick = sys_get_ms();
    if (1 == argc) {
		if((gWorkWorker.WorkSta == 1) || (gWorkWorker.WorkStart == 1)) return -1;	//自动模式下不生效
		if(IS_WALK_WORK()) return -1;
		num = sys_atoi(argv[0]);
        if (num == 2)
        {
			start_to_pm_ctl(__func__, __LINE__);
			gucPmMode = 1;
		}
		else if (num == 1)
		{
			end_pm_ctl(__func__, __LINE__);
			gWorkWorker.StayHomeFlag = 0;
		}
    }
    return -1;
} cmd_register("SetStatus", do_cmd_setstatus, "sleedp/wakeup");

unsigned char time_transfer(uint8_t * pucBuf)
{
	Time_info Time_temp;
	uint8_t ucTemp, ucBuf[3] = {0}, i;

	for(i = 0; i < 14; i++)
	{
		if((pucBuf[i] < '0') || (pucBuf[i] > '9'))
			return 0;
	}

	for(i = 0; i < 14; i++)
	{
		ucBuf[i%2] = pucBuf[i];
		if(i % 2)
		{
			ucTemp = sys_atoi((const char *)ucBuf);
			if(ucTemp < 100)
			{
				switch(i / 2) {
					case 0: Time_temp.Year = ucTemp; break;
					case 1: 
						if((ucTemp <= 12) && (ucTemp > 0))
						{
							Time_temp.Month = ucTemp; 
							break;
						}
						else return 0;
					case 2: 
						if((ucTemp < 32) && (ucTemp > 0))
						{
							Time_temp.Day = ucTemp; 
							break;
						}
						else return 0;
					case 3: 
						if(ucTemp < 24)
						{
							Time_temp.Hour = ucTemp; 
							break;
						}
						else return 0;
					case 4: 
						if(ucTemp < 60)
						{
							Time_temp.Minute = ucTemp; 
							break;
						}
						else return 0;
					case 5: 
						if(ucTemp < 60)
						{
							Time_temp.Second = ucTemp; 
							break;
						}
						else return 0;
					case 6:
						if(ucTemp < 7)
						{
							Time_temp.WeekDay = ucTemp; 
							break;
						}
						else return 0;
					default: return 0;
				}
			}
		}
	}

	gCurrentTime = Time_temp;
	debug_printf("Set time to:%02u/%02u/%02u %02u:%02u:%02u %1u\r\n", gCurrentTime.Year, gCurrentTime.Month, gCurrentTime.Day, gCurrentTime.Hour, gCurrentTime.Minute, gCurrentTime.Second, gCurrentTime.WeekDay);
	return 1;
}

static int _do_cmd_settime(void *pobj, int argc, char *argv[])
{
		gulUnWorkTick = sys_get_ms();
    if (1 == argc) {
        time_transfer((uint8_t *)argv[0]);
		gWorkWorker.ReflashWorkTime = 0;
		gWorkWorker.GetRealWorkTime = 0;
    }

    return -1;
}
cmd_register("Settime", _do_cmd_settime, "YYMMDDHHmmSSWW");		//年月日时分秒周几

static int _do_cmd_setspeed(void *pobj, int argc, char *argv[])
{
	float fSpeed;
	//if(IS_Auto_Mode()) return -1;	//自动模式下不支持变更
	gulUnWorkTick = sys_get_ms();
    if (1 == argc) {
       	fSpeed = sys_atof(argv[0]);
		if((fSpeed >= 0.09f) && (fSpeed <= 0.91f))
		{
			gSysParameter.fManualSpeed = fSpeed * 100.0f;
			gSysParameter.fAutoSpeed = fSpeed * 100.0f;
			set_cloud_push_flag(PROP_PARA);
			write_sys_data();
			if(IS_WALK_FORWARD() && !IS_WALK_LOCK())
				work_forward(gSysParameter.fManualSpeed, __func__, __LINE__);
			else if(IS_WALK_BACKWORAD() && !IS_WALK_LOCK())
				work_backward(gSysParameter.fManualSpeed, __func__, __LINE__);
		}
    }

    return -1;
}
cmd_register("SetSpeed", _do_cmd_setspeed, "0.1-0.9");		//设置行走速度

static int _do_cmd_setsyspara(void *pobj, int argc, char *argv[])
{
	float fTemp;
	unsigned long ulTemp;
	//if(IS_Auto_Mode()) return -1;	//自动模式下不支持变更
	gulUnWorkTick = sys_get_ms();
    if (12 == argc) {
		if((memcmp(argv[0], "speed", 5) != 0) || (memcmp(argv[2], "workvol", 7) != 0) || (memcmp(argv[4], "backvol", 7) != 0) ||
			(memcmp(argv[6], "sleepat", 7) != 0) || (memcmp(argv[8], "sleepbf", 7) != 0) || (memcmp(argv[10], "backmode", 8) != 0))
			return -1;
       	fTemp = sys_atof(argv[1]);
		if((fTemp >= 0.09f) && (fTemp <= 0.91f))
		{
			gSysParameter.fManualSpeed = fTemp * 100.0f;
			gSysParameter.fAutoSpeed = fTemp * 100.0f;
		}
		fTemp = sys_atof(argv[3]);
		if((fTemp > 27.9f) && (fTemp < 42.1f))
			gSysParameter.fAllowWorkVal = fTemp;
		fTemp = sys_atof(argv[5]);
		if((fTemp > 27.9f) && (fTemp < 42.1f))
			gSysParameter.fBackCHargeVal = fTemp;
		if(gSysParameter.fAllowWorkVal < gSysParameter.fBackCHargeVal)	
			gSysParameter.fAllowWorkVal = gSysParameter.fBackCHargeVal;
		ulTemp = sys_atoi(argv[7]);
		if((ulTemp >= 60) && (ulTemp <= 1200))
			gSysParameter.ulSleepAfter = ulTemp;
		ulTemp = sys_atoi(argv[9]);
		if((ulTemp >= 180) && (ulTemp <= 1200))
			gSysParameter.ulSleepBefore = ulTemp;
		ulTemp = sys_atoi(argv[11]);
		if((ulTemp == 0) || (ulTemp == 1))
			gSysParameter.ucLowChargeHandlerWay = ulTemp;
		if(IS_WALK_FORWARD() && !IS_WALK_LOCK())
			work_forward(gSysParameter.fManualSpeed, __func__, __LINE__);
		else if(IS_WALK_BACKWORAD() && !IS_WALK_LOCK())
			work_backward(gSysParameter.fManualSpeed, __func__, __LINE__);
		
		set_cloud_push_flag(PROP_PARA);
		write_sys_data();
    }

    return -1;
}
cmd_register("Setpara", _do_cmd_setsyspara, "syspara");		//设置行走速度

static int _do_cmd_plan_clear(void *pobj, int argc, char *argv[])
{
	if(NULL != sys_strstr(argv[0], "clear"))
	{
		clear_all_work_plan();
	}
	else if(NULL != sys_strstr(argv[0], "view"))
	{
		if(gucFlashInitFinish == 0)
		{
			debug_printf("Flash Uninit Finish!\r\n");
			return -1;
		}
		diplay_all_plan();
	}
	else if(NULL != sys_strstr(argv[0], "get"))
	{
		if(gWorkWorker.GetRealWorkTime == 1)
		{
			debug_printf("plan time:%02u:%02u:%02u, Task:%s TaskID:%u\r\n",gWorkWorker.Worktime.Hour, gWorkWorker.Worktime.Minute, gWorkWorker.Worktime.Second, gWorkWorker.CurrentPlanID, gWorkWorker.usTaskID);
		}
		else
			debug_printf("today has no plan\r\n");
	}

    return -1;
}
cmd_register("Plan", _do_cmd_plan_clear, "clear");		//设置允许启动巡检电压


static int _do_cmd_getsyspara(void *pobj, int argc, char *argv[])
{
	gulUnWorkTick = sys_get_ms();
    debug_printf("当前速度:%.1f\r\n允许巡检电压:%.1f\r\n返航充电电压:%.1f\r\n", gSysParameter.fAutoSpeed / 100.0f, gSysParameter.fAllowWorkVal, gSysParameter.fBackCHargeVal);
	debug_printf("无任务休眠时间:%u\r\n任务开始前启动时间:%u\r\n当前模式:%u\r\n",(unsigned int)gSysParameter.ulSleepAfter, (unsigned int)gSysParameter.ulSleepBefore, gSysParameter.ucLowChargeHandlerWay);

    return -1;
}
cmd_register("GetSysPara", _do_cmd_getsyspara, "NULL");		//无任务下休眠时间


unsigned char time_near(void)
{	//快到巡检时间了 <5min 为巡检计划进行提前准备好
	uint32_t ulSecondTick;
	ulSecondTick = get_time_second_count(gCurrentTime, gWorkWorker.Worktime);
	if(ulSecondTick <= gSysParameter.ulSleepBefore)
		return 1;
	return 0;
}

unsigned char work_time_reach(void)
{	//快到巡检时间了 <5min 为巡检计划进行提前准备好
	uint32_t ulSecondTick;
	ulSecondTick = get_time_second_count(gWorkWorker.Worktime, gCurrentTime);
	if(ulSecondTick == 0xFFFFFFFF)
		return 0;
	else if(ulSecondTick <= 300)
		return 1;
	else
		return 2;
}

void auto_work_by_plan(void)
{
	unsigned char ucTemp;

	if(gucTimeGetFlag == 0) return;	//未获得最新时间， 不执行巡检任务
	if(gWorkWorker.WorkSta == 1) return;
	if(IS_WALK_WORK()) return;

	if(gWorkWorker.ReflashWorkTime == 0)
	{	//获取最近一次的巡检时间
		lookup_work_plan(&gWorkWorker.Worktime);
		gWorkWorker.ReflashWorkTime = 1;
	}
	if((gWorkWorker.ReflashWorkTime == 1) && BAT_VOL_OVER_ALLOW())
	{
		if(gWorkWorker.GetRealWorkTime == 1)
		{
			if(IS_WAKE_UP_MODE() && (gucIsHome) && gWorkWorker.StayHomeFlag   &&
				sys_over_time(gWorkWorker.ulStartTick, gSysParameter.ulSleepAfter * 1000))
			{	//距离下次巡检时间至少还要1小时,进入休眠状态
				start_to_pm_ctl(__func__, __LINE__);
				gucPmMode = 0;
				gWorkWorker.StayHomeFlag = 0;
			}

			if(time_near())
			{
				end_pm_ctl(__func__, __LINE__);
				gWorkWorker.StayHomeFlag= 0;
			}

			ucTemp = work_time_reach();
			if(ucTemp == 1)
			{
				debug_printf("Work Start!\r\n");
				end_pm_ctl(__func__, __LINE__);
				work_all_start(gSysParameter.fAutoSpeed);
				if(gWorkWorker.repeatsta == 0)	//为单次模式则删除
					del_work_plan(gWorkWorker.CurrentPlanID, 1);
			}
			else if(ucTemp == 2)
			{
				debug_printf("time out\r\n");
				gWorkWorker.ReflashWorkTime = 0;	
				gWorkWorker.GetRealWorkTime = 0;
			}
		}
		if(((gWorkWorker.GetRealWorkTime == 2) ||(gWorkWorker.GetRealWorkTime == 0)) && IS_WAKE_UP_MODE() && gucIsHome
			&& gWorkWorker.StayHomeFlag && sys_over_time(gWorkWorker.ulStartTick, gSysParameter.ulSleepAfter * 1000))
		{	//今日无巡检计划且未处于低功耗模式
			gWorkWorker.GetRealWorkTime = 3;
			start_to_pm_ctl(__func__, __LINE__);
			gucPmMode = 0;
			gWorkWorker.StayHomeFlag = 0;
		}
	}
}

void robot_status_reflash(void)
{
	unsigned char ucChange = 0;
	if(IS_SLEEP_MODE())
	{	//休眠模式
		if(gRotbotInfo.ucRobotSta != 1)
		{
			gRotbotInfo.ucRobotSta = 1;
			gRotbotInfo.usRobotErrCode = 0;
			ucChange = 1;
		}
	}
	else if(IS_WALK_ERR())
	{	//电机异常
		if(gRotbotInfo.ucRobotSta != 9)
		{
			gRotbotInfo.ucRobotSta = 9;
			ucChange = 1;
		}
		if(gRotbotInfo.usRobotErrCode != 1)
		{
			gRotbotInfo.usRobotErrCode = 1;	//行走电机故障
			ucChange = 1;
		}
	}
	else if(gLiftDevInfo.ucLiftSta == 9)
	{	//升降电机异常
		if(gRotbotInfo.ucRobotSta != 9)
		{
			gRotbotInfo.ucRobotSta = 9;
			ucChange = 1;
		}
		if(gRotbotInfo.usRobotErrCode != 2)
		{
			gRotbotInfo.usRobotErrCode = 2;	//升降电机故障
			ucChange = 1;
		}
	}
	else if(IS_Charge_Sta())
	{	//充电状态
		if(gRotbotInfo.ucRobotSta != 2)
		{
			gRotbotInfo.ucRobotSta = 2;	
			gRotbotInfo.usRobotErrCode = 0;
			ucChange = 1;
		}
	}
	else if(!IS_Auto_Mode())
	{	//人工操作中
		if(gRotbotInfo.ucRobotSta != 3)
		{
			gRotbotInfo.ucRobotSta = 3;	
			gRotbotInfo.usRobotErrCode = 0;
			ucChange = 1;
		}
	}
	else if(gWorkWorker.WorkSta && IS_Auto_Mode() && gWorkWorker.IsNeedToHome)
	{	//巡检中
		if(gRotbotInfo.ucRobotSta != 4)
		{
			gRotbotInfo.ucRobotSta = 4;	
			gRotbotInfo.usRobotErrCode = 0;
			ucChange = 1;
		}
	}
	else if(gWorkWorker.WorkSta && IS_Auto_Mode() && (gWorkWorker.IsNeedToHome == 0))
	{	//设备回原
		if(gRotbotInfo.ucRobotSta != 6)
		{
			gRotbotInfo.ucRobotSta = 6;	
			gRotbotInfo.usRobotErrCode = 0;
			ucChange = 1;
		}
	}
	else
	{	//空闲状态
		if(gRotbotInfo.ucRobotSta != 0)
		{
			gRotbotInfo.ucRobotSta = 0;	
			gRotbotInfo.usRobotErrCode = 0;
			ucChange = 1;
		}
	}

	if(ucChange)
		set_cloud_push_flag(PROP_STATE);
}

void get_time_handler(void)
{
	static unsigned char sucTimeGetFlag = 0;

	if((gCurrentTime.Hour == 0) && (gCurrentTime.Minute == 0))
	{	//每天凌晨刷新时间
		if(sucTimeGetFlag == 0)
		{
			sucTimeGetFlag = 1;
			gucTimeGetFlag = 0;	//重新获取最新时间
		}
	}
	else
	{	//不在该时间段后重置标志
		if(sucTimeGetFlag == 1)
			sucTimeGetFlag = 0;
	}
}


static void _work_process(void)
{
	uint32_t ulDelay = 300;
	static unsigned long sulTick;
	unsigned char * pData;
	static unsigned char sucMinute;

	if(sucMinute != gCurrentTime.Minute)
	{
		debug_printf("CurrentTime:%02u/%02u/%02u %02u:%02u:%02u w:%u\r\n", gCurrentTime.Year, gCurrentTime.Month, gCurrentTime.Day, gCurrentTime.Hour, gCurrentTime.Minute, gCurrentTime.Second, gWorkWorker.CurrentWork);
		sucMinute = gCurrentTime.Minute;
	}

	get_time_handler();
	robot_status_reflash();

	if(gucManualStop && sys_over_time(gulManualStopTick, 2 * 60 * 1000))	//人工停止两分钟内不允许自动巡检
		gucManualStop = 0;

	if((IS_Auto_Mode()) && gucHomeFlag && (gWorkWorker.WorkSta == 0) && (gWorkWorker.WorkStart == 0) && IS_UNUPDATING() && (gucManualStop == 0))
	{	//设备处于未空闲状态 && 不在升级中
		auto_work_by_plan();
	}

	if((!IS_Auto_Mode()) && sys_over_time(gulUnWorkTick, 5 * 60 * 1000))
	{	//其他模式下五分钟无人员操作则切换模式为自动模式
		lift_high_pos(0);
		gWorkWorker.WorkMode = 0;
		gucFrontCtrTime = 0;
		gucBackCtrTime = 0;
		debug_printf("Out Time Operation, Set Mode To 0\r\n");
		set_cloud_push_flag(PROP_MODE);
	}

	if(!IS_WALK_WORK() && (gWorkWorker.WorkSta == 0))
	{
		if(!BAT_VOL_OVER_ALLOW() && IS_Auto_Mode() && !IS_Charge_Sta() && (gucIsCharge == 0) && sys_over_time(gulChargeTick, 61000))
		{
			if(gucIsHome == 0)
			{	//当前设备未在充电仓位置，需先复位
				if(IS_SLEEP_MODE())
				{
					end_pm_ctl(__func__, __LINE__);
					sys_delay_ms(15000);
				}
				work_home(gSysParameter.fAutoSpeed, 1);
				gulChargeTick = sys_get_ms();
				gucChargeSta = 1;
			}
			else 
			{
				start_charging();
			}
		}
	}
	
	if(wait_module_start())
	{	//等待设备正常启动
		if(IS_WALK_STOP() && (gWorkWorker.WorkSta == 0))	//空闲状态
		{
			if(gWorkWorker.WorkStart == 1)
			{
				if(gWorkWorker.IsNeedToHome)
				{
					if(gNvidiaDevInfo.ucNVIDIASta != 0)
						goto QUIT;
				}
				ulDelay = 50;
				if(gLiftDevInfo.ulHigh /100 > 0)	//启动前升降电机不在原位则回原
					lift_high_pos(0);
				while(gLiftDevInfo.ulHigh /100 != 0)	//等待升降电机回原
					sys_delay_ms(10);
				gWorkWorker.WorkSta = 1;
				if(set_walk_speed(JUDGE_WORK(WORK_WAIT_DISPEAR) ? -10.0f : gWorkWorker.fSpeed, __func__, __LINE__))
				{
					if(gWorkWorker.IsNeedToHome == 1)
					{
						push_plan_status(gWorkWorker.CurrentPlanID, PLAN_START, 0, gWorkWorker.usTaskID);
						sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_SET_PLAN, NULL, 0);
						while(!NVIDIA_send_is_idle())	//等待发送完成
							sys_delay_ms(5);
						sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_SET_TASK, NULL, 0);
					}
					gWorkWorker.WorkStart = 0;
					gWorkWorker.ReflashWorkTime = 0;	//巡检后则重新置位刷新巡检时间
					gWorkWorker.GetRealWorkTime = 0;
					gWorkWorker.ulStartTick = sys_get_ms();
				}
				else
					gWorkWorker.WorkSta = 0;
			}
			else
				ulDelay = 100;
		}
		else
		{
			if(gWorkWorker.IsNeedToHome && PERIPHERAL_ERR())
			{
				debug_printf("err work, N:%u Rfid:%u D1:%u D2:%u L:%u!\r\n", gNvidiaDevInfo.ucNVIDIASta, gRfidDevInfo.ucRfidSta, gDetectDevInfo.ucDetectSta1, gDetectDevInfo.ucDetectSta2, gLiftDevInfo.ucLiftSta);
				walk_stop();
				lift_high_pos(0);
				if((gRfidDevInfo.ucRfidSta == 0) && ((gDetectDevInfo.ucDetectSta1 == 0) || (gDetectDevInfo.ucDetectSta2 == 0)) && (gUltrasDevInfo.ucUltrasErrCode1 != 2) && (gUltrasDevInfo.ucUltrasErrCode2 != 2))
				{
					sys_delay_ms(1000);
					gWorkWorker.WorkSta = 0;
					push_plan_status(gWorkWorker.CurrentPlanID, PLAN_ERR, 3, gWorkWorker.usTaskID);
					work_home(gSysParameter.fAutoSpeed, 0);
				}
			}
			if(JUDGE_WORK(WORK_WALKING) && !BAT_VOL_OVER_BACK())
			{	//巡检过程中检测到电压低于设定值则直接返回充电
				walk_stop();
				sys_delay_ms(1000);
				gWorkWorker.WorkSta = 0;
				push_plan_status(gWorkWorker.CurrentPlanID, PLAN_ERR, 3, gWorkWorker.usTaskID);
				work_home(gSysParameter.fAutoSpeed, 1);
			}
			ulDelay = 50;
			if(JUDGE_WORK(WORK_ASK_PHOTO))
			{	//to do,跟英伟达通讯，获取上升高度
				if(sys_over_time(sulTick, 5000))
				{
					//cfg_data_ref(&pData, CFG_RFID_EPC);
					sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_INSPECT_SET,gRfidDevInfo.ucRfidCard, 12);
					sulTick = sys_get_ms();
				}
			}
			else if(JUDGE_WORK(WORK_ASK_GETHIGH))
			{
				if(sys_over_time(sulTick, 5000))
				{
					sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_SET_HIGH,pData, 0);
					sulTick = sys_get_ms();
				}
			}
			else if(JUDGE_WORK(WORKASKEXE)){
				if(sys_over_time(sulTick, 5000))
				{
					sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_INSPECT_EXE, NULL, 0);
					sulTick = sys_get_ms();
				}
			}
			else if(JUDGE_WORK(WAITEXETIONBACK)) {
				if(sys_over_time(sulTick, 5000))
				{
					sys_urcProc(SYS_MODULE_NVIDIA, SYS_ACT_END_INSPECT, NULL, 1);
					sulTick = sys_get_ms();
				}
			}
			else if(JUDGE_WORK(WORK_VIEW_FINISH))
			{
				if(gWorkWorker.CheckPos)
				{
					if(next_need_to_detect(gWorkWorker.CurrentPos))	//后方无巡检点位
						work_forward(gSysParameter.fAutoSpeed, __func__, __LINE__);
					else
						work_backward(gSysParameter.fAutoSpeed, __func__, __LINE__);
					SET_WORK_STA(WORK_WALKING);
				}
				else
				{	
					if(gWorkWorker.IsNeedToHome)
					{
						SET_CHECK_POS(BIT(0) | BIT(2) |BIT(3) |BIT(4));
						SET_WORK_STA(WORK_GO_HOME);
						push_plan_status(gWorkWorker.CurrentPlanID, PLAN_FINISH, 0, gWorkWorker.usTaskID);
						work_backward(gSysParameter.fAutoSpeed, __func__, __LINE__);
					}
					else
					{
						gWorkWorker.WorkSta = 0;
						SET_WORK_STA(WORK_NONE);
					}
				}
			}
			else if(JUDGE_WORK(WORK_PAUSE))
			{
				if(sys_over_time(gulPauseTime, 5 * 60 * 1000))
				{
					work_resume(gucWorkBack);
				}
			}
			else if(JUDGE_WORK(WORK_BLOCK_FRONT))
			{
				if(gUltrasDevInfo.ucUltrasSta1 == 0)
				{	//障碍物被清除
					SET_WORK_STA(gucWorkBack);
					set_walk_speed(gWorkWorker.fSpeed, __func__, __LINE__);
					debug_printf("block resume\r\n");
				}
				if(sys_over_time(gulBlockTick, 5 * 60 * 1000))
				{	//障碍物超过5分钟无处理，则返回原点
					SET_WORK_STA(WORK_NONE);
					gWorkWorker.WorkSta = 0;
					if(gWorkWorker.IsNeedToHome == 1)
						push_plan_status(gWorkWorker.CurrentPlanID, PLAN_ERR, 1, gWorkWorker.usTaskID);
					work_home(gSysParameter.fAutoSpeed, 0);
				}
			}
			else if(JUDGE_WORK(WORK_BLOCK_BACK))
			{
				if(gUltrasDevInfo.ucUltrasSta2 == 0)
				{	//障碍物被清除
					SET_WORK_STA(gucWorkBack);
					set_walk_speed(gWorkWorker.fSpeed, __func__, __LINE__);
				}
			}
		}
	}
QUIT:
	sys_delay_ms(ulDelay);
}

static void _work_init(void)
{
	gWorkWorker.ReflashWorkTime = 0;
	gWorkWorker.GetRealWorkTime = 0;
	while(gucFlashInitFinish == 0)
	{	//等待FLASH启动
		sys_delay_ms(10);
	}
	sys_delay_ms(20000);	//延时1min，等待rfid驱动器起来
	work_home(gSysParameter.fAutoSpeed, 0);	//开机先回原点
	sys_delay_ms(10000);
	set_cloud_push_flag(PROP_MODE);
	set_cloud_push_flag(PROP_STATE);
}

task_define(work, _work_init, _work_process, 0, 512, 6);

