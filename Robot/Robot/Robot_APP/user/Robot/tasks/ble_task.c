/******************************************************************************
 * @brief    lift control
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

#define _MODULE_TAG                       "BLE"

typedef enum {
    _BLE_STATE_ATE0,
    _BLE_STATE_NAME,
    _BLE_STATE_READY
} e_BLE_cmd;

//static st_blink_dev _ble_dev = {	\
//		.gpio = TGT_BLE_PWR,			\
//		.BlinkSta = DISABLE,			\
//		.NeedAble = ENABLE, 			\
//	};


/* Private function prototypes -----------------------------------------------*/
static int _ble_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num);
static int _ble_cmd_exec(char act, char *pdata);

/* Private variables ---------------------------------------------------------*/
static unsigned char _ble_headbuf[] = {'\r', '\n', '\r', '\n'};
static unsigned char _ble_rxbuf[CMD_RXBUF_SIZE];
static st_uart_info _ble_port;

static st_MODBUS_packet _ble_data_pkt = {
        .dev_info = {
                .id    = 0x01,
                .pobj  = &_ble_port,
                .write = MODBUS_write,
                .read  = MODBUS_read,
                .state = (DEV_STATE_SLAVE | DEV_STATE_PWROFF),
                .pkt_type = PKT_TYPE_ASCII,
                .phead    = _ble_headbuf,
                .head_len = 2,
                .tail_len = 2,
                .is_ignore= 1,
                .crc_poly = 0,
                .crc_chk  = 0,
                .crc_val  = 0,
                .crc_size = 0,
            },
        .dev_rsp_proc = _ble_rsp_proc,
        .rsp = { .timeout = 2000 },
    };

static inline void _ble_update_state(uint8_t state)
{
    st_MODBUS_dev *pdev = &_ble_data_pkt.dev_info;

    pdev->state &= DEV_STATE_INITED;
    pdev->state |= state;
}

/* Private functions ---------------------------------------------------------*/
static int _ble_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
    st_MODBUS_dev *pdev = &_ble_data_pkt.dev_info;

    if (NULL != sys_strstr((char *)pdata, "ready")) {

        if (0 == pdev->state) {
            char tmp_buf[32];

            snprintf(tmp_buf, sizeof(tmp_buf), "AT+BLENAME=%s\r\n", PRJ_NAME);
            _ble_cmd_exec(_BLE_STATE_NAME, tmp_buf);
        }
        else {
            _ble_cmd_exec(_BLE_STATE_READY, "AT+BLEMODE=0\r\n");
        }

        _ble_update_state(DEV_STATE_PWRON);
    }
    else if (NULL != sys_strstr((char *)pdata, "OK")) {
        unsigned char sub_code = _ble_data_pkt.rsp.sub_code;

        if (_BLE_STATE_NAME == sub_code) {
            _ble_cmd_exec(_BLE_STATE_READY, "AT+BLEMODE=0\r\n");
        }
    }
    else if (NULL != sys_strstr((char *)pdata, "BLE_CONNECT")) {
        _ble_update_state(DEV_STATE_READY);
    }
    else if (NULL != sys_strstr((char *)pdata, "BLE_DISCONNECT")) {
        _ble_update_state(DEV_STATE_PWRON);
    }
    else {
        char *start = sys_strstr((char *)pdata, "AT#");

        if (NULL != start) {
            char *end = sys_strstr(start, "#");

            if (start < end && (char *)&pdata[num] >= end) {
                end[-1] = '\0';
                cli_process(&_ble_port, start);
            }
        }
    }
    return func;
}

/*
 * @brief       数据发送处理
 * @retval      none
 */
static int _ble_cmd_exec(char act, char *pdata)
{
    st_MODBUS_response *prsp = &_ble_data_pkt.rsp;

    if (NULL != pdata && 0 < strlen(pdata)) {
        if (0 > MODBUS_send(&_ble_data_pkt.dev_info, (uint8_t *)pdata, strlen(pdata))) {
            return -1;
        }

        prsp->timeout = 2000;
    }
    else {
        prsp->timeout = 0;
    }

    prsp->snd_tick = sys_get_ms();
    prsp->sub_code = act;
    return 0;
}

static void _module_process(void)
{
    //MODBUS_process(&_ble_data_pkt);
}

static void _module_init(void)
{
#ifdef TGT_BLE_CFG
    st_uart_info cfg = TGT_BLE_CFG;

    _ble_port = cfg;
    if (0 <= BSP_uart_init(&_ble_port, _ble_rxbuf, sizeof(_ble_rxbuf))) {
       ring_buf_init(&_ble_port.tx_buf, NULL, 0);
    }
#endif
	//gpio_dev_register(&_ble_dev);
	gulRegisterFlag &= ~BIT(3);
}

task_define(ble, _module_init, _module_process, 500, 256, 4);
