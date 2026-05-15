/******************************************************************************
 * @brief    环形缓冲区管理(参考linux/kfifo)
 *
 * Copyright (c) 2016~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-05-30     Morro        Initial version
 * 2021-02-05     Morro        增加空闲空间获取接口.
 ******************************************************************************/
#include "ringbuffer.h"

#define min(a, b)   ((a) < (b)) ? (a) : (b)
#ifndef NULL
#define NULL        ((void *)0)
#endif

static st_ring_buf *_ringbuf_head = NULL;

static int _findPowerOf2(int num)
{
    int tmp = num - 1;

    tmp |= tmp >> 1;
    tmp |= tmp >> 2;
    tmp |= tmp >> 4;
    tmp |= tmp >> 8;
    tmp |= tmp >> 16;

    return (0 > tmp ? 1 : (tmp + 1));
}

static void _datacopy(unsigned char *pdst, unsigned char *psrc, int num)
{
    int i;

    for (i = 0; num > i; i++) {
        pdst[i] = psrc[i];
    }
}

int ring_buf_init(st_ring_buf *r, unsigned char *pbuf, unsigned short size)
{
    if (NULL != r) {
        st_ring_buf *t;

        for (t = _ringbuf_head; NULL != t && t != r; t = t->next);
        if (NULL == t) {
            r->next = _ringbuf_head;
            _ringbuf_head = r;
        }

        r->front = r->rear = 0;
        r->lock = r->tmp = 0;
        if ((void *)r == (void *)pbuf) {
            size = sizeof(*r) >= size ? 0 : (size - sizeof(*r));
            pbuf += sizeof(*r);
        }

        if (NULL != pbuf && 0 < size) {
            r->pbuf = pbuf;
            r->size = _findPowerOf2(size);
            if (size < r->size) {
                r->size >>= 1;
            }
        }
        else {
            r->pbuf = NULL;
            r->size = 0;
        }

        return 0;
    }
    return  -1;
}

int ring_buf_put(st_ring_buf *r, unsigned char *pdata, unsigned short len)
{
    if (NULL != r && NULL != pdata && 0 < len) {
        if (NULL == r->pbuf) {
            r->tmp  = 1;
            r->pbuf = pdata;
            r->size = _findPowerOf2(len);
            r->rear = len;
            r->front= 0;
            r->update = 1;
            return len;
        }
        else if (0 < r->size) {
            unsigned short left = r->size + r->front - r->rear;
            unsigned short i, j = (r->rear & r->size - 1);

            len = min(len, left);
            i   = min(len, r->size - j);
            _datacopy(&r->pbuf[j], pdata, i);
            _datacopy(r->pbuf, &pdata[i], len - i);
            r->last = r->rear;
            r->rear += len;
            r->update = 1;
            return len;
        }
    }
    return 0;
}

int ring_buf_get(st_ring_buf *r, unsigned char *pdata, unsigned short len)
{
    if (NULL != pdata && 0 < len && NULL != r && NULL != r->pbuf && 0 < r->size) {
        unsigned short left = r->rear - r->front; //left
        unsigned short i, j = (r->front & r->size - 1);

        len = min(len, left);
        i   = min(len, r->size - j);
        _datacopy(pdata, &r->pbuf[j], i);
        _datacopy(&pdata[i], r->pbuf, len - i);
        r->front += len;
        if (r->front == r->rear) {
            r->front = r->rear = 0;
            r->update = 0;
            if (0 != r->tmp) { // temp
                r->pbuf = NULL;
                r->size = 0;
                r->tmp  = 0;
            }
        }
        return len;
    }
    return 0;
}

int ring_buf_getdata(st_ring_buf *r, unsigned char **ppdata, unsigned short len)
{
    if (NULL != ppdata && NULL != r && NULL != r->pbuf && 0 < r->size) {
        unsigned short j = (r->front & r->size - 1);

        if (0 != r->lock && *ppdata == &r->pbuf[j]) {
            r->front += len;
            if (r->front == r->rear) { // temp
                r->front = r->rear = r->last = 0;
                if (0 != r->tmp) {
                    r->pbuf = NULL;
                    r->size = 0;
                    r->tmp  = 0;
                }
            }
            r->lock = 0;
            //return len;
        }

        if (0 != r->update) {
            if (r->rear == r->last) {
                unsigned short left = r->rear - r->front; //left
                unsigned short i;

                if (!len) {
                    len = r->size;
                }

                len = min(len, left);
                i   = min(len, r->size - j);
                if (0 < i) {
                    len = i;
                }
                else {
                    j = 0;
                }

                *ppdata = &r->pbuf[j];
                r->update = 0;
                r->lock = 1;
                len = i;
                return len;
            }
            else {
                r->last = r->rear;
            }
        }
    }
    return 0;
}

int ring_buf_isallempty(void)
{
    st_ring_buf *pnode;

    for (pnode = _ringbuf_head; NULL != pnode; pnode = pnode->next) {
        if (pnode->front != pnode->rear) {
            return 0;
        }
    }
    return 1;
}

int ring_buf_istimeout(st_ring_buf *r, unsigned short timeout)
{
    int pos, i;

    for (pos = 0, i = timeout; (0 != r->tmp || ring_buf_isfull(r)) && 0 < i; i--) {
        if (pos != r->front) {
            pos = r->front;
            i = timeout;
        }
    }
    return (0 < i ? 0 : 1);
}

/*
 *@brief      获取环形缓冲空闲空间
 *@retval     空闲空间
 */
int ring_buf_free_space(st_ring_buf *r)
{
    if (NULL == r || NULL == r->pbuf || 0 == r->size) {
        return 0;
    }

    return r->size - (unsigned short)(r->rear - r->front);
}

int ring_buf_isfull(st_ring_buf *r)
{
    return (NULL != r && NULL != r->pbuf && 0 < r->size && (r->rear - r->front) == r->size);
}

int ring_buf_isempty(st_ring_buf *r)
{
    return (NULL == r || r->front == r->rear);
}

void ring_buf_clear(st_ring_buf *r)
{
    if (NULL != r && 0 == r->lock && 0 == r->tmp) {
        r->front = r->rear = r->lock = 0;
    }
}

