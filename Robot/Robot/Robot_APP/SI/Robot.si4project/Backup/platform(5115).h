/******************************************************************************
 * @brief    设备相关操作
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 *
 ******************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "config.h"
#include "btn.h"
#include "led.h"
#include "bsp.h"
#include "blink.h"
#include "os_port.h"
#include "stm32f4xx_flash.h"

#if defined(__CC_ARM)
#pragma diag_suppress       1296
#elif defined(__ICCARM__)
#elif defined(__GNUC__)
#endif

#define LOWPOWER_MODE
#ifdef LOWPOWER_MODE
#define TGT_SYSTICK_INT
#else
#define TGT_SYSTICK_INT    (INT_CHN(SysTick_IRQn)| INT_MAINPRI(0) | INT_SUBPRI(0))
#endif

extern uint32_t gulDevID;
extern Walk_Para WalkMotor;
extern uint16_t _ultra_adc[];
extern unsigned char gucADCGetFinish;
extern unsigned char * gpucTest;
extern Work_info gWorkWorker;
extern unsigned char gucNVIDIAFlag;
extern unsigned short gusTempValue;
extern unsigned short gusHumiValue;
extern unsigned char gucTempSta;
extern unsigned char gucRequeEndFlag;
extern unsigned char gucIsHome;
extern unsigned char gucPosLRFlag;
extern unsigned long gulHomeTick;
extern Bat_Info gBatDevInfo;
extern Lift_Info gLiftDevInfo;
extern Walk_Info gWalkDevInfo;
extern NVIDIA_Info gNvidiaDevInfo;
extern Led_Info gLedDevInfo;
extern Ultras_Info gUltrasDevInfo;
extern Wire_Info gWireDevInfo;
extern Rotbot_Info gRotbotInfo;
extern Block_Info gBlockDevInfo;
extern Fan_Info gFanDevInfo;
extern Rfid_Dev_Info gRfidDevMachine;
extern Rfid_Info gRfidDevInfo;
extern Detect_Info gDetectDevInfo;
extern line_Info gLineDevInfo;
extern unsigned long gulCloudPushFlag;
extern st_blink_dev gLed_front_dev;
extern st_blink_dev gLed_back_dev;
extern unsigned long gulRegisterFlag;
extern Time_info gCurrentTime;
extern struct lfs_config * gLfsDevice;
extern unsigned char gucBlockTemp[];
extern unsigned char gucReflashWork;
extern unsigned char gucChargeType;
extern unsigned char gucWorkBack;
extern unsigned long gulBlockTick;
extern unsigned char gucNextRfid;
extern unsigned char gucCurRfid;
extern unsigned long gulPauseTime;
extern unsigned char gucTimeGetFlag;
extern unsigned long gulCloudAliveTick;

extern unsigned short gusPacketID;

extern Sys_Para gSysParameter;
extern unsigned char gucHomeFlag;
extern int glWalkCurPos;
extern unsigned char gucFlashInitFinish;
extern unsigned char gucIsCharge;
extern unsigned char gucStartCharge;
extern unsigned long gulChargeTick;
extern unsigned long gulUnWorkTick;
extern unsigned char gucPmMode;
extern unsigned char gucChargeSta;
//extern unsigned char gpucCloudData[];
//extern unsigned char gucMqttBackupFlag;
extern unsigned long gulCloudBackLen;
extern unsigned char gucMqttPinqTry;
extern Plan_Info gPlanDev;
extern unsigned long gulCurrentTick;
extern unsigned char gucCurrentError;
extern unsigned char gucConnetFLag;
extern unsigned long gulTimeTick;
extern unsigned short gusFrontDis;
extern unsigned short gusBackDis;
extern unsigned char gucFrontCtrTime;
extern unsigned char gucBackCtrTime;
extern unsigned char gucTest;
extern unsigned char gucDetectEn;
extern unsigned long gulDetectTick;
extern unsigned char gucStopEnFlag;
extern Gas_Info gGasInfo;
extern unsigned char gucRfidTest;
extern OTA_Status gOTAFlag;
extern os_task_t gTaskHandle[];
extern unsigned char gucPrintFlag;
extern unsigned char gucUpdateBuf[];
extern unsigned char gucManualStop;
extern unsigned long gulManualStopTick;
extern LIFT_CONTL gLiftControl;
//extern unsigned long gulDebugSelect;

#define TGT_SLEEP_MAXTIME    8000
#define BIT(n)			(0x00000001 << (n))

#define SYS_DATA_SECTOR			0	//系统参数存储位置
#define PLAN_NUM_SECTOR			1	//保存巡检计划个数
#define RUN_PLAN_START_SECTOR	2	//实际巡检计划存储位置 每个计划分配64个字节进行数据存储 一个块即可存储64个计划
#define RUN_PLAN_END_SECTOR		18	//巡检计划结束扇区
#define PLAN_DATA_SIZE			64	//一个计划64个字节
#define PLAN_NUM_EACH_SECTOR	64	//每个扇区64个计划
#define PLAN_MAX_NUM			1024
#define LFS_BLOCK_CYCLES        100
#define LFS_BLOCK_SIZE          0x1000
#define LFS_BLOCK_NUM           128
#define LFS_READ_SIZE           128  // page size
#define LFS_WRITE_SIZE          LFS_READ_SIZE // read/write buffer is more large, flash erase times is more little
#define LFS_CACHE_SIZE          LFS_READ_SIZE // 与lfs malloc申请的内存大小有关，读写数据长度也有关
#define LFS_LOOKAHEAD_SIZE      16            // must be 8的倍数
#define LFS_SECTOR_SIZE         0x1000
#define LFS_SECTOR_NUM          2048

#define DEBUG_EN				0

#define IS_Auto_Mode()			(gWorkWorker.WorkMode == 0)		//自动模式
#define IS_Charge_Sta()			(gBatDevInfo.ucBatSta == 0)		//充电状态
#define IS_WALK_STOP()			(gWalkDevInfo.ucWalkSta == 1)	//停止状态
#define IS_WALK_FORWARD()		(gWalkDevInfo.ucWalkSta == 0)	//前进状态
#define IS_WALK_BACKWORAD()		(gWalkDevInfo.ucWalkSta == 2)	//后退状态
#define IS_WALK_WORK()			((gWalkDevInfo.ucWalkSta == 0) || (gWalkDevInfo.ucWalkSta == 2))
#define IS_WALK_ERR()			(gWalkDevInfo.ucWalkSta == 9)
#define PERIPHERAL_ERR()		((gNvidiaDevInfo.ucNVIDIASta == 9) || (gRfidDevInfo.ucRfidSta == 9) || (gLiftDevInfo.ucLiftSta == 9) \
									|| ((gUltrasDevInfo.ucUltrasSta1 == 9) && (gUltrasDevInfo.ucUltrasErrCode1 == 2) && IS_WALK_FORWARD()) \
									||((gUltrasDevInfo.ucUltrasSta2 == 9) && (gUltrasDevInfo.ucUltrasErrCode2 == 2) && IS_WALK_BACKWORAD()))

#define IS_UPDATING()			(gOTAFlag.ucUpdateFlag == 1)
#define IS_UNUPDATING()			(gOTAFlag.ucUpdateFlag == 0)
extern uint8_t last_need_to_detect(uint8_t ucIndex);
extern uint8_t next_need_to_detect(uint8_t ucIndex);

#define WALK_STOP_DIS()			(gucStopEnFlag |= BIT(0))
#define WALK_STOP_EN()			(gucStopEnFlag &= ~BIT(0))
#define WALK_STOP_STA()			(gucStopEnFlag & BIT(0))
#define WALK_STOP_WAIT()		(gucStopEnFlag |= BIT(1))
#define WALK_STOP_CLEAR()		(gucStopEnFlag &= ~BIT(1))
#define IS_WALK_STOP_WAIT()		(gucStopEnFlag & BIT(1))
#define WORK_STOP_WAIT()		(gucStopEnFlag |= BIT(2))
#define WORK_STOP_CLEAR()		(gucStopEnFlag &= ~BIT(2))
#define IS_WORK_WAIT()			(gucStopEnFlag & BIT(2))

//#define RFID_DEBUG_SELECT()		(gulDebugSelect |= BIT(0))
//#define RFID_DEBUG_UNSEL()		(gulDebugSelect &= ~BIT(0))
//#define RFID_DEBUG_STA()		(gulDebugSelect & BIT(0))
//#define NVIDIA_DEBUG_SELECT()	(gulDebugSelect |= BIT(1))
//#define NVIDIA_DEBUG_UNSEL()	(gulDebugSelect &= ~BIT(1))
//#define NVIDIA_DEBUG_STA()		(gulDebugSelect & BIT(1))
//#define WALK_DEBUG_SELECT()		(gulDebugSelect |= BIT(2))
//#define WALK_DEBUG_UNSEL()		(gulDebugSelect &= ~BIT(2))
//#define WALK_DEBUG_STA()		(gulDebugSelect & BIT(2))
	
#define ADDR_FLASH_SECTOR_0     0x08000000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_1     0x08004000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_2     0x08008000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_3     0x0800C000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_4     0x08010000U  /* 64 KB */
#define ADDR_FLASH_SECTOR_5     0x08020000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_6     0x08040000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_7     0x08060000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_8     0x08080000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_9     0x080A0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_10    0x080C0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_11    0x080E0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_12    0x08100000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_13    0x08120000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_14    0x08140000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_15    0x08160000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_16    0x08180000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_17    0x081A0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_18    0x081C0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_19    0x081E0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_20    0x08200000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_21    0x08220000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_22    0x08240000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_23    0x08260000U  /* 128 KB */

#define APP1_ADDR				0x08008000	
#define APP2_ADDR				0x08100000
#define UPDATE_FLAG				0x5555AAAA
#define FLAG_ADDR    			0x08004000

#define NUM_INCREASE(num, limit)		do { \
									if(*num < limit) *num += 1; \
								}while(0)

#ifdef ROBOT_VER_1
#define NET_DEBUG				1
#endif
#ifdef ROBOT_VER_2
#define NET_DEBUG				0
#endif

#define NEW_BOARD				0			

extern FLASH_Status FLASH_EraseApp2(void);

extern FLASH_Status FLASH_WriteWords(uint32_t addr, uint32_t *buf, uint32_t wordCnt);

extern void flash_read_buf(uint32_t addr, uint32_t *buf, uint32_t word_len);

extern void read_sys_data(void);
extern void write_sys_data(void);
extern uint32_t get_plan_id(uint8_t * pucBuff);
extern void push_plan_status(uint8_t * pucBuff, uint8_t uctype, uint16_t usErr, uint16_t usTaskID);
extern uint32_t get_time_second_count(Time_info TimeSrc, Time_info TimeDst);
extern void app_request_upgrade(void);

// watch dog's timeout is 10s
//#define TGT_WDOG_TIMEOUT    (((TGT_SLEEP_MAXTIME) * 10) >> 3)
#ifdef TGT_WDOG_TIMEOUT
#define TGT_WDOG_CFG        (IWDG_OSC(8) | IWDG_PRESC(PRESC128) | IWDG_TIMEOUT(TGT_WDOG_TIMEOUT))
#endif

#define TGT_RCC_CFG         (RCC_OSC(EXTERNAL) | PLL_M(8) | PLL_N(360) | PLL_P(2) | PLL_Q(9))

#define TGT_EXTI_CFG        {    \
                /*UART7 RX PF.6*/ (INT_CHN(EXTI9_5_IRQn) | INT_MAINPRI(5) | INT_SUBPRI(2) | EXTI_PORT(F) | EXTI_LINE(LINE6) | EXTI_MODE(INTI) | EXTI_TRIG(FALL)),  \
                /*UART8 RX PE.0*/(INT_CHN(EXTI0_IRQn)   | INT_MAINPRI(5) | INT_SUBPRI(1) | EXTI_PORT(E) | EXTI_LINE(LINE0) | EXTI_MODE(INTI) | EXTI_TRIG(FALL)), \
                /*RTC wakeup */  (INT_CHN(RTC_WKUP_IRQn)| INT_MAINPRI(0) | INT_SUBPRI(0) | EXTI_LINE(LINE22) | EXTI_MODE(INTI) | EXTI_TRIG(RISE)) \
            }

#define TGT_INTI_CFG        {    \
                /*UART1*/(INT_CHN(USART1_IRQn) | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART2*/(INT_CHN(USART2_IRQn) | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART3*/(INT_CHN(USART3_IRQn) | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART4*/(INT_CHN(UART4_IRQn)  | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART5*/(INT_CHN(UART5_IRQn)  | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART6*/(INT_CHN(USART6_IRQn) | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART7*/(INT_CHN(UART7_IRQn)  | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*UART8*/(INT_CHN(UART8_IRQn)  | INT_MAINPRI(3) | INT_SUBPRI(3)),  \
                /*system tick*/ TGT_SYSTICK_INT  \
            }

#define _TGT_TIM_CFG     { \
                {/* TIMER3 */\
                    .object = TIM3, \
                    .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                    .rccPeri = RCC_APB1Periph_TIM3,  \
                    .cfg = TIM_FREQ(10) | TIM_CLKDIV(0) | TIM_CNTMODE(INC),\
                }, {/* TIMER4 */\
                    .object = TIM4, \
                    .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                    .rccPeri = RCC_APB1Periph_TIM4,  \
                    .cfg = TIM_FREQ(1000) | TIM_CLKDIV(TIM_DIV1) | TIM_CNTMODE(INC), \
                } \
            }

#if (NEW_BOARD == 0)
    // GPIO
/* ETH_RDY PA.8, in, low? */
#define TGT_ETH_INT         BSP_DEV_obj(GPIOA, PIN8 | BSP_GPIO_EN_HIGH)

#define TGT_ETH_CFG         BSP_DEV_obj(GPIOB, PIN0 | BSP_GPIO_DIR_OUT)

/* ROUTERA_RST PC.0, in, low */
#define TGT_ROUTERA_RST     BSP_DEV_obj(GPIOC, (PIN0 | BSP_GPIO_DIR_OUT))
#define TGT_ULTRAS_CTL      BSP_DEV_obj(GPIOC, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* UART1_EN PC.1, out, high */
#define TGT_UART1_EN        BSP_DEV_obj(GPIOC, (PIN1 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* WALK_CTRL PC.13, out, high */
#define TGT_WALK_PWR        BSP_DEV_obj(GPIOC, (PIN13 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

/* ROUTERB_RST PG.10, in, low */
#define TGT_ROUTERB_RST     BSP_DEV_obj(GPIOG, (PIN10 | BSP_GPIO_DIR_OUT))

/* BAT_PWR PE.2, out, high */
#define TGT_BAT_PWR         BSP_DEV_obj(GPIOE, (PIN2 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* Lift PWR PE.3,out,High */
#define TGT_LIFT_PWR        BSP_DEV_obj(GPIOE, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* BLE_PWR PE.4, out, high */
#define TGT_BLE_PWR         BSP_DEV_obj(GPIOE, (PIN4 | BSP_GPIO_DIR_OUT))
/* CAMERA_PWR PE.13, out, high */
#define TGT_CAMERA_PWR      BSP_DEV_obj(GPIOE, (PIN7 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* AUDIO_PWR PE.14, out, high */
#define TGT_AUDIO_PWR       BSP_DEV_obj(GPIOE, (PIN8 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

/* 24V_PWR PF.0, out, low */
#define TGT_24V_PWR         BSP_DEV_obj(GPIOF, (PIN0 | BSP_GPIO_DIR_OUT))
/* 12V_PWR PF.1, out, low */
#define TGT_12V_PWR         BSP_DEV_obj(GPIOF, (PIN1 | BSP_GPIO_DIR_OUT))
/* 5V_PWR PF.2, out, low */
#define TGT_5V_PWR         BSP_DEV_obj(GPIOF, (PIN2 | BSP_GPIO_DIR_OUT))
/* BACK ULTRAS_PWR PF.3, out, high */
#define TGT_BULTRAS_PWR     BSP_DEV_obj(GPIOF, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* FRONT ULTRAS_PWR PF.4, out, high */
#define TGT_ULTRAS_PWR      BSP_DEV_obj(GPIOF, (PIN4 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* ULTRAS_SET PF.8, out, high */
#define TGT_ULTRAS_SET      BSP_DEV_obj(GPIOF, (PIN8 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* MAGNETIC_DET PF.9, in, low */
#define TGT_MAGNETIC_DET1   BSP_DEV_obj(GPIOF, PIN9)
/* MAGNETIC_DET PF.10, in, low */ \
#define TGT_MAGNETIC_DET2   BSP_DEV_obj(GPIOF, PIN10)
/* LIGHT_FWD PF.11, out, high */
#define TGT_LIGHT_FWD       BSP_DEV_obj(GPIOF, (PIN11 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* LIGHT_BKD PF.12, out, high */
#define TGT_LIGHT_BKD       BSP_DEV_obj(GPIOF, (PIN12 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

/* BACK ULTRAS_SET PF.8, out, high */
#define TGT_BULTRAS_SET     BSP_DEV_obj(GPIOH, (PIN2 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* JETSON PWR PH.3 out, high */
#define TGT_JETSON_PWR      BSP_DEV_obj(GPIOH, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
#define TGT_FAN_PWR      	BSP_DEV_obj(GPIOG, (PIN0 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* ROUTERB_SCL PH.4, out, low */
#define TGT_ROUTERA_SCL     BSP_DEV_obj(GPIOH, (PIN4 | BSP_GPIO_DIR_OUT))
#define TGT_ROUTERA_SDA     BSP_DEV_obj(GPIOH, (PIN5 | BSP_GPIO_DIR_OUT))

/* ROUTERB_SCL PH.7, out, low */
#define TGT_ROUTERB_SCL     BSP_DEV_obj(GPIOH, (PIN7 | BSP_GPIO_DIR_OUT))
#define TGT_ROUTERB_SDA     BSP_DEV_obj(GPIOH, (PIN8 | BSP_GPIO_DIR_OUT))
/* ROUTERB_INT PH.14, in, high */
#define TGT_ROUTERB_INT     BSP_DEV_obj(GPIOH, PIN14)

/* ETH_RST PI.1, in, low */
#define TGT_ETH_RST         BSP_DEV_obj(GPIOI, (PIN1 | BSP_GPIO_DIR_OUT))
/* ROUTERA_INT PI.4, in, high */ \
#define TGT_ROUTERA_INT     BSP_DEV_obj(GPIOI, PIN4)
/* PWR_DET PI.5, in, low */
#define TGT_PWR_DET         BSP_DEV_obj(GPIOI, PIN5)
/* CHARGE_PWR PF.14, out, high */
#define TGT_CHARGE_PWR      BSP_DEV_obj(GPIOI, (PIN8 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

#define IS_WAKE_UP_MODE()	(gWorkWorker.sleepMode == 0)
#define IS_SLEEP_MODE()		(gWorkWorker.sleepMode == 1)
#define SET_SLEEP_MODE()	(gWorkWorker.sleepMode = 1)
#define QUIT_SLEEP_MODE()	(gWorkWorker.sleepMode = 0)

#define BAT_VOL_OVER_ALLOW() (gBatDevInfo.usBatVoltage >= (unsigned short)(gSysParameter.fAllowWorkVal * 100.0f))
#define BAT_VOL_OVER_BACK()	(gBatDevInfo.usBatVoltage >= (unsigned short)(gSysParameter.fBackCHargeVal * 100.0f))
#define sys_over_time(CurTick, TimeOut) ((sys_get_ms() - CurTick) > TimeOut)
#define JUDGE_WORK(Work_status)  (gWorkWorker.CurrentWork == Work_status)
#define SET_WORK_STA(Work_status) (gWorkWorker.CurrentWork = Work_status)

#define SET_CHECK_POS(Bits)		(gWorkWorker.CheckPos = (Bits))
#define CHECK_DETECT(Index)		(gWorkWorker.CheckPos & (0x00000001 << (Index - 1)))
#define DELETE_DETECT(Index)	(gWorkWorker.CheckPos &= ~(0x00000001 << (Index - 1)))
#define ADD_DETECT(Index)		(gWorkWorker.CheckPos |= (0x00000001 << (Index - 1)))


// MPU pin configuration
#define TGT_PINS_CFG        {    \
            {/* MAGNETIC_DET PF.9 & PF.10 【in】 */\
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN9) | GPIO_Pins(PIN10) | GPIO_MODE(IN) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* PWR_SIGNAL PI.5 */\
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN5) | GPIO_MODE(IN) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* SYS_PWR PE.2, WALK_PWR PE.3, BLE_PWR PE.4, CAMER_PWR PE.7, AUDIO_PWR PE.8 【out】 */ \
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_Pins(PIN3) | GPIO_Pins(PIN4) | GPIO_Pins(PIN7) | GPIO_Pins(PIN8) | \
                        GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ETH_PWR PF.2 5V-EN */ \
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN0) |GPIO_Pins(PIN1) | GPIO_Pins(PIN2) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* M18_BPWR PF.3 M18_PWR PF.4, M18_PWR PF.8, CHARGE_PWR PF.14, LIGHT_FWD PF.11, LIGHT_BKD PF.12 */ \
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN3) |GPIO_Pins(PIN4) | GPIO_Pins(PIN8) | GPIO_Pins(PIN11) | GPIO_Pins(PIN12) | \
                        GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* JETSON_PWR PH.3 */ \
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN3) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* ROUTER_INTB PH.14 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN14) | GPIO_MODE(IN) | GPIO_EN(HIGH) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ROUTER_INTA PI.4 */\
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN4) | GPIO_MODE(IN) | GPIO_EN(HIGH) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ROUTER_RESETA PC.0 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN0) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ROUTER_RESETB PG.10 */\
                .object = GPIOG, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOG,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_Pins(PIN0) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ETH_RESET PI.1 */\
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ETH_INT PA.8 low */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_MODE(IN) | GPIO_EN(LOW) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* SPI1_CS PA.4 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN4) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI1_SCK PA.5, SPI1_MISO PA.6, SPI1_MOSI PA.7 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN5) | GPIO_Pins(PIN6)  | GPIO_Pins(PIN7) | GPIO_AF(AF_SPI1) |GPIO_MODE(AF) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI2_CS PB.12 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN12) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI2_SCK PB.13, SPI2_MISO PB.14, SPI2_MOSI PB.15 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN13) | GPIO_Pins(PIN14)  | GPIO_Pins(PIN15) | GPIO_MODE(AF) | GPIO_AF(AF_SPI2) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* I2C1_SCL PB.8, I2C1_SDA PB.9 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_Pins(PIN9) | GPIO_MODE(AF) | GPIO_AF(AF_I2C1) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(OD),\
            }, {/* I2C2_SCL PH.4, I2C2_SDA PH.5 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN5) | GPIO_MODE(OUT) | GPIO_PUPD(UP) | GPIO_SPEED(TOP) | GPIO_OTYPE(PP),\
            }, {/* M18_BPWR PH.2*/ \
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN4) |GPIO_Pins(PIN7) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(TOP) | GPIO_OTYPE(PP),\
            }, {/* I2C3_SCL PH.7, I2C3_SDA PH.8 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_MODE(OUT) | GPIO_PUPD(UP) | GPIO_SPEED(TOP) | GPIO_OTYPE(PP),\
            }, {/* UART1_EN PC.1 Walk PWR PC.13 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN1) |GPIO_Pins(PIN13) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART1_TX PA.9, UART1_RX PA.10 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN9) | GPIO_Pins(PIN10) | GPIO_MODE(AF) | GPIO_AF(AF_UART1) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART2_EN PB.0 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN0) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART2_TX PA.2, UART2_RX PA.3 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_Pins(PIN3) |GPIO_MODE(AF) | GPIO_AF(AF_UART2) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART3_EN PD.10 */\
                .object = GPIOD, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOD,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART3_TX PD.8, UART3_RX PD.9 */\
                .object = GPIOD, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOD,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_Pins(PIN9) |GPIO_MODE(AF) | GPIO_AF(AF_UART3) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART4_EN PB.1 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART4_TX PC.10, UART4_TX PC.11 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_Pins(PIN11) | GPIO_MODE(AF) | GPIO_AF(AF_UART4) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* CHARGE PWR PI. 8*/ \
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART5_EN PI.15 */\
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN15) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART5_TX PC.12 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN12) | GPIO_MODE(AF) | GPIO_AF(AF_UART5) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART5_RX PD.2 */\
                .object = GPIOD, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOD,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_MODE(AF) | GPIO_AF(AF_UART5) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART6_EN PC.8 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN8) |GPIO_Pins(PIN3) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART6_TX PC.6, UART6_RX PC.7 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN6) | GPIO_Pins(PIN7) |GPIO_MODE(AF) | GPIO_AF(AF_UART6) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART7_TX PF.7, UART7_RX PF.6 */\
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN7) | GPIO_Pins(PIN6) | GPIO_MODE(AF) | GPIO_AF(AF_UART7) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART8_TX PE.1, UART8_RX PE.0 */\
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_Pins(PIN0) | GPIO_MODE(AF) | GPIO_AF(AF_UART8) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI4_CS PE.11 */\
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN11) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI1_SCK PA.5, SPI1_MISO PA.6, SPI1_MOSI PA.7 */\
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN12) | GPIO_Pins(PIN13) | GPIO_Pins(PIN14) | GPIO_AF(AF_SPI4) |GPIO_MODE(AF) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* EPROM write protect PB.7 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN7) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* FLASH hold pin PB.10 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* FLASH write protect PB.11 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN11) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(LOW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            } \
        }

#define TGT_UART_CFG    {/* enable pin: None*/    \
            {/* UART1 config */\
                .object = USART1, \
                .rccClkCmd = RCC_APB2PeriphClockCmd,  \
                .rccPeri = RCC_APB2Periph_USART1,  \
                .cfg = UART_DEVTYPE(PERI_COM1) | UART_BAUDRATE(19200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART2 config */\
                .object = USART2, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_USART2,  \
                .cfg = UART_DEVTYPE(PERI_COM2) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART3 config */\
                .object = USART3, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_USART3,  \
                .cfg = UART_DEVTYPE(PERI_COM3) | UART_BAUDRATE(57600) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART4 config */\
                .object = UART4, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART4,  \
                .cfg = UART_DEVTYPE(PERI_COM4) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART5 config */\
                .object = UART5, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART5,  \
                .cfg = UART_DEVTYPE(PERI_COM5) | UART_BAUDRATE(9600) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART6 config */\
                .object = USART6, \
                .rccClkCmd = RCC_APB2PeriphClockCmd,  \
                .rccPeri = RCC_APB2Periph_USART6,  \
                .cfg = UART_DEVTYPE(PERI_COM6) | UART_BAUDRATE(9600) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART7 config */\
                .object = UART7, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART7,  \
                .cfg = UART_DEVTYPE(PERI_COM7) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART8 config */\
                .object = UART8, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART8,  \
                .cfg = UART_DEVTYPE(PERI_COM8) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            } \
        }
#define TGT_CHARGE_CFG      {/* enable pin: PC.11*/ \
                .en_pin = BSP_DEV_obj(GPIOC, PIN1 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART1), \
            }

#define TGT_TEMP_CFG      {/* enable pin: PB.0*/ \
                .en_pin = BSP_DEV_obj(GPIOB, PIN0 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART2), \
            }


#define TGT_RFID_CFG        {/* enable pin: PD.10*/ \
                .en_pin = BSP_DEV_obj(GPIOD, PIN10 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART3), \
            }

#define TGT_WALK_CFG        {/* enable pin: PB.1*/  \
                .en_pin = BSP_DEV_obj(GPIOB, PIN1 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(UART4), \
            }

#define TGT_LIFT_CFG        {/* enable pin: PI.15*/ \
                .en_pin = BSP_DEV_obj(GPIOI, PIN15 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(UART5), \
            }

#define TGT_BAT_CFG        {/* enable pin: PC.8*/   \
                .en_pin = BSP_DEV_obj(GPIOC, PIN8 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART6), \
            }

#define TGT_BLE_CFG         {   \
                .port.reg = BSP_Obj2Reg(UART7), \
            }

#define TGT_TTY_CFG         {   \
                .port.reg = BSP_Obj2Reg(UART8), \
            }


#else		//新数字版代码

#define TGT_ETH_INT         BSP_DEV_obj(GPIOA, PIN8 | BSP_GPIO_EN_HIGH)

#define TGT_ETH_CFG         BSP_DEV_obj(GPIOB, PIN0 | BSP_GPIO_DIR_OUT)

/* ROUTERA_RST PC.0, in, low */
#define TGT_ROUTERA_RST     BSP_DEV_obj(GPIOC, (PIN0 | BSP_GPIO_DIR_OUT))
#define TGT_ULTRAS_CTL      BSP_DEV_obj(GPIOC, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* UART1_EN PC.1, out, high */
#define TGT_UART1_EN        BSP_DEV_obj(GPIOJ, (PIN0 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* WALK_CTRL PC.13, out, high */
#define TGT_WALK_PWR        BSP_DEV_obj(GPIOF, (PIN5 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

/* ROUTERB_RST PG.10, in, low */
#define TGT_ROUTERB_RST     BSP_DEV_obj(GPIOG, (PIN10 | BSP_GPIO_DIR_OUT))

/* BAT_PWR PE.2, out, high */
#define TGT_BAT_PWR         BSP_DEV_obj(GPIOE, (PIN2 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* Lift PWR PE.3,out,High */
#define TGT_LIFT_PWR        BSP_DEV_obj(GPIOE, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* BLE_PWR PE.4, out, high */
#define TGT_BLE_PWR         BSP_DEV_obj(GPIOE, (PIN4 | BSP_GPIO_DIR_OUT))
/* CAMERA_PWR PE.13, out, high */
#define TGT_CAMERA_PWR      BSP_DEV_obj(GPIOI, (PIN14 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* AUDIO_PWR PE.14, out, high */
#define TGT_AUDIO_PWR       BSP_DEV_obj(GPIOH, (PIN11 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

/* 24V_PWR PF.0, out, low */
#define TGT_24V_PWR         BSP_DEV_obj(GPIOF, (PIN3 | BSP_GPIO_DIR_OUT))
/* 12V_PWR PF.1, out, low */
#define TGT_12V_PWR         BSP_DEV_obj(GPIOI, (PIN13 | BSP_GPIO_DIR_OUT))
/* 5V_PWR PF.2, out, low */
#define TGT_5V_PWR         BSP_DEV_obj(GPIOF, (PIN1 | BSP_GPIO_DIR_OUT))
/* BACK ULTRAS_PWR PF.3, out, high */
//#define TGT_BULTRAS_PWR     BSP_DEV_obj(GPIOF, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* FRONT ULTRAS_PWR PF.4, out, high */
#define TGT_ULTRAS_PWR      BSP_DEV_obj(GPIOH, (PIN12 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* ULTRAS_SET PF.8, out, high */
#define TGT_ULTRAS_SET      BSP_DEV_obj(GPIOF, (PIN8 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* MAGNETIC_DET PF.9, in, low */
#define TGT_MAGNETIC_DET1   BSP_DEV_obj(GPIOH, PIN9)
/* MAGNETIC_DET PF.10, in, low */ \
#define TGT_MAGNETIC_DET2   BSP_DEV_obj(GPIOF, PIN10)
/* LIGHT_FWD PF.11, out, high */
#define TGT_LIGHT_FWD       BSP_DEV_obj(GPIOI, (PIN12 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* LIGHT_BKD PF.12, out, high */
#define TGT_LIGHT_BKD       BSP_DEV_obj(GPIOF, (PIN0 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

/* BACK ULTRAS_SET PF.8, out, high */
#define TGT_BULTRAS_SET     BSP_DEV_obj(GPIOH, (PIN2 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* JETSON PWR PH.3 out, high */
#define TGT_JETSON_PWR      BSP_DEV_obj(GPIOH, (PIN3 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
#define TGT_FAN_PWR      	BSP_DEV_obj(GPIOH, (PIN10 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))
/* ROUTERB_SCL PH.4, out, low */
#define TGT_ROUTERA_SCL     BSP_DEV_obj(GPIOH, (PIN4 | BSP_GPIO_DIR_OUT))
#define TGT_ROUTERA_SDA     BSP_DEV_obj(GPIOH, (PIN5 | BSP_GPIO_DIR_OUT))

/* ROUTERB_SCL PH.7, out, low */
#define TGT_ROUTERB_SCL     BSP_DEV_obj(GPIOH, (PIN7 | BSP_GPIO_DIR_OUT))
#define TGT_ROUTERB_SDA     BSP_DEV_obj(GPIOH, (PIN8 | BSP_GPIO_DIR_OUT))
/* ROUTERB_INT PH.14, in, high */
#define TGT_ROUTERB_INT     BSP_DEV_obj(GPIOH, PIN14)

/* ETH_RST PI.1, in, low */
#define TGT_ETH_RST         BSP_DEV_obj(GPIOI, (PIN1 | BSP_GPIO_DIR_OUT))
/* ROUTERA_INT PI.4, in, high */ \
#define TGT_ROUTERA_INT     BSP_DEV_obj(GPIOI, PIN4)
/* PWR_DET PI.5, in, low */
#define TGT_PWR_DET         BSP_DEV_obj(GPIOA, PIN1)
/* CHARGE_PWR PF.14, out, high */
#define TGT_CHARGE_PWR      BSP_DEV_obj(GPIOF, (PIN4 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH))

#define IS_WAKE_UP_MODE()	(gWorkWorker.sleepMode == 0)
#define IS_SLEEP_MODE()		(gWorkWorker.sleepMode == 1)
#define SET_SLEEP_MODE()	(gWorkWorker.sleepMode = 1)
#define QUIT_SLEEP_MODE()	(gWorkWorker.sleepMode = 0)

#define BAT_VOL_OVER_ALLOW() (gBatDevInfo.usBatVoltage >= (unsigned short)(gSysParameter.fAllowWorkVal * 100.0f))
#define BAT_VOL_OVER_BACK()	(gBatDevInfo.usBatVoltage >= (unsigned short)(gSysParameter.fBackCHargeVal * 100.0f))
#define sys_over_time(CurTick, TimeOut) ((sys_get_ms() - CurTick) > TimeOut)
#define JUDGE_WORK(Work_status)  (gWorkWorker.CurrentWork == Work_status)
#define SET_WORK_STA(Work_status) (gWorkWorker.CurrentWork = Work_status)

#define SET_CHECK_POS(Bits)		(gWorkWorker.CheckPos = (Bits))
#define CHECK_DETECT(Index)		(gWorkWorker.CheckPos & (0x00000001 << (Index - 1)))
#define DELETE_DETECT(Index)	(gWorkWorker.CheckPos &= ~(0x00000001 << (Index - 1)))
#define ADD_DETECT(Index)		(gWorkWorker.CheckPos |= (0x00000001 << (Index - 1)))


// MPU pin configuration
#define TGT_PINS_CFG        {    \
            {/* MAGNETIC_DET PF.9 & PF.10 【in】 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN6) | GPIO_Pins(PIN9) | GPIO_MODE(IN) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
			}, { \
				.object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* PWR_SIGNAL PI.5 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_MODE(IN) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* SYS_PWR PE.2, WALK_PWR PE.3, BLE_PWR PE.4, CAMER_PWR PE.7, AUDIO_PWR PE.8 【out】 */ \
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_Pins(PIN3) | GPIO_Pins(PIN4) |GPIO_Pins(PIN8) |\
                        GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ETH_PWR PF.2 5V-EN */ \
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* M18_BPWR PF.3 M18_PWR PF.4, M18_PWR PF.8, CHARGE_PWR PF.14, LIGHT_FWD PF.11, LIGHT_BKD PF.12 */ \
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN0) | GPIO_Pins(PIN3) |GPIO_Pins(PIN4) |GPIO_Pins(PIN5) | GPIO_Pins(PIN8) | GPIO_Pins(PIN11) | \
                        GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* JETSON_PWR PH.3 */ \
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN3) |GPIO_Pins(PIN12) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* ROUTER_INTB PH.14 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN14) | GPIO_MODE(IN) | GPIO_EN(HIGH) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ROUTER_INTA PI.4 */\
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN4) | GPIO_MODE(IN) | GPIO_EN(HIGH) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ROUTER_RESETA PC.0 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN0) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ROUTER_RESETB PG.10 */\
                .object = GPIOG, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOG,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ETH_RESET PI.1 */\
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* ETH_INT PA.8 low */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_MODE(IN) | GPIO_EN(LOW) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP), \
            }, {/* SPI1_CS PA.4 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN4) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI1_SCK PA.5, SPI1_MISO PA.6, SPI1_MOSI PA.7 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN5) | GPIO_Pins(PIN6) | GPIO_Pins(PIN7) | GPIO_AF(AF_SPI1) |GPIO_MODE(AF) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI2_CS PB.12 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN12) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI2_SCK PB.13, SPI2_MISO PB.14, SPI2_MOSI PB.15 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN13) | GPIO_Pins(PIN14) | GPIO_Pins(PIN15) | GPIO_MODE(AF) | GPIO_AF(AF_SPI2) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* I2C1_SCL PB.8, I2C1_SDA PB.9 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_Pins(PIN9) | GPIO_MODE(AF) | GPIO_AF(AF_I2C1) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(OD),\
            }, {/* I2C2_SCL PH.4, I2C2_SDA PH.5 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN5) | GPIO_MODE(OUT) | GPIO_PUPD(UP) | GPIO_SPEED(TOP) | GPIO_OTYPE(PP),\
            }, {/* M18_BPWR PH.2*/ \
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN4) |GPIO_Pins(PIN7) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(TOP) | GPIO_OTYPE(PP),\
            }, {/* I2C3_SCL PH.7, I2C3_SDA PH.8 */\
                .object = GPIOH, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOH,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_MODE(OUT) | GPIO_PUPD(UP) | GPIO_SPEED(TOP) | GPIO_OTYPE(PP),\
            }, {/* UART1_EN PJ.1  */\
                .object = GPIOJ, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOJ,  \
                .cfg = GPIO_Pins(PIN0) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART1_TX PA.9, UART1_RX PA.10 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN9) | GPIO_Pins(PIN10) | GPIO_MODE(AF) | GPIO_AF(AF_UART1) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART2_EN PB.0 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN0) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART2_TX PA.2, UART2_RX PA.3 */\
                .object = GPIOA, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOA,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_Pins(PIN3) |GPIO_MODE(AF) | GPIO_AF(AF_UART2) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART3_EN PD.10 */\
                .object = GPIOD, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOD,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART3_TX PD.8, UART3_RX PD.9 */\
                .object = GPIOD, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOD,  \
                .cfg = GPIO_Pins(PIN8) | GPIO_Pins(PIN9) |GPIO_MODE(AF) | GPIO_AF(AF_UART3) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART4_EN PB.1 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART4_TX PC.10, UART4_TX PC.11 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_Pins(PIN11) | GPIO_MODE(AF) | GPIO_AF(AF_UART4) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, { \
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN13) |GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* CHARGE PWR PI. 8*/ \
                .object = GPIOI, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOI,  \
                .cfg = GPIO_Pins(PIN12) |GPIO_Pins(PIN14) | GPIO_Pins(PIN15) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART5_TX PC.12 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN12) | GPIO_MODE(AF) | GPIO_AF(AF_UART5) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART5_RX PD.2 */\
                .object = GPIOD, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOD,  \
                .cfg = GPIO_Pins(PIN2) | GPIO_MODE(AF) | GPIO_AF(AF_UART5) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART6_EN PC.8 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN3) |GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART6_TX PC.6, UART6_RX PC.7 */\
                .object = GPIOC, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOC,  \
                .cfg = GPIO_Pins(PIN6) | GPIO_Pins(PIN7) |GPIO_MODE(AF) | GPIO_AF(AF_UART6) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART7_TX PF.7, UART7_RX PF.6 */\
                .object = GPIOF, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOF,  \
                .cfg = GPIO_Pins(PIN7) | GPIO_Pins(PIN6) | GPIO_MODE(AF) | GPIO_AF(AF_UART7) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* UART8_TX PE.1, UART8_RX PE.0 */\
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN1) | GPIO_Pins(PIN0) | GPIO_MODE(AF) | GPIO_AF(AF_UART8) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI4_CS PE.11 */\
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN11) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(UP) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* SPI1_SCK PA.5, SPI1_MISO PA.6, SPI1_MOSI PA.7 */\
                .object = GPIOE, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOE,  \
                .cfg = GPIO_Pins(PIN12) | GPIO_Pins(PIN13) | GPIO_Pins(PIN14) | GPIO_AF(AF_SPI4) |GPIO_MODE(AF) | GPIO_PUPD(NONE) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* EPROM write protect PB.7 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN7) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(DW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* FLASH hold pin PB.10 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN10) | GPIO_MODE(OUT) | GPIO_EN(LOW) | GPIO_PUPD(HIGH) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            }, {/* FLASH write protect PB.11 */\
                .object = GPIOB, \
                .rccClkCmd = RCC_AHB1PeriphClockCmd,  \
                .rccPeri = RCC_AHB1Periph_GPIOB,  \
                .cfg = GPIO_Pins(PIN11) | GPIO_MODE(OUT) | GPIO_EN(HIGH) | GPIO_PUPD(LOW) | GPIO_SPEED(FAST) | GPIO_OTYPE(PP),\
            } \
        }

#define TGT_UART_CFG    {/* enable pin: None*/    \
            {/* UART1 config */\
                .object = USART1, \
                .rccClkCmd = RCC_APB2PeriphClockCmd,  \
                .rccPeri = RCC_APB2Periph_USART1,  \
                .cfg = UART_DEVTYPE(PERI_COM1) | UART_BAUDRATE(9600) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART2 config */\
                .object = USART2, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_USART2,  \
                .cfg = UART_DEVTYPE(PERI_COM2) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART3 config */\
                .object = USART3, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_USART3,  \
                .cfg = UART_DEVTYPE(PERI_COM3) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART4 config */\
                .object = UART4, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART4,  \
                .cfg = UART_DEVTYPE(PERI_COM4) | UART_BAUDRATE(19200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART5 config */\
                .object = UART5, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART5,  \
                .cfg = UART_DEVTYPE(PERI_COM5) | UART_BAUDRATE(57600) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART6 config */\
                .object = USART6, \
                .rccClkCmd = RCC_APB2PeriphClockCmd,  \
                .rccPeri = RCC_APB2Periph_USART6,  \
                .cfg = UART_DEVTYPE(PERI_COM6) | UART_BAUDRATE(9600) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART7 config */\
                .object = UART7, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART7,  \
                .cfg = UART_DEVTYPE(PERI_COM7) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            }, {/* UART8 config */\
                .object = UART8, \
                .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                .rccPeri = RCC_APB1Periph_UART8,  \
                .cfg = UART_DEVTYPE(PERI_COM8) | UART_BAUDRATE(115200) | UART_STOPBITS(BITS1) | UART_PARITY(NONE) | UART_WORDLEN(BITS8) | UART_MODE(BOTH) | UART_FLOWCTRL(NONE),\
            } \
        }

#define TGT_BAT_CFG      {/* enable pin: PC.11*/ \
                .en_pin = BSP_DEV_obj(GPIOJ, PIN0 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART1), \
            }

#define TGT_TEMP_CFG      {/* enable pin: PB.0*/ \
                .en_pin = BSP_DEV_obj(GPIOB, PIN0 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART2), \
            }


#define TGT_WALK_CFG        {/* enable pin: PD.10*/ \
                .en_pin = BSP_DEV_obj(GPIOD, PIN10 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART3), \
            }

#define TGT_CHARGE_CFG        {/* enable pin: PB.1*/  \
                .en_pin = BSP_DEV_obj(GPIOB, PIN1 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(UART4), \
            }

#define TGT_RFID_CFG        {/* enable pin: PI.15*/ \
                .en_pin = BSP_DEV_obj(GPIOI, PIN15 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(UART5), \
            }

#define TGT_LIFT_CFG        {/* enable pin: PC.8*/   \
                .en_pin = BSP_DEV_obj(GPIOE, PIN8 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH), \
                .port.reg = BSP_Obj2Reg(USART6), \
            }


#define TGT_BLE_CFG         {   \
                .port.reg = BSP_Obj2Reg(UART7), \
            }

#define TGT_TTY_CFG         {   \
                .port.reg = BSP_Obj2Reg(UART8), \
            }


#endif

#define TGT_I2C_CFG { \
                {/* I2C1 config */\
                    .object = I2C1, \
                    .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                    .rccPeri = RCC_APB1Periph_I2C1,  \
                    .cfg = I2C_DEVTYPE(PERI_I2C1) | I2C_MODE(I2C) | I2C_SPEED(400000) | I2C_ACK(ENABLE) | I2C_DUTYCYCLE(CYCLE2) | I2C_ADDRBITS(BITS7),\
                } \
            }

#define TGT_SPI_CFG    { \
                {/* SPI1 config */\
                    .object = SPI1, \
                    .rccClkCmd = RCC_APB2PeriphClockCmd,  \
                    .rccPeri = RCC_APB2Periph_SPI1,  \
                    .cfg = SPI_DEVTYPE(PERI_SPI1) | SPI_MODE(MASTER) | SPI_FSTBIT(MSB) | SPI_DATABIT(_8B) | SPI_CPHA(UNIEDGE) | SPI_CPOL(LOW) | SPI_DIR(DUPLEX) | SPI_NSS(SOFT) | SPI_PRESC(PRESC8) | SPI_CRCPOLY(7),\
                }, {/* SPI2 config */\
                    .object = SPI2, \
                    .rccClkCmd = RCC_APB1PeriphClockCmd,  \
                    .rccPeri = RCC_APB1Periph_SPI2,  \
                    .cfg = SPI_DEVTYPE(PERI_SPI2) | SPI_MODE(MASTER) | SPI_FSTBIT(MSB) | SPI_DATABIT(_8B) | SPI_CPHA(BIEDGE) | SPI_CPOL(HIGH) | SPI_DIR(DUPLEX) | SPI_NSS(SOFT) | SPI_PRESC(PRESC2) | SPI_CRCPOLY(7),\
                }, {/* SPI4 config */\
                    .object = SPI4, \
                    .rccClkCmd = RCC_APB2PeriphClockCmd,  \
                    .rccPeri = RCC_APB2Periph_SPI4,  \
                    .cfg = SPI_DEVTYPE(PERI_SPI4) | SPI_MODE(MASTER) | SPI_FSTBIT(MSB) | SPI_DATABIT(_8B) | SPI_CPHA(UNIEDGE) | SPI_CPOL(LOW) | SPI_DIR(DUPLEX) | SPI_NSS(SOFT) | SPI_PRESC(PRESC32) | SPI_CRCPOLY(7),\
                } \
            }

//
#define TGT_EPROM_CFG   { \
                .mode = _24CL08_E,  \
                .bus = { \
                    .reg  = BSP_Obj2Reg(I2C1), \
                    .addr = 0x50,   \
                }, /* wp PB.7:low is read only */ \
                .wp = BSP_DEV_obj(GPIOB, PIN7 | BSP_GPIO_DIR_OUT | BSP_GPIO_EN_HIGH),  \
            }

/** breif spi_user_data
//cs low is valid
//wp high is valid(read only)
//hold low is valid(can't read and write)
**/
#define TGT_FLASH_CFG   { \
                .dummy = 0xFF, \
                .bus = {  \
                    .port.reg = BSP_Obj2Reg(SPI2),   \
                },  \
/* cs pin PB.12 */.cs = BSP_DEV_obj(GPIOB, (PIN12 | BSP_GPIO_DIR_OUT)),        \
/* write protect PB.11*/ .wp = BSP_DEV_obj(GPIOB, (PIN11 | BSP_GPIO_DIR_OUT)), \
/* hold pin PB.10*/ .hold = BSP_DEV_obj(GPIOB, (PIN10 | BSP_GPIO_DIR_OUT))     \
            }


    //i2c device
#define TGT_RTLB_CFG        { \
            .mdc  = TGT_ROUTERA_SCL, \
            .mdio = TGT_ROUTERA_SDA, \
        }

#define TGT_RTLA_CFG        { \
            .mdc  = TGT_ROUTERB_SCL, \
            .mdio = TGT_ROUTERB_SDA, \
        }

#if 1
#define TGT_NET_CFG        { \
                .dummy = 0xFF, \
                .bus = {  \
                    .port.reg = BSP_Obj2Reg(SPI1),   \
                },  \
				.cs = BSP_DEV_obj(GPIOA, (PIN4 | BSP_GPIO_DIR_OUT))        \
            }
#else
/* #define TGT_TEMP_CFG        {   \ */
#define TGT_NET_CFG         {   \
                .port.reg = BSP_Obj2Reg(USART2), \
            }
#endif
//SPI接4串口
#define TGT_SPI_UART_CFG        { \
				.dummy = 0xFF, \
				.bus = {  \
					.port.reg = BSP_Obj2Reg(SPI4),	 \
				},	\
				.cs = BSP_DEV_obj(GPIOE, (PIN11 | BSP_GPIO_DIR_OUT)) 	   \
			}


    // uart device

#define TGT_ULTRA_ADC_CHNS  (100)

extern void set_cloud_push_flag(uint8_t num);

#define Open_Line_En()	GPIO_SetBits(GPIOI, GPIO_Pin_8);	//开启有线充
#define Close_Line_En()	GPIO_ResetBits(GPIOI, GPIO_Pin_8);	//关闭有线充
#define Open_Wire_En()	GPIO_SetBits(GPIOE, GPIO_Pin_2);	//开启无线充
#define Close_Wire_En()	GPIO_ResetBits(GPIOE, GPIO_Pin_2);	//关闭无线充

//preserve 1 byte end symbol
#define CMD_RXBUF_SIZE		(1024)
#define NET_RXBUF_SIZE		(512)
#define DEV_RXBUF_SIZE      (64)
#endif




