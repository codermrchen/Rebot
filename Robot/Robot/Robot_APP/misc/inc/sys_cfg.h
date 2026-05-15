/******************************************************************************
 * @brief    系统配置文件
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_

#include "sys_util.h"

/*******************************
 * @brief urc interface
 *******************************/
#define sys_urc_register(module, handler)\
                USED ANONY_TYPE(const st_sys_urc, __urc_##handler)\
                        SECTION("_section.sys.urc.1") UNUSED  =  \
                        {module, handler};

#define sys_abs(n)                  ((0 > (n)) ? -(n) : (n))
#define sys_mktime                  mktime
#define sys_atof                    atof
#define sys_atoi                    atoi

//#define USING_FATFS
#define USING_LITTLEFS
#define CFG_PATH_MAXLEN             (32)

#define CFG_DATA_FLOAT              (0x1 << 13)
#define CFG_DATA_SIGN               (0x1 << 14)
#define CFG_DATA_BIGEND             (0x1 << 15)
#define CFG_FILE_MAXNUM             (DATA_TYPE_FILE4 - DATA_TYPE_FILE)

#define OBJ_FLAG_BYTES(n)           (1 < n ? (((n) + 7) >> 3) : 0)
#define OBJ_ARRAY_NUM(obj)              (sizeof(obj)/sizeof(obj[0]))
#define CFG_BITMAP_SIZE(n)          (((sizeof(st_bitmap_info) + (((n) + 7) >> 3) + 3) >> 2) << 2)

#define CFG_DATA_STATE_WRITE(t, i)  (((t) << 8) | (((i) & 0x7F) << 2))
#define CFG_DATA_STATE_READ(t, i)   (CFG_DATA_STATE_WRITE(t, i) | 0x1)

#define CFG_INVALID_ADDR            ((uint32_t)-1)
// one data section less than 1024字节
#define CFG_DATA_ADDR(base, type, byte, size)	\
	            (((base) << 11) | (((size) & 0x7f) << 4) | (((byte) & 0x1) << 3) | ((type) & 0x7))

typedef enum {
    DATA_TYPE_MEMORY = 0,
    DATA_TYPE_EPROM,
    DATA_TYPE_FLASH,
    DATA_TYPE_FILE,
    DATA_TYPE_FILE2,
    DATA_TYPE_FILE3,
    DATA_TYPE_FILE4,
    DATA_TYPE_FILE5,
    DATA_TYPE_MASK = DATA_TYPE_FILE5
} e_data_type;

typedef enum {
	PROP_WIFI = 0x01,
	PROP_BAT,
	PROP_LIFT,
	PROP_WALK,
	PROP_NVIDIA,     // 0x05
	PROP_RFID_DEV,
	PROP_RFID_CARD,
	PROP_LED,
	PROP_ULTRASONIC,
	PROP_HALL_SWITCH,// 0x0A
	PROP_WIRELESS_CHARGE,
	PROP_MODE,
	PROP_STATE,
	PROP_LINE,
	PROP_FANS,       // 0x0F
	PROP_TASK,
	PROP_ACTIVE,     // 0x11
	PROP_PINQ,		//0x12 心跳包
	PROP_TIME,		//0x13 获取时间
	PROP_TEMP,		//0X14 环境温湿度
	PROP_LINE_CHARGE,		//有线充
	PROP_PLAN,		//0X16 巡检计划
	PROP_PARA,		//0x17 系统参数
	PROP_GAS,		//0x18 气体
} e_dev_property;

typedef enum {
	PLAN_ADD = 0x00,
	PLAN_DEL,
	PLAN_CHANGE,
	PLAN_PAUSE,
	PLAN_RESUME,
	PLAN_START,
	PLAN_FINISH,
	PLAN_ERR = 0x09,
}plan_motion_type;

typedef struct {
    uint32_t type:3;  // e_data_type
    uint32_t bytes:1; // byte stream
    uint32_t size:7;  // 0 ~ 128
    uint32_t base:21; // 2M
} st_cfg_addr;

#pragma pack(1) // custom one byte align
typedef struct {
    uint8_t read:1;
    uint8_t index:7;
    uint8_t type;
} st_cfg_cmd;

/* bitmap info */
typedef struct {
    uint8_t   size;
    uint8_t   unit; // 0 ~ 256
    uint16_t  num;  // 1 ~ 10000
    uint8_t   bit_map[0];
} st_bitmap_info;

/*(Unsolicited Result Codes (URCs))处理项 ------------------------------------*/
typedef struct {
    /**
     * @brief function code
     */
    const uint8_t module; // e_sys_module
    /**
     * @brief       urc处理程序
     * @params      ctx - URC 上下文
     */
    int (*handler)(uint16_t type, uint8_t *pdata, uint16_t num);
} st_sys_urc;

#pragma pack()  // cancel custom byte align mothod

int sys_urcProc(uint8_t module, uint16_t type, void *pdata, uint16_t len);

////////////////////////////
int sys_memcmp(uint8_t *pdst, uint8_t *psrc, uint16_t num);
int sys_memcpy(uint8_t *pdst, uint8_t *psrc, uint16_t num);

char *sys_strstr(char *str1, char *str2);
int sys_strchr(const char *str, char c);
int sys_str2float(const char *str, float *pval);
int sys_str2ipnum(const char *s, int *n);
int sys_str2num(const char *s, int * n);
int sys_hexstr2num(const char *s, int * n);

int sys_str2byte(char *str, unsigned char *pdata, char fmt, unsigned char unit, unsigned short num);
int sys_byte2str(char *str, unsigned char *pdata, char *pfmt, unsigned short num);

int sys_hex2str(char *str, uint8_t *pdata, uint16_t num);
int sys_str2hex(char *str, uint8_t *pdata, uint16_t num);

uint32_t sys_hex2bcd(uint32_t val, uint8_t num);
uint32_t sys_bcd2hex(uint32_t val, uint8_t num);

int sys_num2ascii(uint32_t val, uint8_t *pdata, uint8_t num);
int sys_num2byte(uint32_t val, uint8_t *pdata, uint8_t num);
int sys_num2hbyte(uint32_t val, uint8_t *pdata, uint8_t num);
uint32_t sys_byte2num(uint8_t *pdata, uint16_t num);

int sys_hex2ascii(uint8_t * Out, uint8_t *Input, int num);

int sys_data_hton(uint8_t *pdata, uint16_t num);

char sys_ascii2num(unsigned char ucAscii);

////////////////////////////
/** brief
 ** Little endition storage
 ** if isn't enough, before padding zero
 **/
int sys_cfg_read(uint32_t addr, uint8_t *pdata, uint16_t len);
int sys_cfg_write(uint32_t addr, uint8_t *pdata, uint16_t len);
int sys_cfg_init(void);

static inline uint16_t sys_htons(uint16_t num)
{
    sys_data_hton((uint8_t *)&num, sizeof(num));
    return num;
}

static inline uint32_t sys_htonl(uint32_t num)
{
    sys_data_hton((uint8_t *)&num, sizeof(num));
    return num;
}

#endif
