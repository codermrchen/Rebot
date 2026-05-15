/********************************** (C) COPYRIGHT *******************************
* File Name          : CH395CMD.H
* Author             : WCH
* Version            : V1.1
* Date               : 2014/8/1
* Description        : CH395芯片命令接口文件,常用子程序外部调用
*******************************************************************************/

#ifndef __CH395CMD_H__
#define __CH395CMD_H__

#include "bsp.h"
#include "socket.h"

void CH395CMDReset(st_spi_info * spix);                                            /* 复位 */

void CH395CMDSleep(st_spi_info * spix);                                            /* 睡眠 */

unsigned char CH395CMDGetVer(st_spi_info * spix);                                          /* 获取芯片及固件版本号 */

unsigned char CH395CMDCheckExist(st_spi_info * spix, unsigned char testdata);                            /* 测试命令 */

void CH395CMDSetPHY(st_spi_info * spix, unsigned char phystat);                                  /* 设置PHY状态  */

unsigned char CH395CMDGetPHYStatus(st_spi_info * spix);                                    /* 获取PHY状态 */

unsigned char CH395CMDInitCH395(st_spi_info * spix);                                       /* 初始化CH395 */

unsigned char CH395GetCmdStatus(st_spi_info * spix);                                       /* 获取命令执行状态 */

void CH395CMDSetIPAddr(st_spi_info * spix, unsigned char *ipaddr);                               /* 设置CH395的IP地址 */

void CH395CMDSetGWIPAddr(st_spi_info * spix, unsigned char *gwipaddr);                           /* 设置CH395的网关IP地址 */

void CH395CMDSetMASKAddr(st_spi_info * spix, unsigned char *maskaddr);                           /* 设置子网掩码 */

void CH395CMDSetMACAddr(st_spi_info * spix, unsigned char *amcaddr);                             /* 设置CH395的MAC地址 */

void CH395CMDGetMACAddr(st_spi_info * spix, unsigned char *amcaddr);                             /* 获取MAC地址 */

void CH395CMDSetMACFilt(st_spi_info * spix, unsigned char filtype,unsigned long table0,unsigned long table1);  /* 设置CH395的MAC过滤 */

void CH395CMDGetUnreachIPPT(st_spi_info * spix, unsigned char *list);                            /* 获取不可达地址以及端口 */

void CH395CMDGetRemoteIPP(st_spi_info * spix, unsigned char sockindex,unsigned char *list);              /* 获取远端IP和端口，一般在TCP Server下使用 */

void CH395SetSocketDesIP(st_spi_info * spix, unsigned char sockindex,unsigned char *ipaddr);             /* 设置scoket n的目的IP地址 */

void CH395SetSocketProtType(st_spi_info * spix, unsigned char sockindex,unsigned char prottype);         /* 设置socket n的协议类型 */

void CH395SetSocketDesPort(st_spi_info * spix, unsigned char sockindex,unsigned short desprot);          /* 设置socket n的目的端口 */

void CH395SetSocketSourPort(st_spi_info * spix, unsigned char sockindex,unsigned short surprot);         /* 设置socket n的源端口 */

void CH395SetSocketIPRAWProto(st_spi_info * spix, unsigned char sockindex,unsigned char prototype);      /* 在IPRAW模式下，设置socket n的IP包协议字段 */

void CH395SetRecvThresLen(st_spi_info * spix, unsigned char sockindex,unsigned short len);               /* 设置socket n的接收中断阀值 */

void CH395SendData(st_spi_info * spix, unsigned char sockindex,unsigned char *databuf,unsigned short len);       /* 向socket n的发送缓冲区写数据 */

unsigned short CH395GetRecvLength(st_spi_info * spix, unsigned char sockindex);                          /* 获取socket n的接收长度 */

void CH395ClearRecvBuf(st_spi_info * spix, unsigned char sockindex);                             /* 清除socket n的接收缓冲区 */

int CH395GetRecvData(st_spi_info * spix, unsigned char sockindex,unsigned short len,unsigned char *pbuf);       /* 获取接收数据 */

void CH395CMDSetRetryCount(st_spi_info * spix, unsigned char count);                             /* 设置最大重试次数 */

void CH395CMDSetRetryPeriod(st_spi_info * spix, unsigned short period);                          /* 设置最大重试周期 单位 毫秒*/

void CH395CMDGetSocketStatus(st_spi_info * spix, unsigned char sockindex,unsigned char *status) ;        /* 获取socket n的状态 */

unsigned char  CH395OpenSocket(st_spi_info * spix, unsigned char sockindex);                             /* 打开socket n*/

unsigned char  CH395CloseSocket(st_spi_info * spix, unsigned char sockindex);                             /* 关闭socket n*/

unsigned char CH395TCPConnect(st_spi_info * spix, unsigned char sockindex);                               /* TCP连接 */

unsigned char CH395TCPListen(st_spi_info * spix, unsigned char sockindex);                                /* TCP监听 */

unsigned char CH395TCPDisconnect(st_spi_info * spix, unsigned char sockindex);                            /* TCP断开连接 */

unsigned char CH395GetSocketInt(st_spi_info * spix, unsigned char sockindex);                            /* 获取socket n的中断状态 */
                                                                     
unsigned char CH395CRCRet6Bit(unsigned char *mac_addr);                              /* 多播地址CRC32，用于HASH过滤 */
                                                                     /*Function Count = 36 */
void CH395GetIPInf(st_spi_info * spix, unsigned char *addr);

unsigned char  CH395DHCPEnable(st_spi_info * spix, unsigned char flag);

void CH395SetTCPMss(st_spi_info * spix, unsigned short tcpmss);

void CH395SetSocketRecvBuf(st_spi_info * spix, unsigned char sockindex,unsigned char startblk,unsigned char blknum);

void CH395SetSocketSendBuf(st_spi_info * spix, unsigned char sockindex,unsigned char startblk,unsigned char blknum);

void CH395SetStartPara(st_spi_info * spix, unsigned long mdata);

unsigned short CH395CMDGetGlobIntStatus_ALL(st_spi_info * spix);

void CH395SetKeepLive(st_spi_info * spix, unsigned char sockindex,unsigned char cmd);                                 /* keepalive 使能 */

void CH395KeepLiveCNT(st_spi_info * spix, unsigned char cnt);                                                /* keepalive 参数设置 */

void CH395KeepLiveIDLE(st_spi_info * spix, unsigned long idle);                                             /* keepalive 参数设置 */

void CH395KeepLiveINTVL(st_spi_info * spix, unsigned long intvl);                                           /* keepalive 参数设置 */

void CH395SetTTLNum(st_spi_info * spix, unsigned char sockindex,unsigned char TTLnum);                               /* TTL参数设置 */
 
void CH395EnablePing(st_spi_info * spix, unsigned char enable);                                              /* PING 使能 */

extern unsigned char CH395A_Init(st_spi_info * spix);

extern unsigned char CH395SocketInitOpen(st_spi_info * spix, st_socket_info * socket);

extern void CH395GlobalInterrupt(st_spi_info * spix, st_socket_info * socket);
extern unsigned char CH395CMDGetPHYStatus(st_spi_info * spix);


/* TCP模式，0为客户端，1为服务器 */
#define TCP_CLIENT_MODE                                      0
#define TCP_SERVER_MODE                                      1

#define	 Local_Port   1000			 			//本地端口

extern const uint8_t Cloud_IP[];

extern const uint8_t Nvidia_IP[];

extern const uint8_t Net_debug_IP[];

#endif
/**************************** endfile *************************************/
