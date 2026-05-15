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
#include "comdef.h"
#include "modbus.h"
#include "platform.h"

#ifndef NULL
#define NULL    (void *)(0)
#endif

#if 0
#define CRC_SHIFT   (12)
typedef enum {
    CRC16_MODBUS = 0x0,
    CRC16_CCITT,
    CRC32_MODBUS = 0x8,
    CRC8_CHKSUM  = 0xA,
    CRC_NONE = 0xF
} e_CRC_type;
typedef struct {
    unsigned char size;
    unsigned int poly;
    unsigned int val;
    unsigned int check_val;
} st_CRC_info;

static unsigned short _crc_build(st_CRC_info *pcrc, unsigned short num)
{
    uint8_t crc_type = (num >> CRC_SHIFT);

    num ^= (crc_type << CRC_SHIFT);
    if (CRC8_CHKSUM == crc_type) {
        pcrc->size = 1;
        pcrc->poly = 0x0;
        pcrc->val  = 0x0;
        pcrc->check_val = 0xFF;
    }
    else if (CRC32_MODBUS == crc_type) {
        pcrc->size = 4;
        pcrc->poly = 0xEDB88320L; //Inversion bit sequence of 0x04C11DB7(MODBUS)
        pcrc->val  = 0xFFFFFFFF;
        pcrc->check_val = 0xFFFFFFFF;
    }
    else if (CRC16_CCITT == crc_type) {
        pcrc->size = 2;
        pcrc->poly = 0x8408; //Inversion bit sequence of 0x1021(CCITT)
        pcrc->val  = 0x0;
        pcrc->check_val = 0x0;
    }
    else {
        pcrc->size = 2;
        pcrc->poly = 0xA001; //Inversion bit sequence of 0x8005
        pcrc->val  = 0xFFFF;
        pcrc->check_val = 0x0;
    }
    return num;
}
#endif

static int _MODBUS_check(st_MODBUS_dev *pdev, unsigned char *pdata, unsigned short num)
{
    unsigned int is_ascii = (PKT_TYPE_ASCII == pdev->pkt_type ? 1 : 0);
    unsigned int tmp_val = 0;
    unsigned int ncrc;

    for (num -= (pdev->crc_size << is_ascii); 0 < num; num--) {
        ncrc = MODBUS_crc_gen(pdev, pdata, num);
        if (0 != is_ascii) { //get crc value, low at front
            sys_str2hex((char *)&pdata[num], (unsigned char *)&tmp_val, pdev->crc_size);
        } else {// htons/htonl
            sys_num2hbyte(*(unsigned int *)&pdata[num], (unsigned char *)&tmp_val, pdev->crc_size);
        }

        if (ncrc == tmp_val) { // match crc
            num += (pdev->crc_size << is_ascii);
            break;
        }
    }
    return num;
}

static int _MODBUS_sendAscii(st_MODBUS_dev *pdev, unsigned char *pdata, unsigned short num)
{
    unsigned char nhead = (NULL == pdev->phead ? 0 :pdev->head_len);
    unsigned char ntail = (NULL == pdev->phead ? 0 : pdev->tail_len);
    int ret = nhead + ntail + pdev->crc_size;

    if (ret < num) {
        unsigned char *ptail = &pdata[num - 1];
        unsigned char tmp_buf[num << 1];
        unsigned int  ncrc;
        int i;

        for (i = 0; nhead > i; i++) {
            tmp_buf[i] = pdata[i];
        }

        num -= i + ntail + pdev->crc_size;
        i = sys_hex2str((char *)&tmp_buf[i], &pdata[i], num);
        ncrc = MODBUS_crc_gen(pdev, &tmp_buf[nhead], i);
        sys_num2hbyte(ncrc, &pdata[(i >> 1) + nhead], pdev->crc_size);

        i += nhead;
        i += sys_hex2str((char *)&tmp_buf[i], (unsigned char*)&ncrc, pdev->crc_size);
        while (0 < ntail--) {
            tmp_buf[i++] = *ptail++;
        }

        ret = pdev->write(pdev->pobj, tmp_buf, i);
        if (0 <= ret) {
            return i;
        }
    }
    return -1;
}

int MODBUS_send(st_MODBUS_dev *pdev, unsigned char *pdata, unsigned short num)
{
    int ret = -1;

    if (NULL != pdev && NULL != pdev->write && NULL != pdata && 0 < num) {
        if (pdev->state & DEV_STATE_WAITRSP) {
            sys_delay_ms(10);
        }
        else {
            if (0 < pdev->crc_size && pdev->crc_size < num) {
                if (PKT_TYPE_ASCII == pdev->pkt_type) {
                    return _MODBUS_sendAscii(pdev, pdata, num);
                }
                else {
                    unsigned int ncrc;

                    num -= pdev->crc_size;
                    ncrc = MODBUS_crc_gen(pdev, pdata, num);
                    num += sys_num2hbyte(ncrc, &pdata[num], pdev->crc_size);
                }
            }

            ret = pdev->write(pdev->pobj, pdata, num);
            if (0 <= ret) {
                if (!(pdev->state & DEV_STATE_SLAVE)) {
                    pdev->state |= DEV_STATE_WAITRSP;
                }
                return num;
            }
        }
    }
    return ret;
}

/** brief default is modbus-crc16
 **/
unsigned int MODBUS_crc_gen(st_MODBUS_dev *pdev, unsigned char *pdata, unsigned short num)
{
    unsigned char crc_size = (!pdev ?  0x2: pdev->crc_size);
    unsigned int crc_chk   = (!pdev ?  0x0: pdev->crc_chk);
    unsigned int crc_poly  = (!pdev ?  0xA001: pdev->crc_poly);
    unsigned int ncrc = (!pdev ?  0xFFFF: pdev->crc_val);
    unsigned short i, j;

    if (0 != crc_poly) { // crc check
        for (i = 0; num > i; i++) {
            ncrc ^= pdata[i];
            for (j = 0; 8 > j; j++) {
                if (0 != (ncrc & 0x1)) {
                    ncrc = (ncrc >> 1) ^ crc_poly;
                }
                else {
                    ncrc >>= 1;
                }
            }
        }
    }
    else { // checksum
        for (i = 0; num > i; i++) {
            ncrc += pdata[i];
        }
    }

    ncrc ^= crc_chk;
    ncrc &= ((0x1 << (crc_size << 3)) - 1);
    return ncrc;
}

int MODBUS_junlei_databuild(uint8_t *pchn, uint8_t *pdst, uint8_t *pdata,
                    uint16_t unit, uint8_t count, uint8_t id, uint8_t space)
{
    st_junlei_data *pitem;
    uint8_t chn = (!pchn ? 1 : *pchn);
    uint8_t pos = 0;

    while(0 < count-- && (sizeof(*pitem) + (uint8_t)unit) <= (space - pos)) {
        pitem = (st_junlei_data *)&pdst[pos];
        pitem->len = (uint8_t)unit;
        pitem->chn = chn;
        pitem->id = id;
        sys_memcpy(pitem->data, pdata, unit);

        pdata += (uint8_t)unit;
        pos += sizeof(*pitem) + (uint8_t)unit;
        chn ++;
    }

    if (NULL != pchn) {
        *pchn = chn;
    }

    return pos;
}

int MODBUS_junlei_framebuild(st_MODBUS_junlei *pframe, uint16_t len,
                uint8_t cmd, uint8_t rsp, uint16_t serno, uint32_t nodeid)
{
    uint32_t crc;

	pframe->cmd  = cmd;
    pframe->end  = PROTOC_END;
    pframe->ver  = 0x0;//ver;
    pframe->dir  = PROTOC_UP;
    pframe->reply= (!rsp ? PROTOC_NOREPLY : PROTOC_REPLY);
    pframe->type = PROTOC_PASSIVE;
    sys_memcpy(pframe->nodeid, (void *)&nodeid, (SYS_NET_BYTE | sizeof(nodeid)));
    sys_memcpy(pframe->serno, (void *)&serno, (SYS_NET_BYTE | sizeof(serno)));

    len += sizeof(st_MODBUS_junlei) - sizeof(unsigned short); // minus crc16
    sys_memcpy(pframe->len, (void *)&len, (SYS_NET_BYTE | sizeof(len)));

    crc = MODBUS_crc_gen(NULL, (void *)pframe, len); // crc16
    sys_memcpy((unsigned char *)pframe + len, (void *)&crc, sizeof(unsigned short));
    return (len + sizeof(unsigned short)); // plus crc16
}
/*
 * @brief       To decode packet data
 * @retval      none
 */
void MODBUS_process(st_MODBUS_packet *ppkt)
{
    if (NULL == ppkt || NULL == ppkt->dev_info.pobj || NULL == ppkt->dev_info.read) {
        return;
    }

    st_MODBUS_dev *pdev = &ppkt->dev_info;
    unsigned char *phead = pdev->phead;
    unsigned char *ptail = (NULL == phead ? NULL : &phead[pdev->head_len]);
    unsigned char *pdata = NULL;
    unsigned int   tmp_val = 0;
    unsigned short len = 0;
    unsigned short num;
    short i, j, m, n;

    do {
        num = pdev->read(pdev->pobj, &pdata, len);
        if (!num || NULL == pdata) {
            break;
        }

		if(gpucTest == (unsigned char *)ppkt)
		{
			if(num != 0x12)
				gucRfidTest = 1;
		}

        for (i = 0, len = num; 0 < num; num = len - i, tmp_val = 0) {
            for (j = m = 0, n = i; pdev->head_len > j;) {
				if(m > num) break;
				if (pdata[m] == phead[j]) {
					j++;
					n++;
				}
				else
					j = 0;
				m++;
            }

            if ((pdev->head_len > j && 0 == pdev->is_ignore) ||
                (PKT_TYPE_ASCII == pdev->pkt_type && '\0' == pdata[i])) {
                i ++;
                continue;
            }

            for (n = i + num - pdev->tail_len, j = 0; pdev->tail_len > j; j++, n++) {
				if(n > num) break;
                if (pdata[n] != ptail[j]) {
                    break;
                }
            }

            if (pdev->tail_len > j && 0 == pdev->is_ignore) {
				len -= 1;
                continue;
            }

            n = num - m - j;
            m += i;
            if (PKT_TYPE_RTU == pdev->pkt_type) {
                st_MODBUS_rtu *prtu = (st_MODBUS_rtu *)&pdata[m];

                if (sizeof(*prtu) > n) {
                    num = 0;
                    break;
                }

                if (pdev->id != prtu->dev_id) {
                    i ++;
                    continue;
                }

                n = _MODBUS_check(pdev, &pdata[m], n);
                if (0 >= n) {
                    i ++;
                    continue;
                }

                i += n;
                n -= sizeof(*prtu);
                m += offsetof(st_MODBUS_rtu, data);
                tmp_val = prtu->func;
                if (4 < n) {
                    n = n;
                }
            }
            else if (PKT_TYPE_TL == pdev->pkt_type) {
                st_MODBUS_lv *plv = (st_MODBUS_lv *)&pdata[m];

                if (sizeof(*plv) >= n || n <= plv->num) {
                    num = 0;
                    break;
                }

                n = plv->num + 1;
                n = _MODBUS_check(pdev, &pdata[m], n);
                if (pdev->id != plv->dev_id || 0 >= n) {
                    i ++;
                    continue;
                }

                i += n + pdev->head_len + pdev->tail_len;
                m += offsetof(st_MODBUS_lv, data);
                n -= sizeof(*plv);
                tmp_val = plv->cmd;
            }
            else if (PKT_TYPE_TLV == pdev->pkt_type) {
                break;
            }
            else if (PKT_TYPE_BIN == pdev->pkt_type) {
                tmp_val = ppkt->rsp.fun_code;
                i += num;
            }
            else if (0 == pdev->crc_size) {
                tmp_val = ppkt->rsp.fun_code;
                i += num;
            }
            else {
                if (sizeof(st_MODBUS_ascii) >= n) {
                    num = 0;
                    break;
                }

                j = sys_str2hex((void *)&pdata[m], (void *)&tmp_val, 1);
                if (0 >= (int)j || pdev->id != (uint8_t)tmp_val) { //unmatch device id
                    i ++;
                    continue;
                }

                n = _MODBUS_check(pdev, &pdata[m], n);
                if (0 == n) { // match crc
                    i ++;
                    continue;
                }

                i  = m + n + pdev->tail_len;
                j += sys_str2hex((void *)&pdata[m+j], (void *)&tmp_val, 1);
                n -= j + (pdev->crc_size << 1);
                m += j;
            }

            unsigned int tick = ppkt->rsp.snd_tick;

            pdev->state &= ~DEV_STATE_WAITRSP;
            if (0 <= ppkt->dev_rsp_proc(tmp_val, &pdata[m], n) && tick == ppkt->rsp.snd_tick) {
                ppkt->rsp.fun_code = 0;
            }
        }

        pdev->read(pdev->pobj, &pdata, i);
    } while (0 < num);

    if (0 != ppkt->rsp.fun_code && 0 < ppkt->rsp.timeout && sys_istimeout(ppkt->rsp.snd_tick, ppkt->rsp.timeout)) {
        ppkt->rsp.fun_code = 0;
        pdev->state &= ~DEV_STATE_WAITRSP;
        ppkt->dev_rsp_proc(MODBUS_TIMEOUT_CODE, NULL, 0);
    }
}
