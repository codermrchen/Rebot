#ifndef _RFID_TAST_H_
#define _RFID_TAST_H_

typedef struct {
	uint32_t ulRfidCard_ID;
	uint32_t ulRfidCard_No;
	float fDistanse;
	uint8_t ucPushFlag;	//是否需要上报 0:不上报 1:上报
}RFID_PARA;


extern uint8_t get_magnet_all_sta(void);

extern uint8_t get_magnet_sta(uint8_t ucNo);

#endif

