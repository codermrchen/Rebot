/******************************************************************************
 * @brief    charge control
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


#define _CHARGE_MULTIMODE       0x3001
typedef enum {
    _CHARGE_QUERY = 0x01,
    _CHARGE_STATE = 0x04,
    _CHARGE_ERROR = 0x05,
    _CHARGE_SYSTEM= 0x06,
    _CHARGE_START = 0xB1,
    _CHARGE_STOP  = 0xB2,
    _CHARGE_ADDR_SET = 0x07,
    _CHARGE_ADDR_GET = 0x87,
    _CHARGE_VOLTAGE_SET = 0x0E,
    _CHARGE_VOLTAGE_GET = 0x8E,
    _CHARGE_CURRENT_SET = 0x10,
    _CHARGE_CURRENT_GET = 0x90
} e_CHARGE_cmd;

typedef struct {
    uint16_t voltage;
    uint16_t current;
    uint16_t tempature;
    uint16_t charge_state; // 4
    uint16_t err_code;  // 5
    uint16_t sys_state; // 6
    uint16_t slv_addr;
    uint16_t baud_rate;
    uint16_t version;
    uint16_t magnet_field; //10
    uint16_t vol_sample;
    uint16_t cur_sample;
    uint16_t tmp_sample;
    uint16_t vol_set;
    uint16_t vol_preset; // 15
    uint16_t cur_preset;
} st_CHARGE_info;

typedef struct {
    uint8_t current[3]; // unit A, precision 3
    uint8_t voltage[2]; // unit V, precision 2
    uint8_t temp[2];    // unit °C, precision 2
    uint8_t error[2];
    uint8_t state;      // 0 nocharge, 1 charging, 2 protect, 9 error
} st_charge_state;

typedef struct {
    uint8_t capacity[2];// unit V, precision 2
    uint8_t temp[2];    // unit °C, precision 2
    uint8_t error[2];
    uint8_t state;      // 0 is charging, 1 is discharging, 9 is error
} st_battery_info;

/* Private function prototypes -----------------------------------------------*/
static int _charge_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num);

/* Private variables ---------------------------------------------------------*/
static st_battery_info _battery_info = {
                            .state = 1, // discharge
                        };
static st_charge_state _charge_info = {
                            .state = SYS_STATE_PWRDN,
                        };

static st_MODBUS_packet _charge_data_pkt = {
        .dev_info = {
                .id       = 0x01,
                .read     = MODBUS_read,
                .write    = MODBUS_write,
                .pkt_type = PKT_TYPE_RTU,
                .phead    = NULL,
                .crc_poly = 0xA001,
                .crc_val  = 0xFFFF,
                .crc_size = 2,
            },
        .dev_rsp_proc = _charge_rsp_proc,
        .rsp = { .timeout  = 2000 },
    };

void line_charge_judge(void)
{
	static uint8_t ucLineSta = 0;
	static uint16_t usErrCode = 0;
	if((ucLineSta != gLineDevInfo.ucLineSta) || (usErrCode != gLineDevInfo.usErrcode))
	{
		ucLineSta = gLineDevInfo.ucLineSta;
		usErrCode = gLineDevInfo.usErrcode;
		set_cloud_push_flag(PROP_LINE_CHARGE);
	}
}

void wire_charge_judge(void)
{
	static uint8_t ucWireSta = 0;
	static uint16_t usErrCode = 0;
	if((ucWireSta != gWireDevInfo.ucWireSta) || (usErrCode != gWireDevInfo.usWireErrCode))
	{
		ucWireSta = gWireDevInfo.ucWireSta;
		usErrCode = gWireDevInfo.usWireErrCode;
		set_cloud_push_flag(PROP_WIRELESS_CHARGE);
	}
}


/* Private functions ---------------------------------------------------------*/
/*
 * @brief       数据发送处理
 * @retval      none
 */
static int _charge_req_rpoc(e_CHARGE_cmd act)
{
    uint8_t tmp_buf[16];
    uint8_t num = 0;
    uint8_t retries = 0;
    st_MODBUS_rtu *pframe = (st_MODBUS_rtu *)tmp_buf;

    pframe->dev_id = _charge_data_pkt.dev_info.id;
    switch(act) {
        case _CHARGE_START:
        case _CHARGE_STOP: {
            st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;

            preq->reg_addr = 0;
            preq->reg_info = sys_htons(act);
            pframe->func = RTU_WRITE; 
            num += sizeof(*preq);
            break;
        }
        case _CHARGE_CURRENT_SET: {
            st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;
            uint16_t val = 500;

            preq->reg_addr = sys_htons(act);
            preq->reg_info = sys_htons(val);
            pframe->func = RTU_WRITE;
            num += sizeof(*preq);
            break;
        }
        case _CHARGE_VOLTAGE_SET: {
            st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;
            uint16_t val = 42000; // 42V

            preq->reg_addr = sys_htons(act);
            preq->reg_info = sys_htons(val);
            pframe->func = RTU_WRITE;
            num += sizeof(*preq);
            break;
        }
        case _CHARGE_ADDR_SET: {
            st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;

            preq->reg_addr = sys_htons(act);
            preq->data[num++] = pframe->dev_id >> 8;
            preq->data[num++] = pframe->dev_id & 0xFF;
            pframe->func = RTU_WRITE;
            num += sizeof(*preq);
            break;
        }
        case _CHARGE_CURRENT_GET:
        case _CHARGE_VOLTAGE_GET:
        case _CHARGE_ADDR_GET: {
            st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;

            preq->reg_addr = sys_htons(act & 0x7F);
            preq->reg_info = sys_htons(2);
            pframe->func = RTU_READ;
            num = sizeof(*preq);
            break;
        }
        case _CHARGE_QUERY: {
            st_MODBUS_req *preq = (st_MODBUS_req *)pframe->data;

            preq->reg_addr = sys_htons(act);
            preq->reg_info = sys_htons(sizeof(st_CHARGE_info)>>1);
            pframe->func = RTU_READ;
            num = sizeof(*preq);
            retries = 3;
            break;
        }
        default:
            return -1;
    }

    num += sizeof(*pframe);
    if (0 <  MODBUS_send(&_charge_data_pkt.dev_info, tmp_buf, num)) {
        _charge_data_pkt.dev_info.state |= DEV_STATE_BUSY;
        _charge_data_pkt.rsp.retries  = retries;
        _charge_data_pkt.rsp.snd_tick = sys_get_ms();
        _charge_data_pkt.rsp.fun_code = pframe->func;
        _charge_data_pkt.rsp.sub_code = act;
    }
	else
	{
		if((_charge_data_pkt.dev_info.state & DEV_STATE_WAITRSP) && sys_over_time(_charge_data_pkt.rsp.snd_tick, 1000))
		{
			_charge_data_pkt.dev_info.state &= ~DEV_STATE_WAITRSP;
		}
	}

    return num;
}

static int _charge_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
    _charge_data_pkt.dev_info.state &= ~DEV_STATE_BUSY;

    if (MODBUS_TIMEOUT_CODE == func && 0 < _charge_data_pkt.rsp.retries--) {
        _charge_req_rpoc(_CHARGE_QUERY);
    } else if (_CHARGE_QUERY == _charge_data_pkt.rsp.sub_code) {
        st_CHARGE_info *pinfo = (st_CHARGE_info *)(pdata + 1);
        float zoomout = 100.0;
        uint8_t status = 0;

        pinfo->err_code = sys_htons(pinfo->err_code);
        pinfo->sys_state= sys_htons(pinfo->sys_state);
        pinfo->voltage  = sys_htons(pinfo->voltage);
        pinfo->current  = sys_htons(pinfo->current);
        pinfo->tempature= sys_htons(pinfo->tempature);
		gWireDevInfo.usWireVoltage = pinfo->voltage;
		gWireDevInfo.usWireTemp = pinfo->tempature;
		gWireDevInfo.usWireCurrent = pinfo->current;
		gWireDevInfo.ucWireSta = (pinfo->sys_state == 0) ? 0 : ((pinfo->sys_state == 2) ? 1 : 0); 
		gWireDevInfo.usWireErrCode = pinfo->err_code;
        debug_printf("vol %f, cur %f, tmp %f, charge state %x, system state %x, errcode %x\r\n",
            pinfo->voltage/zoomout, pinfo->current/zoomout, pinfo->tempature/zoomout,
            sys_htons(pinfo->charge_state), pinfo->sys_state, pinfo->err_code);
        if (0x2 == pinfo->sys_state) {
            uint8_t warn_temp;

            status = 0x1;
            if (0 < cfg_data_read((void *)&warn_temp, CFG_CHARGE_WARNTEMP, sizeof(warn_temp))
                && (int)(warn_temp * zoomout) < pinfo->tempature) {
                _charge_req_rpoc(_CHARGE_STOP);
            }
        }
        else if (0x4 == pinfo->sys_state) {
            _charge_req_rpoc(_CHARGE_QUERY);
            status = 2;
        }
        else if (0x0 != pinfo->sys_state) {
            _charge_req_rpoc(_CHARGE_QUERY);
            return 0;
        }
        else { // no charge
            if (0x0 != pinfo->err_code && 0x09 != pinfo->err_code) {
                status = 0x9;
            }
            else {
                pinfo->err_code = 0;
            }

            cfg_data_write((void *)&pinfo->err_code, CFG_CHARGE_ERR, sizeof(pinfo->err_code));
        }

        cfg_data_write(&status, CFG_CHARGE_STATE, sizeof(status));
    }
    else if (RTU_WRITE == func) {
        _charge_req_rpoc(_CHARGE_QUERY);
    }

    return 0;
}

static int _charge_data_urc(unsigned short cmd, unsigned char *pdata, unsigned short num)
{
    st_cfg_cmd *pcmd = (st_cfg_cmd *)&cmd;

    if (!pcmd->index && NULL != pdata && !num) {
        switch (pcmd->type) {
        case CFG_BAT_CAPACITY:
            *(void **)pdata = _battery_info.capacity;
            return sizeof( _battery_info.capacity);
        case CFG_BAT_TEMP:
            *(void **)pdata = _battery_info.temp;
            return sizeof( _battery_info.temp);
        case CFG_BAT_STATE:
            *(void **)pdata = &_battery_info.state;
            return sizeof(_battery_info.state);
        case CFG_BAT_ERR:
            *(void **)pdata = &_battery_info.error;
            return sizeof(_battery_info.error);
        case CFG_CHARGE_CURRENT:
            *(void **)pdata = _charge_info.current;
            return sizeof(_charge_info.current);
        case CFG_CHARGE_VOLTAGE:
            *(void **)pdata = _charge_info.voltage;
            return sizeof(_charge_info.voltage);
        case CFG_CHARGE_TEMP:
            *(void **)pdata = _charge_info.temp;
            return sizeof(_charge_info.temp);
        case CFG_CHARGE_STATE:
            *(void **)pdata = &_charge_info.state;
            return sizeof(_charge_info.state);
        case CFG_CHARGE_ERR:
            *(void **)pdata = _charge_info.error;
            return sizeof(_charge_info.error);
        default:
            *(void **)pdata = NULL;
            break;
        }
    }
    return 0;
}
sys_urc_register(SYS_MODULE_CHARGE, _charge_data_urc);

void start_charging(void)
{	//优先有线充	
	if(gucChargeType == 0)
	{
		debug_printf("Start Line Charge!\r\n");
		Open_Line_En();
		gLineDevInfo.ucLineSta = 1;
		gLineDevInfo.usErrcode = 0;
		gucStartCharge = 1;
	}
	else
	{
		debug_printf("Start Wire Charge!\r\n");
		Open_Wire_En();
		sys_delay_ms(5);
		_charge_req_rpoc(_CHARGE_START);
		gucStartCharge = 1;
	}
	gucChargeSta = 2;
	gucIsCharge = 1;
	gulChargeTick = sys_get_ms();
}

void stop_charge(void)
{	
	if(gucChargeType == 0)
	{
		Close_Line_En();
		gLineDevInfo.ucLineSta = 0;
		gLineDevInfo.usErrcode = 0;
		gucStartCharge = 0;
	}
	else
	{
		Close_Wire_En();
		_charge_req_rpoc(_CHARGE_STOP);
		gucStartCharge = 0;
	}
	gucIsCharge = 0;
	gucChargeSta = 0;
}

static int _do_cmd_chargectrl(void *pobj, int argc, char *argv[])
{
	gulUnWorkTick = sys_get_ms();
    if (0 < argc) {
        if (NULL != sys_strstr(argv[0], "query")) {
            _charge_req_rpoc(_CHARGE_QUERY);
            return 0;
        }
		else if (NULL != sys_strstr(argv[0], "linestart")) {
			if(gucIsHome == 0) 
			{
				debug_printf("Machine Isn't Stay Home!\r\n");
				return -1;
			}
			if(gucIsCharge == 1) 
			{
				debug_printf("Machine Is Charging!\r\n");
				return -1;
			}
			if(gLineDevInfo.ucLineSta == 9)
			{
				debug_printf("Line Charge Is Error!\r\n");
				return -1;
			}
            Open_Line_En();
            gucIsCharge	= 1;
			gulChargeTick = sys_get_ms();
			gLineDevInfo.ucLineSta = 1;
			gLineDevInfo.usErrcode = 0;
			gucStartCharge = 1;
            return 0;
        }
		else if (NULL != sys_strstr(argv[0], "linestop")) {
            Close_Line_En();
            gLineDevInfo.ucLineSta = 0;
			gLineDevInfo.usErrcode = 0;
			if(gucChargeType == 0)
				gucStartCharge = 0;
            gucIsCharge	= 0;
			gucChargeSta = 0;
            return 0;
        }
        else if (NULL != sys_strstr(argv[0], "start")) {
			if(gucIsHome == 0) return -1;
			if(gucIsCharge == 1) return -1;
			Open_Wire_En();
			sys_delay_ms(5);
            _charge_req_rpoc(_CHARGE_START);
			gucStartCharge = 1;
			gucIsCharge	= 1;
			gulChargeTick = sys_get_ms();
            return 0;
        }
        else if (NULL != sys_strstr(argv[0], "stop")) {
			Close_Wire_En();
            _charge_req_rpoc(_CHARGE_STOP);
			if(gucChargeType == 1)
				gucStartCharge = 0;
			gucIsCharge	= 0;
			gucChargeSta = 0;
            return 0;
        }
		else if(NULL != sys_strstr(argv[0], "bat"))
		{
			debug_printf("电池电量:%.3f, 充电状态:%s,有线充状态:%s,无线充状态:%s\r\n", (float)gBatDevInfo.usBatVoltage / 100.0f, IS_Charge_Sta() ? "充电" : "放电", 
				(gLineDevInfo.ucLineSta == 9) ? "故障" : ((gLineDevInfo.ucLineSta == 0) ? "正常" : "充电"), 
				(gWireDevInfo.ucWireSta == 9) ? "故障" : ((gWireDevInfo.ucWireSta == 0) ? "正常" : "充电"));
		}
    }

    return -1;
}
cmd_register("Charge", _do_cmd_chargectrl, "Charge:start/stop");

int _do_cmd_setCharge(void *pobj, int argc, char *argv[])
{
    if (1 == argc && NULL != argv[0]) {
		gulUnWorkTick = sys_get_ms();
        uint8_t status = (uint8_t)atoi(argv[0]);

        if (!status && !(_charge_info.state & SYS_STATE_CHARGE)) {
			//if(gucIsHome == 0) return -1;
			start_charging();

            status = SYS_STATE_CHARGE;
        }
        else if (0 != status) {
            stop_charge();

            status = 0;
        }

        while (0 != _charge_data_pkt.rsp.fun_code) {
            sys_delay_ms(10);
        }

        if (status == _charge_info.state) {
            return 0;
        }
    }
    return -1;
}
cmd_register("SetCharge", _do_cmd_setCharge, "Charge setting");

static void _module_process(void)
{
	line_charge_judge();
	wire_charge_judge();
    MODBUS_process(&_charge_data_pkt);
	if((gucIsCharge == 1) && (gucStartCharge == 1))
	{	//有线充正在充电
		if(sys_over_time(gulChargeTick, 60000))
		{	//充电一分钟电池充不上电
			if(!IS_Charge_Sta())
			{	//关闭有线充
				stop_charge();
				if(gucChargeType == 0)
				{
					gLineDevInfo.ucLineSta = 9;
					gLineDevInfo.usErrcode = 1;
				}
				else 
				{
					gWireDevInfo.ucWireSta = 9;
					gWireDevInfo.usWireErrCode = 1;
				}
				gucStartCharge = 0;
				gucChargeType = !gucChargeType;
				start_charging();
			}
			else
				gucStartCharge = 0;
		}
		else
		{
			if(IS_Charge_Sta())
				gucStartCharge = 0;
		}
	}
//	if((gucIsCharge == 1) && (gBatDevInfo.usBatVoltage > 4180))	//电量大于41.8V，停止充电
//		stop_charge();
}

static void _module_init(void)
{
#ifdef TGT_CHARGE_CFG
    static unsigned char   _charge_rxbuf[DEV_RXBUF_SIZE];
    static st_uart_info    _charge_port = TGT_CHARGE_CFG;

    if (0 <= BSP_uart_init(&_charge_port, _charge_rxbuf, sizeof(_charge_rxbuf))) {
        ring_buf_init(&_charge_port.tx_buf, NULL, 0);
        _charge_data_pkt.dev_info.pobj = &_charge_port;
    }
#endif
	gWireDevInfo.usWireVoltage = 0;
	gWireDevInfo.usWireTemp = 25;
	gWireDevInfo.usWireCurrent = 0;
	gWireDevInfo.ucWireSta = 0;
	gWireDevInfo.usWireErrCode = 0;
	set_cloud_push_flag(PROP_WIRELESS_CHARGE);
	gLineDevInfo.ucLineSta = 0;
	gLineDevInfo.usErrcode = 0;
	set_cloud_push_flag(PROP_LINE_CHARGE);
}

task_define(charge, _module_init, _module_process, 100, MODBUS_TASK_STACK, 9);
