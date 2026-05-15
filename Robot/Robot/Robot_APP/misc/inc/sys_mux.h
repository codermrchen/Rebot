/******************************************************************************
 * @brief    基于gsm0710协议串口多路复用管理(multiplexing)
 *
 * Copyright (c) 2020 <master_roger@sina.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-05-27     Morro        Initial version. 
 ******************************************************************************/
#ifndef _MUX_H_
#define _MUX_H_

#include <stdbool.h>

#define CMUX_DBG(...) printf(__VA_ARGS__)

#define MAX_MUX_FRAME_SIZE               512

//帧类型
#define MUX_SABM          0x2F     //通道建立      
#define MUX_UA            0x63     //连接/断开响应
#define MUX_DM            0xF      
#define MUX_DISC          0x43     //断开连接
#define MUX_UIH           0xEF     //数据帧
#define MUX_UI            0x3 

typedef struct {
    //读写接口
    unsigned int (*write)(const void *buf, unsigned int size);
    unsigned int (*read)(void *buf, unsigned int size);
    //系统定时器(ms)
    unsigned int (*get_tick)(void); 
    //接收事件
    void         (*recvEvent)(int channel, unsigned char type, const void *buf, 
                              int size);
}mux_adater_t;

/*mux 适配器 -----------------------------------------------------------------*/
typedef struct {
    const mux_adater_t   *adt;
    unsigned short len;
    unsigned short recvcnt;
    unsigned short offset;               //数据域偏移
    unsigned char  state;                //解析状态        
    unsigned int   timer;      
    unsigned char  data[MAX_MUX_FRAME_SIZE];
}mux_obj_t;

void mux_init(mux_obj_t *obj, const mux_adater_t *adt);

bool mux_send_frame(mux_obj_t *obj, int channel, unsigned char type, 
                    const char *buf, int size);

bool mux_open_channel(mux_obj_t *obj, int channel);

bool mux_close_channel(mux_obj_t *obj, int channel);
                    
void mux_process(mux_obj_t *obj);

#endif



