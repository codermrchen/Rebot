/******************************************************************************
 * @brief    HTTP 客户端管理
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-20     Morro        Initial version
 *******************************************************************************
 * @details
 *
 * HTTP客户端使用流程：
 *
 * 1. 调用http_client_create 函数创建HTTP 客户端实例,如果成功则返回非NULL值
 *
 * 2. 调用http_start_download 启动下载，填写下载文件名称,下载超时时间,启动成功之后
 *    系统会通过http_event_t反馈当前下载状态
 *
 ******************************************************************************/
#ifndef _HTTP_CLIANTE_H_
#define _HTTP_CLIANTE_H_

#define HTTP_DBG(...)           RIL_INFO("HTTP "__VA_ARGS__)

#define MAX_HTTPBUF_SIZE        1500                   /* http 包缓冲区*/
 
#define MAX_RECV_TIMEOUT        30                     /* 单包接收超时时间s*/

#define MAX_HTTP_REQUEST_SIZE   (50 * 1024)            /* 每次最大请求数据长度*/

/* http 状态 -----------------------------------------------------------------*/
#define HTTP_STAT_START          0                      /* 开始下载 */
#define HTTP_STAT_DATA           1                      /* 接收数据*/
#define HTTP_STAT_DONE           2                      /* 下载完成*/
#define HTTP_STAT_FAILED         3                      /* 下载失败*/

struct http_client;

/*HTTP 事件参数 ---------------------------------------------------------------*/
typedef struct {
    struct http_client *client;                             
    unsigned char state;                                /* 当前状态   */
    unsigned int  filesize;                             /* 文件大小   */
    unsigned int  spand_time;                           /* 已使用时间 */    
    unsigned int  offset;                               /* 写指针偏移 */
    unsigned char *data;                                /* 数据指针   */
    unsigned int  datalen;                              /* 数据长度   */
}http_event_args_t;

typedef void (*http_event_t)(http_event_args_t *args); /* http 事件回调*/

/*HTTP 客户端 -----------------------------------------------------------------*/
typedef struct http_client{    
    http_event_t   event;                               /* HTTP事件 */
}http_client_t;

/*创建http 客户端*/
http_client_t *http_client_create(http_event_t e, const char *host, 
                                  unsigned short port);
/*销毁http 客户端*/
void http_client_destroy(http_client_t *);

/* 启动HTTP下载 */
int http_start_download(http_client_t *hc, const char *filename, 
                        unsigned int timeout);
/* 终止HTTP下载 */
void http_stop_download(http_client_t *);               

#endif
