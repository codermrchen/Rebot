#include <stdlib.h>
#include "sys_task.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"
#include "work_task.h"
#include "gas_task.h"
#include "sfud_def.h"

static spi_user_data spi_uart = TGT_SPI_UART_CFG;

const unsigned char gucSendBuf[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

static unsigned char _wk2204Exchange(unsigned char d )  
{  
    return BSP_spi_sendByte((st_spi_info *)&spi_uart, d);
}

/**********************WkWriteGlobalRegister*************
*@param greg:Global register 6-bit address
*@param dat:Writes the value of the global register
*@retval 
*/		
static void _WkWriteGlobalRegister(uint8_t ucGreg,uint8_t ucData)
{ 
	uint8_t ucCmd;
	ucCmd=0|ucGreg;
	taskENTER_CRITICAL();
	xWk2204CmdStart();
	_wk2204Exchange(ucCmd);
	_wk2204Exchange(ucData);
	xEndWk2204Cmd();
	taskEXIT_CRITICAL();
}

/********************** WkReadGlobalRegister()***********
**
*@param  greg:Global register 6-bit address
*@retval :Read the value of the global register
*/
static uint8_t _WkReadGlobalRegister(uint8_t ucGreg)
{
	uint8_t ucCmd, ucData;
	ucCmd=0x40|ucGreg;
	taskENTER_CRITICAL();
	xWk2204CmdStart();
	_wk2204Exchange(ucCmd);
	ucData = _wk2204Exchange(0xFF);
	xEndWk2204Cmd();
	taskEXIT_CRITICAL();
	return ucData;
}

/************************WkWriteSlaveRegister()*******************
**
* @param port:Uart port
* @param sreg:slave rigister 4-bit address 
* @param dat:Writes the value of the slave register
* @retval
**
*/
static void _WkWriteSlaveRegister(uint8_t ucPort,uint8_t ucSreg,uint8_t ucData)
{
	uint8_t ucCmd;
	ucCmd=0x0|((ucPort-1)<<4)|ucSreg;
	taskENTER_CRITICAL();
	xWk2204CmdStart();
	_wk2204Exchange(ucCmd);
	_wk2204Exchange(ucData);
	xEndWk2204Cmd();
	taskEXIT_CRITICAL();
}

/********************** WkReadSlaveRegister()***********
**
*@param  sreg:slave register 4-bit address
*@retval :Read the value of the slave register
**
*/
uint8_t WkReadSlaveRegister(uint8_t ucPort,uint8_t ucSreg)
{
	uint8_t ucCmd, ucData;
	ucCmd=0x40|((ucPort-1)<<4)|ucSreg;
	taskENTER_CRITICAL();
	xWk2204CmdStart();
	_wk2204Exchange(ucCmd);
	ucData = _wk2204Exchange(0xFF);
	xEndWk2204Cmd();
	taskEXIT_CRITICAL();
	return ucData;
}

/************************WkWriteSlaveFifo()*******************
**
* @param port:Uart port
* @param dat:pointer to transmission data buffer
* @param len:amount of data to be sent
* @retval
**
*/
void WkWriteSlaveFifo(uint8_t ucPort,uint8_t *pucSend,uint16_t usLen)
{
	uint8_t ucCmd;
	int i;
	ucCmd=0x80|((ucPort-1)<<4);
	taskENTER_CRITICAL();
	xWk2204CmdStart();
	_wk2204Exchange(ucCmd);
	for(i=0;i<usLen;i++)
		_wk2204Exchange(pucSend[i]);
	xEndWk2204Cmd();
	taskEXIT_CRITICAL();
}

/************************WkReadSlaveFifo()*******************
**
* @param port:Uart port
* @param rec:pointer to reception data buffer
* @param len:amount of data to be received
* @retval
**
*/
void WkReadSlaveFifo(uint8_t ucPort,uint8_t *pucRecv,int wLen)
{
	uint8_t ucCmd;
	int i;
	ucCmd=0xc0|((ucPort-1)<<4);
	taskENTER_CRITICAL();
	xWk2204CmdStart();
	_wk2204Exchange(ucCmd);
	for(i=0;i<wLen;i++)
		pucRecv[i] = _wk2204Exchange(0xFF);
	xEndWk2204Cmd();
	taskEXIT_CRITICAL();
}

void WkUartInit(uint8_t ucPort)
{
	uint8_t ucGena,ucGrst,ucGier,ucSier,ucScr;
	uint8_t ucBit = 1<<(ucPort-1);
	
	//使能子串口时钟
	ucGena=_WkReadGlobalRegister(WK2204_GENA);
	ucGena=ucGena|ucBit;
	_WkWriteGlobalRegister(WK2204_GENA,ucGena);
	while((_WkReadGlobalRegister(WK2204_GENA) & ucBit) == 0)
		sys_delay_ms(5);
	
	//软件复位子串口
	ucGrst=_WkReadGlobalRegister(WK2204_GRST);
	ucGrst=ucGrst|ucBit;
	_WkWriteGlobalRegister(WK2204_GRST,ucGrst);
	
	//使能串口总中断
	ucGier=_WkReadGlobalRegister(WK2204_GIER);
	ucGier=ucGier|ucBit;
	_WkWriteGlobalRegister(WK2204_GIER,ucGier);
	while((_WkReadGlobalRegister(WK2204_GIER) & ucBit) == 0)
		sys_delay_ms(5);
	
	//使能子串口接收触点中断和超时中断
	ucSier=WkReadSlaveRegister(ucPort,WK2204_SIER); 
	ucSier |= WK2204_RFTRIG_IEN|WK2204_RXOUT_IEN;
	_WkWriteSlaveRegister(ucPort,WK2204_SIER,ucSier);
//	ucSier=WkReadSlaveRegister(ucPort,WK2204_SIER); 
//	printf("SIER:%x\r\n", ucSier);
	
	//初始化FIFO和设置固定中断触点
	_WkWriteSlaveRegister(ucPort,WK2204_FCR,0XFF);  
//	ucSier=WkReadSlaveRegister(ucPort,WK2204_FCR); 
//	printf("SIER:%x\r\n", ucSier);
	//设置任意中断触点，如果下面的设置有效，
	//那么上面FCR寄存器中断的固定中断触点将失效
	_WkWriteSlaveRegister(ucPort,WK2204_SPAGE,1);//切换到page1
	_WkWriteSlaveRegister(ucPort,WK2204_RFTL,0X40);//设置接收触点为64个字节
	_WkWriteSlaveRegister(ucPort,WK2204_TFTL,0X10);//设置发送触点为16个字节
	_WkWriteSlaveRegister(ucPort,WK2204_SPAGE,0);//切换到page0 
//	ucSier=WkReadSlaveRegister(ucPort,WK2204_RFTL); 
//	printf("RFTL:%x\r\n", ucSier);
//	ucSier=WkReadSlaveRegister(ucPort,WK2204_TFTL); 
//	printf("TFTL:%x\r\n", ucSier);
	//使能子串口的发送和接收使能
	ucScr=WkReadSlaveRegister(ucPort,WK2204_SCR); 
	ucScr|=WK2204_RXEN|WK2204_TXEN;
	_WkWriteSlaveRegister(ucPort,WK2204_SCR,ucScr);
//	ucSier=WkReadSlaveRegister(ucPort,WK2204_SCR); 
//	printf("SCR:%x\r\n", ucSier);
}

/**************************Wk_SetBaud*******************************************************/
//函数功能：设置子串口波特率函数、此函数中波特率的匹配值是根据11.0592Mhz下的外部晶振计算的
// port:子串口号
// baud:波特率大小.波特率表示方式，
/**************************Wk2114SetBaud*******************************************************/
uint8_t WkUartSetBaud(uint8_t ucPort,uint32_t ulBaudrate)
{  
	uint32_t ulTemp,ulFreq;
	uint8_t ucScr;
	uint8_t ucBaud1,ucBaud0,ucPres;
	ulFreq=11059200;/*芯片外部时钟频率*/
	if(ulFreq>=(ulBaudrate*16))
	{
		ulTemp=(ulFreq)/(ulBaudrate*16);
		ulTemp=ulTemp-1;
		ucBaud1=(uint8_t)((ulTemp>>8)&0xff);
		ucBaud0=(uint8_t)(ulTemp&0xff);
		ulTemp=(((ulFreq%(ulBaudrate*16))*100)/(ulBaudrate));
		ucPres=(ulTemp+100/2)/100;
		//关掉子串口收发使能
		ucScr=WkReadSlaveRegister(ucPort,WK2204_SCR); 
		_WkWriteSlaveRegister(ucPort,WK2204_SCR,0);
		//设置波特率相关寄存器
		_WkWriteSlaveRegister(ucPort,WK2204_SPAGE,1);//切换到page1
		_WkWriteSlaveRegister(ucPort,WK2204_BAUD1,ucBaud1);
		_WkWriteSlaveRegister(ucPort,WK2204_BAUD0,ucBaud0);
		_WkWriteSlaveRegister(ucPort,WK2204_PRES,ucPres);
//		ucPres=WkReadSlaveRegister(ucPort,WK2204_BAUD1); 
//		printf("BAUD1:%x\r\n", ucPres);
//		ucPres=WkReadSlaveRegister(ucPort,WK2204_BAUD0); 
//		printf("BAUD0:%x\r\n", ucPres);
//		ucPres=WkReadSlaveRegister(ucPort,WK2204_PRES); 
//		printf("PRES:%x\r\n", ucPres);
		_WkWriteSlaveRegister(ucPort,WK2204_SPAGE,0);//切换到page0 
		//使能子串口收发使能
		_WkWriteSlaveRegister(ucPort,WK2204_SCR,ucScr);
//		ucPres=WkReadSlaveRegister(ucPort,WK2204_SCR); 
//		printf("SCR:%x\r\n", ucPres);
	}
	else
		return 1;
	return 0;
}

int WkUartRxChars(uint8_t port, uint8_t *recbuf)
{
	uint8_t ucFsr=0,ucRfcnt=0,ucRfcnt2=0,ucSifr=0;
  	int wLen=0;
	ucSifr=WkReadSlaveRegister(port, WK2204_SIFR);
	
	if((ucSifr&WK2204_RFTRIG_INT)||(ucSifr&WK2204_RXOVT_INT))//有接收中断和接收超时中断
	{ 
		ucFsr =WkReadSlaveRegister(port,WK2204_FSR);
		if (ucFsr& WK2204_RDAT){
			ucRfcnt=WkReadSlaveRegister(port,WK2204_RFCNT);
			if(ucRfcnt==0){
				ucRfcnt=WkReadSlaveRegister(port,WK2204_RFCNT);
			}
			ucRfcnt2=WkReadSlaveRegister(port,WK2204_RFCNT);
			if(ucRfcnt2==0){
				ucRfcnt2=WkReadSlaveRegister(port,WK2204_RFCNT);
			}
			/*判断fifo中数据个数*/
			ucRfcnt=(ucRfcnt2>=ucRfcnt)?ucRfcnt:ucRfcnt2;
			wLen=(ucRfcnt==0)?50:ucRfcnt;	
#if 1
			WkReadSlaveFifo(port,recbuf,wLen);
#else
		for(n=0;n<len;n++)
		*(recbuf+n)=WkReadSlaveRegister(port,WK2XXX_FDAT_REG);
#endif	
			return wLen;
		}
		else{
			return wLen;
		}
	}
	return wLen;
}

void test_hex_out(uint8_t * pucBuf, int  wLen)
{
	if(wLen <= 0)
		return;
	for(int i = 0; i < wLen; i++)
	{
		debug_printf("%02X ", pucBuf[i]);
	}
	debug_printf("\r\n");
}

unsigned char gas_check(uint8_t * pucBuf, int wLen)
{
	unsigned char ucCheck = 0;
	
	for(int i = 0; i < (wLen - 1); i++)
		ucCheck += pucBuf[i];
	ucCheck = 0xFF - ucCheck;
	if(ucCheck == pucBuf[wLen - 1])
		return 1;
	return 0;
}

static void _gas_process(void)
{
	uint8_t ucGifr;
	static unsigned long sulTick;
	static unsigned char sucGasStatus = 0, sucTryCount = 0;
	uint8_t pucRxBuff[50] ={0};
	int wDataLen = 0;

	if(IS_WAKE_UP_MODE())
	{
		if(gGasInfo.ucGasSta == 1)
		{
			gGasInfo.ucGasSta = 0;
			set_cloud_push_flag(PROP_GAS);
		}
		if(sucGasStatus == 0)
		{
			if(sys_over_time(sulTick, 60000))	//1分钟检测一次
			{
				WkWriteSlaveFifo(GAS_PORT, (uint8_t *)gucSendBuf,sizeof(gucSendBuf));
				sulTick = sys_get_ms();
				sucGasStatus = 1;
			}
		}
		else if(sucGasStatus == 1)
		{
			ucGifr = _WkReadGlobalRegister(WK2204_GIFR);
			if(ucGifr)
				wDataLen=WkUartRxChars(GAS_PORT,pucRxBuff);
			if((wDataLen == 11) && gas_check(pucRxBuff, wDataLen))
			{
				sucTryCount = 0;
				sulTick = sys_get_ms();
				sucGasStatus = 0;
				gGasInfo.usCOPpm = pucRxBuff[2] * 256 + pucRxBuff[3];
				gGasInfo.usH2SPpm = pucRxBuff[4] * 256 + pucRxBuff[5];
				gGasInfo.usO2Vol = pucRxBuff[6] * 256 + pucRxBuff[7];
				gGasInfo.usCH4LEL = pucRxBuff[8] * 256 + pucRxBuff[9];
				gGasInfo.ucGasSta = 0;
				//debug_printf("CO:%u H2S:%u O2:%u CH4:%u\r\n", gGasInfo.usCOPpm, gGasInfo.usH2SPpm, gGasInfo.usO2Vol, gGasInfo.usCH4LEL);
				set_cloud_push_flag(PROP_GAS);
			}
			if((sucGasStatus == 1) && sys_over_time(sulTick, 1000))
			{
				NUM_INCREASE(&sucTryCount, 5);
				if(sucTryCount < 3)
					WkWriteSlaveFifo(GAS_PORT, (uint8_t *)gucSendBuf,sizeof(gucSendBuf));
				else
				{
					if(gGasInfo.ucGasSta != 9)
					{
						gGasInfo.ucGasSta = 9;
						set_cloud_push_flag(PROP_GAS);
					}
					sucGasStatus = 0;
					sucTryCount = 0;
				}
				sulTick = sys_get_ms();
			}
		}
	}
	else
	{
		if(gGasInfo.ucGasSta != 1)
		{
			gGasInfo.ucGasSta = 1;
			set_cloud_push_flag(PROP_GAS);
			sucGasStatus = 0;
			sucTryCount = 0;
		}
	}
}

static void _wk2204_init(void)
{	//初始化wk2204
	sys_delay_ms(10000);

	WkUartInit(GAS_PORT);
	WkUartSetBaud(GAS_PORT, 9600);
}


task_define(wk2204, _wk2204_init, _gas_process, 50, 256, 4);


