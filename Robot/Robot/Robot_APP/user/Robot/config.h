/******************************************************************************
 * @brief    系统配置文件
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "comdef.h"
#include "ccronexpr.h"
#include "sys_cfg.h"
#include "net.h"

#define PRJ_TYPE                    (0x1)
#define PRJ_NAME                    "iCIVIL-Robot" /* 项目名称 */
#ifdef ROBOT_VER_1
#define SW_VER                      "V1.26" //软件版本号
#endif
#ifdef ROBOT_VER_2
#define SW_VER                      "V1.16" //软件版本号
#endif
//#pragma message(PRJ_NAME""SW_VER" <"__DATE__" "__TIME__">")

// 设备对应产品标识符（32位字节）
#define PRODUCT_KEY                 "37da7b1058c140b8a477352a8a89ed17"
#ifdef ROBOT_VER_1
#define DEVICE_KEY                     "320adb9c7fbb4e788f04609776ee2572"
#endif
#ifdef ROBOT_VER_2
#define DEVICE_KEY                    "084a2d1cdf214ac7936eb787dc0e8bcb"
#endif

extern const char net_dev_sn[];
extern char cloud_client_ID[];
extern char nvidia_client_ID[];

#define BOOT_SIZE                   0x8000//(0x4000)//应用程序开始地址0x08004000
#define APP_ADDRESS                 (FLASH_BASE | BOOT_SIZE)//应用程序开始地址0x08004000

#define MAGIC_CFG                   (0x5A5A)

#define NVIDIA_SVR_PORT             1883
#define MQTT_SVR_PORT               1883

#define NVIDIA_SVR_IP               "192.168.0.101"
#define NVIDIA_USR_NAME             "mqtt_user"
#define NVIDIA_USR_PWD              "jlgd@1234"
#if 1 // test server
#define MQTT_SVR_IP                 "47.115.47.210"
#define MQTT_USR_NAME               "mqtt_user"
#define MQTT_USR_PWD                "jlgd@1234"

#else // product server
#define MQTT_SVR_IP                 "47.115.51.248"
#define MQTT_USR_NAME               "mqtt_user"
#define MQTT_USR_PWD                "JLGD@jlgd11"

#endif

#define _NET_NVIDIA_OPT           "Nvidia:" 
#define _NET_PROPERTY_GET         "GetProperty:"
#define _NET_INSPECT_SET          "SetInspection:"
#define _NET_INSPECT_EXE          "ExeInspection:"
#define _NET_INSPECT_END          "EndInspection:"
#define _NET_SET_HIGH              "SetHigh:01"
#define _NET_SET_PLAN		"SetSchemeid:"
#define _NET_SET_TASK		"settaskid:"

typedef struct 
{
	uint32_t ulTargetTurn;	//目标转动圈数
	int lCurTurn;			//当前已转动圈数
	int	   lTargetPos;		//最后位置
	unsigned int ulLastPos;	//上一次读取到的位置
	unsigned char IsPosControl : 1;	//是否为位置控制模式
	unsigned char ucResv : 7;
}Walk_Para;

typedef enum {
	WORK_NONE = 0,			//无状态
	WORK_GO_HOME,			//回原点
	WORK_GO_HOME_BACK,		//返回巡点
	WORK_WALKING,			//行走中
	WORK_DETECTION,			//磁铁感应
	WORK_ASK_PHOTO,			//请求拍照
	WORK_ASK_GETHIGH,		//请求升降电机高度
	WAITLIFTREACH,			//等待升降电机到位
	WORKASKEXE,				//请求执行执行项
	WAITEXETIONBACK,		//等待巡检项执行结果
	WORK_VIEW_FINISH,		//视觉检查完毕
	WORK_PAUSE,				//暂停
	WORK_RESUME,			//恢复
	WORK_BLOCK_FRONT,		//前方遇到障碍物
	WORK_BLOCK_BACK,		//后方遇到障碍物
	WORK_WAIT_CHARGE,		//等待充电
	WORK_WAIT_FRONT_OPEN, //等待前方打开门，用于设备外出巡检
	WORK_WAIT_BACK_OPEN,	//等待后方关闭门，用于设备回原
	WORK_DETECT_FRONT,		//检测磁铁，停下，等待门开
	WORK_DETECT_BACK,		//后方检测磁铁，停下，等待门开
	WORK_WAIT_DISPEAR,		//等待磁感应消失
}Work_Sta;

typedef struct {
	unsigned short usBatVoltage;  //电压
	unsigned short usTemp;		//温度
	unsigned char ucBatSta;		//电池状态	0:充电 1:放电 9:故障
	unsigned short usErrcode;	//故障信息
}Bat_Info;	//电池信息

typedef struct {
	unsigned long ulHigh;		//高度
	unsigned short usTemp;		//温度
	unsigned long ulSpeed;		//速度
	unsigned char ucLiftSta;	//状态 0:正常 9:故障
	unsigned short usErrcode;	//故障状态
}Lift_Info;	//升降电机信息

typedef struct {
	unsigned long ulPos;		//距离
	long lSpeed;				//速度
	unsigned short usTemp;		//温度
	unsigned short usErrcode;	//故障状态
	unsigned char ucWalkSta;	//状态 0:正常 9:故障
	unsigned char ucWalkLock;	//移动速度锁，置1时表示该段移动时不允许被参数修改变更
}Walk_Info;	//行走电机信息

typedef struct {
	unsigned char ucNVIDIASta;	//英伟达状态
	unsigned short usErrcode;	//故障信息
}NVIDIA_Info;	//英伟达信息

typedef struct {
	unsigned char ucLedSta1 : 4;     //前照灯状态
	unsigned char ucLedSta2 : 4;     //后照灯状态
	unsigned char ucLedErrCode1; //故障信息
	unsigned char ucLedErrCode2; //故障信息
}Led_Info;	//LED信息

typedef struct {
	unsigned char ucUltrasSta1;				//超声波状态
	unsigned char ucUltrasSta2;				//超声波状态
	unsigned char ucUltrasErrCode1 : 4;		//超声波故障信息
	unsigned char ucUltrasErrCode2 : 4;		//超声波故障信息
}Ultras_Info;	//超声波信息

typedef struct {
	unsigned short usWireVoltage;	//无线充电压
	unsigned short usWireTemp;		//无线充温度
	unsigned short usWireCurrent;	//无线充电流
	unsigned char ucWireSta;		//无线充状态
	unsigned short usWireErrCode;	//无线充故障信息
}Wire_Info;	//超声波信息

typedef struct {
	unsigned char ucRobotSta;		//机器人状态
	unsigned short usRobotErrCode;	//机器人故障信息
}Rotbot_Info;	//机器人状态信息

typedef struct {
	unsigned long ulBlockDis;		//障碍距离
	unsigned short usBlockErrCode;	//障碍异常信息
}Block_Info;	//线路障碍信息

typedef struct {
	unsigned char ucFanSta1 : 2;		//风扇状态
	unsigned char ucFanSta2 : 2;		//风扇状态
	unsigned char ucFanErrSta1 : 2;		//风扇故障信息
	unsigned char ucFanErrSta2 : 2;		//风扇故障信息
}Fan_Info;	//风扇信息

typedef struct {
	unsigned char ucDevSta;
	unsigned char ucDevErrCode;
}Rfid_Dev_Info;
	
typedef struct {
	unsigned char ucRfidCard[13];
	unsigned long ulRfidDistanse : 24;		//Rfid距离
	unsigned long ucRfidSta : 8;		//Rfid状态
	unsigned short usRfidErrSta;		//Rfid故障信息	//1:未开始检测Rfid卡
}Rfid_Info;	//风扇信息

typedef struct {
	unsigned char ucDetectSta1;			//1号霍尔状态
	unsigned char ucDetectSta2;			//2号霍尔状态
	unsigned char ucDetectErrSta1;		//1号霍尔状态
	unsigned char ucDetectErrSta2;		//1号霍尔状态
}Detect_Info;	//风扇信息

typedef struct {
	unsigned char ucLineSta;		//有线充状态
	unsigned short usErrcode;	//故障信息
}line_Info;	//有线充

typedef struct {
	unsigned long ulPlanID : 24;		//巡检计划ID
	unsigned long ucPlanSta : 8;		//巡检状态
	unsigned short usPlanErrCode;		//巡检计划异常Code
	unsigned short usTaskID;
}Plan_Info;	//巡检计划上报

typedef struct {
	unsigned short usO2Vol;		//氧气浓度
	unsigned short usCH4LEL;	//CH4 LEL浓度
	unsigned short usH2SPpm;	//H2SPPM浓度
	unsigned short usCOPpm;		//COPPM浓度
	unsigned char ucGasSta;
}Gas_Info;	//气体信息

typedef struct {
	uint8_t ucUpdateFlag : 1;	//升级标志
	uint8_t ucUpdateFinish : 1;	//升级成功标志
	uint8_t ucResv : 6;
	uint8_t ucLastPos;
	uint16_t usUpdatePagNum;	//升级包总数
	uint16_t usUpdateCurrent;	//当前升级位置
	uint32_t ulUpdatePagBit;	//升级包字节数
	uint32_t ulLastRecvTick;	//上次接收时间戳	
	uint32_t ulCRCCheck;
	uint32_t ulLastPos;
}OTA_Status;	//升级信息

typedef struct {
	uint8_t ucLiftStatus;
	uint32_t ulControlTick;
}LIFT_CONTL;


typedef struct{
	uint8_t Year;		//年份
	uint8_t Month;		//月份
	uint8_t Day;		//日期
	uint8_t Hour;		//时
	uint8_t Minute;	//分钟
	uint8_t Second;	//秒
	uint8_t WeekDay;	//周几
}Time_info;


typedef struct {
	uint32_t CheckPos;	//巡检地点	BIT0为1 BIT2为2 最大可支持32个RFID卡位置识别
	uint32_t ulStartTick;  //开始运行时间
	uint32_t ulWalkCheckTick;	//检测机器人是否堵转时间戳
	uint8_t InspectSta : 1;   //巡检状态 0:回原 1:巡检
	uint8_t WorkSta : 1;   //工作状态 0:停机状态 1:运行状态
	uint8_t WorkStart : 1;   //启动工作
	uint8_t CurrentPos : 5;	//当前位置
	uint8_t CurrentWork;	//当前工作状态
	uint8_t isNeedCharge : 1;  //是否需要充电
	uint8_t ReflashWorkTime : 1;	//刷新巡检时间标志
	uint8_t GetRealWorkTime : 2;	//正确获得最近巡检时间标志	0:未获取到巡检时间 1:已获取到巡检时间 2:今日无巡检计划
	uint8_t repeatsta : 1;			//当前计划是否为重复计划
	uint8_t sleepMode : 1;			//休眠状态 0:正常状态 1:休眠状态
	uint8_t WorkMode : 2;		//当前设备状态 0:自动模式 1:人工控制模式 2:部署模式
	uint8_t IsNeedToHome : 1;	//是否需要回原
	uint8_t IsNeedPhoto : 1;	//是否需要视觉拍照
	uint8_t StartCheck : 1;		//是否需要进行判断RFID识卡器
	uint8_t StayHomeFlag : 1;	//原点标志位
	uint8_t PosDoorFlag : 1;	//机器人位置判断 0:充电仓外 1:充电仓内
	uint8_t PosKnownFlag : 1;	//位置状态是否已知标志
	uint8_t Resv : 2;
	uint8_t CurrentPlanID[7];	//当前计划ID
	uint16_t usTaskID;
	float fSpeed;
	Time_info Worktime;
}Work_info;

typedef struct {
	float fManualSpeed;							//人工操作移动速度
	float fAutoSpeed;							//自动巡检速度
	float fAllowWorkVal;						//巡检允许电压
	float fBackCHargeVal;						//返航充电电压
	unsigned long ulSleepAfter;					//无任务下休眠时间
	unsigned long ulSleepBefore;  				//任务开始前启动时间
	unsigned char ucLowChargeHandlerWay : 1;	//人工操作下低电量机器人运动方式 0:自动返航充电 1:预警提示
	unsigned char ucResv : 7;				
}Sys_Para;


//Inspect plan
#define CFG_INSPECT_POINTS      (1000)
#define CFG_INSPECT_PLANS       (256)
#define CFG_INSPECT_RECORDS     (8)
#define CFG_PLANID_LEN          (32)

//RFID
#define CFG_RFID_LEN            (13)
#define CFG_NAME_MAXLEN         (32)

/***** MQTT server define *****/
#define MQTT_HEARTBEAT          (300) //unit: ms
#define MQTT_RECV_SIZE          (2048)
#define MQTT_SEND_SIZE			(1024)
#define NVIDIA_RECV_SIZE		(256)
#define NVIDIA_SEND_SIZE		(512)

#define MQTT_PUB_NAME  		    "sys"
#define MQTT_SUB_NAME  		    "dev"
#define MQTT_QOS                (QOS0)
#define MQTT_VER                (0)
/** breif MQTT订阅的主题
  * sys: Device send to server
  * dev: server send to device
 **/
#define MQTT_TOPTIC(pdata, len, type, key, issever) \
            snprintf((char *)pdata, len, "%s/%s/%s/%s/%s", type, PRODUCT_KEY, key, \
                (key != DEVICE_KEY ? "activate" : (!issever ? "property" : "server")), \
                (((MQTT_SUB_NAME == type && 0 != issever) || \
                (MQTT_PUB_NAME == type && !issever)) ? "pub" : "pub_reply"))
#define NVIDIA_PUB_TOPTIC       "control/robot/message"
#define NVIDIA_SUB_TOPTIC       "control/jetson/message"

typedef enum { // data config item
        // table config item
    CFG_INSPECT_RECORD = 0,
    CFG_INSPECT_PLAN,
    CFG_INSPECT_POINT,
    CFG_FLASH_START,
    CFG_PLACE_ID,
	CFG_INSPECT_ID,
	CFG_INSPECT_POS,
	CFG_INSPECT_TIME,
        // BAT
    CFG_BAT_STATE,
    CFG_BAT_ERR,
    CFG_BAT_TEMP,
    CFG_BAT_CAPACITY,
        // CHARGE
    CFG_CHARGE_STATE,
    CFG_CHARGE_ERR,
    CFG_CHARGE_TEMP,
    CFG_CHARGE_CURRENT,
    CFG_CHARGE_VOLTAGE,
    CFG_CHARGE_WARNTEMP,
    CFG_CHARGE_WARNBAT,
        // ULTRA
    CFG_ULTRA_ADC,
        // RFID
    CFG_RFID_EPC,
    CFG_RFID_POS,
	CFG_RFID_TIME,
	CFG_RFID_STATE,
	CFG_RFID_ERR,
        // LIFT
    CFG_LIFT_STATE,
    CFG_LIFT_ERR,
    CFG_LIFT_TEMP,
    //CFG_LIFT_SPEED,
    CFG_LIFT_HIGH,
    CFG_LIFT_MIN,
    CFG_LIFT_MAX,
        // WALK
    CFG_WALK_STATE,
    CFG_WALK_ERR,
    CFG_WALK_TEMP,
    CFG_WALK_SPEED,
    CFG_WALK_POS,
    CFG_WALK_CYCLE,
        // LINE
    CFG_LINE_ERR,
    CFG_LINE_POS,
        // TASK
    CFG_TASK_STATE,
        // NVIDIA
    CFG_NVIDIA_STATE,
    CFG_NVIDIA_ERR,
        // ULTRA
    CFG_ULTRA_STATE,
    CFG_ULTRA_ERR,
        // LED
    CFG_LED_STATE,
    CFG_LED_ERR,
        // FANS
    CFG_FAN_STATE,
    CFG_FAN_ERR,
        // HALL
    CFG_HALL_STATE,
    CFG_HALL_ERR,
    	// net config
    CFG_LOCAL_INFO,
        CFG_LOCAL_IP,
        CFG_LOCAL_GW,
        CFG_LOCAL_SN,
        CFG_LOCAL_PORT,
    CFG_NVIDIA_INFO,
        CFG_NVIDIA_HOST,
        CFG_NVIDIA_PORT,
    CFG_CLOUD_INFO,
        CFG_CLOUD_HOST,
        CFG_CLOUD_PORT,
    CFG_CLOUD_PKTSER,
    CFG_SYS_MODE, // OPT
        // device config
    CFG_SYS_STATE,
    CFG_SYS_ERR,
    CFG_SYS_CFG,
    CFG_SYS_SN,
    CFG_MAX
} e_CFG_dataType; // To adapt DEF_CFG_INFO

typedef enum {
    SYS_MODULE_CFG = 0,
    SYS_MODULE_BAT,
    SYS_MODULE_FAN,
    SYS_MODULE_LED,
    SYS_MODULE_BLE,
    SYS_MODULE_NET,
    SYS_MODULE_HALL,
    SYS_MODULE_LIFT,
    SYS_MODULE_WALK,
    SYS_MODULE_RFID,
    SYS_MODULE_ULTRA,
    SYS_MODULE_CHARGE,
    SYS_MODULE_NVIDIA,
    SYS_MODULE_CLOUD,  // cloud platform
    SYS_MODULE_ALL
} e_sys_module;

// 0 close, 1 sleep, 2 idle, 3 charging, 4, inspecting, 5 deploy, 9 error
typedef enum {
    SYS_STATE_PWRUP   = 0x0,
	SYS_STATE_PWRDN,  // pwrdn -> pwrup, x -> pwrdn
	SYS_STATE_SLEEP,  // idle <-> sleep
	SYS_STATE_IDLE,
	SYS_STATE_CHARGE, // pwrdn <- charge <- x
	SYS_STATE_INSPECT,// = 5 pwrdn <- inspect <- idle
	SYS_STATE_DEPLOY, // = 6 pwrdn <- deploy <- idle
    SYS_STATE_RETURN, // = 7
    SYS_STATE_LOWBAT, // = 8
    SYS_STATE_RESET,  // = 9 reset -> pwrup
    SYS_STATE_ERROR,  // =10 pwrup clear
    SYS_STATE_INITED, // =11 only one
    SYS_STATE_UPDATE, // =12
    SYS_STATE_QUERY   // =13
} e_sys_state;

typedef enum {
    SYS_ACT_INSPECT_SET = 0,
    SYS_ACT_INSPECT_EXE,
    SYS_ACT_LIFT_RETURN,
    SYS_ACT_WALK_STOP,
    SYS_ACT_SET_HIGH,
    SYS_ACT_END_INSPECT,
    SYS_ACT_SET_PLAN,
    SYS_ACT_SET_TASK,
} e_sys_act;
/****************net data define ********************/
typedef enum {
    ACT_SET_MODE,
    ACT_MAX
} e_DEV_act;

#pragma pack(1) // custom one byte align

/*To define mqtt info ---------------------------------------------------------*/
typedef struct {
    char     user[CFG_NAME_MAXLEN];   /* user name   */
    char     pwd[CFG_NAME_MAXLEN];    /* passward    */
    char     host[CFG_NAME_MAXLEN];   /* server ip   */
    uint16_t port;  /* server port */
} st_SVR_info;

typedef struct {
    uint8_t  rfid[CFG_RFID_LEN];
    uint8_t  pos[2];     // unit cm, precision 2
    uint8_t  type;       // 0x1 巡检点，0x2 充电仓，0x3 既是巡检点又是充电仓, 0x4 防火门
    uint8_t  state;      // 0 is ok, 1 is error
    uint8_t  error[2];
} st_inspect_point;

typedef struct {
    uint8_t id[CFG_PLANID_LEN];
    //uint8_t type:6;    // 0 new, 1 update, 2 delete, 3 pause, 4 valid
    uint8_t enable:1;    // 0 is pause, 1 is enable
    uint8_t force:1;     // 0 is optional, 1 is force
    uint8_t repeat:1;    // 0 one time, 1 repeat
    cron_expr cycle;     // 计划结束时间及计划执行周期时间表达式(repeat == 1 is valid)
	uint32_t start_time; // start time
	uint32_t expire_time;// expect time
} st_inspect_plan;

typedef struct {
    uint8_t id[CFG_PLANID_LEN];
    uint32_t time;
    uint32_t rusult;
} st_inspect_record;

typedef struct {
    uint32_t id:9;      // bit0 ~ 8,   rang 1 ~ 511
    uint32_t day:5;     // bit9 ~ 13,  rang 1 ~ 31
    uint32_t month:4;   // bit14 ~ 13, rang 1 ~ 12
    uint32_t year:7;    // bit18 ~ 24, rang 0 ~ 99
    uint32_t type:7;    // bit25 ~ 31, rang 0 ~ 99
} st_dev_sn;

typedef struct {
    uint8_t state;
    uint8_t error[2];
} st_dev_state;

typedef struct {
    uint8_t  id[CFG_PLANID_LEN];
    uint32_t time;
    uint16_t index;
	uint16_t nxtIndex;
} st_inspect_time;

typedef struct {
    uint16_t      magic;
	uint16_t      version; // initial is 0
    st_dev_sn     dev_sn;
    st_NET_info   local_info;
    st_SVR_info   nvidia_info;
    st_SVR_info   cloud_info;
    uint16_t      pkt_ser;
 	uint8_t       charge_warnbat; //unit %
    uint8_t       charge_warntemp;//unit °
	uint16_t 	  lift_min;       // 370 ~ 769, unit mm
    uint16_t      lift_max;
    uint8_t       lift_curpos[3]; // unit mm ?
    uint8_t       walk_curpos[3]; // unit cm
                                  // inspect pos: next + next next
    uint8_t       walk_speed[3];  // unit mm/s
    float         walk_cycle;     // mm: wheel circumference
} st_eprom_cfg;


#pragma pack()  // cancel custom byte align mothod

void sys_datetime_set(unsigned char *pdata, unsigned char num);
// type: 'H' is hex, 'B' is bcd, 'S'
int sys_datetime_fill(unsigned char *pdata, uint8_t type);
uint32_t sys_dev_sn(uint8_t *pdata, uint8_t len);

int cfg_data_init(void);
int cfg_data_match(uint8_t *pdata, uint8_t type, uint16_t len);
int cfg_item_ref(uint8_t **pdata, uint8_t type, uint16_t index);
int cfg_item_read(uint8_t *pdata, uint8_t type, uint16_t index, uint16_t len);
int cfg_item_write(uint8_t *pdata, uint8_t type, uint16_t index, uint16_t len);
int cfg_data_update(uint8_t type, float fold, float fnew);
int cfg_data_del(uint8_t *pdata, uint8_t type, uint16_t len);
int cfg_item_clear(uint8_t type, uint16_t index);

static inline int cfg_data_clear(uint8_t type)
{
    return cfg_item_write(NULL, type, 0, 0);
}

static inline int cfg_data_write(uint8_t *pdata, uint8_t type, uint16_t len)
{
    return cfg_item_write(pdata, type, 0, len);
}

static inline int cfg_data_read(uint8_t *pdata, uint8_t type, uint16_t len)
{
    return cfg_item_read(pdata, type, 0, len);
}

static inline int cfg_data_ref(uint8_t **ppdata, uint8_t type)
{
    return cfg_item_ref(ppdata, type, 0);
}

#endif
