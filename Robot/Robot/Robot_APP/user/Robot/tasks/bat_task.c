/******************************************************************************
 * @brief    battery
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
#include "sys_pm.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"
#include "charge_task.h"

#define _BAT_VER_ID         (0x52) //82
typedef enum {
    _BAT_PROCT_PARAM = 0x01,
    _BAT_REAL_PARAM  = 0x02,
    _BAT_PARAM_SET   = 0x05,
    _BAT_CAPACITY    = 0x10,
    _BAT_STORE_MODE  = 0x14,
    _BAT_RESET_OPT   = 0x30,
    _BAT_FET_INFO    = 0x06,
    _BAT_VER_INFO    = 0x09,
    _BAT_SOC_INFO    = 0x13,
    _BAT_SN_INFO     = 0x22
} e_bat_func;

typedef enum {
    _BAT_ACT_NONE       = 0x0,
    _BAT_ACT_RESET      = 0x1,
    _BAT_ACT_PACKSN     = 0x2,
    _BAT_ACT_CAPACITY   = 0x4,
    _BAT_ACT_TEMP       = 0x8,
    _BAT_ACT_VER        = 0x10,
} e_bat_act;

#pragma pack(1) // custom one byte align
typedef struct {
    uint8_t  ver; //0x52
    uint16_t num;
    uint8_t  data[0];
} st_bat_data;

typedef struct {
    uint8_t  soc;
    uint8_t  soh;
    uint16_t chargeTime;
    uint16_t dischargeTime;
} st_bat_soc;

typedef struct {
    uint32_t  time;
    uint16_t  vbat; // unit: 2mv
    uint8_t   cellNum;
    uint16_t  cellVol[0]; // cellNum
    uint16_t  current[2];
    uint8_t   tempNum;
    uint16_t  temp[0]; // tempNum
    uint16_t  volState;
    uint16_t  curState;
    uint16_t  tempState;
    uint16_t  alarmCnt;
    uint8_t   fetState;
    uint16_t  warnVov;
    uint16_t  warnVuv;
    uint16_t  highWarns;
    uint16_t  lowWarns;
} st_bat_param;

static st_blink_dev _bat_dev = {	\
	.gpio = TGT_BAT_PWR,			\
	.BlinkSta = DISABLE,			\
	.NeedAble = ENABLE,				\
	};

//static st_blink_dev _fan_dev = {	\
//		.gpio = TGT_FAN_PWR,			\
//		.BlinkSta = DISABLE,			\
//		.NeedAble = ENABLE, 			\
//	};



#pragma pack()  // cancel custom byte align mothod

/* Private function prototypes -----------------------------------------------*/
static int _bat_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num);

/* Private variables ---------------------------------------------------------*/
static st_MODBUS_packet _bat_data_pkt = {
        .dev_info = {
                .id = 0x1,
                .read = MODBUS_read,
                .write= MODBUS_write,
            .is_ignore= 1,
                .pkt_type = PKT_TYPE_ASCII,
                .crc_poly = 0,
                .crc_chk  = 0xFF,
                .crc_size = 1,
            },
        .dev_rsp_proc = _bat_rsp_proc,
        .rsp  = {
                .fun_code = 0,
                .timeout = 10000,
            },
    };

/* Private functions ---------------------------------------------------------*/
static int _bat_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
	unsigned short usTemp = 0, usCount, usTemputure = 0;
	char cData;
	int i;
	
    if ((RTU_RSP_FLAG & func) && NULL != pdata && (sizeof(st_bat_data) << 1) < (int)num) {

        if((pdata[0] == 0x35) && (pdata[1] == 0x32))
        {
			pdata += 24;
			for(i = 0; i < 2; i++)
			{	//获取电池组数
				cData = sys_ascii2num(*pdata++);
				if(cData == (char)-1) return (int)-1;
				usTemp = (usTemp << 4) |cData;
			}
			pdata += usTemp * 4;
			usTemp = 0;
			for(i = 0; i < 4; i++)
			{	//获取充电电压值
				cData = sys_ascii2num(*pdata++);
				if(cData == (char)-1) return (int)-1;
				usTemp = (usTemp << 4) |cData;
			}
			if(usTemp > 0) 
			{
				gBatDevInfo.ucBatSta = 0;
			}
			else
			{
				gBatDevInfo.ucBatSta = 1;
				if(gucChargeSta == 2) gucChargeSta = 0;
				if((gucIsCharge == 1) && (gucStartCharge == 0)) gucIsCharge = 0;
				if((gWireDevInfo.ucWireSta == 1) && (gucStartCharge == 0))
				{
					gWireDevInfo.ucWireSta = 0;
					gWireDevInfo.usWireErrCode = 0;
				}
				if((gLineDevInfo.ucLineSta == 1) && (gucStartCharge == 0))
				{
					gLineDevInfo.ucLineSta = 0;
					gLineDevInfo.usErrcode = 0;
				}
			}
			pdata += 4;	//放电电压忽略
			usTemp = 0;
			for(i = 0; i < 2; i++)
			{	//获取温度组数量
				cData = sys_ascii2num(*pdata++);
				if(cData == (char)-1) return (int)-1;
				usTemp = (usTemp << 4) |cData;
			}
			usCount = usTemp;
			for(i = 0; i < usCount; i++)
			{
				usTemp = 0;
				for(int j = 0; j < 2; j++)
				{	//获取电池组数
					cData = sys_ascii2num(*pdata++);
					if(cData == (char)-1) return (int)-1;
					usTemp = (usTemp << 4) |cData;
				}
				if(usTemputure < usTemp)	//获取最高的温度值
					usTemputure = usTemp;
			}

			gBatDevInfo.usTemp = (usTemputure - 40) * 100;
		}
    }
    return -1;
}

static int _bat_req_rpoc(uint8_t act)
{
    switch (act) {
        case _BAT_SN_INFO:
        case _BAT_SOC_INFO:
        case _BAT_VER_INFO:
        case _BAT_CAPACITY:
        case _BAT_PROCT_PARAM:
        case _BAT_REAL_PARAM:
        case _BAT_STORE_MODE:
        case _BAT_RESET_OPT:
            break;
        default:
            return -1;
    }

    uint8_t num = 0;
    uint8_t append_num;
    uint8_t tmp_buf[16];
    st_MODBUS_dev *pdev = &_bat_data_pkt.dev_info;
    st_MODBUS_ascii *pframe = (st_MODBUS_ascii *)tmp_buf;
    st_bat_data *pdata = (st_bat_data *)pframe->data;

    pframe->soi = pdev->phead[0];
    pframe->cmd = act;
    append_num = pdev->head_len + pdev->tail_len;
    num = sizeof(*pframe) + sizeof(*pdata);
    pdata->num = sys_htons(append_num + ((num - append_num) << 1));

    tmp_buf[num - 1] = pdev->phead[pdev->head_len];
	tmp_buf[1] = 0x00;
    if (0 < MODBUS_send(&_bat_data_pkt.dev_info, tmp_buf, num)) {
        _bat_data_pkt.rsp.snd_tick = sys_get_ms();
        _bat_data_pkt.rsp.fun_code = (RTU_RSP_FLAG | act);
        _bat_data_pkt.rsp.sub_code = act;
    }
	else
	{
		if (_bat_data_pkt.dev_info.state & DEV_STATE_WAITRSP && sys_over_time(_bat_data_pkt.rsp.snd_tick, 2000)) {
			_bat_data_pkt.dev_info.state &= ~DEV_STATE_WAITRSP;
		}
	}
    return 0;
}

//////////////////////////////////////////
//static int _do_cmd_batctrl(void *pobj, int argc, char *argv[])
//{
//    for (; 0 < argc; argc = 0) {
//        int ret = -1;
//
//        if (NULL != sys_strstr(argv[0], "reset")) {
//            ret = _bat_req_rpoc(_BAT_RESET_OPT);
//        }
//        else if (NULL != sys_strstr(argv[0], "ver")) {
//            ret = _bat_req_rpoc(_BAT_VER_INFO);
//        }
//        else if (NULL != sys_strstr(argv[0], "sn")) {
//            ret = _bat_req_rpoc(_BAT_SN_INFO);
//        }
//        else if (NULL != sys_strstr(argv[0], "soc")) {
//            ret = _bat_req_rpoc(_BAT_SOC_INFO);
//        }
//        else if (NULL != sys_strstr(argv[0], "capacity")) {
//            ret = _bat_req_rpoc(_BAT_CAPACITY);
//        }
//        else if (NULL != sys_strstr(argv[0], "proctParam")) {
//            ret = _bat_req_rpoc(_BAT_PROCT_PARAM);
//        }
//        else if (NULL != sys_strstr(argv[0], "realParam")) {
//            ret = _bat_req_rpoc(_BAT_REAL_PARAM);
//        }
//        else if (NULL != sys_strstr(argv[0], "storemode")) {
//            ret = _bat_req_rpoc(_BAT_STORE_MODE);
//        }
//        else {
//            break;
//        }
//
//        return ret;
//    }
//
//    return -1;
//} cmd_register("Bat", _do_cmd_batctrl, "Bat:reset/ver/sn/soc/capacity");

static int _bat_state_urc(unsigned short state, unsigned char *pdata, unsigned short num)
{
    static uint16_t _module_state = SYS_STATE_PWRDN;
    uint16_t last_state = _module_state;
    uint16_t ntmp = (0x1 << state);

    switch (state) {
        case SYS_STATE_PWRDN:
            _module_state = ntmp | (_module_state & SYS_STATE_INITED);
            break;
        case SYS_STATE_PWRUP:
            _module_state &= ~ntmp;
            break;
        case SYS_STATE_RESET:
            cfg_data_clear(CFG_SYS_ERR);
            _module_state &= (0x1 << SYS_STATE_INITED);
            break;
        case SYS_STATE_INITED:
        case SYS_STATE_IDLE:
        case SYS_STATE_SLEEP:
            break;
        case SYS_STATE_CHARGE:
        case SYS_STATE_INSPECT:
        case SYS_STATE_DEPLOY:
            if (_module_state & (SYS_STATE_PWRDN | SYS_STATE_CHARGE | SYS_STATE_INSPECT
                | SYS_STATE_DEPLOY | SYS_STATE_LOWBAT | SYS_STATE_RESET | SYS_STATE_RETURN)) {
                _module_state &= ~ntmp;
            } else if (0 != (_module_state & SYS_STATE_IDLE)) { // idle
                _module_state |= ntmp;
                _module_state &= ~(0x1 << SYS_STATE_SLEEP);
                //sys_pm_disable();
            }
            else {
                return -1;
            }
            break;
        case SYS_STATE_RETURN:
            if (_module_state & ntmp) {
                _module_state &= ~ntmp;
            }
            else {
                _module_state |= ntmp;
            }
            break;
        case SYS_STATE_LOWBAT:
            if (!pdata && !num) {
                _module_state &= ~ntmp;
            }
            else {
                cfg_data_write((void *)&state, CFG_SYS_ERR, sizeof(state));
                _module_state |= (0x1 << SYS_STATE_ERROR);
            }
            break;
        case SYS_STATE_ERROR:
            if (!pdata || !num) {
                cfg_data_clear(CFG_SYS_ERR);
                _module_state &= ~ntmp;
            }
            else {
                cfg_data_write(pdata, CFG_SYS_ERR, num);
                _module_state |= ntmp;
            }
            break;
        default: return -1;
    }

    if (last_state != _module_state) {
        if (_module_state & (0x1 << SYS_STATE_ERROR)) {
            state = SYS_STATE_ERROR;
        }
        else {
            state = SYS_STATE_IDLE;
            for (num = SYS_STATE_DEPLOY; 0 < num; num--) {
                if (_module_state & (0x1 << num)) {
                    state = num - 1;
                    break;
                }
            }
        }

        cfg_data_write((void *)&state, CFG_SYS_STATE, sizeof(state));
    }
    return 0;
}
sys_urc_register(SYS_MODULE_BAT, _bat_state_urc);

static float caculate_bat_vol(void)
{
	unsigned short usLess = 2500, usMax = 0;
	unsigned int ulSum = 0;
	float fBatVol;
	
	for(int i = 0; i < TGT_ULTRA_ADC_CHNS; i++)
	{
		usMax = (_ultra_adc[i] > usMax) ? _ultra_adc[i] : usMax;
		usLess = (_ultra_adc[i] < usLess) ? _ultra_adc[i] : usLess;
		ulSum += _ultra_adc[i];
	}
	ulSum = (ulSum - usMax - usLess) / (TGT_ULTRA_ADC_CHNS - 2);
	fBatVol = (float)ulSum / 90.5f;
	if(fBatVol > 42.01f) fBatVol = 42.0f;
	if(fBatVol < 27.99f) fBatVol = 28.0f;
	return fBatVol;
}

static void _module_process(void)
{
	static unsigned long sulTick, sulTick1, sulTick2;
	float fBatVol;

	if(sys_over_time(sulTick, 10000) && ((_bat_data_pkt.dev_info.state & DEV_STATE_WAITRSP) == 0))
	{
		_bat_req_rpoc(_BAT_REAL_PARAM);
		sulTick = sys_get_ms();
	}
	if(sys_over_time(sulTick2, 10000))
	{
		fBatVol = caculate_bat_vol();
		gBatDevInfo.usBatVoltage = fBatVol * 100;
		if((fBatVol >= 42.0f) && IS_Charge_Sta())
			stop_charge();
		sulTick2 = sys_get_ms();
	}
    MODBUS_process(&_bat_data_pkt);
	if(sys_over_time(sulTick1, 60 * 1000))
	{	//每隔一分钟上报一次电池参数
		set_cloud_push_flag(PROP_BAT);
		sulTick1 = sys_get_ms();
	}

	sys_delay_ms(1000);
}

void sys_data_check(void)
{
	unsigned long ulChange = 0, ulCheck = 0;
	unsigned char * pdata = (unsigned char *)&gSysParameter;
	for(int i = 0; i < sizeof(Sys_Para); i++)
		ulChange += *pdata++;
	if(!((gSysParameter.fAutoSpeed > 9.9f) && (gSysParameter.fAutoSpeed < 90.1f)))
		gSysParameter.fAutoSpeed = 10.0f;
	if(!((gSysParameter.fManualSpeed > 9.9f) && (gSysParameter.fManualSpeed < 90.1f)))
		gSysParameter.fManualSpeed = 10.0f;
	if(!((gSysParameter.fBackCHargeVal > 27.99f) && (gSysParameter.fBackCHargeVal < 42.01f)))
		gSysParameter.fBackCHargeVal = 30.0f;
	if(!((gSysParameter.fAllowWorkVal > 27.999f) && (gSysParameter.fAllowWorkVal < 42.01f)))
		gSysParameter.fAllowWorkVal = 30.8f;
	if(gSysParameter.fBackCHargeVal > gSysParameter.fAllowWorkVal)
		gSysParameter.fAllowWorkVal = gSysParameter.fBackCHargeVal;
	if(gSysParameter.ucLowChargeHandlerWay > 1)
		gSysParameter.ucLowChargeHandlerWay = 0;
	if((gSysParameter.ulSleepAfter < 60) || (gSysParameter.ulSleepAfter > 1200))
		gSysParameter.ulSleepAfter = 300;
	if((gSysParameter.ulSleepBefore < 180) || (gSysParameter.ulSleepBefore > 1200))
		gSysParameter.ulSleepBefore = 300;
	pdata = (unsigned char *)&gSysParameter;
	for(int i = 0; i < sizeof(Sys_Para); i++)
		ulCheck += *pdata++;
	if(ulChange != ulCheck)
		write_sys_data();
	
}

/*
 * @brief    module initialize
 */
static void _module_init(void)
{
#ifdef TGT_BAT_CFG
    static uint8_t _bat_headbuf[] = {':', '~'};
    static uint8_t _bat_rxbuf[512];
    static st_uart_info  _bat_port = TGT_BAT_CFG;
    st_MODBUS_dev *pdev = &_bat_data_pkt.dev_info;

    pdev->pobj = &_bat_port;
    pdev->phead= _bat_headbuf;
    pdev->head_len = 1;
    pdev->tail_len = 1;
    if (0 <= BSP_uart_init(&_bat_port, _bat_rxbuf, sizeof(_bat_rxbuf))) {
        ring_buf_init(&_bat_port.tx_buf, NULL, 0);
    }
#endif
	gpio_dev_register(&_bat_dev);
	//gpio_dev_register(&_fan_dev);
	gulRegisterFlag &= ~BIT(2);
	sys_delay_ms(10000);
	cfg_data_init();
	sys_delay_ms(1000);
	read_sys_data();
	sys_data_check();
	gucFlashInitFinish = 1;
	set_cloud_push_flag(PROP_PARA);
	gBatDevInfo.ucBatSta = 1;
}

/*
 * @brief    module task(100ms loop one time)
 */
task_define(bat, _module_init, _module_process, MODBUS_TASK_INTVERAL, 256, 3);
