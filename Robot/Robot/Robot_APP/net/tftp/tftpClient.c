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
 ******************************************************************************/
#include "ril.h"
#include "ril_socket.h"
#include "comdef.h"
#include "tftp_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BLK_SIZE         512

/* TFTP 操作码 ---------------------------------------------------------------*/
#define TFTP_RRQ	      1                            /* 读请求 */
//#define TFTP_WRQ 	      2                            /* 写请求*/
#define TFTP_DATA 	      3                            /* 数据 */
#define TFTP_ACK 	      4                            /* 确认 */
#define TFTP_ERROR 	      5
#define TFTP_OACK	      6

/*TFTP信息*/
typedef struct {
    ril_socket_t   socket;   
    tftp_client_t  client;
    bool           abort;
    char           host[128];                           // 远程主机名称
    const char     *path;                               // 文件路径
    unsigned short port;                                // 远程端口        
    unsigned short max_timeout;                         // 最大超时时间s  
    unsigned short recv_cnt;                            // 接收计数
    unsigned char  retry;                               
    unsigned char  state;  
    unsigned int   timer;                               //超时定时器
    unsigned int   retry_timer;
    unsigned int   recv_bytes;                          //已接收字节数    
    unsigned int   total_bytes;                         //总字节数
    unsigned int   speed;                               //平均下载速度
    unsigned int   blknum;                              //当前包号s
    unsigned char  buf[BLK_SIZE + 4];
}tftp_info_t;

/**
 * @brief       等待TFTP应答
 * @param[in]   state - 当前状态
 * @retval      buf   - 
 */
static void dataRecvProc(tftp_info_t *info, int state, void *buf, unsigned int size)
{
    tftp_event_args_t e;
    e.client  = &info->client;
    e.state   = state;
    e.data    = buf;
    e.datalen = size;
    e.filesize= info->total_bytes;
    e.offset  = info->recv_bytes;
    e.spand_time = ril_get_ms() - info->timer;
    info->client.event(&e);  
    info->recv_bytes += size;
}

/**
 * @brief      tftp读请求
 */
static int read_request(tftp_info_t *ti)
{
    unsigned char *p = ti->buf;
    int len = 0;
    /*------------------------------------------------------------------------
        optcode    filename     mode        opt1           opt2
     -------------------------------------------------------------------------
       1    |      xxx\0 |    octet\0    |   tsize\0 | blksize\0 |    
    --------------------------------------------------------------------------*/        
    *p++ = 0;
    *p++ = TFTP_RRQ; 
    /*文件名*/
    len = snprintf((char *)p, sizeof(ti->buf) - (p - ti->buf), "%s", ti->path);
    p += len;
    *p++ = 0;
    
    /*工作模式(二进制)-------*/
    len = snprintf((char *)p, sizeof(ti->buf) - (p - ti->buf),"octet");
    p += len;
    *p++ = 0x00;
    
    //获取文件大小
    len = snprintf((char *)p, sizeof(ti->buf) - (p - ti->buf),"tsize");
    p += len;
    *p++ = 0x00;    
  
    len = snprintf((char *)p, sizeof(ti->buf) - (p - ti->buf),"blksize");
    p += len; 
    *p++ = 0x00;
    
    len = snprintf((char *)p, sizeof(ti->buf) - (p - ti->buf),"%d",BLK_SIZE);    
    *p++ = 0x00; 
    
    return ril_sock_send_async(ti->socket, ti->buf, p - ti->buf);
}

/**
 * @brief   回复确认帧
 */
static int tftp_ack(tftp_info_t *ti, unsigned short blknum)
{
    unsigned char buf[4];
    buf[0] = TFTP_ACK >> 8;                              
    buf[1] = TFTP_ACK; 
    buf[2] = blknum >> 8;
    buf[3] = blknum;
    TFTP_DBG("Ack the %d block\r\n", blknum);
    return ril_sock_send_async(ti->socket, buf, 4);
}

/**
 * @brief   解析oack帧
 */
static void parse_oack(tftp_info_t *ti)
{
    int i, j;
    char *argv[9];
    argv[0] = (char *)&ti->buf[2];
    for (i = 3, j = 1; i < ti->recv_cnt && j < 8; i++) {
        if (ti->buf[i] == '\0')
            argv[j++] = (char *)&ti->buf[++i];
    }
    for (i = 0; i < j; i++) {
        if (strcmp("tsize", argv[i]) == 0 && i < j) {
            ti->total_bytes = atoi(argv[i + 1]);
            TFTP_DBG("File size:%d\r\n", ti->total_bytes);
            return;
        }
    }
}

/**
 * @brief   帧解析处理
 */
static int tftp_data_parse(tftp_info_t *ti)
{
    unsigned short opcode;                           
    unsigned short blknum;     
    opcode = (ti->buf[0] << 8) | ti->buf[1];         /*操作码 */
    blknum = (ti->buf[2] << 8) | ti->buf[3];         /*块编号 */ 
    switch (opcode) {
    case TFTP_OACK:        
        TFTP_DBG("OACK:%s\r\n",(char *)&ti->buf[4]);
        parse_oack(ti);
        tftp_ack(ti, 0);
        ti->blknum = 1;                             /*接收第一块数据*/
        return RIL_ONGOING;
    case TFTP_DATA:
        ril_delay(10);
        tftp_ack(ti, blknum);                       /* 发送确认帧 */
        if (blknum == ti->blknum - 1) {             /* 重复上一块,直接丢弃 */                   
            return RIL_ONGOING;
        } else if (blknum == ti->blknum) {        
            /*将数据递交到上层 --------------------------------------------*/  
            dataRecvProc(ti, TFTP_STAT_DATA, &ti->buf[4], ti->recv_cnt - 4);
            if (ti->recv_bytes >= ti->total_bytes) { /*最后一块数据 -----------*/
                dataRecvProc(ti, TFTP_STAT_DONE, &ti->buf[4], 0);           
                return RIL_OK;
            } 
            ti->blknum++;
            ti->recv_bytes += ti->recv_cnt - 4;
            ti->recv_cnt    = 0;
            return RIL_ONGOING;
        }
        break;
        default:
            TFTP_DBG("TFTP ERROR:%d,%s\r\n", opcode, &ti->buf[4]);
        break; 
    }
    dataRecvProc(ti, TFTP_STAT_FAILED, &ti->buf[4], 0);
    return RIL_ERROR;  
}


/**
 * @brief   接收解析处理
 */
static int tftp_recv_process(tftp_info_t *ti)
{
    int ret = RIL_ONGOING;
    unsigned short len;
    unsigned short bufsize = sizeof(ti->buf) - ti->recv_cnt;    
    len = ril_sock_recv(ti->socket, &ti->buf[ti->recv_cnt], bufsize);
    
    if (ti->recv_cnt == 0 && ril_istimeout(ti->retry_timer, 3000)) {
        if (++ti->retry > 5) {
            TFTP_DBG("Download failed\r\n");
            return RIL_FAILED;
        }          
        tftp_ack(ti, ti->blknum - 1); 
        ti->retry_timer  = ril_get_ms();                             //3s没回重发
        TFTP_DBG("Reconfirm block %d\r\n", ti->blknum - 1);
    }
    if (len) {
        ti->recv_cnt += len;
        ti->retry_timer  = ril_get_ms();
    }
    if (ti->recv_cnt == BLK_SIZE + 4 || 
        (ti->recv_cnt && ril_istimeout(ti->retry_timer, 2000))) {
        ret          = tftp_data_parse(ti);
        ti->recv_cnt = 0;
        ti->retry_timer  = ril_get_ms();
        ti->retry    = 0;
    } else if (ril_istimeout(ti->timer, ti->max_timeout * 1000)) {  //下载超时
        ret = RIL_TIMEOUT;
        TFTP_DBG("Download timeout.\r\n", ti->blknum);
    }   
    return ret;
   
}

/**
 * @brief      创建tftp客户端
 * @param[in]  e    - 事件处理接口
 * @param[in]  host - 主机地址(www.xxx.com)
 * @param[in]  port - 端口(一般填80)
 * @return     NULL - 创建失败, 其它值 - http客户端
 */
tftp_client_t *tftp_client_create(tftp_event_t e, const char *host, 
                                  unsigned short port)
{
    tftp_info_t *info;
    info = (tftp_info_t *)ril_malloc(sizeof(tftp_info_t));
    
    if (info == NULL)
        return NULL;
    memset(info, 0, sizeof(tftp_info_t));    
    info->client.event = e;
    snprintf(info->host, sizeof(info->host), "%s", host);
    info->port        = port;       
    return &info->client;
}

/**
 * @brief      销毁tftp客户端
 */
void tftp_client_destroy(tftp_client_t *tc)
{       
    tftp_info_t *info = container_of(tc, tftp_info_t, client);
    ril_free(info);
}

/**
 * @brief      启动TFTP下载
 * @param[in]  tc      -  tftp客户端
 * @param[in]  file    -  下载文件名称(如/demo.hex)
 * @param[in]  timeout -  下载超时时间(ms)
 * @return     RIL_OK  -  下载成功, 其它值 - 下载失败
 */
int tftp_start_download(tftp_client_t *tc, const char *file, unsigned int timeout)
{
    tftp_info_t *info = container_of(tc, tftp_info_t, client);
    int ret;
    if (!ril_isonline())
        return RIL_REJECT;
    
    TFTP_DBG("Start\r\n");
    info->socket = ril_sock_create(NULL, 512);
    if (info->socket <= 0) {
        TFTP_DBG("Socket create failed...\r\n");
        return RIL_NOMEM;
    }
    info->abort       = false;
    info->path        = file;
    info->recv_cnt    = 0;
    info->recv_bytes  = 0;
    info->blknum      = 1;
    info->max_timeout =  timeout;
    ret = ril_sock_connect(info->socket, info->host, info->port, RIL_SOCK_UDP);
    if (ret != RIL_OK)
        goto END;
    TFTP_DBG("%s to connect to server.\r\n", ret == RIL_OK ? "Successfully":"Failed");                   
    if ((ret = read_request(info)) != RIL_OK)          //发送读请求命令
        goto END;
    info->retry_timer = info->timer = ril_get_ms();    
    /* 数据接收解析处理 -------------------------------------------------------*/
    do {
        ret = tftp_recv_process(info);                  
        ril_delay(10);
    } while (ret == RIL_ONGOING);
END:
    ril_sock_disconnect(info->socket);
    ril_sock_destroy(info->socket);    
    return ret;
}
/**
 * @brief      终止TFTP下载
 */
void tftp_stop_download(tftp_client_t *tc)
{
    container_of(tc, tftp_info_t, client)->abort = true;
}
