/******************************************************************************
 * @brief    平台相关初始化(带低功耗管理版本)
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
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
#include "lfs.h"
#include "config.h"
#include "sfud.h"

static work_async_t _plat_workqueue;  /* 异步作业*/
static work_node_t _plat_worknode[8]; /* 作业队列结点*/

/***********************************************************************************
********************全局变量，用于耦合各任务***********************************
***********************************************************************************/
uint32_t gulDevID = 0x01;	//设备ID
Walk_Para WalkMotor = {0};
uint16_t _ultra_adc[TGT_ULTRA_ADC_CHNS];
unsigned char gucADCGetFinish = 0;	//超声波采集完成
unsigned char * gpucTest;
Work_info gWorkWorker;				//工作点
unsigned char gucNVIDIAFlag = 1;	//英伟达控制状态
unsigned short gusTempValue = 250;
unsigned short gusHumiValue = 500;
unsigned char gucTempSta = 0;
unsigned char gucRequeEndFlag = 0;
unsigned char gucIsHome = 0;		//设备是否已经复位
unsigned char gucPosLRFlag = 2;		//0:左边 1:右边 2:未知
unsigned long gulHomeTick;
Bat_Info gBatDevInfo;
Lift_Info gLiftDevInfo;
Walk_Info gWalkDevInfo;
NVIDIA_Info gNvidiaDevInfo;
Led_Info gLedDevInfo;
Ultras_Info gUltrasDevInfo;
Wire_Info gWireDevInfo;
Rotbot_Info gRotbotInfo;
Block_Info gBlockDevInfo;
Fan_Info gFanDevInfo;
Rfid_Dev_Info gRfidDevMachine;
Rfid_Info gRfidDevInfo;
Detect_Info gDetectDevInfo;
line_Info gLineDevInfo;
unsigned long gulCloudPushFlag = 0;	//云平台上报标志
unsigned long gulRegisterFlag = 0x0000003F;
Time_info gCurrentTime = { \
	.Year = 25,	\
	.Month = 05,	\
	.Day = 06,	\
	.Hour = 16,	\
	.Minute = 0,	\
	.Second = 0		\
	};
struct lfs_config * gLfsDevice;
unsigned char gucBlockTemp[LFS_BLOCK_SIZE];
unsigned char gucReflashWork = 0;
unsigned char gucChargeType = 0;	//优先
unsigned char gucWorkBack = 0;
unsigned long gulBlockTick;
unsigned char gucNextRfid = 0;
unsigned char gucCurRfid = 0;
unsigned long gulPauseTime;
unsigned char gucTimeGetFlag = 0;
unsigned long gulCloudAliveTick = 0;

unsigned short gusPacketID = 0;

Sys_Para gSysParameter;
unsigned char gucHomeFlag = 0;	//复位标志 0:未复位 1:已复位
int glWalkCurPos = 0;
unsigned char gucFlashInitFinish = 0;
unsigned char gucIsCharge = 0;	//充电状态
unsigned char gucStartCharge = 0;
unsigned long gulChargeTick;	//开始充电时间戳
unsigned long gulUnWorkTick;	//非自动模式下时间
unsigned char gucPmMode = 0;		//0:自动进入休眠模式 1:手动进入休眠模式
unsigned char gucChargeSta = 0;		//0:未充电 1:等待充电 2:正在充电
//unsigned char gpucCloudData[MQTT_RECV_SIZE] = {0};
//unsigned char gucMqttBackupFlag = 0;
unsigned long gulCloudBackLen = 0;
unsigned char gucMqttPinqTry = 0;		//心跳包异常3次后再断开连接
Plan_Info gPlanDev;
unsigned long gulCurrentTick;
unsigned char gucCurrentError = 0;
unsigned char gucConnetFLag = 0;
unsigned long gulTimeTick;
unsigned short gusFrontDis = 2500;
unsigned short gusBackDis = 2500;
unsigned char gucFrontCtrTime = 0;	//手动/部署模式下前方手动前进次数
unsigned char gucBackCtrTime = 0;
unsigned char gucTest = 0;
unsigned char gucDetectEn = 0;		//测距检测使能 0:关闭 1:打开
unsigned long gulDetectTick;
unsigned char gucStopEnFlag = 0;	//BIT0: 是否禁止停止	BIT1:是否存在停止操作 BIT2:是否存在停止巡检操作
Gas_Info gGasInfo = {0};
unsigned char gucRfidTest = 0;
OTA_Status gOTAFlag = {0};
os_task_t gTaskHandle[20] = {0};
unsigned char gucPrintFlag = 0;
unsigned char gucUpdateBuf[256] = {0};
unsigned char gucManualStop = 0;	//人工停止标志，手动停下后两分钟内不允许自动巡检
unsigned long gulManualStopTick;
LIFT_CONTL gLiftControl = {0};
//unsigned long gulDebugSelect = 0xFFFFFFFF;

static void _led_front_pm_before(void);
static void _led_back_pm_before(void);

st_blink_dev gLed_front_dev = {		\
		.gpio = TGT_LIGHT_BKD,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = DISABLE,			\
		.pm_doing_befor = _led_front_pm_before,	\
	};

st_blink_dev gLed_back_dev = { 		\
		.gpio = TGT_LIGHT_FWD,			\
		.BlinkSta = DISABLE,			\
		.NeedAble = DISABLE,			\
		.pm_doing_befor = _led_back_pm_before,	\
	};

static void _led_front_pm_before(void)
{
	gLed_front_dev.ReadyToPm = 0;
}

static void _led_back_pm_before(void)
{
	gLed_back_dev.ReadyToPm = 0;
}

void read_sys_data(void)
{
	Flash_read_data(SYS_DATA_SECTOR, 0, &gSysParameter, sizeof(Sys_Para));
}


void write_sys_data(void)
{
	Flash_write_data(SYS_DATA_SECTOR, 0, &gSysParameter, sizeof(Sys_Para));
}

uint32_t get_time_second_count(Time_info TimeSrc, Time_info TimeDst)
{
	uint32_t ulSecondCount = 0xFFFFFFFF;
	uint8_t ucSecond, ucMinute, ucHour, ucTemp, ucFlag = 0;
	
	if(TimeDst.Hour < TimeSrc.Hour)
		return ulSecondCount;
	if((TimeDst.Hour == TimeSrc.Hour) && (TimeDst.Minute < TimeSrc.Minute))
		return ulSecondCount;
	if((TimeDst.Hour == TimeSrc.Hour) && (TimeDst.Minute == TimeSrc.Minute) && (TimeDst.Second < TimeSrc.Second))
		return ulSecondCount;

	if(TimeSrc.Second > TimeDst.Second)
	{
		ucSecond = TimeDst.Second + 60 - TimeSrc.Second;
		if(TimeDst.Minute == 0)
		{
			ucTemp = 59;
			ucFlag = 1;
		}
		else
			ucTemp = TimeDst.Minute - 1;
	}
	else
	{
		ucSecond = TimeDst.Second - TimeSrc.Second;
		ucTemp = TimeDst.Minute;
	}
	if(ucTemp >= TimeSrc.Minute)
	{
		ucMinute = ucTemp - TimeSrc.Minute;
		if(ucFlag)
		{
			if(TimeDst.Hour == 0)
				ucTemp = 23;
			else
				ucTemp = TimeDst.Hour - 1;
		}
		else
			ucTemp = TimeDst.Hour;
	}
	else
	{
		ucMinute = ucTemp + 60 - TimeSrc.Minute;
		if(TimeDst.Hour == 0)
			ucTemp = 23;
		else
			ucTemp = TimeDst.Hour - 1;
		if(ucFlag)
		{
			if(ucTemp == 0)
				ucTemp = 23;
			else
				ucTemp = ucTemp - 1;
		}
	}
	ucHour = ucTemp - TimeSrc.Hour;
	ulSecondCount = ucHour * 3600 + ucMinute * 60 + ucSecond;
	return ulSecondCount;
}

uint8_t last_need_to_detect(uint8_t ucIndex)
{	//判断当前位置的前方是否还有需要检测的点位,有则返回1，否则返回0
	uint32_t ulCheckPos = gWorkWorker.CheckPos;

	ulCheckPos &= ~0x0000001F;	//清除前面无关bit
	if(ulCheckPos & ((0x00000001 << (ucIndex - 1)) - 1))
		return 1;
	return 0;
}

uint8_t next_need_to_detect(uint8_t ucIndex)
{	//判断后方是否仍有需要检测的点位
	uint32_t ulCheckPos = gWorkWorker.CheckPos;
	ulCheckPos &= ~0x0000001F;	//清除前面无关bit
	ulCheckPos &= ~((0x00000001 << (ucIndex - 1)) - 1);
	if(ulCheckPos)
		return 1;
	return 0;
}

/*
	20250320 YHT 1.修复斜坡定点功能 
				2.完成设备定距行走
				3.优化任务调度延时
*/

#ifdef LOWPOWER_MODE

/*
 * @brief	   功耗管理任务
 */
//task_define(pm, SYS_NOP, sys_pm_process, 1000, 128, 1);
//pm_dev_register("sys", BSP_sys_isIdle, NULL, NULL);
#endif

//#undef TGT_RTC_CFG
#ifdef TGT_RTC_CFG

#elif !defined(SYS_OS_TYPE) && !defined(TIM_ENABLE)
/*
 * @brief	   系统滴答中断
 * @param[in]   none
 * @return 	   none
 */
void SysTick_Handler(void)
{
    BSP_systick_inc();
}
#endif

void set_cloud_push_flag(uint8_t num)
{	
	gulCloudPushFlag |= (BIT(0) << num);
}

uint32_t get_plan_id(uint8_t * pucBuff)
{
	uint32_t ulIdNum = 0;
	
	for(uint8_t ucIndex = 0; ucIndex < 6; ucIndex++)
	{
		ulIdNum = ulIdNum * 10 + pucBuff[ucIndex] - '0';
	}
	return ulIdNum;
}

void push_plan_status(uint8_t * pucBuff, uint8_t uctype, uint16_t usErr, uint16_t usTaskID)
{
	if(gucConnetFLag == 0) return;
	gPlanDev.ulPlanID = get_plan_id(pucBuff);
	gPlanDev.ucPlanSta = uctype;
	gPlanDev.usPlanErrCode = usErr;
	gPlanDev.usTaskID = usTaskID;
	set_cloud_push_flag(PROP_PLAN);
}

/*
 * @brief	   硬件驱动初始化
 * @param[in]   none
 * @return 	    none
 */
static void bsp_init(void)
{
    uint8_t index;

#ifdef TGT_RCC_CFG
    BSP_rcc_cfg(TGT_RCC_CFG);
#else
    SystemCoreClockUpdate();
#endif
	__disable_irq();
	SCB->VTOR = APP_ADDRESS;
	__enable_irq();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

#if defined(LOWPOWER_MODE)
    RCC_APB2PeriphClockLPModeCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    BSP_rtc_cfg();
    BSP_rtc_wkupcfg(1000/BSP_TICK_INTERVAL);
	
#elif !defined(SYS_OS_TYPE)
 	if (SysTick_Config(SystemCoreClock / (1000 / BSP_TICK_INTERVAL))) {
        while(1);
 	}
#endif
	ADC_ULTRAS_Init();

#ifdef TGT_WDOG_CFG
    BSP_wdog_cfg(TGT_WDOG_CFG);
#endif

#ifdef TGT_PINS_CFG
    st_DEV_cfg pinCfg[] = TGT_PINS_CFG;

    index = OBJ_ARRAY_NUM(pinCfg);
    while (0 < index--) {
        BSP_gpio_cfg(&pinCfg[index]);
    }
	GPIO_SetBits(GPIOH, GPIO_Pin_5 | GPIO_Pin_8);
	GPIO_ResetBits(GPIOH, GPIO_Pin_4 | GPIO_Pin_7);
#endif

#ifdef TGT_UART_CFG
    st_DEV_cfg uartCfg[] = TGT_UART_CFG;

    index = OBJ_ARRAY_NUM(uartCfg);
    while (0 < index--) {
        BSP_uart_cfg(&uartCfg[index]);
    }
#endif
	//uart_dma_init(UART8, DMA_Channel_0, DMA1_Stream0);

#ifdef TGT_SPI_CFG
    st_DEV_cfg spiCfg[] = TGT_SPI_CFG;

    index = OBJ_ARRAY_NUM(spiCfg);
    while (0 < index--) {
        BSP_spi_cfg(NULL, &spiCfg[index]);
    }
#endif

#ifdef TGT_I2C_CFG
    st_DEV_cfg i2cCfg[] = TGT_I2C_CFG;

    index = OBJ_ARRAY_NUM(i2cCfg);
    while (0 < index--) {
        BSP_i2c_cfg(NULL, &i2cCfg[index], 0);
    }
#endif

  	//ADC_ULTRAS_Init();

#ifdef TGT_EXTI_CFG
    uint32_t extiCfg[] = TGT_EXTI_CFG;

    index = OBJ_ARRAY_NUM(extiCfg);
    while (0 < index--) {
        BSP_exti_cfg(extiCfg[index], ENABLE);
    }
#endif

#ifdef TGT_INTI_CFG
    uint32_t intiCfg[] = TGT_INTI_CFG;

    index = OBJ_ARRAY_NUM(intiCfg);
    while (0 < index--) {
        BSP_inti_cfg(intiCfg[index], ENABLE);
    }
#endif

    //初始化异步作业
    async_work_init(&_plat_workqueue, _plat_worknode, OBJ_ARRAY_NUM(_plat_worknode));
    sys_init_set(1);
}

static void idle_process(void)
{
    async_work_process(NULL);
}

system_init("bsp", bsp_init);
task_define(idle, SYS_NOP, idle_process, 1000, 256, 1);

uint32_t GetSector(uint32_t Address)
{	//返回相应扇区
    uint32_t sector = 0;

    if ((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
        sector = FLASH_Sector_0;
    else if ((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
        sector = FLASH_Sector_1;
    else if ((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
        sector = FLASH_Sector_2;
    else if ((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
        sector = FLASH_Sector_3;
    else if ((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
        sector = FLASH_Sector_4;
    else if ((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
        sector = FLASH_Sector_5;
    else if ((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
        sector = FLASH_Sector_6;
    else if ((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
        sector = FLASH_Sector_7;
    else if ((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
        sector = FLASH_Sector_8;
    else if ((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
        sector = FLASH_Sector_9;
    else if ((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
        sector = FLASH_Sector_10;
    else if ((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11))
        sector = FLASH_Sector_11;
    else if ((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12))
        sector = FLASH_Sector_12;
    else if ((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13))
        sector = FLASH_Sector_13;
    else if ((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14))
        sector = FLASH_Sector_14;
    else if ((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15))
        sector = FLASH_Sector_15;
    else if ((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16))
        sector = FLASH_Sector_16;
    else if ((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17))
        sector = FLASH_Sector_17;
    else if ((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18))
        sector = FLASH_Sector_18;
    else if ((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19))
        sector = FLASH_Sector_19;
    else if ((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20))
        sector = FLASH_Sector_20;
    else if ((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21))
        sector = FLASH_Sector_21;
    else if ((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22))
        sector = FLASH_Sector_22;
    else if ((Address < ADDR_FLASH_SECTOR_23 + 0x20000) && (Address >= ADDR_FLASH_SECTOR_23))
        sector = FLASH_Sector_23;
    else
        sector = 0xFFFFFFFF;  

    return sector;
}

FLASH_Status FLASH_EraseApp2(void)
{
    uint16_t secStart = GetSector(APP2_ADDR);
    uint16_t secEnd = GetSector(APP2_ADDR - APP1_ADDR + APP2_ADDR - 1);
    FLASH_Unlock();
    for (uint16_t i = (secStart >> 3); i <= (secEnd >> 3); i++) {
        if (FLASH_EraseSector((i << 3), VoltageRange_3) != FLASH_COMPLETE)
            return FLASH_ERROR_PGP;
    }
    FLASH_Lock();
    return FLASH_COMPLETE;
}

FLASH_Status FLASH_WriteWords(uint32_t addr, uint32_t *buf, uint32_t wordCnt)
{
    FLASH_Unlock();
    for (uint32_t i = 0; i < wordCnt; i++) {
        if (FLASH_ProgramWord(addr + i * 4, buf[i]) != FLASH_COMPLETE)
            return FLASH_ERROR_PGP;
    }
    FLASH_Lock();
    return FLASH_COMPLETE;
}

void app_request_upgrade(void)
{
    FLASH_Unlock();
    FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
    FLASH_ProgramWord(FLAG_ADDR, UPDATE_FLAG);
    FLASH_Lock();
    NVIC_SystemReset();   // 重启即进入搬运流程
}

uint32_t flash_read_word(uint32_t addr)
{
    return *(__IO uint32_t *)addr;          /* 强制转换即可 */
}

/* 读取任意长度（以字为单位） */
void flash_read_buf(uint32_t addr, uint32_t *buf, uint32_t word_len)
{
    for (uint32_t i = 0; i < word_len; i++) {
        buf[i] = *(__IO uint32_t *)addr;
        addr += 4;
    }
}

/*
 * @brief    wdog任务(1ms 轮询1次)
 */
#ifdef TGT_WDOG_CFG
static void wdg_process(void)
{
    IWDG_ReloadCounter();
}


task_define(wdog, SYS_NOP, wdg_process, TGT_SLEEP_MAXTIME, 64, 10);
#endif

