

/******************************************************************************
 * @brief    walk任务
 *
 * Copyright (c) 2020, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-20     worker       inited
 ******************************************************************************/
#include <stdlib.h>
#include "sys_task.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"
#include "walk_task.h"
#include "blink.h"
#include "work_task.h"
#include "rfid_task.h"
#include "charge_task.h"

/* Private function prototypes -----------------------------------------------*/
static void _walk_update_speed(float speed, uint8_t isRun);
static int _walk_opt_req(uint16_t fun, uint16_t act, int32_t param);
static int _walk_rps_proc(unsigned char func, unsigned char *pdata, unsigned short num);

float _walk_radius = 10.0;

/* Private variables ---------------------------------------------------------*/

static uint32_t _walk_accel   = 40000;
static uint32_t _walk_decel   = 40000;
static int _walk_erpm   = 0;
static uint32_t _walk_hb_time = 0;
static uint32_t _walk_curact  = 0;
static float sfWalkSpeed = 0;
static uint8_t _walk_heat_count = 0;
//static uint8_t _walk_heat_flag =0;
static void _walk_pm_before(void);

static st_MODBUS_packet _walk_data_pkt = {
        .dev_info = {
                .id = 0x01,
                .read  = MODBUS_read,
                .write = MODBUS_write,
                .state = DEV_STATE_PWROFF,
                .pkt_type = PKT_TYPE_RTU,
                .crc_poly = 0xA001,
                .crc_val  = 0xFFFF,
                .crc_size = sizeof(uint16_t),
                .phead = NULL,
            },
        .dev_rsp_proc = _walk_rps_proc,
        .rsp = {
                .timeout = 50,
            },
    };
	
static st_blink_dev _walk_dev = {	\
	.gpio = TGT_WALK_PWR,			\
	.BlinkSta = DISABLE,			\
	.NeedAble = ENABLE,				\
	.pm_doing_befor = _walk_pm_before,
	};

static void _walk_pm_before(void)
{
	_walk_dev.ReadyToPm = 0;
}

static void set_walk_sta(uint8_t ucWalkSta, uint16_t usWalkErr)
{
	gWalkDevInfo.ucWalkSta = ucWalkSta;
	gWalkDevInfo.usErrcode = usWalkErr;
}

void walk_speed_change_side_way(float fSpeed)
{
	if(IS_WALK_FORWARD())
		work_forward(fSpeed, __func__, __LINE__);
	else if(IS_WALK_BACKWORAD())
		work_backward(fSpeed, __func__, __LINE__);
}

void walk_sta_change(void)
{
	static unsigned char sucWalkSta = 100;
	static unsigned long sulTick;
	
	if(sucWalkSta == 100)
	{
		sucWalkSta = gWalkDevInfo.ucWalkSta;
		sulTick = sys_get_ms();
	}
	else if(sucWalkSta != gWalkDevInfo.ucWalkSta)
	{
		sucWalkSta = gWalkDevInfo.ucWalkSta;
		set_cloud_push_flag(PROP_WALK);
		sulTick = sys_get_ms();
	}
	else if(IS_WALK_WORK())
	{	//运行状态下每分钟刷新下行走电机状态
		if(sys_over_time(sulTick, 60 * 1000))
		{
			set_cloud_push_flag(PROP_WALK);
			sulTick = sys_get_ms();
		}
	}
}

void walk_move_block_invoid(void)
{	//障碍检测功能
	if(gucDetectEn == 0) return;	//未开启障碍检测时直接退出
	if(IS_WALK_FORWARD() && IS_Auto_Mode())
	{	//机器人处于前进状态 && 自动模式
		if((gUltrasDevInfo.ucUltrasSta1 == 9) && (gUltrasDevInfo.ucUltrasErrCode1 == 1))
			{
				if(gWorkWorker.WorkSta)
				{
					gucWorkBack = gWorkWorker.CurrentWork;
					SET_WORK_STA(WORK_BLOCK_FRONT);
					gulBlockTick = sys_get_ms();
				}
				walk_stop();
			debug_printf("block front\r\n");
		}
	}
	else if(IS_WALK_BACKWORAD() && IS_Auto_Mode())
		{
			if((gUltrasDevInfo.ucUltrasSta2 == 9) && (gUltrasDevInfo.ucUltrasErrCode2 == 1))
			{
				if(gWorkWorker.WorkSta)
				{
					gucWorkBack = gWorkWorker.CurrentWork;
					SET_WORK_STA(WORK_BLOCK_BACK);
					gulBlockTick = sys_get_ms();
				}
				walk_stop();
			debug_printf("test back\r\n");
		}
	}
}

void speed_change_to_locate(float fSpeed, long lLen)
{
	float fTemp = fSpeed;
	
	if(lLen < 5000)
		fTemp = 10.0f;
	else if(lLen < 10000)
		fTemp = 20.0f;
	else if(lLen < 16000)
		fTemp = 30.0f;
	else if(lLen < 23000)
		fTemp = 40.0f;
	else if(lLen < 31000)
		fTemp = 50.0f;
	else if(lLen < 40000)
		fTemp = 60.0f;
	else if(lLen < 50000)
		fTemp = 70.0f;
	else if(lLen < 60000)
		fTemp = 80.0f;

	if((fSpeed - fTemp) > 1.0f)
		walk_speed_change_side_way(fTemp);
}

/* Private functions ---------------------------------------------------------*/
static int _walk_rps_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
    st_MODBUS_response *prsp = &_walk_data_pkt.rsp;
    uint16_t ntemp, usErr;
	int lWalkPos/*, iSpeed*/;
	static unsigned char sucTurnFlag = 0;
	//static unsigned long sulTick;
	short sCurrent = 0;

    if (MODBUS_TIMEOUT_CODE == func) {
        int16_t error = SYS_TIMEOUT;

        cfg_data_write((void *)&error, CFG_WALK_ERR, sizeof(error));
        return 0;
    }
    else if (func & RTU_RSP_FLAG) {
        debug_printf("<= Walkt request_%x is error[opt %x, err %x]", prsp->sub_code, func, pdata[0]);
        return 0;
    }
    else if (!num) {
        return -1;
    }

    switch (prsp->sub_code) {
    case _WALK_FUN_HBREG:
		_walk_heat_count = 0;
        break;
    case _WALK_FUN_MODE:
		if (sizeof(uint32_t) > num) {
            st_MODBUS_data *prlt = (st_MODBUS_data *)pdata;

            sys_data_hton(prlt->data, prlt->data_num);
            if (_WALK_RUN_MODE != *(uint16_t *)prlt->data) {
                _walk_opt_req(RTU_WRITE, _WALK_FUN_MODE, _WALK_RUN_MODE);
            }
            else {
				set_walk_sta(1, 0);	//停止状态
                _walk_curact &= ~_WALK_ACT_MODE;
            }
        }
        break;
    case _WALK_FUN_STATE:
		pdata++;
        usErr = sys_htons(*(uint16_t *)pdata);
        if (0 != usErr) {
            cfg_data_write((void *)&ntemp, CFG_WALK_ERR, sizeof(ntemp));
        }
		pdata += 2;
		//iSpeed = sys_htonl(*(uint32_t *)pdata);
		pdata += 10;
		sCurrent = sys_htons(*(uint16_t *)pdata);
		sCurrent *= 10;
//		if((sys_get_ms() - sulTick) > 500)	//读取实时电流
//		{
//			printf("C:%d mA r:%d\r\n", sCurrent, iSpeed);
//			sulTick = sys_get_ms();
//		}
		if(abs(sCurrent) < 9000)					//电流大于9000mA认为电机已完全堵转
			gulCurrentTick = sys_get_ms();
		if((sys_get_ms() - gulCurrentTick) > 10000)
		{
			gucCurrentError = 1;
			gucTest = 3;
		}
//		if((IS_WALK_WORK() && (abs(sCurrent) > 200)) ||IS_WALK_STOP())	//误检过高，屏蔽
//			sulTick = sys_get_ms();
//		if((sys_get_ms() - sulTick) > 60000)
//		{
//			gucTest = 4;
//			gucCurrentError = 1;
//		}
		pdata += 4;
    case _WALK_FUN_REALTEMP:
		if(prsp->sub_code == _WALK_FUN_REALTEMP)
		{	//第一次开机和运行结束后刷新实时位置
			pdata++;
			_walk_curact &= ~_WALK_ACT_TEMP;
		}
        ntemp = sys_htons(*(uint16_t *)pdata);			
		gWalkDevInfo.usTemp = ntemp * 100;
		pdata += 4;
    case _WALK_FUN_REALPOS:
        lWalkPos = sys_htonl(*(uint32_t *)pdata);
		WalkMotor.ulLastPos = lWalkPos;
		WalkMotor.ulLastPos = 0xFFFFFFFF -WalkMotor.ulLastPos + 1;	//调换方向 
		if(IS_WALK_FORWARD())	//前进方向
		{
			if((sucTurnFlag == 1) && (WalkMotor.ulLastPos < 0x0FFFFFFF))
			{	//上一次的数值是负数，现在变成正数，增加了一圈数据
				WalkMotor.lCurTurn++;
			}
		}
		else if(IS_WALK_BACKWORAD())	//后退方向
		{
			if((sucTurnFlag == 0) && WalkMotor.ulLastPos > 0x0FFFFFFF)
			{	//上次数值是正数，现在变成负数，
				WalkMotor.lCurTurn--;
			}
		}
		gWalkDevInfo.ulPos = WalkMotor.lCurTurn * (0xFFFFFFFF * _WALK_STEP_DISTANCE) + WalkMotor.ulLastPos * _WALK_STEP_DISTANCE;	
		if(IS_WALK_WORK())
		{	//前进方向
			if(WalkMotor.IsPosControl == 1)
			{	//非自动模式下&&距离控制方式
				long lLen = abs(gWalkDevInfo.ulPos - WalkMotor.lTargetPos);
				float fSpeed;
				fSpeed = (gWorkWorker.fSpeed < 0.01f) ? (-gWorkWorker.fSpeed) : gWorkWorker.fSpeed;
				speed_change_to_locate(fSpeed, lLen);
				if(lLen < 800)	//0.1m
				{
					debug_printf("lLen:%d\r\n", lLen);
					walk_stop();
				}
			}
		}
		if(WalkMotor.ulLastPos > 0x0FFFFFFF)
			sucTurnFlag = 1;
		else
			sucTurnFlag = 0;
        break;
    case _WALK_FUN_CURPOS:
        _walk_curact &= ~(_WALK_ACT_POS);
        break;
    case _WALK_FUN_REALSPEED:
        if (sizeof(uint32_t) < num) {
            st_MODBUS_data *prlt = (st_MODBUS_data *)pdata;

            _walk_erpm = sys_htonl(*(uint32_t *)prlt->data);
        }
    case _WALK_FUN_SPEED:
        if (_WALK_ACT_STOP & _walk_curact) {

            //cfg_data_write((void *)&ntmp, CFG_WALK_POS, sizeof(ntmp));
            _walk_curact &= ~_WALK_ACT_STOP;
        }
        else {
            _walk_curact &= ~_WALK_ACT_SPEED;
        }
        break;
	case _WALK_FUN_TRACKSPEED:
		break;
	case _WALK_FUN_TGTABSPOS:
		break;
    case _WALK_FUN_DECEL:
        //_walk_curact &= ~_WALK_ACT_DECEL;
        if (sizeof(uint32_t) < num) {
            st_MODBUS_data *prlt = (st_MODBUS_data *)pdata;

            if(sys_htonl(*(uint32_t *)prlt->data) == _walk_decel)
				_walk_curact &= ~_WALK_ACT_DECEL;
        }
        break;
    case _WALK_FUN_ACCEL:
        //_walk_curact &= ~_WALK_ACT_ACCEL;
        if (sizeof(uint32_t) < num) {
            st_MODBUS_data *prlt = (st_MODBUS_data *)pdata;

            if(sys_htonl(*(uint32_t *)prlt->data) == _walk_accel)
				_walk_curact &= ~_WALK_ACT_ACCEL;
        }
        break;
    default: break;
    }

    return 0;
}

/*
 * @brief       数据发送处理
 * @retval      none
 */
static int _walk_req_rpoc(uint16_t act, uint8_t *pdata, uint8_t num)
{
    uint8_t  fun = RTU_READ;

    if (!num) pdata = NULL;
    switch (act) {
        case _WALK_FUN_TRACKSPEED:
        case _WALK_FUN_TRACKDECEL:
        case _WALK_FUN_TRACKACCEL:
        case _WALK_FUN_TGTRELLAST:
        case _WALK_FUN_TGTRELCUR:
        case _WALK_FUN_TGTABSPOS:
        case _WALK_FUN_TGTSPEED:
        case _WALK_FUN_SPEEDACCEL:
        case _WALK_FUN_SPEEDDECEL:
        case _WALK_FUN_CURPOS:
            if (!pdata) {
                if (_WALK_FUN_TGTRELCUR == act
                    || _WALK_FUN_TGTRELLAST == act
                    || _WALK_FUN_TGTABSPOS == act
                    || _WALK_FUN_CURPOS == act) {
                    act = _WALK_FUN_STATE;
                    fun = RTU_READS;
					num = 12;
                }
                else if (_WALK_FUN_TGTSPEED == act
                    || _WALK_FUN_TRACKSPEED == act) {
                    act = _WALK_FUN_REALSPEED;
                    fun = RTU_READS;
					num = 2;
                }
            }
            else {
                fun = RTU_WRITES;
            }
            break;
        case _WALK_FUN_HANDBRAKECURR:
        case _WALK_FUN_BRAKECURRENT:
        case _WALK_FUN_CLIMBCURRENT:
        case _WALK_FUN_CLIMBACCEL:
        case _WALK_FUN_MAXTORQUE:
        case _WALK_FUN_ZEROMODE:
        case _WALK_FUN_TGTCURRENT:
        case _WALK_FUN_TGTDUTY:
        case _WALK_FUN_CFGTBL:
        case _WALK_FUN_MODE:
            if (!pdata) {
                num = 1;
                if (_WALK_FUN_CLIMBCURRENT == act
                    || _WALK_FUN_BRAKECURRENT == act
                    || _WALK_FUN_HANDBRAKECURR == act) {
                    act = _WALK_FUN_REALCURRENT;
                    fun = RTU_READS;
                }
                else if (_WALK_FUN_TGTDUTY == act) {
                    act = _WALK_FUN_REALDUTY;
                    fun = RTU_READS;
                }
            }
            else {
                fun = RTU_WRITE;
            }
            break;
        case _WALK_FUN_HBREG:
            if (!pdata) {
                num = 1;
            }
            else {
                fun = RTU_WRITE;
                if (4 < *pdata) *pdata = 4;
                else if (!*pdata) *pdata = 1;
            }
            break;
        case _WALK_FUN_REALTEMP:
        case _WALK_FUN_STATE: // readonly
            fun = RTU_READS;
            pdata = NULL;
            num = (_WALK_FUN_REALTEMP == act) ? 4 : 12;
            break;
        default: return -1;
    }

/////////////
    uint16_t addr = act;
    uint8_t  tmp_buf[32];
    st_MODBUS_rtu *prtu = (st_MODBUS_rtu *)tmp_buf;
    st_MODBUS_req *preq = (st_MODBUS_req *)prtu->data;

    prtu->dev_id = _walk_data_pkt.dev_info.id;
    prtu->func = fun;

    preq->reg_addr = sys_htons(addr);
    if (!pdata) {
        preq->reg_info = sys_htons(num);
        num = 0;
    }
    else if (RTU_WRITE == fun) {
        if (sizeof(uint8_t) < num) {
            preq->reg_info = sys_htons(*(uint16_t *)pdata);
        }
        else if (0 < num) {
            preq->reg_info = sys_htons(*pdata);
        }
        else {
            preq->reg_info = 0;
        }

        num = 0;
    }
    else {
        st_MODBUS_data *pval = (st_MODBUS_data *)preq->data;
        char n = sizeof(uint32_t);

        sys_data_hton(pdata, num);
        if (n > num) {
            char i;

            for (i = 0; (num + i) < n; pval->data[i++] = 0);
            for (; 0 < num--; pval->data[i++] = pdata[num]);
            i = n;
        }
        else {
            for (n = 0; num > n; pval->data[n] = pdata[n], n++);
            if (n & 0x1) pval->data[n++] = 0;
        }

        pval->data_num = n;
        preq->reg_info = sys_htons(n >> 1);
        num = sizeof(st_MODBUS_data) + pval->data_num;
    }

    num += sizeof(*prtu) + sizeof(*preq);
    if (0 > MODBUS_send(&_walk_data_pkt.dev_info, tmp_buf, num)) {
        return SYS_ERROR;
    }

    return act;
}

static int _walk_opt_req(uint16_t fun, uint16_t act, int32_t param)
{
    uint8_t num = sizeof(param);

    if (DEV_STATE_WAITRSP & _walk_data_pkt.dev_info.state) {
        return -1;
    }

    if (RTU_READ == fun || RTU_READS == fun) num = 0;
    act = _walk_req_rpoc(act, (void *)&param, num);
    if (0 < (short)fun) {
        _walk_data_pkt.rsp.snd_tick = sys_get_ms();
        _walk_data_pkt.rsp.fun_code = fun;
        _walk_data_pkt.rsp.sub_code = act;
        return 0;
    }
    return -1;
}

static void _walk_update_speed(float speed, uint8_t isRun)
{
	float fTemp;
    int ntmp;


    fTemp = speed;

    ntmp = _WALK_SPEED(fTemp);
    if (ntmp != _walk_erpm) {
        _walk_erpm = ntmp;
    }

    if (true == isRun) {
        _walk_curact |= _WALK_ACT_SPEED;
        _walk_curact |= _WALK_ACT_STATE;
    }
    else {
        _walk_curact &= _WALK_ACT_PWRON;	//清除所有状态
        _walk_curact |= _WALK_ACT_STOP;
        _walk_opt_req(RTU_WRITE, _WALK_FUN_SPEED, 0);
    }
//    else {
//        _walk_curact &= _WALK_ACT_PWRON;
//    }
}

void walk_stop(void)
{
	debug_printf("walk stop\r\n");
	gWalkDevInfo.lSpeed = 0;
	gWalkDevInfo.ucWalkSta = 1;
	_walk_update_speed(0, false);
	_walk_curact &= ~_WALK_ACT_STATE;
	_walk_curact |= _WALK_ACT_TEMP;	//更新当前位置
	if(gWorkWorker.WorkSta == 1)
	{	//自动模式下自动关灯
		gpio_dev_set_status(&gLed_back_dev, DISABLE);
		gpio_dev_set_status(&gLed_front_dev, DISABLE);
	}
}

void walk_set_cur_pos(long lPos)
{
	glWalkCurPos = lPos;
	_walk_curact |= _WALK_SET_CUR_POS;
}

unsigned char set_walk_speed(float speed, const char * func, int line)
{
	if((speed > -0.001f) && (speed < 0.001f))	//避免过慢的速度
		walk_stop();
	else
	{
		gWorkWorker.fSpeed = speed;
		gWorkWorker.StartCheck = 1;						//启动移动时开始检测
		gWorkWorker.ulWalkCheckTick = sys_get_ms();
		if(gWorkWorker.WorkSta == 1)
		{	//自动模式下自动选择方向开关灯
			gpio_dev_set_status(&gLed_back_dev, (speed > 0) ? ENABLE : DISABLE);
			gpio_dev_set_status(&gLed_front_dev, (speed > 0) ? DISABLE : ENABLE);
		}
		if(IS_Auto_Mode() && (speed < 0) && (gUltrasDevInfo.ucUltrasSta1 == 9) && gucDetectEn)	//速度小于0，向前行走，判定前方是否有障碍物
			return 0;
		if(IS_Auto_Mode() && (speed > 0) && (gUltrasDevInfo.ucUltrasSta2 == 9) && gucDetectEn)	//速度大于0，往后行走，判定后方是否有障碍物
			return 0;
		if(IS_Auto_Mode() && (gRfidDevMachine.ucDevSta == 9)) return 0;	//自动模式下RFID识卡器异常
		//if(IS_Charge_Sta()) return 0;					//充电时不允许运行
		if(gucCurrentError && IS_Auto_Mode()) return 1;	//电机电流异常并且自动模式下不允许动作
		gucCurrentError = 0;
		if(gucIsHome && (gucStartCharge || IS_Charge_Sta())) stop_charge();
		if(speed > 0.1f)
			debug_printf("%s:%d:walk backward:%.1f\r\n", func, line, speed);
		else
			debug_printf("%s:%d:walk forward:%.1f\r\n", func, line, speed);
		gWalkDevInfo.lSpeed = (long)(speed * 1000.0f);
		_walk_update_speed(speed, true);
		gucIsHome = 0;
	}
	return 1;
}

unsigned char work_forward(float fSpeed, const char * func, int line)
{
	if(fSpeed < -0.01f)
		fSpeed = -fSpeed;
	UNLOCK_WALK();
	return set_walk_speed(-fSpeed, func, line);
}

unsigned char work_backward(float fSpeed, const char * func, int line)
{
	if(fSpeed < -0.01f)
		fSpeed = -fSpeed;
	UNLOCK_WALK();
	return set_walk_speed(fSpeed, func, line);
}

unsigned char work_forward_lock(float fSpeed, const char * func, int line)
{
	if(fSpeed < -0.01f)
		fSpeed = -fSpeed;
	LOCK_WALK();
	return set_walk_speed(-fSpeed, func, line);
}

unsigned char work_backward_lock(float fSpeed, const char * func, int line)
{
	if(fSpeed < -0.01f)
		fSpeed = -fSpeed;
	LOCK_WALK();
	return set_walk_speed(fSpeed, func, line);
}


static int _walk_data_urc(unsigned short cmd, unsigned char *pdata, unsigned short num)
{
    //st_cfg_cmd *pcmd = (st_cfg_cmd *)&cmd;

	if(!is_dev_enable(&_walk_dev))	//低功耗模式下不响应动作
		return -1;
    if (SYS_ACT_WALK_STOP == cmd) {
        if (_WALK_ACT_STATE & _walk_curact) {
            walk_stop();
        }
    }
    return 0;
}
sys_urc_register(SYS_MODULE_WALK, _walk_data_urc);

char _str2num(char *str1, int * num)
{
	uint8_t ucLen, ucNegFlag = 0;
	
	*num = 0;
	ucLen = strlen(str1);
	for(uint8_t i = 0; i < ucLen; i++)
	{
		if(str1[i] == '-')
			ucNegFlag = 1;
		else if(str1[i] =='+')
			continue;
		else if((str1[i] >= '0') && (str1[i] <= '9'))
			*num = *num * 10 + str1[i] - '0';
		else
			return (char)-1;
	}
	if(ucNegFlag) *num = -*num;
	return 0;
}

static int _do_cmd_walkctrl(void *pobj, int argc, char *argv[])
{
    //float param = 0;
	float ftmp;

	gulUnWorkTick = sys_get_ms();
	if(!is_dev_enable(&_walk_dev))	//低功耗模式 || 自动模式下
		return -1;

    if (0 < argc) {
		WalkMotor.IsPosControl = 0;
        if (NULL != sys_strstr(argv[0], "stop")) {
			gWorkWorker.WorkSta = 0;
			if(WALK_STOP_STA())
			{
				if(IS_WALK_FORWARD())	//仅在出来时才生效
					WALK_STOP_WAIT();
			}
			else
            	walk_stop();
        }
        else {
//            if (1 < argc) {
//                param = atof(argv[1]);
//            }
//			else	//缺省值
//				param = gSysParameter.fManualSpeed;

            if (NULL != sys_strstr(argv[0], "backward")) {	
				if(gucIsHome) return -1;		//不允许在充电仓位置左移，避免撞充电仓
				if((gWorkWorker.WorkSta == 1) && !JUDGE_WORK(WORK_PAUSE)) return -1;
				if(gWorkWorker.CurrentPos <= 4)
					work_home(10.0f, 0);
				else
					work_backward(gSysParameter.fManualSpeed, __func__, __LINE__);
            }
            else if (NULL != sys_strstr(argv[0], "forward")) {
				if((gWorkWorker.WorkSta == 1) && !JUDGE_WORK(WORK_PAUSE)) return -1;
				if(gucIsHome) 
				{
					if(get_magnet_all_sta())
						SET_WORK_STA(WORK_WAIT_DISPEAR);
					gulDetectTick = sys_get_ms();
					work_forward_lock(10.0, __func__, __LINE__);
				}
				else
				{
					work_forward(gSysParameter.fAutoSpeed, __func__, __LINE__);
				}
            }
			else if (NULL != sys_strstr(argv[0], "radius")) {
				if(argc < 2)
					return - 1;
				if (0 > sys_str2float(argv[1], (float *)&ftmp) || (ftmp < 0.001f)) {
					return -1;
				}

				_walk_radius = ftmp;
			}
			else if (NULL != sys_strstr(argv[0], "pos"))
			{
				debug_printf("Current Pos:%u, Turn:%d Pos:%u\r\n", gWalkDevInfo.ulPos, WalkMotor.lCurTurn, WalkMotor.ulLastPos);
			}
            else {
                cli_print(pobj, "Parameter error...\r\n");
                return -1;
            }
        }
    }

    return 0;
}
cmd_register("Walk", _do_cmd_walkctrl, "Walk:stop/forward/backward/status/powon/powoff");

/** brief
  *
 **/
static int _do_cmd_setwalk(void *pobj, int argc, char *argv[])
{
	gulUnWorkTick = sys_get_ms();
	if(!is_dev_enable(&_walk_dev) || gWorkWorker.WorkSta || IS_UPDATING() || gWorkWorker.WorkStart)	//低功耗模式 || 设备自动运行中 || 设备升级中 || 设备等待自动运行中
		return -1;

    if (3 == argc) {
        uint8_t act = sys_atoi(argv[2]);
        int distance = 0;
        float ftmp = 0;
        int sign = -1;

        switch (act) {
        case 3:  // return orgin
        	if(gWorkWorker.WorkSta == 1) return -1;
            work_home(gSysParameter.fManualSpeed, 0);
			break;
        case 2:  // backward
            sign = 1;
        case 0:  // forward
        	if((gWorkWorker.WorkSta == 1) && !JUDGE_WORK(WORK_PAUSE)) return -1;
            if (3 != act /*&& 0 < _walk_wheel_cycle*/) {
                if (0 > sys_str2float(argv[1], (float *)&ftmp) || 0 == (int)ftmp) {
                    break;
                }

                distance = ftmp * 100.0f;
            }

            if (!sys_str2float(argv[0], (float *)&ftmp) && 0 != (int)ftmp) {
                //ftmp *= 1000;
            }

            if (!distance) break;
			WalkMotor.lTargetPos = gWalkDevInfo.ulPos - sign * distance;
			if(WalkMotor.lTargetPos < 0)
				WalkMotor.lTargetPos = 0;
			WalkMotor.IsPosControl = 1;
			sfWalkSpeed = (float)sign * ftmp;
			if(sign == 1)
			{
				if(gucIsHome) return 0;
				if(gWorkWorker.CurrentPos <= 4)
					work_home(10.0, 0);
				else
					work_backward(sfWalkSpeed, __func__, __LINE__);
			}
			else
			{
				if(gucIsHome) 
				{
					if(get_magnet_all_sta())
						SET_WORK_STA(WORK_WAIT_DISPEAR);
					gulDetectTick = sys_get_ms();
					work_forward_lock(10.0, __func__, __LINE__);
				}
				else
					work_forward(sfWalkSpeed, __func__, __LINE__);
			}
			
            return 0;
        case 1: // stop
            walk_stop();
            return 0;
        default: break;
        }
    }
    return -1;
}
cmd_register("SetWalk", _do_cmd_setwalk, "SetWalk:speed;distance");

static void _module_process(void)
{	
	static unsigned char sucFlag = 0x07;
	static unsigned long sulTick;
	unsigned int ulTaskDelay = 50;

	walk_sta_change();	//判定行走电机状态是否变更，变更则刷新状态
	walk_move_block_invoid();	//障碍规避功能
	if(IS_WALK_WORK())
		gulUnWorkTick = sys_get_ms();

	if(gucCurrentError && IS_WALK_WORK())	//电流异常 && 电机运动中
		work_stop();

	if(is_dev_enable(&_walk_dev))
	{	//等待使能口使能
		ulTaskDelay = IS_WALK_STOP() ? 50 : 10;
	    MODBUS_process(&_walk_data_pkt);	
		ulTaskDelay = (_walk_curact & _WALK_ACT_STATE) ? 10 : ulTaskDelay;

		if(!WALK_STOP_STA())
		{
			if(IS_WALK_STOP_WAIT())
			{
				WALK_STOP_CLEAR(); 
				gWorkWorker.WorkSta = 0;
				walk_stop();
			}
			if(IS_WORK_WAIT())
			{
				WORK_STOP_CLEAR();
				work_stop();
			}
		}

		if(gucCurrentError && IS_WALK_STOP())
		{
			set_walk_sta(9, 3);
			set_cloud_push_flag(PROP_WALK);
			if((gWorkWorker.IsNeedToHome == 1) && gWorkWorker.WorkSta)
			{
				push_plan_status(gWorkWorker.CurrentPlanID, PLAN_ERR, 2, gWorkWorker.usTaskID);
				gWorkWorker.WorkSta = 0;
			}
		}

	    if (_WALK_ACT_CYCLE & _walk_curact) {	//驱动器上电后先设置速度为0
	        walk_stop();
			gucTest = 0;
			set_walk_sta(9, 2);
	        _walk_curact |= _WALK_ACT_MODE;
	    }
		else if (_WALK_ACT_MODE & _walk_curact) {	//设置模式为速度模式
	        _walk_opt_req(RTU_READ, _WALK_FUN_MODE, _WALK_RUN_MODE);
	    }
		else if ((sucFlag & 0x01) && (_WALK_ACT_PWRON & _walk_curact)) {
	        if(_walk_opt_req(RTU_WRITE, _WALK_FUN_ACCEL, _walk_accel) == 0)	//加速度:速度环加速度
				sucFlag &= ~0x01;
	    }
	    else if ((sucFlag & 0x02) && (_WALK_ACT_PWRON & _walk_curact)) {
	        if(_walk_opt_req(RTU_WRITE, _WALK_FUN_DECEL, _walk_decel) == 0)	//减速度:速度环减速度
				sucFlag &= ~0x02;
	    }
		else if (sys_istimeout(_walk_hb_time, _WALK_BEATTIME)) {
	        static uint8_t hb_reg = 0;

	        if(_walk_opt_req(RTU_WRITE, _WALK_FUN_HBREG,  _WALK_HBTIME_REG - (0x1 & hb_reg++)) == 0)
	        {
				_walk_heat_count++;
				if(_walk_heat_count > 10)
				{
					gucTest = 1;
					gWalkDevInfo.ucWalkSta = 9;		//初始化为故障状态
					gWalkDevInfo.usErrcode = 2;		//故障信息为未初始化
					_walk_curact = _WALK_ACT_CYCLE;
					_walk_heat_count = 0;
				}
		        _walk_hb_time = _walk_data_pkt.rsp.snd_tick;
	        }
	    }
		else if (_WALK_ACT_SPEED & _walk_curact) {	//设置速度
	        if(_walk_opt_req(RTU_WRITE, _WALK_FUN_SPEED, _walk_erpm) == 0)
	        {
				if(_walk_erpm == 0) gWalkDevInfo.ucWalkSta = 1;	//停机状态
				else if(_walk_erpm > 0) gWalkDevInfo.ucWalkSta = 2;	//后退状态
				else gWalkDevInfo.ucWalkSta = 0;	//前进状态
	        }
	    }
	    else if (_WALK_ACT_STOP & _walk_curact) {	//停机时不读取位置及速度
	        if(_walk_opt_req(RTU_WRITE, _WALK_FUN_SPEED, 0) == 0)
				_walk_curact &= ~_WALK_ACT_STATE;
	    }
		else if (_WALK_SET_CUR_POS & _walk_curact) {	//停机时不读取位置及速度
	        if(_walk_opt_req(RTU_WRITE, _WALK_FUN_CURPOS, glWalkCurPos) == 0)
	        {
	        	if(glWalkCurPos == 0)
					WalkMotor.lCurTurn = 0;
				_walk_curact &= ~_WALK_SET_CUR_POS;
	        }
	    }
	    else if (((_WALK_ACT_STATE & _walk_curact) && ((sys_get_ms() - sulTick) > 1000)) || ((sys_get_ms() - sulTick) > 2000)) {
	        _walk_opt_req(RTU_READ, _WALK_FUN_STATE, 0);
			sulTick = sys_get_ms();
	    }
		else if ((_WALK_ACT_TEMP & _walk_curact) || (sucFlag & 0x04)) {	//上电前先获取位置信息
	        _walk_opt_req(RTU_READ, _WALK_FUN_REALTEMP, 0);
			sucFlag &= ~0x04;
	    }
	}
	else	//低功耗模式或未完成使能则初始化状态
	{
		_walk_curact = _WALK_ACT_CYCLE;
		gucTest = 2;
		set_walk_sta(9, 1);
		gulCurrentTick = sys_get_ms();
	}

	sys_delay_ms(ulTaskDelay);
}

/*
 * @brief    walk初始化
 */
static void _module_init(void)
{
#ifdef TGT_WALK_CFG
    static unsigned char _walk_rxbuf[DEV_RXBUF_SIZE];
    static st_uart_info _walk_port = TGT_WALK_CFG;

    if (0 <= BSP_uart_init(&_walk_port, _walk_rxbuf, sizeof(_walk_rxbuf))) {
        ring_buf_init(&_walk_port.tx_buf, NULL, 0);
        _walk_data_pkt.dev_info.pobj = &_walk_port;
        _walk_curact = _WALK_ACT_CYCLE;
    }
#endif
	gpio_dev_register(&_walk_dev);	//注册GPIO使能口
	pm_dev_register(&_walk_dev);	//注册行走电机为低功耗部件
	gulRegisterFlag &= ~BIT(0);
	gWalkDevInfo.lSpeed = 0;
	gWalkDevInfo.ulPos = 0;
	gWalkDevInfo.usTemp =2500;
	set_cloud_push_flag(PROP_WALK);	//上报行走电机状态
}

task_define(walk, _module_init, _module_process, MODBUS_TASK_INTVERAL, 1024, MODBUS_TASK_PRI);
