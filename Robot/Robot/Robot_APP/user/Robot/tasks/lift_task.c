/******************************************************************************
 * @brief    lift control
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
#include "lift_task.h"

/* Private function prototypes -----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

//////////////////////////////////////////


/* Private function prototypes -----------------------------------------------*/
//static void _module_poweron(void);
static int _lift_req_rpoc(e_RTU_func func, unsigned short act, unsigned short val);
static int _lift_rps_proc(unsigned char func, unsigned char *pdata, unsigned short num);
static void _lift_pm_before(void);

/* Private varibles ---------------------------------------------------------*/
static uint16_t _lift_expect_high = 0;
static uint8_t _lift_working = 0;
static uint8_t _lift_get_time = 0;	//0:单次获取 1:持续获取
static uint8_t _lift_status = 0;
static st_lift_info _lift_info = {
            .state = SYS_STATE_PWRDN,
        };

static st_MODBUS_packet _lift_data_pkt = {
        .dev_info = {
                .id = 0xAB,
                .read = MODBUS_read,
                .write = MODBUS_write,
                .pkt_type = PKT_TYPE_RTU,
                .phead = NULL,
                .crc_poly = 0xA001,
                .crc_val  = 0xFFFF,
                .crc_chk  = 0,
                .crc_size = 2,
            },
        .dev_rsp_proc = _lift_rps_proc,
        .rsp = {
                .timeout = 2000,
            },
    };

static st_blink_dev _lift_dev = {	\
		.gpio = TGT_LIFT_PWR,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE, 			\
		.pm_doing_befor = _lift_pm_before,	\
	};

static void _lift_pm_before(void)
{
	static unsigned char sucSta = 0;

	if(gLiftDevInfo.ucLiftSta == 9)
	{	//设备异常时不等待设备下降
		_lift_dev.ReadyToPm = 0;
		return;
	}
	
	if(sucSta == 0)
	{	//断电前先回0位
		lift_high_pos(0);
		sucSta = 1;
		_lift_dev.tick = sys_get_ms();
	}
	else if(sucSta == 1)
	{	//等待
		if(_lift_dev.ReadyToPm == 1)
		{
			if(sys_over_time(_lift_dev.tick, 10000))
				sucSta = 0;
		}
		else if(_lift_dev.ReadyToPm == 2)
		{
			sucSta = 0;
			_lift_dev.ReadyToPm = 0;
		}
	}
}

static void set_lift_status(uint8_t ucLiftSta, uint16_t usErrorCode)
{
	gLiftDevInfo.ucLiftSta = ucLiftSta;
	gLiftDevInfo.usErrcode = usErrorCode;
}

/* Private functions ---------------------------------------------------------*/
static int _lift_rsp_process(unsigned char *pdata, unsigned short num)
{
	uint16_t usErrorSta = 0;
	_lift_status = 0;
    switch (_lift_data_pkt.rsp.sub_code) {
        case _ACT_GET_STAT:
			usErrorSta = *pdata;
			pdata++;
			usErrorSta |= (*pdata << 8);
			if(usErrorSta >= 2)
				return _lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0);
			break;
            //return _lift_req_rpoc(RTU_READ, _ACT_GET_ERR, 0);
        case _ACT_GET_ERR:
			usErrorSta = *pdata;
			pdata++;
			usErrorSta |= (*pdata << 8);
			debug_printf("Error Code:%04X\r\n", usErrorSta);
			_lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x03);
            //cfg_data_write(pdata, CFG_LIFT_ERR, num);
            break;
        case _ACT_GET_HIGH: {
            uint32_t ntmp = sys_byte2num(pdata, num);
			gLiftDevInfo.ulHigh = (ntmp - LIFT_DEFAULT_HIGH) * 100;
			if(gLiftDevInfo.ulHigh > 100000)
				gLiftDevInfo.ulHigh = 0;
			set_cloud_push_flag(PROP_LIFT);
			if(_lift_working == Lift_GetHigh)
			{
	            if (ntmp == _lift_expect_high) {
					_lift_working = Lift_None;
					if(ntmp == _lift_expect_high)
					{
						if(_lift_dev.ReadyToPm == 1)
							_lift_dev.ReadyToPm = 2;
					}
					if(JUDGE_WORK(WAITLIFTREACH))
						SET_WORK_STA(WORKASKEXE);
					if(JUDGE_WORK(WAITEXETIONBACK))
						SET_WORK_STA(WORK_VIEW_FINISH);
                    _lift_expect_high = 0;
	                break;
	            }
//	            else
//				{
//					sys_delay_ms(10);
//					_lift_req_rpoc(RTU_READ, _ACT_GET_HIGH, 0);
//	            }
			}
			else
				_lift_working = Lift_None;

            cfg_data_write((void *)&ntmp, CFG_LIFT_HIGH, sizeof(ntmp));
			break;
        }
        case _ACT_SET_HIGH:
            sys_delay_ms(300);
            _lift_req_rpoc(RTU_READ, _ACT_GET_HIGH, 0);
			_lift_working = Lift_GetHigh;
            break;
        case _ACT_GET_DIST:
        case _ACT_RUN_OBJ:
			break;
        case _ACT_RUN_OPT:
			sys_delay_ms(10);
			_lift_req_rpoc(RTU_READ, _ACT_GET_HIGH, 0);
            break;
        default:
            return -1;
    }
    return 0;
}

static int _lift_write_rps(unsigned char func, unsigned char *pdata, unsigned short num)
{
    if (MODBUS_TIMEOUT_CODE == func) {
		if(_lift_working == Lift_SetHigh)
        	_lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, _lift_expect_high);
		else if(_lift_working == Lift_GetHigh)
			_lift_req_rpoc(RTU_READ, _ACT_GET_HIGH, 0);
    }
    else if (NULL != pdata && sizeof(st_MODBUS_data1) <= num) {
        st_MODBUS_data1 *prps = (st_MODBUS_data1 *)pdata;

        if (num == (sys_htons(prps->data_num) + sizeof(*prps))) {
            _lift_rsp_process(prps->data, num);
            return 0;
        }
    }
    return -1;
}

static int _lift_read_rps(unsigned char func, unsigned char *pdata, unsigned short num)
{
    if (NULL != pdata && sizeof(st_MODBUS_data) <= num) {
        st_MODBUS_data *prps = (st_MODBUS_data *)pdata;

        if (num == (prps->data_num + sizeof(*prps))) {
            _lift_rsp_process(prps->data, prps->data_num);
            return 0;
        }
    }
    return -1;
}

static int _lift_rps_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
    if (RTU_READ == func) {
        return _lift_read_rps(func, pdata, num);
    }

    return _lift_write_rps(func, pdata, num);
}

static int _lift_req_rpoc(e_RTU_func func, unsigned short act, unsigned short val)
{
    uint8_t tmp_buf[16];
    uint8_t num = 0;
    st_MODBUS_rtu *pframe = (st_MODBUS_rtu *)tmp_buf;
    st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;

    pframe->dev_id = _lift_data_pkt.dev_info.id;
    pframe->func = func;
    switch(func) {
        case RTU_READ: {
            preq->reg_addr = sys_htons(act);
            preq->reg_info = sys_htons(1);
            num = sizeof(*preq);
            break;
        }
        case RTU_WRITE: {
            preq->reg_addr = sys_htons(act);
            preq->reg_info = sys_htons(val);
            num = sizeof(*preq);
            break;
        }
        default:
            return -1;
    }

	if((act == _ACT_SET_HIGH) && (func == RTU_WRITE))
	{
		_lift_working = Lift_SetHigh;
		if(_lift_expect_high != val)
			_lift_expect_high = val;
	}

    num += sizeof(*pframe);
    //_module_poweron();
    if (0 <  MODBUS_send(&_lift_data_pkt.dev_info, tmp_buf, num)) {
        _lift_data_pkt.rsp.snd_tick = sys_get_ms();
        _lift_data_pkt.rsp.fun_code = func;
        _lift_data_pkt.rsp.sub_code = act;
    }
	else
	{
		if (_lift_data_pkt.dev_info.state & DEV_STATE_WAITRSP && sys_over_time(_lift_data_pkt.rsp.snd_tick, 500)) {
			_lift_data_pkt.dev_info.state &= ~DEV_STATE_WAITRSP;
		}
	}
    return num;
}

static int _lift_ctrl_urc(unsigned short act, unsigned char *pdata, unsigned short num)
{
    st_cfg_cmd *pcmd = (st_cfg_cmd *)&act;

    if (!pcmd->index && NULL != pdata && !num) {
        switch (pcmd->type) {
        case CFG_LIFT_TEMP:
            *(void **)pdata = _lift_info.temp;
            return sizeof(_lift_info.temp);
        case CFG_LIFT_STATE:
            *(void **)pdata = &_lift_info.state;
            return sizeof(_lift_info.state);
        case CFG_LIFT_ERR:
            *(void **)pdata = _lift_info.error;
            return sizeof(_lift_info.error);
        default:
            *(void **)pdata = NULL;
            break;
        }
    }
    else if (SYS_ACT_LIFT_RETURN == act) {
		_lift_working = Lift_SetHigh;
        return _lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, 0);
    }
    return 0;
}
sys_urc_register(SYS_MODULE_LIFT, _lift_ctrl_urc);

/** brief
 * example: "SetHeight:12.124;10.0"
 **/
static int _do_cmd_setheight(void *pobj, int argc, char *argv[])
{
    uint16_t high;
    int ret = -1;

	gulUnWorkTick = sys_get_ms();
	if(!is_dev_enable(&_lift_dev) || gucIsHome)	//
		return -1;

	if(IS_WALK_WORK()) return -1;

    if (1 == argc) {
		high = sys_atoi(argv[0]);
		if(high <= 402)
		{
	        high += LIFT_DEFAULT_HIGH;
	        ret = _lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, high);
			_lift_get_time = 1;
		}
    }
    return ret;
}
cmd_register("SetHeight", _do_cmd_setheight, "SetHeight:speed;high");

static int _do_cmd_liftctrl(void *pobj, int argc, char *argv[])
{
	uint16_t high;

	gulUnWorkTick = sys_get_ms();
	if(!is_dev_enable(&_lift_dev) || gucIsHome)	//低功耗模式 || 在充电仓位置
		return -1;
	if(IS_WALK_WORK()) return -1;
    if(argc == 1){
        if (NULL != sys_strstr(argv[0], "status")) {
            return _lift_req_rpoc(RTU_READ, _ACT_GET_STAT, 0);
        } else if (NULL != sys_strstr(argv[0], "error")) {
            return _lift_req_rpoc(RTU_READ, _ACT_GET_ERR, 0);
        } else if (NULL != sys_strstr(argv[0], "distance")) {
            return _lift_req_rpoc(RTU_READ, _ACT_GET_DIST, 0);
        } else if (NULL != sys_strstr(argv[0], "high")) {
            if (1 < argc) {high = (uint16_t)(sys_atof(argv[1]));
				high += LIFT_DEFAULT_HIGH;
                return _lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, high);
            }
        }
		else if (NULL != sys_strstr(argv[0], "rise")) {
			if(gLiftControl.ucLiftStatus != Lift_IDLE) return -1;
			else
			{
				gLiftControl.ucLiftStatus = Lift_RISE_WAIT;
				gLiftControl.ulControlTick = sys_get_ms();
				return -1;
			}
            //return _lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x01);
		}
		else if (NULL != sys_strstr(argv[0], "fall")) {
			if(gLiftControl.ucLiftStatus != Lift_IDLE) return -1;
			else
			{
				gLiftControl.ucLiftStatus = Lift_FALL_WAIT;
				gLiftControl.ulControlTick = sys_get_ms();
				return -1;
			}
            //return _lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x02);
		}
		else if (NULL != sys_strstr(argv[0], "stop")) {
			if(gLiftControl.ucLiftStatus == Lift_ERROR_STA) return -1;
			else if(gLiftControl.ucLiftStatus == Lift_IDLE)
			{
				gLiftControl.ucLiftStatus = Lift_ERROR_STA;
				gLiftControl.ulControlTick = sys_get_ms();
				return -1;
			}
			if((gLiftControl.ucLiftStatus != Lift_STOP_WAIT) && (sys_get_ms() - gLiftControl.ulControlTick) < 500)
			{
				if(gLiftControl.ucLiftStatus == Lift_RISE_WAIT)
				{
					high = gLiftDevInfo.ulHigh / 100 + 40 + LIFT_DEFAULT_HIGH;
					if(high > (402 + LIFT_DEFAULT_HIGH))
						high = 402 + LIFT_DEFAULT_HIGH;
				}
				else if(gLiftControl.ucLiftStatus == Lift_FALL_WAIT)
				{
					high = gLiftDevInfo.ulHigh / 100 - 40 + LIFT_DEFAULT_HIGH;
					if(high < LIFT_DEFAULT_HIGH)
						high = LIFT_DEFAULT_HIGH;
				}
				gLiftControl.ucLiftStatus = Lift_IDLE;
				_lift_get_time = 1;
				//debug_printf("high:%u", high);
				return _lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, high);
			}
			_lift_working = Lift_GetHigh;
			_lift_get_time = 0;
			gLiftControl.ucLiftStatus = Lift_IDLE;
			//debug_printf("stop\r\n");
            return _lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x00);
		}
    }

    return -1;
}
cmd_register("Lift", _do_cmd_liftctrl, "Lift:powerOn/powerOff/reset/status/error/distance/high");

void lift_high_pos(uint16_t high)
{
	if(!is_dev_enable(&_lift_dev))	
		return;
	
	high += LIFT_DEFAULT_HIGH;	
	_lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, high);
}

static void lift_post_update(void)
{
	static uint8_t sucLitfSta = 0;
	static uint16_t susLiftError = 0;

	if((sucLitfSta != gLiftDevInfo.ucLiftSta) || (susLiftError != gLiftDevInfo.usErrcode))
	{
		sucLitfSta = gLiftDevInfo.ucLiftSta;
		susLiftError = gLiftDevInfo.usErrcode;
		set_cloud_push_flag(PROP_LIFT);
	}
}

void lift_control_work(void)
{
	if(gLiftControl.ucLiftStatus && ((sys_get_ms() - gLiftControl.ulControlTick) > 500))
	{
		if(gLiftControl.ucLiftStatus == Lift_RISE_WAIT)
		{
			gLiftControl.ucLiftStatus = Lift_STOP_WAIT;
			_lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x01);
		}
		else if(gLiftControl.ucLiftStatus == Lift_FALL_WAIT)
		{
			gLiftControl.ucLiftStatus = Lift_STOP_WAIT;
			_lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x02);
		}
		else if(gLiftControl.ucLiftStatus == Lift_ERROR_STA)
		{
			gLiftControl.ucLiftStatus = Lift_IDLE;
		}
		else if(gLiftControl.ucLiftStatus == Lift_STOP_WAIT)
		{
			if((sys_get_ms() - gLiftControl.ulControlTick) > 15 * 1000)
			{
				gLiftControl.ucLiftStatus = Lift_IDLE;
				_lift_req_rpoc(RTU_WRITE, _ACT_RUN_OPT, 0x00);
			}
		}
	}
}


static void _module_process(void)
{
	//static unsigned long sulTick;
	
	if(is_dev_enable(&_lift_dev))
	{
    	MODBUS_process(&_lift_data_pkt);
	
		if((_lift_working == Lift_SetHigh) && sys_over_time(_lift_data_pkt.rsp.snd_tick, 500))
		{
			_lift_req_rpoc(RTU_WRITE, _ACT_SET_HIGH, _lift_expect_high);
		}
		else if((_lift_working == Lift_GetHigh) && sys_over_time(_lift_data_pkt.rsp.snd_tick, 1000))
		{
			_lift_req_rpoc(RTU_READ, _ACT_GET_HIGH, 0);
			if(_lift_get_time == 0)
				_lift_working = Lift_None;
		}
		else if((_lift_working == Lift_None) && sys_over_time(_lift_data_pkt.rsp.snd_tick, 10 * 1000) && ((_lift_data_pkt.dev_info.state & DEV_STATE_WAITRSP) == 0))
		{	//升降电机无其他处理时每隔一分钟读取升降电机状态
			_lift_req_rpoc(RTU_READ, _ACT_GET_STAT, 0);
			NUM_INCREASE(&_lift_status, 10);
			if(_lift_status >= 6)
				set_lift_status(9, 2);
			else
				set_lift_status(0, 0);
		}
		lift_control_work();
	}
	else
	{
		gLiftControl.ucLiftStatus = Lift_IDLE;
		set_lift_status(9, 1);
	}

	lift_post_update();
	
	sys_delay_ms(50);
}

/*
 * @brief    walk初始化
 */
static void _module_init(void)
{
#ifdef TGT_LIFT_CFG
    static unsigned char _lift_rxbuf[DEV_RXBUF_SIZE];  /*receive buffer area*/
    static st_uart_info _lift_port;
    st_uart_info cfg = TGT_LIFT_CFG;

    _lift_port = cfg;
    if (0 <= BSP_uart_init(&_lift_port, _lift_rxbuf, sizeof(_lift_rxbuf))) {
        /* 初始化发送环形缓冲区 */
        //ring_buf_init(&_lift_port.tx_buf, NULL, 0);
        _lift_data_pkt.dev_info.pobj = &_lift_port;
    }
#endif
	gpio_dev_register(&_lift_dev);
	pm_dev_register(&_lift_dev);
	gulRegisterFlag &= ~BIT(4);
	gLiftDevInfo.usTemp = 25;
	gLiftDevInfo.ulSpeed = 10;
	set_lift_status(0, 0);
	set_cloud_push_flag(PROP_LIFT);
	lift_high_pos(0);
	sys_delay_ms(15000);
	gLiftDevInfo.ulHigh = 0;
	_lift_working = Lift_GetHigh;
}

task_define(lift, _module_init, _module_process, MODBUS_TASK_INTVERAL, MODBUS_TASK_STACK, MODBUS_TASK_PRI);
