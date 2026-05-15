/******************************************************************************
 * @brief    Socket related interface definition.
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-14     Morro        Initial version.
 ******************************************************************************/
#ifndef _SOCKET_INTERNAL_H_
#define _SOCKET_INTERNAL_H_

//#include "socket.h"

/**
 * @brief     Socket 通知类型定义.
 */
typedef enum {
    /**
     * @brief       Online (successfully connected to server.)
     */
    SOCK_NOTFI_ONLINE = 0,
    /**
     * @brief       由于外部原因造成的离线(如服务器主动断开)
     */
    SOCK_NOTFI_OFFLINE,
    /**
     * @brief       数据发送失败
     */
    SOCK_NOTFI_SEND_FAILED,
    /**
     * @brief       数据发送成功
     */
    SOCK_NOTFI_SEND_SUCCESS,
    /**
     * @brief       模组收到数据,等待读取(适用于主动读取数据的模组)
     * @param[in]   data -> [int unread_data],剩余待读取数据长度(如果未知,填0)
     */
    SOCK_NOTFI_DATA_INCOMMING,
    /**
     * @brief       模组数据上报(适用于主动上报数据的模组)
     * @param[in]   data -> [unsigned char *buf],数据缓冲区
     * @param[in]   size -> 数据长度
     */
    SOCK_NOTFI_DATA_REPORT
}e_socket_notify;

/*socket 类型 ----------------------------------------------------------------*/
typedef enum {
    SOCK_TYPE_TCP = 0x0, /* TCP */
    SOCK_TYPE_UDP        /* UDP */
} e_socket_type;

/* socket basic information -----------------------------------------------------------*/
typedef struct {
    e_socket_type        type;  /* socket type */
    unsigned char        id;    /* index, auto malloc when creating */
    unsigned short       port;  /* remote server port*/
    const char          *host;  /* remote server */
    void                *tag;   /* append data */
} st_socket_base;

/**
 * @brief  socket 通知
 */
void socket_notify(st_socket_base *s, e_socket_notify type, void *data, int size);

/**
 * @brief       为socket设置附属数据
 * @param[in]   s       - socket
 * @param[in]   tag     - 附属数据
 */
void socket_set_tag(st_socket_base *s, void *tag);

/**
 * @brief  通过id查询socket
 */
st_socket_base *socket_find_by_id(int id);

/**
 * @brief  通过tag查询socket
 */
st_socket_base *socket_find_by_tag(void *tag);

#endif
