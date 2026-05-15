/******************************************************************************
 * @brief        短信管理
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-22     Morro        Initial version. 
 ******************************************************************************/
#include "ril_core.h"
#include "ril_device.h"
#include "string_tools.h"
#include <string.h>
#include <stdlib.h>

#define MAX_READ_ONE_SMS_SIZE    2048                    //短信接收缓冲区长度
#define MAX_SMS_BUF              256                     //短信长度
#define MAX_SMS_NUM              32                      //每次读取最大短信数
#define MAX_PHONE_NUM            18                      //号码长度

typedef struct {
    char recvbuf[MAX_SMS_BUF + 32];                      //接收缓冲区
    sms_info_t sms;
}sms_t;

#if 0


/*
 * @brief    sms pdu 7位编码
 */
static int pdu_8to7bit(const void *buf, int size)
{
    char *src = (unsigned char *)buf;
    char *des = src;
    unsigned char data;
    unsigned char  remain;                                /*剩余未处理数据*/
    unsigned char  bitcnt;                                /*剩余未处理数据位数*/
    int i = 0;
    remain = 0;
    bitcnt = 0;

    while (size--) {
        data = *src++ << 1; 

        if (bitcnt == 0) {
            bitcnt = 7;
            remain = data;
        } else {
            *des++ = (unsigned char)( remain | (data >> bitcnt) ); 			   			                   
            remain = data << (8 - bitcnt);  
			bitcnt = 7 - (8 - bitcnt);     			                  
            i++;            
        }
    }
    return i;
}
#endif

/*
 * @brief 短信配置初始化
 */
int sms_init(struct ril_device *r)
{
    const char *cmds[] = {
        "AT+CMGF=1",                                     /*短消息模式,TEXT*/                              
        "AT+CNMI=2,1,0,0,1",   
        "AT+CSMS=0",
        "AT+CPMS=\"ME\",\"ME\",\"ME\"",                  /*全部通过模组存储*/
        NULL
    };
    return ril_send_multiline(cmds);     
}

/*
 * @brief 短信发送
 */
int sms_send(struct ril_device *r, const char *phone, const char *msg)
{
    bool ret;
    char recv[64];
    if (strlen(phone) == 0)
        return RIL_REJECT;
    /*
     * 短信发送时长依赖网络信号质量
     */
    at_respond_t resp = {">", recv, sizeof(recv), 30 * 1000}; 
    ret = ril_exec_cmdx(&resp, "AT+CMGS=\"%s\"", phone); 
    if (ret != RIL_OK)
        return RIL_ERROR;
    resp.matcher = "OK";
    return ril_exec_cmdx(&resp, "%s\x1A", msg);/* sms + [ctrl+z=0x1A] */    
}


/*
 * @brief 读取指定索引的短信
 *
 */
static bool read_one_sms(struct ril_device *r, int index, sms_t *si)
{
    bool ret = false;                   
    char *start,*end;
    char *argv[3];
    /*
    AT+CMGR=1

    +CMGR: "REC UNREAD","+8612345612345",,"..."
    <text>

    OK
    */   
    at_respond_t resp = {"OK", si->recvbuf, sizeof(si->recvbuf), 5 * 1000};    

    ret = ril_exec_cmdx(&resp, "AT+CMGR=%d", index); 
    if (ret == RIL_OK) {
       start = strstr(resp.recvbuf, "+CMGR:"); 
       if (start)
           end   = strstr(start, "\r\n"); 
       if (start && end ) {
           *end = '\0';
           if (strsplit(start, ",", argv, 3) > 2) {
               strtrim(argv[1], "\"");               /* 去掉手机前后的双引号 */
               snprintf(si->sms.phone, sizeof(si->sms.phone), "%s", argv[1]);
           }
           start  = end + 2;
           if (strtok(start, "\r\n") != NULL) {
               si->sms.len = snprintf((char *)si->sms.msg, MAX_SMS_BUF, "%s", start);
               ril_notify(RIL_NOTIF_SMS, &si->sms, sizeof(si->sms) + MAX_SMS_BUF);               
           }
           ril_exec_cmdx(NULL, "AT+CMGD=%d", index); /* 删除已读取的短信 */    
       }
    }
    return ret;
}

/*
 * @brief 短信接收事件处理
 */
static void on_sms_recv(void *null, ril_obj_t *r, void *index)
{
    sms_t *si;
    si = (sms_t *)ril_malloc(sizeof(sms_t) + MAX_SMS_BUF);
    if (si == NULL)
        return;
    read_one_sms(&r->dev, (int)(index), si);  
    ril_free(si);    
}

/*
 * @brief   短信到来事件
 * @example [+CMTI: "ME",10]
 *
 */
static void read_one_sms_handler(at_urc_ctx_t *ctx)
{    
    long i;
    char *p = strchr(ctx->buf, ',');
    if (p != NULL) {
        i = atoi(p + 1);
        ril_do_async_work((void *)i, on_sms_recv);        
    }
}ril_urc_register("+CMTI: ", read_one_sms_handler);
