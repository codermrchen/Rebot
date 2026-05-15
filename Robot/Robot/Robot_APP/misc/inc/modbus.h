/******************************************************************************
 * @brief    Modbus rtu protocol interface
 *
 * Copyright (c) 2024, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-20     Worker       Initial version
 ******************************************************************************/
#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "bsp.h"

/* system interface  ---------------------------------------------------------------*/
#define MODBUS_read             (MODBUS_opt)BSP_uart_read
#define MODBUS_write            (MODBUS_opt)BSP_uart_write

/*******************************
 * @brief task define
 *******************************/
#define MODBUS_TASK_INTVERAL    (0)
#define MODBUS_TASK_STACK       (256) // >= 256
#define MODBUS_TASK_PRI         (5)

#define MODBUS_TIMEOUT_CODE     (0xFF)

#define MODBUS_JUNLEI_MAXLEN    (64)

typedef int (*MODBUS_opt)(void *pobj, void *pbuf, unsigned short len);

typedef enum {
    DEV_STATE_PWROFF  = 0x00,
    DEV_STATE_PWRON   = 0x01, // disconnected
    DEV_STATE_READY   = 0x02, // connected
    DEV_STATE_BUSY    = 0x04, // working
    DEV_STATE_SWITCH  = 0x08,
    DEV_STATE_INITED  = 0x10,
    DEV_STATE_WAITRSP = 0x20,
    DEV_STATE_RESTART = 0x40,
    DEV_STATE_SLAVE   = 0x80
} e_dev_status;

typedef enum {
    PROTOC_UP = 0,
    PROTOC_DWN,
    PROTOC_ACTIVE = 0,
    PROTOC_PASSIVE,
    PROTOC_END = 0,
    PROTOC_NOEND,
    PROTOC_NOREPLY = 0,
    PROTOC_REPLY,
    PROTOC_lISTEN
} e_junlei_protocol;

typedef enum {
	DATA_TEMP_LOW = 0x01,
	DATA_TEMP     = 0x02, // 2 byte, x 100, unit °C
	DATA_DISTANCE = 0x40, // 3 byte, x 100, unit CM
	DATA_CH4	  = 0x49,
	DATA_RSSI     = 0x60, // 2 byte
	DATA_CO		  = 0x61,
	DATA_CAPAC    = 0x64, // 2 byte
	DATA_SPEED    = 0x6C, // 3 byte, x 1000, unit M/s
	DATA_STATE    = 0x70, // 1 byte(battery: 0 charge, 1 discharge, 9 error)
    DATA_ERROR    = 0x71,
    DATA_NUMBER   = 0x72,
	DATA_DEV_SN   = 0xB1, //
	DATA_PROPERTY,
	DATA_RFID_ID,
} e_junlei_data;

typedef enum {
    CMD_TRANSPORT  = 0x11, // Transparent transmission message
    CMD_TEST       = 0x30, // test packet
    CMD_PUB			= 0xB2,	//上传
} e_junlei_command;

typedef enum {
    PKT_TYPE_TL,
    PKT_TYPE_TLV,
    PKT_TYPE_RTU,
    PKT_TYPE_BIN,
    PKT_TYPE_ASCII
} e_MODBUS_pktType;

typedef enum {
    RTU_READ    = 0x03, // read hold register
    RTU_READS   = 0x04, // read input register
    RTU_WRITE   = 0x06, // write sigle register
    RTU_WRITES  = 0x10, // write multiregister
    RTU_RSP_FLAG= 0x80
} e_RTU_func;

#pragma pack(1) // custom one byte align

typedef struct {
    unsigned char  fun_code;
    unsigned short sub_code;
    unsigned short retries:3;  // 0 ~ 7
    unsigned short timeout:13; // 0 ~ 8192ms
    unsigned int   snd_tick;
    void     *psndobj;
} st_MODBUS_response;

/**/
typedef struct {
    void *pobj;
    unsigned char id;
    unsigned char state;
    unsigned char pkt_type:6;   //e_MODBUS_pktType
    unsigned char tail_len:2;
    unsigned char head_len:4;
    unsigned char is_ignore:1;
    unsigned char crc_size:3;   //example:2(size)
    unsigned int  crc_chk;      //example:0x0000(check value)
    unsigned int  crc_poly;     //example:0xA001(polynomial)
    unsigned int  crc_val;      //example:0xFFFF(preset value)
    unsigned char *phead;
    /**
     * @brief 通信数据(串口)读接口
     */
    MODBUS_opt read;//nt (*read) (void *pobj, void *pbuf, unsigned short len);
    /**
     * @brief 通信数据(串口)写接口
     */
    MODBUS_opt write;//int (*write)(void *pobj, const void *pbuf, unsigned short len);
} st_MODBUS_dev;

typedef struct {
    //unsigned char type; //e_URC_type
    st_MODBUS_dev dev_info;
    st_MODBUS_response rsp;
    int (* dev_rsp_proc)(unsigned char func, unsigned char *pdata, unsigned short num);
} st_MODBUS_packet;

typedef struct {
	unsigned char len[2];
	    // protocol id
	unsigned char ver  :3;	 // 0 ~ 3
	unsigned char reply:2;   // 0 no, 1 reply, 2 listen, 3 reserve
	unsigned char end  :1;   // 0 is end, 1 isn't end
	unsigned char type :1;   // 0 active, 1 passive
	unsigned char dir  :1;
	unsigned char nodeid[4];
	unsigned char serno[2];
	unsigned char cmd;
	unsigned char data[2];  // data + checksum
} st_MODBUS_junlei;

typedef struct {
	unsigned char id;   //标识(0xB1)
	unsigned char len;  //长度
	unsigned char chn;  //通道1or2
	unsigned char data[0];
} st_junlei_data;

/*
 * @brief: Agreement agreement
 *      crc: Low in the front, high in the back
 *           polynomial = 0xA001, preset_value = 0xFFFF, check_value = 0
 *      util-bytes data: High in the front, low in the back
 */
typedef struct { // motor
    uint8_t  dev_id;
    uint8_t  func;
    uint8_t  data[0];
    uint16_t crc;
} st_MODBUS_rtu;

typedef struct { // battery
    uint8_t  soi;
    uint8_t  addr;
    uint8_t  cmd;
    uint8_t  data[0];
    uint8_t  crc;
    uint8_t  eoi;
} st_MODBUS_ascii;

typedef struct { // rfic
    uint8_t  num;
    uint8_t  dev_id;
    uint8_t  cmd;
    uint8_t  data[0];
    uint16_t crc;
} st_MODBUS_lv;

typedef struct { //func_code = 0x1/0x3/0x4/5/6
    uint16_t reg_addr;
    uint16_t reg_info;
    uint8_t  data[0];
} st_MODBUS_req;

typedef struct { //func_code = 0x6
    uint16_t reg_addr;
    uint16_t reg_num;
    uint8_t  data_len;
    uint8_t  data[0];
} st_MODBUS_multiWriteReq;

typedef struct { //func_code = 0x6
    uint8_t  data_num;
    uint8_t  data[0];
} st_MODBUS_data;

typedef struct { //func_code = 0x6
    uint16_t  data_num;
    uint8_t  data[0];
} st_MODBUS_data1;

#pragma pack()  // cancel custom byte align mothod

int MODBUS_send(st_MODBUS_dev *pdev, unsigned char *pdata, unsigned short num);

unsigned int MODBUS_crc_gen(st_MODBUS_dev *pdev, unsigned char *pdata, unsigned short num);

int MODBUS_junlei_databuild(uint8_t *pchn, uint8_t *pdst, uint8_t *pdata,
                        uint16_t unit, uint8_t count, uint8_t id, uint8_t space);

int MODBUS_junlei_framebuild(st_MODBUS_junlei *pframe, uint16_t len, uint8_t cmd,
                        uint8_t rsp, uint16_t serno, uint32_t nodeid);

void MODBUS_process(st_MODBUS_packet *ppkt);

#endif
