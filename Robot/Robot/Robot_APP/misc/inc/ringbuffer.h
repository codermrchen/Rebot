/******************************************************************************
 * @brief    环形缓冲区管理(参考linux/kfifo)
 *
 * Copyright (c) 2016~2021, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-25     worker       Initial version.
 ******************************************************************************/

#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Loop buffer manager */
typedef struct {
     void *next;
     unsigned short update:1;   /* data update flag */
     unsigned short front:15;   /* data head position */
     unsigned short rear;       /* data tail position */
     unsigned short last:15;    /* last tail position*/
     unsigned short tmp:1;      /* empty is release */
     unsigned short size:15;    /* buffer size */
     unsigned short lock:1;     /* data read lock */
     unsigned char *pbuf;       /* buffer area */
} st_ring_buf;

int ring_buf_init(st_ring_buf *r, unsigned char *pbuf, unsigned short size);

int ring_buf_put(st_ring_buf *r, unsigned char *pdata, unsigned short len);

int ring_buf_getdata(st_ring_buf *r, unsigned char **ppdata, unsigned short len);

int ring_buf_get(st_ring_buf *r, unsigned char *pdata, unsigned short len);

int ring_buf_istimeout(st_ring_buf *r, unsigned short timeout);

int ring_buf_free_space(st_ring_buf *r);

int ring_buf_isallempty(void);

int ring_buf_isempty(st_ring_buf *r);

int ring_buf_isfull(st_ring_buf *r);

void ring_buf_clear(st_ring_buf *r);


#ifdef __cplusplus
}
#endif

#endif
