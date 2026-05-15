/******************************************************************************
 * @brief    ril socket
 *
 * Copyright (c) 2020~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-20     Morro        Initial version.
 * 2021-04-23     Morro        Fix the issue of not exiting the critical region
 *                             correctly when allocating Socket id.
 * 2021-05-04     Morro        Fix the issue of repeatedly closing the socket
 *                             when the remote host is disconnected.
 * 2021-12-08     Morro        Fix the problem of receiving abnormal data under
 *                             multitasking system
 ******************************************************************************/
#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "net.h"
#include "socket_internal.h"

#define    SOCKET_INVALID                  0

/*默认socket 接收缓冲区大小 ---------------------------------------------------*/
#define    DEF_SOCK_RECV_BUFSIZE           128           /* 默认接收缓冲区大小*/
#define    MAX_SOCK_CONN_TIME              120           /* 最大连接时间(s)*/
#define    MAX_SOCK_SEND_TIME              120           /* 最大发送时间(s)*/

/**
 * @brief socket descriptor
 */
typedef long  st_socket_desc;

/**
 * @brief socket 事件类型定义
 */
typedef enum {
    /**
     * @brief       连接状态更新(可以通过ril_sock_connstat 获取连接状态)
     */
    SOCK_EVENT_CONN = 0,
    /**
     * @brief       数据发送状态更新(可以通过ril_sock_sendstat 获取发送状态)
     */
    SOCK_EVENT_SEND,
    /**
     * @brief       数据接收事件(可以通过ril_sock_recv 接口读取数据)
     */
    SOCK_EVENT_RECV

} e_socket_event;

/**
 * @brief socket (发送/连接)状态
 */
typedef enum {
    SOCK_STAT_UNKNOW,       /* 未知 */
    SOCK_STAT_BUSY,         /* 进行中 */
    SOCK_STAT_DONE,         /* 请求完成 */
    SOCK_STAT_FAILED,       /* 请求失败 */
    SOCK_STAT_TIMEOUT,      /* 请求超时 */
    SOCK_STAT_MAX
} e_socket_status;

/* socket 相关接口 -------------------------------------------------------*/
typedef struct sock_dev_ops {
    /**
     * @brief     获取连接状态
     */
    e_socket_status (*conn_status)(uint8_t SockIndex);

    /**
     * @brief     获取发送状态
     */
    e_socket_status (*send_status)(uint8_t SockIndex);

    /**
     * @brief     获取网络连接状态
     */
    int (*isonline)(uint8_t SockIndex);

    /**
     * @brief     连接服务器
     * @attention 为了避免在这里停留时间太长，影响其它任务运行，在这里应只发送
     * 连接请求给模组，不需要在这里等待连接结果,因为RIL会定时去查询连接状态,另
     * 外，还可以通过捕获URC事件将连接结果通过ril_socket_nofity上报给RIL
     */
    int (*connect)(uint8_t SockIndex);

    /**
     * @brief     断开服务器连接
     */
    int (*disconnect)(uint8_t SockIndex);

    /**
     * @brief     发送数据
     * @attention 为了避免在这里停留时间太长，影响其它任务运行，在这里应只发送
     * 数据请求给模组，不需要在这里等待发送结果,因为RIL会定时去查询发送状态,另
     * 外，还可以通过捕获URC事件将发送结果通过ril_socket_nofity上报给RIL
     */
    int (*send)(uint8_t SockIndex, void *pbuf, unsigned int len);

    /**
     * @brief     接收数据
     * @attention 一些模组不支持主动读取socket数据,这里可以直接返回0.当收到URC
     *            主动上报来的数据之后,可以通过ril_socket_nofity递交给RIL.
     */
    int (*recv)(uint8_t SockIndex, void *pbuf, unsigned int len);
} st_socket_ops;

typedef struct 
{
    uint8_t IPAddr[4];                                           /* socket目标IP地址 32bit*/
    uint8_t MacAddr[6];                                          /* socket目标地址 48bit*/
    uint8_t ProtoType;                                           /* 协议类型 */
    uint8_t ScokStatus;                                          /* socket状态，参考scoket状态定义 */
    uint8_t TcpMode;                                             /* TCP模式 */
    uint32_t IPRAWProtoType;                                      /* IPRAW 协议类型 */
	uint32_t SendTick;											/*发送时间*/
	uint32_t RecvTick;											/*接收时间*/
	uint32_t PinqTick;											/*心跳包时间*/
    uint16_t DesPort;                                             /* 目的端口 */
    uint16_t SourPort;                                            /* 目的端口 */
    uint16_t SendLen;                                             /* 发送数据长度 */
    uint16_t RemLen;                                              /* 功能码长度 */
	uint16_t RecvLen;											/*接收数据长度*/
	uint16_t PacketID;
	uint16_t usSendMaxLen;
	uint16_t usRecvMaxLen;
	uint8_t Index;												/*socket index*/
    uint8_t *pSend;                                              /* 发送指针 */       
	uint8_t *pRecvBuf;											/*接收缓冲区*/
	uint8_t Ping : 1;											/*心跳包状态*/
	uint8_t SendSta : 1;										/*发送完成标志*/
	uint8_t RecvSta : 1;										/*接收完成标志*/
	uint8_t SendWaitFlag : 1;									/*等待发送完成标志*/
	uint8_t RecvWaitFlag : 1;									/*等待接收完成标志*/
	uint8_t NeedHandle : 1;										/*数据需要处理*/
	uint8_t TimeOutFlag : 1;									/*超时标志*/
	uint8_t Resv : 1;												/*备用*/
	int (* send)(uint8_t SockIndex, void * pbuf, unsigned int len);               /*发送函数*/
}st_socket_info;

typedef enum 
{
	SOCKET_UNINIT = 0x01,			//socket未初始化
	SOCKET_WAIT_SUCCESS = 0x02,		//socket等待连接成功
	SOCKET_CONNECTED = 0x04,		//socket成功连接
	MQTT_CONNECTED = 0x08,			//MQTT成功连接
	MQTT_SUBLISHED = 0x10,			//MQTT订阅成功
	MQTT_WAIT_RESPONSE = 0x20,		//MQTT发布等待回复
}st_socket_sta;

/**
 * @brief socket 事件
 */
typedef void (*sock_evt_handle)(st_socket_desc s, e_socket_event type);

st_socket_desc socket_create(sock_evt_handle e, unsigned int bufsize);

void socket_destroy(st_socket_desc s);

//void socket_init(st_socket_ops *ops);

int socket_connect(uint8_t SockIndex, const char *host, unsigned short port, e_socket_type type);

int socket_send(st_socket_info * socket, const void *buf, unsigned int len);

/** 非阻塞接口 ---------------------------------------------------------------*/
int socket_recv(st_socket_info * socket);

//int socket_connect_async(uint8_t SockIndex, const char *host, unsigned short port, e_socket_type type);

int socket_send_async(st_socket_info * socket, const void *buf, unsigned int len);

e_socket_status socket_sendstat(st_socket_desc s);

e_socket_status socket_connstat(st_socket_desc s);

int socket_disconnect(st_socket_desc s);

/** socket 状态相关 ----------------------------------------------------------*/
bool socket_phy_online(st_socket_desc sockfd);

bool socket_online(st_socket_desc s);

bool socket_busy(st_socket_desc s);

void socket_status_watch(void);

#endif
