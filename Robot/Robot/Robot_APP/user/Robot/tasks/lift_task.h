#ifndef _LIFT_TAST_H_
#define _LIFT_TAST_H_

typedef enum {
    _ACT_RUN_OPT  = 0x00,
    _ACT_SET_HIGH = 0x02,
    _ACT_RUN_OBJ  = 0x04,
    _ACT_GET_STAT = 0x10,
    _ACT_GET_ERR  = 0x12,
    _ACT_GET_HIGH = 0x14,
    _ACT_GET_DIST = 0x16,
    _ACT_RESET = 0x03,
    _ACT_MAX
} e_LIFT_act;

typedef enum {
    Lift_None = 0,
	Lift_SetHigh,
	Lift_GetHigh,
} e_LIFT_sta;

typedef enum {
    Lift_IDLE = 0,
	Lift_RISE_WAIT,
	Lift_FALL_WAIT,
	Lift_STOP_WAIT,
	Lift_ERROR_STA,
} LIFT_shake;



typedef struct {
    uint8_t temp[2];    // unit °„C, precision 2
    uint8_t error[2];
    uint8_t state;      // 0 is ok, 9 is error
} st_lift_info;

#define LIFT_DEFAULT_HIGH	370

extern void lift_high_pos(uint16_t high);

#endif

