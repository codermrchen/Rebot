/******************************************************************************
 * @brief    rific control
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
#include "charge_task.h"
#include "walk_task.h"
#include "work_task.h"
#include "tempture_task.h"
#include "rfid_task.h"

#define _RFID_REG_WRITE                 (0x02)
#define _RFID_REG_READ                  (0x03)
#define _RFID_REG_AUTO                  (0xEE)

static uint8_t suclastRfidCard = 0;
static uint8_t sucDetectSta = 0;

RFID_PARA Rfid_data[] = {
	{0x5D9,  1,  0.0f, 1},		//最左侧Rfid
	{0x5bf,	 2,	 0.5f, 0},		//充电仓内右侧RFID卡
	{0x5D1,	 3,	 1.0F, 0},		//充电仓门外最左侧RFID卡，用于返回开门
	{0x5C0,  4,  1.5f, 0},		//磁感应开关左侧
	{0x5C2,  5,  2.50f, 0},	//回原磁感应RFID卡
	{0x5DA,  6,  9.60f, 1},	//第一个巡检点
	{0x5C8,  7,  18.70f, 1},	//第二个巡检点
	{0x5C7,  8,  20.30f, 1},	//第三个巡检点 目前暂未开启使用
};

#ifdef TGT_RFID_CFG
static unsigned char _rfid_rxbuf[DEV_RXBUF_SIZE];
static st_uart_info _rfid_port;
#endif

uint8_t get_magnet_sta(uint8_t ucNo)
{
	uint8_t ucSta;
	if(ucNo == 1)
		ucSta = !GPIO_ReadInputDataBit(DETECT1_PORT, DETECT1_PIN);
	else if(ucNo == 2)
		ucSta = !GPIO_ReadInputDataBit(DETECT2_PORT, DETECT2_PIN);
	return ucSta;
}

uint8_t get_magnet_all_sta(void)
{
	if(get_magnet_sta(1) ||get_magnet_sta(2))
		return 1;
	return 0;
}

//匹配RFID卡是否一致
uint8_t RFID_MATCH(uint32_t RfID)
{
	uint32_t ulNum, ulCount;
	
	ulNum = OBJ_ARRAY_NUM(Rfid_data);
	for(ulCount = 0; ulCount < ulNum; ulCount++)
	{
		if(Rfid_data[ulCount].ulRfidCard_ID == RfID)
		{
			gRfidDevInfo.ulRfidDistanse = Rfid_data[ulCount].fDistanse * 100;
			return Rfid_data[ulCount].ulRfidCard_No;
		}
	}

	return 0;
}

/* Private function prototypes -----------------------------------------------*/
static int _rfid_auto_rsp(unsigned char func, unsigned char *pdata, unsigned short num);

/* Private variables ---------------------------------------------------------*/
//static st_pos_info _rfid_info;
static st_MODBUS_packet _rfid_data_pkt = {
        .dev_info = {
                .id   = 0x01, //local device's id
                .read     = MODBUS_read,
                .write    = MODBUS_write,
                .pkt_type = PKT_TYPE_TL,
                .phead    = NULL,
                .crc_poly = 0x8408,
                .crc_size = 2,
            },
        .dev_rsp_proc = _rfid_auto_rsp,
        .rsp = {
                .timeout  = 3000,
            },
    };

/* Private functions ---------------------------------------------------------*/
static int _rfid_auto_rsp(unsigned char func, unsigned char *pdata, unsigned short num)
{
	uint32_t rfid_card = 0;
	uint8_t ucRfidMatch;
	
    _rfid_data_pkt.rsp.snd_tick = sys_get_ms();
	memcpy(gRfidDevInfo.ucRfidCard, pdata + 1, (num <= 13) ? (num - 1) : 12); 
	for(int i = 0; i < num; i++)
		rfid_card += *pdata++;

	if(gRfidDevInfo.ucRfidSta == 9)
	{
		gRfidDevInfo.ucRfidSta = 0;
		gRfidDevInfo.usRfidErrSta = 0;
	}
	if(gRfidDevMachine.ucDevSta == 9)
	{
		gRfidDevMachine.ucDevSta = 0;
		gRfidDevMachine.ucDevErrCode = 0;
		set_cloud_push_flag(PROP_RFID_DEV);
	}
	ucRfidMatch = RFID_MATCH(rfid_card);
	if(suclastRfidCard == 0)
		suclastRfidCard = ucRfidMatch;
	else
	{
		if(suclastRfidCard != ucRfidMatch)
		{	
			debug_printf("rfid:%u\r\n", ucRfidMatch);	//只打印一次
			suclastRfidCard = ucRfidMatch;
			if(sucDetectSta == 0 && ucRfidMatch > 5)
			{	//两个Rfid之间识别不到磁铁则认为磁铁感应器损坏
				if((gDetectDevInfo.ucDetectSta1 != 9) || (gDetectDevInfo.ucDetectSta2 != 9))
				{
					gDetectDevInfo.ucDetectSta1 = 9;
					gDetectDevInfo.ucDetectSta2 = 9;
					set_cloud_push_flag(PROP_HALL_SWITCH);
				}
			}
			sucDetectSta = 0;
		}
	}
	if(ucRfidMatch)
	{
		gWorkWorker.PosDoorFlag = (ucRfidMatch <= 2) ? 1 : 0;	//当前位置小于等2时，设备位于充电仓内，否则充电仓外
		gWorkWorker.PosKnownFlag = 1;
		if(gucNextRfid == 0)
			gucNextRfid = ucRfidMatch;
		if(gucCurRfid == 0)
		{
			gucCurRfid = ucRfidMatch;
			set_cloud_push_flag(PROP_RFID_DEV);
		}
		if(gucCurRfid != ucRfidMatch)
		{
			if(Rfid_data[ucRfidMatch - 1].ucPushFlag)
				set_cloud_push_flag(PROP_RFID_DEV);
			if(ucRfidMatch > gucCurRfid)
			{
				gucNextRfid = (ucRfidMatch < 5) ? (ucRfidMatch + 1) : ucRfidMatch;
			}
			else {
				gucNextRfid = (ucRfidMatch > 1) ? (ucRfidMatch - 1) : ucRfidMatch;
			}
			gucCurRfid = ucRfidMatch;
		}
		else if(IS_WALK_FORWARD())
		{
			gucNextRfid = (ucRfidMatch < 3) ? (ucRfidMatch + 1) : ucRfidMatch;
			if(ucRfidMatch >= 4 && WALK_STOP_STA())	//设备出来后使能停止
				WALK_STOP_EN();
		}
		else if(IS_WALK_BACKWORAD())
		{
			gucNextRfid = (ucRfidMatch > 1) ? (ucRfidMatch - 1) : ucRfidMatch;
			if(ucRfidMatch <= 4 && !WALK_STOP_STA())	//设备进去后禁止停止
				WALK_STOP_DIS();
		}
		
		gWorkWorker.CurrentPos = ucRfidMatch;
		if(IS_WALK_FORWARD())	//设备往右走，设备在当前识别到的rfid卡右边
			gucPosLRFlag = 1;
		else if(IS_WALK_BACKWORAD()) //设备往左走，设备在当前识别到的rfid卡的左边
		{
			gucPosLRFlag = 0;
			if(!JUDGE_WORK(WORK_GO_HOME) && !JUDGE_WORK(WORK_DETECT_BACK))
			{
				if(ucRfidMatch == 5)
				{
					gWorkWorker.WorkSta = 1;
					work_backward_lock(10.0f, __func__, __LINE__);
					SET_WORK_STA(WORK_DETECT_BACK);
					SET_CHECK_POS(BIT(0));
				}
				else if(ucRfidMatch == 3 || ucRfidMatch == 4)
				{
					gWorkWorker.WorkSta = 1;
					work_forward_lock(10.0f, __func__, __LINE__);
					SET_WORK_STA(WORK_GO_HOME);
					SET_CHECK_POS(BIT(0) | BIT(4));
				}
			}
		}
		if(JUDGE_WORK(WORK_GO_HOME) && IS_WALK_BACKWORAD() && (ucRfidMatch == 2) && !((gWorkWorker.fSpeed < 15.0f) && (gWorkWorker.fSpeed > -15.0f)))	//进来后检测到第二个RFID卡减速，避免撞到充电仓内部
			work_backward_lock(10.0f, __func__, __LINE__);
		if((ucRfidMatch > 5) || ((ucRfidMatch == 5) && (gucPosLRFlag == 1))) gucDetectEn = 1;	//在减速RFID卡后方使能检测功能，否则关闭
		else gucDetectEn = 0;
		if(gWorkWorker.WorkSta && CHECK_DETECT(ucRfidMatch))
		{	//到达检测点及设备处于运行状态
			if(JUDGE_WORK(WORK_WALKING) ||JUDGE_WORK(WORK_GO_HOME))
			{
				DELETE_DETECT(ucRfidMatch);
				if(JUDGE_WORK(WORK_GO_HOME))
				{
					if(ucRfidMatch > 5)
					{
						if(IS_WALK_BACKWORAD())
							work_backward(gSysParameter.fAutoSpeed, __func__, __LINE__);
						SET_CHECK_POS(BIT(0) | BIT(4));
					}
					else if(ucRfidMatch == 5)
					{
						work_backward_lock(10.0f, __func__, __LINE__);
						SET_WORK_STA(WORK_DETECT_BACK);
						SET_CHECK_POS(BIT(0));
					}
					else if((ucRfidMatch == 3) || (ucRfidMatch == 4))
					{
						work_forward_lock(10.0f, __func__, __LINE__);
						SET_CHECK_POS(BIT(0) | BIT(4));
					}
					else if(ucRfidMatch == 1)
					{
						work_forward_lock(10.0f, __func__, __LINE__);
						SET_WORK_STA(WORK_GO_HOME_BACK);
					}
				}
				else {	//巡检时直接进行减速即可
					if(!last_need_to_detect(ucRfidMatch))
					{
						work_forward_lock(10.0f, __func__, __LINE__);
						SET_WORK_STA(WORK_DETECTION);	
					}
					else
					{	//前方漏检了，则往回走
						work_backward(gSysParameter.fAutoSpeed, __func__, __LINE__);
						ADD_DETECT(ucRfidMatch);
					}
				}
			}
		}
		else if(gWorkWorker.WorkSta && (ucRfidMatch == 8))
		{	//检测到终点RFID卡，强制往回走
			work_backward(gSysParameter.fAutoSpeed, __func__, __LINE__);
		}
	}
    return 0;
}


static void _module_process(void)
{
	uint8_t ucInputSta1, ucInputSta2;
	static uint32_t sulChargeTick;//, sulTick;
	
    MODBUS_process(&_rfid_data_pkt);
	if(gucRfidTest == 1)
	{
		debug_printf("Rfid Error!\r\n");
		BSP_uart_init(&_rfid_port, _rfid_rxbuf, sizeof(_rfid_rxbuf));
		gucRfidTest = 0;
	}
	ucInputSta1 = get_magnet_sta(1);
	ucInputSta2 = get_magnet_sta(2);
	if(ucInputSta1 ||ucInputSta2)
	{
		if(ucInputSta1 && gDetectDevInfo.ucDetectSta1 == 9)
		{
			gDetectDevInfo.ucDetectSta1 = 0;
			gDetectDevInfo.ucDetectErrSta1 = 0;
			set_cloud_push_flag(PROP_HALL_SWITCH);
		}

		if(ucInputSta2 && gDetectDevInfo.ucDetectSta2 == 9)
		{
			gDetectDevInfo.ucDetectSta2 = 0;
			gDetectDevInfo.ucDetectErrSta2 = 0;
			set_cloud_push_flag(PROP_HALL_SWITCH);
		}

		if(sucDetectSta == 0)
			sucDetectSta = 1;
	}
	if(ucInputSta1 || ucInputSta2)
	{
		if(JUDGE_WORK(WORK_DETECTION) || JUDGE_WORK(WORK_GO_HOME_BACK) || 
			JUDGE_WORK(WORK_DETECT_FRONT) || JUDGE_WORK(WORK_DETECT_BACK))
		{	//巡检下一个点或回原
			walk_stop();
			if(JUDGE_WORK(WORK_DETECTION))
			{
				if(gWorkWorker.IsNeedPhoto == 1)
				{
					SET_WORK_STA(WORK_ASK_PHOTO);	
					debug_printf("start to view\r\n");
				}
				else
				{
					SET_WORK_STA(WORK_NONE);	
					gWorkWorker.WorkSta = 0;
				}
			}
			else if(JUDGE_WORK(WORK_GO_HOME_BACK))
			{
				debug_printf("go to home finish!\r\n");
				gucIsHome = 1;
				gucHomeFlag = 1;
				if(IS_Auto_Mode())
				{
					gWorkWorker.StayHomeFlag = 1;
					gWorkWorker.ulStartTick = sys_get_ms();
				}
				sys_delay_ms(300);
				walk_set_cur_pos(0);	//设置位置为0
				if((gWorkWorker.isNeedCharge == 1) || (gucChargeSta == 1))
				{
					gucChargeSta = 0;
					SET_WORK_STA(WORK_WAIT_CHARGE);
					sulChargeTick = sys_get_ms();
				}
				else
				{
					gWorkWorker.WorkSta = 0;
					SET_WORK_STA(WORK_NONE);	
				}
			}
			else if(JUDGE_WORK(WORK_DETECT_FRONT))
			{
				walk_stop();
				SET_WORK_STA(WORK_WAIT_FRONT_OPEN);
				gulDetectTick = sys_get_ms();
			}
			else
			{
				walk_stop();
				SET_WORK_STA(WORK_WAIT_BACK_OPEN);
				gulDetectTick = sys_get_ms();
			}
		}
	}
	else
	{
		if(JUDGE_WORK(WORK_WAIT_DISPEAR))
			SET_WORK_STA(WORK_DETECT_FRONT);
	}

	if(JUDGE_WORK(WORK_WAIT_FRONT_OPEN))
	{
		if(get_front_distance() < 500)
			gulDetectTick = sys_get_ms();
		if(sys_over_time(gulDetectTick, 10000))
		{
			work_forward(gSysParameter.fAutoSpeed, __func__, __LINE__);
			SET_WORK_STA((gWorkWorker.WorkSta)? WORK_WALKING : WORK_NONE);
		}
	}
	if(JUDGE_WORK(WORK_WAIT_BACK_OPEN))
	{
		if(get_back_distance() < 500)
			gulDetectTick = sys_get_ms();
		if(sys_over_time(gulDetectTick, 10000))
		{	//回原时默认使用0.3的速度，避免移动过程中门关闭
			work_backward_lock(30.0f, __func__, __LINE__);
			SET_WORK_STA(WORK_GO_HOME);
		}
	}	

	if(JUDGE_WORK(WORK_WAIT_CHARGE) && sys_over_time(sulChargeTick, 15 * 1000)) //到点后稳定15s后再充电)
	{
		gucChargeSta = 2;
		start_charging();
		SET_WORK_STA(WORK_NONE);
		gWorkWorker.WorkSta = 0;
	}
	if(gWorkWorker.StartCheck && IS_WALK_WORK() && sys_over_time(gWorkWorker.ulWalkCheckTick, 150 * 1000))
	{
		work_stop();
		gRfidDevMachine.ucDevSta = 9;
		gRfidDevMachine.ucDevErrCode = 1;
		set_cloud_push_flag(PROP_RFID_DEV);
	}
	if(JUDGE_WORK(WORK_PAUSE))
		gWorkWorker.ulStartTick = sys_get_ms();
	
	sys_delay_ms(10);
}

static void _module_init(void)
{
#ifdef TGT_RFID_CFG
    _rfid_port = (st_uart_info)TGT_RFID_CFG;

    if (0 <= BSP_uart_init(&_rfid_port, _rfid_rxbuf, sizeof(_rfid_rxbuf))) {
        ring_buf_init(&_rfid_port.tx_buf, NULL, 0);
        _rfid_data_pkt.dev_info.pobj = &_rfid_port;
    }
#endif
	gRfidDevInfo.ucRfidSta = 0;
	gRfidDevInfo.usRfidErrSta = 0;
	gRfidDevInfo.ulRfidDistanse = 0;
	gDetectDevInfo.ucDetectSta1 = 0;
	gDetectDevInfo.ucDetectSta2 = 0;
	gDetectDevInfo.ucDetectErrSta1 = 0;
	gDetectDevInfo.ucDetectErrSta2 = 0;
	set_cloud_push_flag(PROP_HALL_SWITCH);
	gRfidDevMachine.ucDevSta = 0;
	gRfidDevMachine.ucDevErrCode = 0;
	set_cloud_push_flag(PROP_RFID_DEV);
	gpucTest = (unsigned char *)&_rfid_data_pkt;
}

task_define(rfid, _module_init, _module_process, MODBUS_TASK_INTVERAL, MODBUS_TASK_STACK, 9);
