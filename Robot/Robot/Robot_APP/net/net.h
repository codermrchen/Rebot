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
#ifndef _NET_H_
#define _NET_H_

#include "sys_util.h"
#include "modbus.h"

#define net_sem_t               sys_sem_t

#define net_enter_critical      sys_enter_critical
#define net_exit_critical       sys_exit_critical

#define net_istimeout           sys_istimeout
#define net_delay               sys_delay_ms
#define net_get_ms              sys_get_ms

#define net_malloc              sys_malloc
#define net_free                sys_free

#define net_sem_new             sys_sem_new
#define net_sem_free            sys_sem_free
#define net_sem_post            sys_sem_post
#define net_sem_wait            sys_sem_wait

#define NET_OK                  SYS_OK
#define NET_ERROR               SYS_ERROR
#define NET_FAILED              SYS_FAILED
#define NET_NOMEM               SYS_NOMEM
#define NET_REJECT              SYS_REJECT
#define NET_ONGOING             SYS_ONGOING
#define NET_INVALID             SYS_INVALID
#define NET_TIMEOUT             SYS_TIMEOUT

/**
 * @ingroup DATA_TYPE
 *  It used in setting dhcp_mode of @ref st_NET_info.
 */
typedef enum {
   NETINFO_STATIC = 1,    ///< Static IP configuration by manually.
   NETINFO_DHCP           ///< Dynamic IP configruation from a DHCP sever
} e_DHCP_mode;

/**
 * @ingroup DATA_TYPE
 *  Network Information for WIZCHIP
 */
typedef struct wiz_NetInfo_t {
   uint8_t mac[6];  ///< Source Mac Address
   uint8_t ip[4];   ///< Source IP Address
   uint8_t sn[4];   ///< Subnet Mask
   uint8_t gw[4];   ///< Gateway IP Address
   uint8_t dns[4];  ///< DNS server IP Address
   e_DHCP_mode dhcp;///< 1 - Static, 2 - DHCP
   uint16_t port;   ///< local port */
} st_NET_info;

/**
 * @ingroup DATA_TYPE
 *  Used in CN_SET_TIMEOUT or CN_GET_TIMEOUT of @ref wizchip_ctrl() for timeout configruation.
 */
typedef struct wiz_NetTimeout_t {
    uint8_t  retry_cnt;     ///< retry count
    uint16_t time_100us;    ///< time unit 100us
} st_NET_timeout;

#endif
