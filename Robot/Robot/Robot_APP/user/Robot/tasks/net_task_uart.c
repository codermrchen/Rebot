/******************************************************************************
 * @brief    net任务
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
#include "sys_util.h"
#include "sys_pm.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"

#include "socket.h"
#include "mqttClient.h"
#include "sfud_def.h"
#include "ch395cmd.h"
#include "ch395spi_hw.h"
#include "mqttPacket.h"
#include "blink.h"
#include "work_task.h"
#include "rtl8367s.h"

#define _NET_CFG_RATE             9600
#define _NET_TRANS_RATE           115200

#define _NET_DATA_SEPARTOR        ';'
#define RTL9367_ADDR    0x04    //RTL8367 i2c's address

st_socket_info socket[3];

static void _nvidia_pm_before(void);
static void _camera_pm_before(void);
//static void _5V_pm_before(void);
static void _12V_pm_before(void);
static void _24V_pm_before(void);


static st_blink_dev _nvidia_dev = {	\
		.gpio = TGT_JETSON_PWR,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE,				\
		.pm_doing_befor = _nvidia_pm_before,	\
	};

static st_blink_dev _camera_dev = {       \
		.gpio = TGT_CAMERA_PWR, 		\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE, 			\
		.pm_doing_befor = _camera_pm_before,	\
	};

//static st_blink_dev _sys_5v_dev = { 	\
//		.gpio = TGT_5V_PWR,			\
//		.BlinkSta = DISABLE,			\
//		.NeedAble = ENABLE,			\
//		.pm_doing_befor = _5V_pm_before,	\
//	};
static st_blink_dev _sys_12v_dev = { 	\
		.gpio = TGT_12V_PWR,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE,			\
		.pm_doing_befor = _12V_pm_before,	\
	};
static st_blink_dev _sys_24v_dev = { 	\
		.gpio = TGT_24V_PWR,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE,			\
		.pm_doing_befor = _24V_pm_before,	\
	};


static st_blink_dev _routa_dev = { \
		.gpio = TGT_ROUTERA_RST,		\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE,			\
	};


static st_blink_dev _routb_dev = { \
		.gpio = TGT_ROUTERB_RST, 		\
		.BlinkSta = DISABLE,			\
		.NeedAble = ENABLE, 			\
	};

static st_blink_dev _eth_rst_dev = { \
		.gpio = TGT_ETH_RST, 		\
		.BlinkSta = DISABLE,			\
		.NeedAble = DISABLE, 			\
	};



typedef enum {
    _NET_CFG_MODE     = 0x00,
    _NET_CFG_VER      = 0x01,
    _NET_CFG_RESET    = 0x02,
    _NET_CFG_TCPSTATE = 0x03,
    _NET_CFG_SAVE     = 0x0D,
    _NET_CFG_UPDATE   = 0x0E,
    _NET_CFG_TCPCLT   = 0x10,
    _NET_CFG_IP       = 0x11,
    _NET_CFG_SN       = 0x12,
    _NET_CFG_GW       = 0x13,
    _NET_CFG_LOCPORT  = 0x14,
    _NET_CFG_SVRIP    = 0x15,
    _NET_CFG_SVRPORT  = 0x16,
    _NET_CFG_BAUDRATE = 0x21,
    _NET_CFG_CLRCNT   = 0x24,
    _NET_CFG_CLRUART  = 0x26,
    _NET_CFG_DHCP     = 0x33,
    _NET_CFG_TCPSVR   = 0x34,
    _NET_CFG_TRANS    = 0xFE,
    _NET_CFG_INIT     = 0xFF,
} e_net_cfg;



/* Private function prototypes -----------------------------------------------*/
static uint8_t _cloud_buf[MQTT_SEND_SIZE] = {0};			//云平台发送缓冲区
static uint8_t _nvidia_buf[NVIDIA_SEND_SIZE] = {0};			//英伟达发送缓冲区		
static char _mqtt_topic[128]  = {0};
static uint16_t _mqtt_serno = 0x1;

//static uint8_t SocketBuf[2][MQTT_RECV_SIZE] = {{0}, {0}};	//socket的接收缓冲区
static uint8_t _cloud_recv_buf[MQTT_RECV_SIZE] = {0};
static uint8_t _nvidia_recv_buf[NVIDIA_RECV_SIZE] = {0};

#ifndef SFUD_DEMO_MODE
static spi_user_data net_spi = TGT_NET_CFG;
#else
static spi_user_data net_spi = { .spix = SPI1, .cs_gpiox = GPIOA, .cs_gpio_pin = GPIO_Pin_4 };
#endif

#if (NET_DEBUG == 1)
//static uint8_t _debug_send_buf[256] = {0};
static uint8_t _debug_recv_buf[256] = {0};
void net_debug_printf(char * pcBuf, int wLen) 
{
	if(socket[2].ScokStatus != SOCKET_CONNECTED) return;
	socket[2].send(2, pcBuf, wLen);
}

#endif
#if (NET_DEBUG == 1)
#define MAX_NET		3
#else
#define MAX_NET		2
#endif

static int _Net_Send(uint8_t SockIndex, void *pbuf, unsigned int len)
{
	CH395SendData((st_spi_info *)&net_spi, SockIndex, pbuf, len);
	return len;
}

uint8_t NVIDIA_send_is_idle(void)
{
	if(socket[1].SendLen == 0) return 1;
	else return 0;
}

uint8_t cloud_send_is_idle(void)
{
	if(socket[0].SendLen == 0) return 1;
	else return 0;
}

static void _nvidia_pm_before(void)
{	//英伟达进入关闭前先断开socket连接
//	socket[1].ScokStatus = SOCKET_UNINIT;
//	while(CH395TCPDisconnect((st_spi_info *)&net_spi, socket->Index) == CH395_ERR_UNKNOW);
	_nvidia_dev.ReadyToPm = 0;
}

static void _camera_pm_before(void)
{
	_camera_dev.ReadyToPm = 0;
}

//static void _5V_pm_before(void)
//{
//	_sys_5v_dev.ReadyToPm = 0;
//}

static void _12V_pm_before(void)
{
	_sys_12v_dev.ReadyToPm = 0;
}

static void _24V_pm_before(void)
{
	_sys_24v_dev.ReadyToPm = 0;
}


static int _mqtt_pkt_send(st_socket_info * socket, char *pkey, st_MODBUS_junlei *pframe, uint16_t len, uint8_t cmd, uint8_t replay)
{
	int ret;
	
    len = MODBUS_junlei_framebuild(pframe, len, cmd, replay, _mqtt_serno, gulDevID);
    len = sys_byte2str((char *)socket->pSend, (void *)pframe, "%02x", len);// plus crc16
    if (socket->Index == 0) {
        MQTT_TOPTIC(_mqtt_topic, sizeof(_mqtt_topic), MQTT_PUB_NAME, pkey, (CMD_TRANSPORT == cmd ? 1 : 0));
        ret = mqtt_client_publish(socket, _mqtt_topic, (void *)socket->pSend, len, MQTT_QOS);
    }
    else {
        ret = mqtt_client_publish(socket, NVIDIA_PUB_TOPTIC, (void *)socket->pSend, len, MQTT_QOS);
    }

	if(ret == NET_OK)
	    return len;
	else
		return NET_ERROR;;
}

//static int _do_cmd_net_test(void *pobj, int argc, char *argv[])
//{
//	unsigned char ucSta;
//	if(NULL != sys_strstr(argv[0], "test"))
//	{
//		while(1)
//		{	//初始化CH395A
//			if(CH395A_Init((st_spi_info *)&net_spi) != CH395_ERR_UNKNOW) break;

//			sys_delay_ms(200);
//		}

//		while(1)
//		{	//等待以太网连接成功
//			ucSta = CH395CMDGetPHYStatus((st_spi_info *)&net_spi);
//			if(ucSta != PHY_DISCONN)
//				break;

//			sys_delay_ms(200);
//		}


//		InitSocketParam();
//	}
//	else if(NULL != sys_strstr(argv[0], "physet"))
//	{
//		CH395CMDSetPHY((st_spi_info *)&net_spi, PHY_AUTO);
//	}
//	else if(NULL != sys_strstr(argv[0], "phy"))
//	{
//		printf("phy:%u\r\n",CH395CMDGetPHYStatus((st_spi_info *)&net_spi));
//	}

//    return -1;
//}
//cmd_register("Net", _do_cmd_net_test, "test");		//设置允许启动巡检电压


static int _mqtt_cmdresponse(st_socket_info * socket, unsigned char *pdata, unsigned short num)
{
    if (NULL != pdata && 0 < num) {
        uint16_t space = 256; //MODBUS_JUNLEI_MAXLEN;
        st_MODBUS_junlei *pframe = (st_MODBUS_junlei *)
            &socket->pSend[socket->usSendMaxLen - sizeof(st_MODBUS_junlei) - space];
        uint32_t type = 0;
        int pos;

        pos = sys_strchr((void *)pdata, ':');
        if (num > pos++) {
            uint8_t replay = 1;

            if (!sys_strstr((void *)pdata, _NET_NVIDIA_OPT)
                && !sys_strstr((void *)pdata, _NET_PROPERTY_GET)
                && !sys_strstr((void *)pdata, _NET_INSPECT_SET)
                && !sys_strstr((void *)pdata, _NET_SET_HIGH)
                && !sys_strstr((void *)pdata, _NET_INSPECT_END)
                && !sys_strstr((void *)pdata, _NET_SET_PLAN)
                && !sys_strstr((void *)pdata, _NET_SET_TASK)
                && !sys_strstr((void *)pdata, _NET_INSPECT_EXE)) {
                replay = 0;
            }

            sys_memcpy(pframe->data, pdata, pos);
            if (pos <= num) {
                if (NULL != sys_strstr((void *)pdata, _NET_PROPERTY_GET)) {
                    sys_str2hex((void *)&pdata[pos], socket->pSend, num - pos);
                    if (socket->Index == 0) { // Cloud platform
                        pos += sys_datetime_fill(&pframe->data[pos], 'S');
                    }
                    else {
                        sys_memcpy(&pframe->data[pos], (void *)gulDevID, sizeof(gulDevID));
                        pos += sizeof(gulDevID);
                    }

                    for (type = 0, num = 0; 0 != socket->pSend[num] && socket->usSendMaxLen > num; num++) {
                        type |= (0x1 << socket->pSend[num]);
                    }
                }
                else if (0 != replay) {
                    sys_memcpy(pframe->data, pdata, num);
                    pos = num;
                }
                else {
					sys_memcpy(socket->pSend, pdata, num);
					socket->pSend[num] = '\0';
					memcpy(&pframe->data[pos], &pdata[pos], 2);
					pos += 2;
					_mqtt_serno = gusPacketID;
                }
            }

			if(socket->Index == 0)
				gucPrintFlag = 1;

            _mqtt_pkt_send(socket, DEVICE_KEY, pframe, pos, CMD_TRANSPORT, replay);
			gucPrintFlag = 0;
			_mqtt_serno = 0x01;

            return num;
        }
    }
    return 0;
}

#define HIGHOFFSET  8
#define LOWOFFSET   0

/**
* @brief  save_robot_sta
* @param  pucTarget:目标数据
* @param  ucRoad:通道
* @param  ucNum:数据大小
* @param  data:数据内容
* @retval page size
*/
//uint8_t save_robot_sta(uint8_t * pucTarget, uint8_t ucRoad, uint8_t ucNum, uint8_t * data, uint8_t ucDatalen)
//{
//	*pucTarget++ = ucRoad;
//	*pucTarget++ = ucNum;
//	*pucTarget++ = 0x01;
//	for(uint8_t i = 0; i < ucNum; i++)
//	{
//		if(ucDatalen == 1)
//			*pucTarget++ = (*(uint8_t *)data >> (ucNum - 1 - i));
//		else if(ucDatalen == 2)
//			*pucTarget++ = (*(uint16_t *)data >> (ucNum - 1 - i));
//		else if(ucDatalen == 4)
//			*pucTarget++ = (*(uint32_t *)data >> (ucNum - 1 - i));
//	}
//	return (ucNum + 3);
//}

char mqtt_publish(st_socket_info * socket, uint8_t cmd, uint32_t type)
{
    uint16_t space = 256;
    st_MODBUS_junlei *pframe = (st_MODBUS_junlei *)
        &socket->pSend[socket->usSendMaxLen - sizeof(st_MODBUS_junlei) - space];
    uint8_t *ptypeset = (uint8_t *)socket->pSend;
    char *pkey = DEVICE_KEY;
    uint16_t num = 0, count;
    uint16_t pos = 0;
    uint8_t  i;
	unsigned long ulTemp;

    pos += sys_datetime_fill(&pframe->data[pos], 'B');
    for (i = 0; 0 != type; type >>= 1, i++) {
        if (0 == (type & 0x1)) continue;
		if((i != PROP_PINQ) && (i != PROP_TIME))
		{
			ptypeset[num++] = 0xB2;
			ptypeset[num++] = 0x01;
			ptypeset[num++] = 0x01;
		}
        switch (i) {
            case PROP_TASK:
				ptypeset[num++] = i;
				count = 0x01;
                ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
                break;
            case PROP_BAT:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_CAPAC;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gBatDevInfo.usBatVoltage >> HIGHOFFSET);
				ptypeset[num++] = (gBatDevInfo.usBatVoltage >> LOWOFFSET);
				ptypeset[num++] = DATA_TEMP;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gBatDevInfo.usTemp >> HIGHOFFSET);
				ptypeset[num++] = (gBatDevInfo.usTemp >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gBatDevInfo.ucBatSta;
                ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gBatDevInfo.usErrcode >> HIGHOFFSET);
				ptypeset[num++] = (gBatDevInfo.usErrcode >> LOWOFFSET);
                break;
            case PROP_LIFT:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_DISTANCE;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gLiftDevInfo.ulHigh >> 16);
				ptypeset[num++] = (gLiftDevInfo.ulHigh >> HIGHOFFSET);
				ptypeset[num++] = (gLiftDevInfo.ulHigh >> LOWOFFSET);
				ptypeset[num++] = DATA_TEMP;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gLiftDevInfo.usTemp >> HIGHOFFSET);
				ptypeset[num++] = (gLiftDevInfo.usTemp >> LOWOFFSET);
				ptypeset[num++] = DATA_SPEED;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gLiftDevInfo.ulSpeed >> 16);
				ptypeset[num++] = (gLiftDevInfo.ulSpeed >> HIGHOFFSET);
				ptypeset[num++] = (gLiftDevInfo.ulSpeed >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gLiftDevInfo.ucLiftSta;
                ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gLiftDevInfo.usErrcode >> HIGHOFFSET);
				ptypeset[num++] = (gLiftDevInfo.usErrcode >> LOWOFFSET);
				//debug_printf("Lift h:%u\r\n",gLiftDevInfo.ulHigh);
                break;
            case PROP_WALK:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_DISTANCE;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				if(gucIsHome) gWalkDevInfo.ulPos = 0;
				ptypeset[num++] = (gWalkDevInfo.ulPos >> 16);
				ptypeset[num++] = (gWalkDevInfo.ulPos >> HIGHOFFSET);
				ptypeset[num++] = (gWalkDevInfo.ulPos >> LOWOFFSET);
				ptypeset[num++] = DATA_TEMP;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gWalkDevInfo.usTemp >> HIGHOFFSET);
				ptypeset[num++] = (gWalkDevInfo.usTemp >> LOWOFFSET);
				ptypeset[num++] = DATA_SPEED;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gWalkDevInfo.lSpeed >> 16);
				ptypeset[num++] = (gWalkDevInfo.lSpeed >> HIGHOFFSET);
				ptypeset[num++] = (gWalkDevInfo.lSpeed >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gWalkDevInfo.ucWalkSta;
                ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gWalkDevInfo.usErrcode >> HIGHOFFSET);
				ptypeset[num++] = (gWalkDevInfo.usErrcode >> LOWOFFSET);
                break;
			case PROP_NVIDIA:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gNvidiaDevInfo.ucNVIDIASta;
				ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gNvidiaDevInfo.usErrcode >> HIGHOFFSET);
				ptypeset[num++] = (gNvidiaDevInfo.usErrcode >> LOWOFFSET);
				break;
            case PROP_RFID_DEV:			//无法确认RFID卡是否损坏,默认为正常
            	ptypeset[num++] = i;
				ptypeset[num++] = DATA_RFID_ID;
				ptypeset[num++] = 0x0C;
				ptypeset[num++] = 0x01;
				for(count = 0; count < 12; count++)
					ptypeset[num++] = gRfidDevInfo.ucRfidCard[count];
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				if(gRfidDevMachine.ucDevSta == 9)
					ptypeset[num++] = gRfidDevMachine.ucDevSta;
				else
					ptypeset[num++] = gucPosLRFlag + 1;
				ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = gRfidDevMachine.ucDevErrCode;
                break;
			case PROP_RFID_CARD:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_RFID_ID;
				ptypeset[num++] = 0x0C;
				ptypeset[num++] = 0x01;
				for(count = 0; count < 12; count++)
					ptypeset[num++] = gRfidDevInfo.ucRfidCard[count];
				ptypeset[num++] = DATA_DISTANCE;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = 0x00;
				break;
            case PROP_LINE:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_DISTANCE;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gBlockDevInfo.ulBlockDis >> 16);
				ptypeset[num++] = (gBlockDevInfo.ulBlockDis >> HIGHOFFSET);
				ptypeset[num++] = (gBlockDevInfo.ulBlockDis >> LOWOFFSET);
                ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gBlockDevInfo.usBlockErrCode >> HIGHOFFSET);
				ptypeset[num++] = (gBlockDevInfo.usBlockErrCode >> LOWOFFSET);
                break;
            case PROP_LED:
				for(count = 0; count < 2; count++)
				{
					if(count == 1)
					{
						ptypeset[num++] = 0xB2;
						ptypeset[num++] = 0x01;
						ptypeset[num++] = 0x01;
					}
					ptypeset[num++] = i;
					ptypeset[num++] = DATA_NUMBER;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = count;
					ptypeset[num++] = DATA_STATE;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = (count == 0) ? gLedDevInfo.ucLedSta1 : gLedDevInfo.ucLedSta2;
	                ptypeset[num++] = DATA_ERROR;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = (count == 0) ? gLedDevInfo.ucLedErrCode1 : gLedDevInfo.ucLedErrCode2;
				}
                break;
            case PROP_FANS:
                for(count = 0; count < 2; count++)
				{
					if(count == 1)
					{
						ptypeset[num++] = cmd;
						ptypeset[num++] = 0x01;
						ptypeset[num++] = 0x01;
					}
					ptypeset[num++] = i;
					ptypeset[num++] = DATA_NUMBER;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = count;
					ptypeset[num++] = DATA_STATE;
					ptypeset[num++] = (count == 0) ? gFanDevInfo.ucFanSta1 : gFanDevInfo.ucFanSta2;
	                ptypeset[num++] = DATA_ERROR;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = (count == 0) ? gFanDevInfo.ucFanErrSta1 : gFanDevInfo.ucFanErrSta2;
				}
                break;
            case PROP_ULTRASONIC:
                for(count = 0; count < 2; count++)
				{
					if(count == 1)
					{
						ptypeset[num++] = 0xB2;
						ptypeset[num++] = 0x01;
						ptypeset[num++] = 0x01;
					}
					ptypeset[num++] = i;
					ptypeset[num++] = DATA_NUMBER;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = count;
					ptypeset[num++] = DATA_STATE;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = (count == 0) ? gUltrasDevInfo.ucUltrasSta1 : gUltrasDevInfo.ucUltrasSta2;
	                ptypeset[num++] = DATA_ERROR;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = (count == 0) ? gUltrasDevInfo.ucUltrasErrCode1 : gUltrasDevInfo.ucUltrasErrCode2;
				}
                break;
			case PROP_HALL_SWITCH:
				for(count = 0; count < 2; count++)
				{
					if(count == 1)
					{
						ptypeset[num++] = 0xB2;
						ptypeset[num++] = 0x01;
						ptypeset[num++] = 0x01;
					}
					ptypeset[num++] = i;
					ptypeset[num++] = DATA_NUMBER;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = count;
					ptypeset[num++] = DATA_STATE;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = (count == 0) ? gDetectDevInfo.ucDetectSta1 : gDetectDevInfo.ucDetectSta2;
	                ptypeset[num++] = DATA_ERROR;
					ptypeset[num++] = 0x02;
					ptypeset[num++] = 0x01;
					ptypeset[num++] = 0x00;
					ptypeset[num++] = (count == 0) ? gDetectDevInfo.ucDetectErrSta1 : gDetectDevInfo.ucDetectErrSta2;
				}
                break;
            case PROP_WIRELESS_CHARGE:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_CAPAC;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gWireDevInfo.usWireVoltage >> HIGHOFFSET);
				ptypeset[num++] = (gWireDevInfo.usWireVoltage >> LOWOFFSET);
				ptypeset[num++] = DATA_TEMP;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gWireDevInfo.usWireTemp >> HIGHOFFSET);
				ptypeset[num++] = (gWireDevInfo.usWireTemp >> LOWOFFSET);
                ptypeset[num++] = DATA_SPEED;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x00;
				ptypeset[num++] = (gWireDevInfo.usWireCurrent >> HIGHOFFSET);
				ptypeset[num++] = (gWireDevInfo.usWireCurrent >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gWireDevInfo.ucWireSta;
				ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gWireDevInfo.usWireErrCode >> HIGHOFFSET);
				ptypeset[num++] = (gWireDevInfo.usWireErrCode >> LOWOFFSET);
                break;
			case PROP_MODE:
				ptypeset[num++] = i;
                ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gWorkWorker.WorkMode;
                break;
            case PROP_STATE:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gRotbotInfo.ucRobotSta;
                ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gRotbotInfo.usRobotErrCode >> HIGHOFFSET);
				ptypeset[num++] = (gRotbotInfo.usRobotErrCode >> LOWOFFSET);
                break;
            case PROP_ACTIVE:
				ptypeset[num++] = i;
                pkey = (char *)cloud_client_ID;
                break;
			case PROP_PINQ:
				if(num != 0) return 0;
				ptypeset[num++] = 0xB2;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x12;
				break;
			case PROP_TIME:
				if(num != 0) return 0;
				ptypeset[num++] = 0xB2;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x13;
				break;
			case PROP_TEMP:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_TEMP_LOW;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gusTempValue >> HIGHOFFSET);
				ptypeset[num++] = (gusTempValue >> LOWOFFSET);
                ptypeset[num++] = 0x3B;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gusHumiValue >> HIGHOFFSET);
				ptypeset[num++] = (gusHumiValue >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gucTempSta;
				ptypeset[num++] = DATA_DISTANCE;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				if(gucIsHome) gWalkDevInfo.ulPos = 0;
				ptypeset[num++] = (gWalkDevInfo.ulPos >> 16);
				ptypeset[num++] = (gWalkDevInfo.ulPos >> HIGHOFFSET);
				ptypeset[num++] = (gWalkDevInfo.ulPos >> LOWOFFSET);
                break;
			case PROP_LINE_CHARGE:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gLineDevInfo.ucLineSta;
                ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gLineDevInfo.usErrcode >> HIGHOFFSET);
				ptypeset[num++] = (gLineDevInfo.usErrcode >> LOWOFFSET);
                break;
			case PROP_PLAN:
				ptypeset[num++] = i;
                ptypeset[num++] = 0x88;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gPlanDev.ulPlanID >> 16);
				ptypeset[num++] = (gPlanDev.ulPlanID >> HIGHOFFSET);
				ptypeset[num++] = (gPlanDev.ulPlanID >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gPlanDev.ucPlanSta;
				ptypeset[num++] = DATA_CH4;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gPlanDev.usTaskID >> HIGHOFFSET);
				ptypeset[num++] = (gPlanDev.usTaskID >> LOWOFFSET);
				ptypeset[num++] = DATA_ERROR;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gPlanDev.usPlanErrCode >> HIGHOFFSET);
				ptypeset[num++] = (gPlanDev.usPlanErrCode >> LOWOFFSET);
                break;
			case PROP_PARA:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_SPEED;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ulTemp = (unsigned long)(gSysParameter.fAutoSpeed * 10);
				ptypeset[num++] = (ulTemp >> 16);
				ptypeset[num++] = (ulTemp >> HIGHOFFSET);
				ptypeset[num++] = (ulTemp >> LOWOFFSET);
				ptypeset[num++] = 0x6D;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ulTemp = gSysParameter.fAllowWorkVal * 1000;
				ptypeset[num++] = (ulTemp >> 16);
				ptypeset[num++] = (ulTemp >> HIGHOFFSET);
				ptypeset[num++] = (ulTemp >> LOWOFFSET);
				ptypeset[num++] = 0x6E;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ulTemp = gSysParameter.fBackCHargeVal * 1000;
				ptypeset[num++] = (ulTemp >> 16);
				ptypeset[num++] = (ulTemp >> HIGHOFFSET);
				ptypeset[num++] = (ulTemp >> LOWOFFSET);
				ptypeset[num++] = 0x88;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ulTemp = gSysParameter.ulSleepAfter;
				ptypeset[num++] = (ulTemp >> 16);
				ptypeset[num++] = (ulTemp >> HIGHOFFSET);
				ptypeset[num++] = (ulTemp >> LOWOFFSET);
				ptypeset[num++] = 0x56;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				ulTemp = gSysParameter.ulSleepBefore;
				ptypeset[num++] = (ulTemp >> 16);
				ptypeset[num++] = (ulTemp >> HIGHOFFSET);
				ptypeset[num++] = (ulTemp >> LOWOFFSET);
				ptypeset[num++] = 0x70;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gSysParameter.ucLowChargeHandlerWay;
                break;
			case PROP_GAS:
				ptypeset[num++] = i;
				ptypeset[num++] = DATA_TEMP_LOW;			//O2
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gGasInfo.usO2Vol >> HIGHOFFSET);
				ptypeset[num++] = (gGasInfo.usO2Vol >> LOWOFFSET);
				ptypeset[num++] = DATA_CH4;				//CH4
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gGasInfo.usCH4LEL >> HIGHOFFSET);
				ptypeset[num++] = (gGasInfo.usCH4LEL >> LOWOFFSET);
				ptypeset[num++] = DATA_RSSI;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gGasInfo.usH2SPpm >> HIGHOFFSET);
				ptypeset[num++] = (gGasInfo.usH2SPpm >> LOWOFFSET);
				ptypeset[num++] = DATA_CO;
				ptypeset[num++] = 0x02;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = (gGasInfo.usCOPpm >> HIGHOFFSET);
				ptypeset[num++] = (gGasInfo.usCOPpm >> LOWOFFSET);
				ptypeset[num++] = DATA_DISTANCE;
				ptypeset[num++] = 0x03;
				ptypeset[num++] = 0x01;
				if(gucIsHome) gWalkDevInfo.ulPos = 0;
				ptypeset[num++] = (gWalkDevInfo.ulPos >> 16);
				ptypeset[num++] = (gWalkDevInfo.ulPos >> HIGHOFFSET);
				ptypeset[num++] = (gWalkDevInfo.ulPos >> LOWOFFSET);
				ptypeset[num++] = DATA_STATE;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = 0x01;
				ptypeset[num++] = gGasInfo.ucGasSta;
                break;
				
            default:return 0;
        }
    }

	memcpy(&pframe->data[pos], ptypeset, num);
	pos += num;
    _mqtt_serno ++;
    cfg_data_write((void *)&_mqtt_serno, CFG_CLOUD_PKTSER, sizeof(_mqtt_serno));
    if(_mqtt_pkt_send(socket, pkey, pframe, pos, cmd, true) == NET_ERROR)
		return (char)NET_ERROR;
	return i;
}

bool IsSocketConnected(st_socket_info * socket)
{
	return (socket->ScokStatus & SOCKET_CONNECTED);
}

bool IsMQTTConneted(st_socket_info * socket)
{
	return (socket->ScokStatus & MQTT_CONNECTED);
}

bool IsMQTTSUBLISH(st_socket_info * socket)
{
	return (socket->ScokStatus & MQTT_SUBLISHED);
}

static void _module_process(void)
{	
	//用于建立socket连接及数据接收
	for(unsigned char ucNum = 0; ucNum < MAX_NET; ucNum++)
	{
		if(!is_dev_enable(&_nvidia_dev) && ucNum == 1)	//英伟达低功耗模式充值状态后退出
		{
			socket[ucNum].ScokStatus = SOCKET_UNINIT;	
			break;
		}
		if((ucNum < 2) && (socket[ucNum].ScokStatus & SOCKET_UNINIT) && sys_istimeout(socket[ucNum].SendTick, 200))
		{
			if(CH395SocketInitOpen((st_spi_info *)&net_spi, &socket[ucNum]) == CMD_ERR_SUCCESS)
				socket[ucNum].ScokStatus = SOCKET_WAIT_SUCCESS;
		}
		if((ucNum < 2) && (socket[ucNum].TimeOutFlag == 1) && ((sys_get_ms() - socket[ucNum].SendTick) > 2000))
		{
			socket[ucNum].ScokStatus = SOCKET_UNINIT;
			socket[ucNum].TimeOutFlag = 0;
		}
		if((ucNum < 2) && (socket[ucNum].ScokStatus & SOCKET_WAIT_SUCCESS) && sys_istimeout(socket[ucNum].SendTick, 20000))
			socket[ucNum].ScokStatus = SOCKET_UNINIT;	
	}

	if(!(socket[0].ScokStatus & SOCKET_UNINIT) || !(socket[1].ScokStatus & SOCKET_UNINIT))
	{	//至少一个socket不在未初始化状态
		CH395GlobalInterrupt((st_spi_info *)&net_spi, socket);
	}

	sys_delay_ms(10);
}

uint8_t ten_to_hex_ascii(uint8_t ucTen)
{
	uint8_t ucAscii = '0';
	if(ucTen <= 9)
		ucAscii = ucTen + '0';
	return ucAscii;
}

static int _net_nvidia_urc(unsigned short func, unsigned char *pdata, unsigned short num)
{
	if(IsMQTTSUBLISH(&socket[1]))
	{	//等待英伟达连接成功
        void *pcmd = NULL;

        if (SYS_ACT_INSPECT_SET == func) {
            pcmd = _NET_INSPECT_SET;
        }
        else if (SYS_ACT_INSPECT_EXE == func) {
            pcmd = _NET_INSPECT_EXE;
			socket[1].SendLen = strlen(pcmd);
			sys_memcpy(socket[1].pSend, pcmd, socket[1].SendLen);
			socket[1].pSend[socket[1].SendLen++] = '0';
			socket[1].pSend[socket[1].SendLen++] ='1';
        }
		else if(SYS_ACT_SET_HIGH == func) {
			pcmd = _NET_SET_HIGH;
			socket[1].SendLen = strlen(pcmd);
			sys_memcpy(socket[1].pSend, pcmd, socket[1].SendLen);
		}
		else if(SYS_ACT_END_INSPECT == func) {
			pcmd = _NET_INSPECT_END;
			socket[1].SendLen = strlen(pcmd);
			sys_memcpy(socket[1].pSend, pcmd, socket[1].SendLen);
			socket[1].pSend[socket[1].SendLen++] = '0';
			if(num == 1)
				socket[1].pSend[socket[1].SendLen++] ='1';
			else
				socket[1].pSend[socket[1].SendLen++] ='9';
		}
		else if(SYS_ACT_SET_PLAN == func)
		{
			pcmd = _NET_SET_PLAN;
			socket[1].SendLen = strlen(pcmd);
			sys_memcpy(socket[1].pSend, pcmd, socket[1].SendLen);
			memcpy(&socket[1].pSend[socket[1].SendLen], gWorkWorker.CurrentPlanID, 6);
			socket[1].SendLen += 6;
		}
		else if(SYS_ACT_SET_TASK == func)
		{
			pcmd = _NET_SET_TASK;
			socket[1].SendLen = strlen(pcmd);
			sys_memcpy(socket[1].pSend, pcmd, socket[1].SendLen);
			socket[1].pSend[socket[1].SendLen++] = ten_to_hex_ascii((uint8_t)(gWorkWorker.usTaskID / 1000));
			socket[1].pSend[socket[1].SendLen++] = ten_to_hex_ascii((uint8_t)((gWorkWorker.usTaskID % 1000) /100));
			socket[1].pSend[socket[1].SendLen++] = ten_to_hex_ascii((uint8_t)((gWorkWorker.usTaskID % 100) /10));
			socket[1].pSend[socket[1].SendLen++] = ten_to_hex_ascii((uint8_t)(gWorkWorker.usTaskID % 10));
		}

        if (NULL != pcmd && NULL != pdata && 0 < num) {
            int ntmp = strlen(pcmd);

            sys_memcpy(socket[1].pSend, pcmd, ntmp);
            ntmp += sys_hex2str((void *)&socket[1].pSend[ntmp], pdata, num);
            socket[1].SendLen = ntmp;
            return 0;
        }
	}

	return -1;
}
sys_urc_register(SYS_MODULE_NVIDIA, _net_nvidia_urc);

void keepalive_cloud(st_socket_info * socket)
{
	debug_printf("start pingreq\r\n");

    mqtt_publish(socket, CMD_TEST, (BIT(0) << PROP_PINQ));
	socket->Ping = 1;
	socket->PinqTick = sys_get_ms();
	gucMqttPinqTry++;
}

void ascii2ascii(uint8_t * pucTarget, uint8_t * pucData, uint16_t usDataLen)
{
	uint16_t i = 0;
	while(i < usDataLen)
	{
		if(pucData[i] >= '0' && pucData[i] <= '9')
			pucTarget[i / 2] |= (pucData[i] - '0') << ((1 - (i % 2)) * 4);
		else if(pucData[i] >= 'A' && pucData[i] <= 'F')
			pucTarget[i / 2] |= (pucData[i] - 'A' + 10) << ((1 - (i % 2)) * 4);
		else if(pucData[i] >= 'a' && pucData[i] <= 'f')
			pucTarget[i / 2] |= (pucData[i] - 'a' + 10) << ((1 - (i % 2)) * 4);
		if((i % 2) == 1)
		{	
			gOTAFlag.ulCRCCheck += pucTarget[i/2];
		}
		i++;
	}
}

void update_reply_success(void)
{
	memset(socket[0].pSend, 0, socket[0].usSendMaxLen);
	memcpy(socket[0].pSend, "Update:01", 9);
	socket[0].SendLen = 9;
}

void update_reply_error(void)
{
	memset(socket[0].pSend, 0, socket[0].usSendMaxLen);
	memcpy(socket[0].pSend, "Update:09", 9);
	socket[0].SendLen = 9;
}

uint8_t APP2_data_write(uint32_t *buf, uint32_t wordCnt)
{
	for(uint8_t i = 0; i < 3; i++)
	{
		if(FLASH_WriteWords(gOTAFlag.ulLastPos, (uint32_t *)buf, wordCnt) != FLASH_COMPLETE)
		{
			if(i == 2)
			{	//升级失败
				debug_printf("Write Error!\r\n");
				update_reply_error();
				gOTAFlag.ucUpdateFlag = 0;
				return 1;
			}
		}
		else
		{
			gOTAFlag.ulLastPos += 256;
			return 0;
		}
	}
	return 0;
}

static int _do_cmd_OTA_select(void *pobj, int argc, char *argv[])
{
	if(NULL != sys_strstr(argv[0], "start"))
	{
		if(argc != 2) return -1;
		if(gucIsHome == 0) {	//设备仅允许在原点时进行升级
			update_reply_error();
			return -1;
		}
		if((gOTAFlag.ucUpdateFlag == 0) && sys_str2num(argv[1], (int *)&gOTAFlag.ulUpdatePagBit) == 0)
		{
			gOTAFlag.ucUpdateFlag = 1;
			gOTAFlag.usUpdatePagNum = gOTAFlag.ulUpdatePagBit / 256 + ((gOTAFlag.ulUpdatePagBit % 256) ? 1 : 0);
			gOTAFlag.usUpdateCurrent = 0;
			gOTAFlag.ulCRCCheck = 0;
			for(uint8_t i = 0; i < 3; i++)
			{
				if(FLASH_EraseApp2() != FLASH_COMPLETE)
				{	//启动升级时需擦除APP2片区
					if(i == 3)
					{	//升级失败
						debug_printf("Erase APP2 falied");
						update_reply_error();
						return -1;
					}
				}
				else
					break;
			}
			gOTAFlag.ulLastRecvTick = sys_get_ms();
			gOTAFlag.ulLastPos = APP2_ADDR;
			start_to_pm_ctl(__func__, __LINE__);	//设备升级过程中进入休眠
			update_reply_success();
		}
		else
			update_reply_error();
	}
	else if(NULL != sys_strstr(argv[0], "data"))
	{
		if(argc != 3) return -1;
		if(gOTAFlag.ucUpdateFlag == 1)
		{
			uint16_t usDataNum;
			uint8_t pucTarget[256] = {0};
			if(sys_hexstr2num(argv[1], (int *)&usDataNum) == 0)
			{
				if(usDataNum == gOTAFlag.usUpdateCurrent)
				{
					usDataNum = (gOTAFlag.usUpdateCurrent < (gOTAFlag.usUpdatePagNum - 1)) ? 256 : (gOTAFlag.ulUpdatePagBit % 256);
					if((strlen(argv[2]) / 2) < usDataNum)
					{
						update_reply_error();
						return -1;
					}
					if(usDataNum % 2) usDataNum -= 1;
					ascii2ascii(pucTarget, (uint8_t *)argv[2], usDataNum * 2);
					memcpy(gucUpdateBuf, pucTarget, usDataNum);
					if(APP2_data_write((uint32_t *)gucUpdateBuf, 256 / 4))
						return -1;
					gOTAFlag.usUpdateCurrent++;
					if(gOTAFlag.usUpdateCurrent == gOTAFlag.usUpdatePagNum)
						gOTAFlag.ucUpdateFlag = 0;
					gOTAFlag.ulLastRecvTick = sys_get_ms();
					update_reply_success();
				}
				else if(usDataNum < gOTAFlag.usUpdateCurrent)
					update_reply_success();
				else
					update_reply_error();
			}
			else
				update_reply_error();
		}
		else
			update_reply_error();
	}
	else if(NULL != sys_strstr(argv[0], "cancel"))
	{
		gOTAFlag.ucUpdateFlag = 0;
		update_reply_success();
	}
	else if(NULL != sys_strstr(argv[0], "fail"))
	{
		gOTAFlag.ucUpdateFlag = 0;
		update_reply_success();
	}
	else if(NULL != sys_strstr(argv[0], "finish"))
	{
		uint8_t pucBuff[4];
		uint32_t ulCheck = 0;
		uint16_t usLen = (gOTAFlag.ulUpdatePagBit / 4 + ((gOTAFlag.ulUpdatePagBit % 4) ? 1 : 0));

		gOTAFlag.ucUpdateFlag = 0;
		for(uint16_t usIndex = 0;usIndex < usLen; usIndex++)
		{
			flash_read_buf(APP2_ADDR + usIndex * 4, (uint32_t *)pucBuff, 1);
			uint8_t ucGetNum = 4;
			if(usIndex == (usLen - 1) && (gOTAFlag.ulUpdatePagBit % 4))
				ucGetNum = gOTAFlag.ulUpdatePagBit % 4;
			for(uint8_t i = 0; i < ucGetNum; i++)
				ulCheck += pucBuff[i];
			//debug_printf("%02X%02X%02X%02X", pucBuff[0],pucBuff[1], pucBuff[2], pucBuff[3]);
		}
		if(ulCheck == gOTAFlag.ulCRCCheck)
		{
			debug_printf("Write success!\r\n");
			update_reply_success();
			gOTAFlag.ucUpdateFinish = 1;
		}
		else
		{
			debug_printf("Write fail!\r\n");
			update_reply_error();
		}
	}

    return -1;
}
cmd_register("Update", _do_cmd_OTA_select, "start/data/cancel");	


void request_curtime(st_socket_info * socket)
{
	char * dataIndex = NULL;

    mqtt_publish(socket, CMD_TEST, (BIT(0) << PROP_TIME));

	while(!net_istimeout(socket->SendTick, 30000))
	{
		if(socket->RecvSta == 1)
		{
			for(int i = 0; i < socket->RecvLen; i++)
			{
				if(socket->pRecvBuf[i] == 0)
					socket->pRecvBuf[i] = ' ';
			}

			dataIndex = strstr((const char *)socket->pRecvBuf, "pub_reply");
			if(dataIndex != NULL) 
			{
				dataIndex += 29;
				if(time_transfer((unsigned char *)dataIndex))
					gucTimeGetFlag = 1;
				return;
			}
			socket->RecvSta = 0;
		}

		sys_delay_ms(10);
	}

	socket->ScokStatus = SOCKET_CONNECTED | MQTT_CONNECTED;
}

static void _cloud_process(void)
{	//用于MQTT连接及数据发送
	int ret;
	st_socket_info * CurSocket;
	mqtt_qos qos;
	unsigned long ulTemp;
	
	CurSocket = &socket[0];
	if(gOTAFlag.ucUpdateFinish && (CurSocket->SendLen == 0))	//升级包已下载完成且已回复设备
	{
		sys_delay_ms(1000);
		app_request_upgrade();
	}
	if(gOTAFlag.ucUpdateFlag && sys_over_time(gOTAFlag.ulLastRecvTick, 60000))	//一分钟内没接收到下一包数据则退出升级状态
		gOTAFlag.ucUpdateFlag = 0;
	if(IsSocketConnected(CurSocket))
	{	//socket已建立连接
		if(!IsMQTTConneted(CurSocket))
		{	//MQTT连接
			gucConnetFLag = 0;
			memcpy(cloud_client_ID, net_dev_sn, 13);
			cloud_client_ID[13] = sys_get_ms() % 10 + 48;
			cloud_client_ID[14] = sys_get_ms() % 26 + 97;
			cloud_client_ID[15] = sys_get_ms() % 16 + 65;
			if(mqtt_client_connect(CurSocket, (char *)cloud_client_ID) == NET_OK)
				CurSocket->ScokStatus |= MQTT_CONNECTED;
		}
		else if(!IsMQTTSUBLISH(CurSocket))
		{	//订阅主题
			ret = NET_OK;
			
			if (0 < MQTT_TOPTIC(_mqtt_topic, sizeof(_mqtt_topic), MQTT_SUB_NAME, cloud_client_ID, false) && (ret == NET_OK)) {
                ret = mqtt_client_subscribe(CurSocket, (void *)_mqtt_topic, MQTT_QOS, &qos);
            }

            if (0 < MQTT_TOPTIC(_mqtt_topic, sizeof(_mqtt_topic), MQTT_SUB_NAME, DEVICE_KEY, false) && (ret == NET_OK)) {
                ret = mqtt_client_subscribe(CurSocket, _mqtt_topic, MQTT_QOS, &qos);
            }

            if (0 < MQTT_TOPTIC(_mqtt_topic, sizeof(_mqtt_topic), MQTT_SUB_NAME, DEVICE_KEY, true) && (ret == NET_OK)) {
                ret = mqtt_client_subscribe(CurSocket, _mqtt_topic, MQTT_QOS, &qos);
            }
			if(ret == NET_OK)
			{
				CurSocket->ScokStatus |= MQTT_SUBLISHED;
				//gpio_dev_set_status(&gLed_back_dev, ENABLE);
				start_gpio_blink(&gLed_back_dev, 500, 500, 3);
				debug_printf("MQTT Connected!\r\n");
				gulCloudAliveTick = sys_get_ms();
				CurSocket->Ping = 0;
				gucConnetFLag = 1;
			}
		}
		else
		{
			if((CurSocket->Ping) && ((sys_get_ms()- CurSocket->PinqTick) > 30 * 1000))
			{
				CurSocket->Ping = 0;
				if(gucMqttPinqTry >= 3)
				{
					CurSocket->ScokStatus = SOCKET_CONNECTED;
					gpio_dev_set_status(&gLed_back_dev, DISABLE);
				}
				debug_printf("Unreceive pingreq reply\r\n");
			}

			if(CurSocket->SendLen)
			{
				debug_printf("reply:%s\r\n", CurSocket->pSend);
				_mqtt_cmdresponse(CurSocket, CurSocket->pSend, CurSocket->SendLen);
				CurSocket->SendLen = 0;
			}
			else if(CurSocket->RecvSta == 1)
			{	//接收到数据
				for(int i = 0; i < CurSocket->RecvLen; i++)
				{
					if(CurSocket->pRecvBuf[i] == 0)
						CurSocket->pRecvBuf[i] = ' ';
				}
				publish_packet_process(CurSocket);
				CurSocket->RecvSta = 0;
			}
			else if(gucTimeGetFlag == 0)
				request_curtime(CurSocket);
			else if((socket->Ping == 0) && sys_over_time(gulCloudAliveTick, 60 * 1000))
				keepalive_cloud(CurSocket);
			else if(IS_UNUPDATING() && gulCloudPushFlag)
			{	//升级过程中不上报状态，避免过多数据
				for(int i = 0; i < 32; i++)
				{
					if((gulCloudPushFlag >> i) & BIT(0))
					{
						ulTemp = BIT(i);
						gulCloudPushFlag &= ~ulTemp;
						break;
					}
				}
				mqtt_publish(CurSocket, CMD_TEST, ulTemp);
			}
		}
	}

	sys_delay_ms(50);
}

extern int mqtt_data_handler(unsigned char * Buf);

static void _NVIDIA_process(void)
{	//用于MQTT连接及数据发送
	int ret;
	st_socket_info * CurSocket;
	mqtt_qos qos;
	static unsigned long sulTick;
	
	CurSocket = &socket[1];

#if (NET_DEBUG == 1)
	if(socket[2].ScokStatus & SOCKET_CONNECTED)
	{
		if(socket[2].RecvSta)
		{
			mqtt_data_handler(socket[2].pRecvBuf);
			socket[2].RecvSta = 0;
		}
	}
#endif

	if(IsSocketConnected(CurSocket) && is_dev_enable(&_nvidia_dev))
	{	//socket已建立连接
		if(!IsMQTTConneted(CurSocket))
		{	//MQTT连接
			memcpy(nvidia_client_ID, net_dev_sn, 13);
			nvidia_client_ID[13] = sys_get_ms() % 10 + 48;
			nvidia_client_ID[14] = sys_get_ms() % 26 + 97;
			nvidia_client_ID[15] = sys_get_ms() % 16 + 65;
			if(mqtt_client_connect(CurSocket, (char *)nvidia_client_ID) == NET_OK)
				CurSocket->ScokStatus |= MQTT_CONNECTED;
		}
		else if(!IsMQTTSUBLISH(CurSocket))
		{	//订阅主题
			ret = NET_OK;
			ret = mqtt_client_subscribe(CurSocket, NVIDIA_SUB_TOPTIC, QOS0, &qos);
			if(ret == NET_OK)
			{
				//gpio_dev_set_status(&gLed_front_dev, ENABLE);
				start_gpio_blink(&gLed_front_dev, 500, 500, 3);
				CurSocket->ScokStatus |= MQTT_SUBLISHED;
				debug_printf("NVIDIA Connected!\r\n");
				gNvidiaDevInfo.ucNVIDIASta = 0;
				gNvidiaDevInfo.usErrcode = 0;
				set_cloud_push_flag(PROP_NVIDIA);
			}
		}
		else
		{
			if(CurSocket->RecvSta == 1)
			{		//接收到数据则进行数据处理
				for(int i = 0; i < CurSocket->RecvLen; i++)
				{
					if(CurSocket->pRecvBuf[i] == 0)
						CurSocket->pRecvBuf[i] = ' ';
				}
				publish_packet_process(CurSocket);
				CurSocket->RecvSta = 0;
			}
			else if(CurSocket->SendLen)
			{
				_mqtt_cmdresponse(CurSocket, CurSocket->pSend, CurSocket->SendLen);
				CurSocket->SendLen = 0;
			}
			else if(sys_over_time(sulTick, 10000) && ((gWorkWorker.CurrentWork < WORK_ASK_PHOTO) || (gWorkWorker.CurrentWork > WORK_VIEW_FINISH)))
			{
				char tmpbuf[16] = _NET_NVIDIA_OPT;
				int pos = strlen(tmpbuf);

				tmpbuf[pos++] = '0';
				tmpbuf[pos++] = '1';
				_mqtt_cmdresponse(CurSocket, (void *)tmpbuf, pos);
				sulTick = CurSocket->SendTick = sys_get_ms();
			}
			else	//未接收到数据则判断是否需要发送心跳包
				keepalive(CurSocket);
		}
	}

	if(!is_dev_enable(&_nvidia_dev))
	{	//英伟达未上电
		if((gNvidiaDevInfo.ucNVIDIASta != 9) ||(gNvidiaDevInfo.usErrcode != 2))
		{	//休眠
			gNvidiaDevInfo.ucNVIDIASta = 9;
			gNvidiaDevInfo.usErrcode = 2;
			set_cloud_push_flag(PROP_NVIDIA);
		}
	}
	else
	{
		if((gNvidiaDevInfo.ucNVIDIASta == 9) && (gNvidiaDevInfo.usErrcode == 2))
		{	//休眠被唤醒
			gNvidiaDevInfo.ucNVIDIASta = 9;
			gNvidiaDevInfo.usErrcode = 1;
			set_cloud_push_flag(PROP_NVIDIA);
		}
	}

	sys_delay_ms(100);
}


void InitSocketParam(void)
{
	for(unsigned char Num = 0; Num < 2; Num++)
	{
	    memset(&socket[Num],0,sizeof(st_socket_info));                /* 将Socket[0]全部清零*/
	    socket[Num].DesPort = MQTT_SVR_PORT;                             /* 目的端口 */
	    socket[Num].SourPort = (Local_Port + Num);                         /* 源端口 */
	    socket[Num].ProtoType = PROTO_TYPE_TCP;                       /* TCP模式 */
	    socket[Num].TcpMode = TCP_CLIENT_MODE;                        /* TCP客户端模式 */
		socket[Num].ScokStatus = SOCKET_UNINIT;						//将socket状态置为初始状态
		socket[Num].Index = Num;									/*socket 索引*/
		socket[Num].send = _Net_Send;
		socket[Num].RemLen = 0;										
	}

	memcpy(socket[0].IPAddr,Cloud_IP,sizeof(uint8_t) * 4);     	 	 /* 将目的IP地址写入 */
	socket[0].pSend = _cloud_buf;									/*发送缓冲区*/
	socket[0].usSendMaxLen = MQTT_SEND_SIZE;						/*发送最大长度*/
	socket[0].pRecvBuf = _cloud_recv_buf;							/*接收缓冲区*/
	socket[0].usRecvMaxLen = MQTT_RECV_SIZE;
    memcpy(socket[1].IPAddr,Nvidia_IP,sizeof(uint8_t) * 4);     	 /* 将目的IP地址写入 */
	socket[1].pSend = _nvidia_buf;									/*发送缓冲区*/
	socket[1].usSendMaxLen = NVIDIA_SEND_SIZE;
	socket[1].pRecvBuf = _nvidia_recv_buf;
	socket[1].usRecvMaxLen = NVIDIA_RECV_SIZE;
#if (NET_DEBUG == 1)
	memset(&socket[2],0,sizeof(st_socket_info));
	//socket[2].DesPort = 1004;
	socket[2].SourPort = 5000;
	//socket[2].ProtoType = PROTO_TYPE_TCP; 
	//socket[2].TcpMode = TCP_CLIENT_MODE;
	//socket[2].TcpMode = TCP_SERVER_MODE;
	socket[2].ScokStatus = SOCKET_WAIT_SUCCESS;
	socket[2].Index = 2;
	socket[2].send = _Net_Send;
	socket[2].RemLen = 0;
	//memcpy(socket[2].IPAddr,Net_debug_IP,sizeof(uint8_t) * 4); 
	//socket[2].pSend = _debug_send_buf;
	socket[2].usSendMaxLen = 256;						/*发送最大长度*/
	socket[2].pRecvBuf = _debug_recv_buf;							/*接收缓冲区*/
	socket[2].usRecvMaxLen = 256;
	CH395SetSocketProtType((st_spi_info *)&net_spi, 2, PROTO_TYPE_TCP);
	CH395SetSocketSourPort((st_spi_info *)&net_spi, 2, 5000);
	while(CH395OpenSocket((st_spi_info *)&net_spi, 2) != CMD_ERR_SUCCESS)
		sys_delay_ms(20);
	while(CH395TCPListen((st_spi_info *)&net_spi, 2) != CMD_ERR_SUCCESS)
		sys_delay_ms(20);
#endif
}


static void _module_init(void)
{	//初始化网卡
	unsigned char ucSta;
	//uint16_t usID;

	gpio_dev_register(&gLed_front_dev);
	gpio_dev_register(&gLed_back_dev);
	gpio_dev_register(&_nvidia_dev);
	gpio_dev_register(&_camera_dev);
	//gpio_dev_register(&_sys_5v_dev);
	gpio_dev_register(&_sys_12v_dev);
	gpio_dev_register(&_sys_24v_dev);
	gpio_dev_register(&_routa_dev);
	gpio_dev_register(&_routb_dev);
	gpio_dev_register(&_eth_rst_dev);

	//pm_dev_register(&gLed_front_dev);
	//pm_dev_register(&gLed_back_dev);
	pm_dev_register(&_nvidia_dev);
	pm_dev_register(&_camera_dev);
	//pm_dev_register(&_sys_5v_dev);
	pm_dev_register(&_sys_12v_dev);
	pm_dev_register(&_sys_24v_dev);

	gulRegisterFlag &= ~BIT(1);
	
	while(1)
	{	//初始化CH395A
		if(CH395A_Init((st_spi_info *)&net_spi) == CMD_ERR_SUCCESS) break;

		sys_delay_ms(200);
	}

	while(1)
	{	//等待以太网连接成功
		ucSta = CH395CMDGetPHYStatus((st_spi_info *)&net_spi);
		if(ucSta != PHY_DISCONN)
			break;

		sys_delay_ms(200);
	}


	InitSocketParam();
	cfg_data_read((void *)&_mqtt_serno, CFG_CLOUD_PKTSER, sizeof(_mqtt_serno));
	gNvidiaDevInfo.ucNVIDIASta = 9;		//默认英伟达为未连接状态
	gNvidiaDevInfo.usErrcode = 1;
	set_cloud_push_flag(PROP_NVIDIA);
	//GPIO_SetBits(GPIOF, GPIO_Pin_1);
}

task_define(net, _module_init, _module_process, MODBUS_TASK_INTVERAL, 512, 4);
task_define(cloud, SYS_NOP, _cloud_process, MODBUS_TASK_INTVERAL, 2560, 4);
task_define(nvidia, SYS_NOP, _NVIDIA_process, MODBUS_TASK_INTVERAL, 2048, 4);

