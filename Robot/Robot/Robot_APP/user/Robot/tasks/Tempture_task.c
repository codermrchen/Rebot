#include <stdlib.h>
#include "sys_task.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"
#include "work_task.h"
#include "ultras_task.h"

static unsigned long sulTempTick;
static unsigned char sucTempCount = 0;


/* Private function prototypes -----------------------------------------------*/
static int _temp_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num);


static st_MODBUS_packet _temp_data_pkt = {
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
        .dev_rsp_proc = _temp_rsp_proc,
        .rsp = { .timeout  = 1000 },
    };

//static unsigned char sucCount[4] = {0};
//static unsigned char sucError[4] = {0};

/* Private functions ---------------------------------------------------------*/
/*
 * @brief       数据发送处理
 * @retval      none
 */
static int _read_temp(void)
{
    uint8_t tmp_buf[16] = {0};

	tmp_buf[0] = 0x01;	
	tmp_buf[1] = 0x03;
	tmp_buf[2] = 0x00;
	tmp_buf[3] = 0x00;
	tmp_buf[4] = 0x00;
	tmp_buf[5] = 0x02;
	
    if (0 <  MODBUS_send(&_temp_data_pkt.dev_info, tmp_buf, 8)) {
        _temp_data_pkt.dev_info.state |= DEV_STATE_BUSY;
        _temp_data_pkt.rsp.snd_tick = sys_get_ms();
		_temp_data_pkt.rsp.fun_code = 1;
    }
	_temp_data_pkt.dev_info.id = 0x01;

    return 6;
}

//static int _read_ultras_distance(uint8_t ucAddr)
//{
//    uint8_t puctBuf[10];

//	puctBuf[0] = ucAddr;	
//	puctBuf[1] = 0x03;
//	puctBuf[2] = 0x00;
//	puctBuf[3] = 0x10;
//	puctBuf[4] = 0x00;
//	puctBuf[5] = 0x01;
//	
//    if (0 <  MODBUS_send(&_temp_data_pkt.dev_info, puctBuf, 8)) {
//        _temp_data_pkt.dev_info.state |= DEV_STATE_BUSY;
//        _temp_data_pkt.rsp.snd_tick = sys_get_ms();
//		_temp_data_pkt.rsp.fun_code = ucAddr;
//    }
//	_temp_data_pkt.dev_info.id = ucAddr;

//    return 6;
//}


static int _temp_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
	unsigned short usData;
	//unsigned char ucIndex;
	//static unsigned long sulTick;
	//static unsigned short susLastDistance[4] = {4000};

	if(func == MODBUS_TIMEOUT_CODE) return 1;
	
    _temp_data_pkt.dev_info.state &= ~DEV_STATE_BUSY;

	if(_temp_data_pkt.dev_info.id == 0x01)
	{	//温湿度
		sucTempCount = 0;
		if(gucTempSta == 9)
		{
			gucTempSta = 0;
			set_cloud_push_flag(PROP_TEMP);
		}
		sulTempTick = sys_get_ms();
		pdata++;
		usData = *pdata;
		pdata++;
		usData = (usData << 8) | *pdata;
		gusHumiValue = usData;
		pdata++;
		usData = *pdata;
		pdata++;
		usData = (usData << 8) | *pdata;
		gusTempValue = usData;
		set_cloud_push_flag(PROP_TEMP);
	}
//	else
//	{
//		ucIndex = _temp_data_pkt.dev_info.id - 2;
//		sucCount[ucIndex] = 0;
//		pdata++;
//		susLastDistance[ucIndex] = 0;
//		susLastDistance[ucIndex] |= *pdata++;
//		susLastDistance[ucIndex] = (susLastDistance[ucIndex] << 8) | *pdata++;
//		if((susLastDistance[ucIndex] < 5) ||(susLastDistance[ucIndex] > 4000))
//			susLastDistance[ucIndex] = 4000;
//		gusFrontDis = susLastDistance[0] < susLastDistance[1] ? susLastDistance[0] : susLastDistance[1];
//		gusBackDis = susLastDistance[2] < susLastDistance[3] ? susLastDistance[2] : susLastDistance[3];
//		if(((sys_get_ms() - sulTick) > 3000) || (gusFrontDis < 500) || (gusBackDis < 500))
//		{
//			debug_printf("front:%u %u back:%u %u\r\n", susLastDistance[0],susLastDistance[1], susLastDistance[2], susLastDistance[3]);
//			sulTick = sys_get_ms();
//		}
//	}

    return 0;
}

static void _module_process(void)
{	
	//static unsigned char sucIndex = 1;

	if(IS_WAKE_UP_MODE())
	{
		if(gucTempSta == 1)
		{
			gucTempSta = 0;
			set_cloud_push_flag(PROP_TEMP);
		}
		if((_temp_data_pkt.dev_info.state & DEV_STATE_WAITRSP) == 0)
		{
			if(sys_over_time(sulTempTick, 60 * 1000))
			{
				if(_read_temp() > 0)
				{
					NUM_INCREASE(&sucTempCount, 10);
				}
				if(sucTempCount >= 5)
				{
					if(gucTempSta == 0)
					{
						gucTempSta = 9;
						set_cloud_push_flag(PROP_TEMP);
					}
					sulTempTick = sys_get_ms();
				}
			}
//			else
//			{
//				_read_ultras_distance(sucIndex + 2);
//				if(sucCount[sucIndex] < 15)
//					sucCount[sucIndex]++;
//				if(sucCount[sucIndex] >= 10)
//				{
//					sucError[sucIndex] = 1;
//					if((sucIndex == 0 && sucError[1] == 1) || (sucIndex == 1 && sucError[0] == 1))
//					{
//						gusFrontDis = 4000;
//						if(IS_WALK_FORWARD() && IS_Auto_Mode())
//							work_stop();
//						if((gUltrasDevInfo.ucUltrasSta1 != 9) ||(gUltrasDevInfo.ucUltrasErrCode1 != 2))
//						{
//							gUltrasDevInfo.ucUltrasSta1 = 9;
//							gUltrasDevInfo.ucUltrasErrCode1 = 2;
//						}
//					}
//					if((sucIndex == 2 && sucError[3] == 1) || (sucIndex == 3 && sucError[2] == 1))
//					{
//						gusBackDis = 4000;
//						if(IS_WALK_BACKWORAD() && IS_Auto_Mode())
//							work_stop();
//						if((gUltrasDevInfo.ucUltrasSta2 != 9) ||(gUltrasDevInfo.ucUltrasErrCode2 != 2))
//						{
//							gUltrasDevInfo.ucUltrasSta2 = 9;
//							gUltrasDevInfo.ucUltrasErrCode2 = 2;
//						}
//					}
//				}
//				else {
//					if(sucIndex <= 1)
//					{
//						if((gUltrasDevInfo.ucUltrasSta1 == 9) && (gUltrasDevInfo.ucUltrasErrCode1 == 2))
//						{
//							gUltrasDevInfo.ucUltrasSta1 = 0;
//							gUltrasDevInfo.ucUltrasErrCode1 = 0;
//						}
//					}
//					else
//					{
//						if((gUltrasDevInfo.ucUltrasSta2 == 9) && (gUltrasDevInfo.ucUltrasErrCode2 == 2))
//						{
//							gUltrasDevInfo.ucUltrasSta2 = 0;
//							gUltrasDevInfo.ucUltrasErrCode2 = 0;
//						}
//					}
//				}
//				sucIndex = (sucIndex < 3) ? (sucIndex + 1) : 0;
//			}
		}
	}
	else
	{
		if(gucTempSta != 1)
		{
			gucTempSta = 1;
			set_cloud_push_flag(PROP_TEMP);
		}
	}
    MODBUS_process(&_temp_data_pkt);
	
	sys_delay_ms(10);
}

uint16_t get_front_distance(void)
{
	return gusFrontDis;
}

uint16_t get_back_distance(void)
{
	return gusBackDis;
}


static void _module_init(void)
{
#ifdef TGT_TEMP_CFG
    static unsigned char   _temp_rxbuf[DEV_RXBUF_SIZE];
    static st_uart_info    _temp_port = TGT_TEMP_CFG;

    if (0 <= BSP_uart_init(&_temp_port, _temp_rxbuf, sizeof(_temp_rxbuf))) {
        //ring_buf_init(&_temp_port.tx_buf, NULL, 0);
        _temp_data_pkt.dev_info.pobj = &_temp_port;
    }
#endif
}

task_define(temp, _module_init, _module_process, 0, 256, 9);

