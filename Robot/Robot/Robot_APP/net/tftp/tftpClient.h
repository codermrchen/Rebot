/******************************************************************************
 * @brief    TFTP 客户端管理
 *
 * Copyright (c) 2021  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-04-12     Morro        Initial version
 *******************************************************************************
 * @details
 *
 * TFTP客户端使用流程：
 *
 * 1. 调用tftp_client_create 函数创建TFTP 客户端实例,如果成功则返回非NULL值
 *
 * 2. 调用tftp_start_download 启动下载，启动成功之后系统会通过tftp_event_t反馈当前
 *    下载状态
 *
 ******************************************************************************/
#ifndef _TFTP_CLIENT_H_
#define _TFTP_CLIENT_H_

#define TFTP_DBG(...)           RIL_INFO("TFTP "__VA_ARGS__)
#define MAX_RECV_TIMEOUT        10                     /* 单包接收超时时间s*/

/* tftp 状态 -----------------------------------------------------------------*/
#define TFTP_STAT_START          0                      /* 开始下载 */
#define TFTP_STAT_DATA           1                      /* 接收数据*/
#define TFTP_STAT_DONE           2                      /* 下载完成*/
#define TFTP_STAT_FAILED         3                      /* 下载失败*/

struct tftp_client;

/*TFTP 事件参数 ---------------------------------------------------------------*/
typedef struct {
    struct tftp_client *client;                             
    unsigned char state;                                /* 当前状态   */    
    unsigned int  filesize;                             /* 文件大小   */
    unsigned int  spand_time;                           /* 已使用时间 */    
    unsigned int  offset;                               /* 写指针偏移 */
    unsigned char *data;                                /* 数据指针   */
    unsigned int  datalen;                              /* 数据长度   */
}tftp_event_args_t;

typedef void (*tftp_event_t)(tftp_event_args_t *args); /* tftp 事件回调*/

/*TFTP 客户端 -----------------------------------------------------------------*/
typedef struct tftp_client{    
    tftp_event_t   event;                              /* TFTP事件 */
} tftp_client_t;

/*创建tftp 客户端*/
tftp_client_t *tftp_client_create(tftp_event_t e, const char *host, 
                                  unsigned short port);
/*销毁tftp 客户端*/
void tftp_client_destroy(tftp_client_t *);

/* 启动HTTP下载 */
int tftp_start_download(tftp_client_t *tc, const char *file, unsigned int timeout);
/* 终止HTTP下载 */
void tftp_stop_download(tftp_client_t *);  

#endif
