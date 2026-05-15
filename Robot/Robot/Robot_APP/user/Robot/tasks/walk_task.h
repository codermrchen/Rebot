#ifndef _WALK_TAST_H_
#define _WALK_TAST_H_

#define _WALK_BEATTIME              150

#define _WALK_HBTIME_REG            (4)   // timeout is 1000ms
#define _WALK_MOTOR_POLES           (1)
#define _WALK_POS(a, c)             (uint32_t)((a) * (c) / (10 * 36000 * _WALK_MOTOR_POLES)) // 0.01° -> cm
#define _WALK_SPEED(s)           	((int)((float)(s) * 64.7148f))//s单位为cm/s
#define _WALK_ACCELERATE(s)         (400 > (s) ? 200 : ((s) >> 1)) // unit erpm
#define _WALK_DECELERATE(s)         (800 > (s) ? 200 : ((s) >> 2)) // unit erpm
#define _WALK_RUN_MODE              _WALK_MODE_SPEED

#define _WALK_FUN_INIT              _WALK_FUN_MODE
#define _WALK_FUN_RUN               _WALK_FUN_HBREG
#define _WALK_FUN_STOP              _WALK_FUN_TGTSPEED
#define _WALK_FUN_SPEED             _WALK_FUN_TGTSPEED
#define _WALK_FUN_ACCEL             _WALK_FUN_SPEEDACCEL
#define _WALK_FUN_DECEL             _WALK_FUN_SPEEDDECEL

#define _WALK_STEP_DISTANCE			(0.003808f)

/////////////////////////////////////////////////////////////
#define _WALK_FUN_CLIMBCURRENT      0x178E
#define _WALK_FUN_CLIMBACCEL        0x178D // unit: %
#define _WALK_FUN_MAXTORQUE         0x178C // unit: mA
#define _WALK_FUN_ZEROMODE          0x178B
#define _WALK_FUN_SPEEDDECEL        0x1789
#define _WALK_FUN_CFGTBL            0x1788
#define _WALK_FUN_TRACKDECEL        0x1786
#define _WALK_FUN_TRACKACCEL        0x1784
#define _WALK_FUN_TRACKSPEED        0x1782
#define _WALK_FUN_SPEEDACCEL        0x1780
#define _WALK_FUN_HANDBRAKECURR     0x177F
#define _WALK_FUN_BRAKECURRENT      0x177E
#define _WALK_FUN_CURPOS            0x177C
#define _WALK_FUN_TGTRELCUR         0x177A
#define _WALK_FUN_TGTRELLAST        0x1778
#define _WALK_FUN_TGTABSPOS         0x1776
#define _WALK_FUN_TGTDUTY           0x1775
#define _WALK_FUN_TGTSPEED          0x1773
#define _WALK_FUN_TGTCURRENT        0x1772
#define _WALK_FUN_MODE              0x1771
#define _WALK_FUN_HBREG             0x1770
#define _WALK_FUN_REALPOS           0x1392
#define _WALK_FUN_REALTEMP          0x1390
#define _WALK_FUN_REALCURRENT       0x138E
#define _WALK_FUN_REALDUTY          0x138B
#define _WALK_FUN_REALSPEED         0x1389
#define _WALK_FUN_STATE             0x1388

#define LOCK_WALK()					do {		\
										gWalkDevInfo.ucWalkLock = 1;	\
										}while(0);
#define UNLOCK_WALK()				do {		\
										gWalkDevInfo.ucWalkLock = 0;	\
									}while(0);
#define IS_WALK_LOCK()				(gWalkDevInfo.ucWalkLock == 1)

typedef enum {
    _WALK_MODE_NONE = -1,
    _WALK_MODE_CURRENT = 0,
    _WALK_MODE_SPEED,
    _WALK_MODE_DUTY,
    _WALK_MODE_POS_ABS,
    _WALK_MODE_POS_REL,
    _WALK_MODE_POS_CUR,			//相对当前位置移动模式
    _WALK_MODE_BRAKE,
    _WALK_MODE_HANDLE, // manual brake
    _WALK_MODE_MAX
} e_walk_mode;

typedef enum {
    _WALK_ACT_NONE  = 0x0,	
    _WALK_ACT_POS   = 0x1,		//实时位置
    _WALK_ACT_MODE  = 0x2,		//模式状态
    _WALK_ACT_SPEED = 0x4,		//速度设置
    _WALK_ACT_ACCEL = 0x8,		//加速度设置
    _WALK_ACT_DECEL = 0x10,		//减速度设置
    _WALK_ACT_TEMP  = 0x20,		//温度读取标志
    _WALK_ACT_WORK  = 0x40,		//电机正在动作中标志
    _WALK_ACT_STOP  = 0x80,		//电机停止标志
    _WALK_ACT_PWRON = 0x100,	//电机供电标志
    _WALK_ACT_MOVE_POS = 0x200,	//电机相对位置移动距离
    _WALK_ACT_CYCLE = 0x400,	//
    _WALK_ACT_STATE = 0x800,	//电机状态 bit: 0:停止状态 1:运行状态
    _WALK_SET_CUR_POS = 0x1000,	//设置当前位置
} e_walk_act;

typedef struct {
    uint8_t temp[2];    // unit °C, precision 2
    uint8_t error[2];
    uint8_t state;      // 0 is forward, 1 is stop, 2 is return origin, 9 is error
} st_walk_info;

typedef struct {
    uint8_t pos[3];    // unit cm, precision 2
    uint8_t error[2];
} st_walk_line;


extern unsigned char set_walk_speed(float speed, const char * func, int line);
extern void walk_stop(void);
extern void walk_set_cur_pos(long lPos);

extern unsigned char work_forward(float fSpeed, const char * func, int line);
extern unsigned char work_backward(float fSpeed, const char * func, int line);
extern unsigned char work_forward_lock(float fSpeed, const char * func, int line);
extern unsigned char work_backward_lock(float fSpeed, const char * func, int line);

#endif	//_WALK_TAST_H_

