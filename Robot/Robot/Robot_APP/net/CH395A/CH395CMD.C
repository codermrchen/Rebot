/********************************** (C) COPYRIGHT *******************************
* File Name          : CH395CMD.C
* Author             : WCH
* Version            : V1.1
* Date               : 2014/8/1
* Description        : CH395芯片命令接口文件
*                      
*******************************************************************************/

/* 头文件包含*/
#include "ch395spi_hw.h"
#include "ch395cmd.h"
#include "sys_util.h"
#include "config.h"
#include "platform.h"

static unsigned char sucLock = 0;

#ifdef ROBOT_VER_1
const uint8_t Local_IP[4] = {192,168,0,130};	//本地IP
//const uint8_t Local_IP[4] = {192,168,137,130};
const uint8_t Nvidia_IP[4] = {192,168,0,211};	//英伟达IP地址
const uint8_t Local_GW[4] = {192,168,0,1};		//网关
//const uint8_t Local_GW[4] = {192,168,137,1};
#endif
#ifdef ROBOT_VER_2
const uint8_t Local_IP[4] = {192,168,0,128};
//const uint8_t Local_IP[4] = {192,168,137,128};
const uint8_t Nvidia_IP[4] = {192,168,0,212};	//英伟达IP地址
const uint8_t Local_GW[4] = {192,168,0,1};		//网关
//const uint8_t Local_GW[4] = {192,168,137,1};
#endif

const uint8_t Local_SN[4] = {255,255,255,0};	//子网掩码
#if 0
const uint8_t Cloud_IP[4] = {47,115,47,210};	//前云平台IP地址
#else
const uint8_t Cloud_IP[4] = {192,168,0,210};	//云平台IP地址
#endif

#if (NET_DEBUG == 1)
const uint8_t Net_debug_IP[4] = {192,168,0,74};
#endif


static void waiting_spi_release(void)
{
	while(sucLock == 1)
	{
		sys_delay_ms(2);
	}
}

static void release_spi(void)
{
	sucLock = 0;
}

static void lock_spi(void)
{
	sucLock = 1;
}

//CH395重启
void CH395CMDReset(st_spi_info * spix)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD00_RESET_ALL);
    xEndCH395Cmd();
	release_spi();
}

//CH395休眠
void CH395CMDSleep(st_spi_info * spix)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD00_ENTER_SLEEP);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDSleep
* Description    : 获取芯片以及固件版本号，1字节，高四位表示芯片版本，
                   低四位表示固件版本
* Input          : None
* Output         : None
* Return         : 1字节芯片及固件版本号
*******************************************************************************/
unsigned char CH395CMDGetVer(st_spi_info * spix)
{
    unsigned char i;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD01_GET_IC_VER);
    i = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
    return i;
}

/********************************************************************************
* Function Name  : CH395CMDCheckExist
* Description    : 测试命令，用于测试硬件以及接口通讯
* Input          : testdata 1字节测试数据
* Output         : None
* Return         : 硬件OK，返回 testdata按位取反
*******************************************************************************/
unsigned char CH395CMDCheckExist(st_spi_info * spix, unsigned char testdata)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD11_CHECK_EXIST);
    xWriteCH395Data(spix, testdata);
    i = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
    return i;
}

/********************************************************************************
* Function Name  : CH395CMDSetPHY
* Description    : 设置PHY，主要设置CH395 PHY为100/10M 或者全双工半双工，CH395默
                    为自动协商。
*******************************************************************************/
void CH395CMDSetPHY(st_spi_info * spix, unsigned char phystat)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD10_SET_PHY);
    xWriteCH395Data(spix, phystat);
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395CMDGetPHYStatus
* Description    : 获取PHY的状态
* Input          : None
* Output         : None
* Return         : 当前CH395PHY状态，参考PHY参数/状态定义
*******************************************************************************/
unsigned char CH395CMDGetPHYStatus(st_spi_info * spix)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD01_GET_PHY_STATUS);
    i = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
    return i;
}

/********************************************************************************
* Function Name  : CH395CMDInitCH395
* Description    : 初始化CH395芯片。
* Input          : None
* Output         : None
* Return         : 返回执行结果
*******************************************************************************/
unsigned char CH395CMDInitCH395(st_spi_info * spix)
{
    unsigned char i = 0;
    unsigned char s = 0;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD0W_INIT_CH395);
    xEndCH395Cmd();
	release_spi();
    while(1)
    {
        sys_delay_ms(10);                                              /* 延时查询，建议2MS以上*/
        s = CH395GetCmdStatus(spix);                                     /* 不能过于频繁查询*/
        if(s !=CH395_ERR_BUSY)break;                                 /* 如果CH395芯片返回忙状态*/
        if(i++ > 200)return CH395_ERR_UNKNOW;                        /* 超时退出,本函数需要500MS以上执行完毕 */
    }
    return s;
}

/*******************************************************************************
* Function Name  : CH395GetCmdStatus
* Description    : 获取命令执行状态，某些命令需要等待命令执行结果
*******************************************************************************/
unsigned char CH395GetCmdStatus(st_spi_info * spix)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD01_GET_CMD_STATUS);
    i = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
    return i;
}

/********************************************************************************
* Function Name  : CH395CMDSetIPAddr
* Description    : 设置CH395的IP地址
* Input          : ipaddr 指IP地址
*******************************************************************************/
void CH395CMDSetIPAddr(st_spi_info * spix, unsigned char *ipaddr)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD40_SET_IP_ADDR);
    for(i = 0; i < 4;i++)xWriteCH395Data(spix, *ipaddr++);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDSetGWIPAddr
* Description    : 设置CH395的网关IP地址
* Input          : ipaddr 指向网关IP地址
*******************************************************************************/
void CH395CMDSetGWIPAddr(st_spi_info * spix, unsigned char *gwipaddr)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD40_SET_GWIP_ADDR);
    for(i = 0; i < 4;i++)xWriteCH395Data(spix, *gwipaddr++);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDSetMASKAddr
* Description    : 设置CH395的子网掩码，默认为255.255.255.0
* Input          : maskaddr 指子网掩码地址
*******************************************************************************/
void CH395CMDSetMASKAddr(st_spi_info * spix, unsigned char *maskaddr)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD40_SET_MASK_ADDR);
    for(i = 0; i < 4;i++)xWriteCH395Data(spix, *maskaddr++);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDSetMACAddr
* Description    : 设置CH395的MAC地址。
* Input          : amcaddr MAC地址指针
*******************************************************************************/
void CH395CMDSetMACAddr(st_spi_info * spix, unsigned char *amcaddr)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD60_SET_MAC_ADDR);
    for(i = 0; i < 6;i++)xWriteCH395Data(spix, *amcaddr++);
    xEndCH395Cmd();
    sys_delay_ms(100); 
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDGetMACAddr
* Description    : 获取CH395的MAC地址。
* Input          : amcaddr MAC地址指针
*
*******************************************************************************/
void CH395CMDGetMACAddr(st_spi_info * spix, unsigned char *amcaddr)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD06_GET_MAC_ADDR);
    for(i = 0; i < 6;i++)*amcaddr++ = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
 }

/*******************************************************************************
* Function Name  : CH395CMDSetMACFilt
* Description    : 设置MAC过滤。
* Input          : filtype 参考 MAC过滤
                   table0 Hash0
                   table1 Hash1
*******************************************************************************/
void CH395CMDSetMACFilt(st_spi_info * spix,unsigned char filtype,unsigned long table0,unsigned long table1)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD90_SET_MAC_FILT);
    xWriteCH395Data(spix, filtype);
    xWriteCH395Data(spix, (unsigned char)table0);
    xWriteCH395Data(spix, (unsigned char)((unsigned short)table0 >> 8));
    xWriteCH395Data(spix, (unsigned char)(table0 >> 16));
    xWriteCH395Data(spix, (unsigned char)(table0 >> 24));

    xWriteCH395Data(spix, (unsigned char)table1);
    xWriteCH395Data(spix, (unsigned char)((unsigned short)table1 >> 8));
    xWriteCH395Data(spix, (unsigned char)(table1 >> 16));
    xWriteCH395Data(spix, (unsigned char)(table1 >> 24));
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDGetUnreachIPPT
* Description    : 获取不可达信息 (IP,Port,Protocol Type)
* Input          : list 保存获取到的不可达
                        第1个字节为不可达代码，请参考 不可达代码(CH395INC.H)
                        第2个字节为IP包协议类型
                        第3-4字节为端口号
                        第4-8字节为IP地址
*******************************************************************************/
void CH395CMDGetUnreachIPPT(st_spi_info * spix, unsigned char *list)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD08_GET_UNREACH_IPPORT);
    for(i = 0; i < 8; i++)
    {
        *list++ = xReadCH395Data(spix);
    }   
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDGetRemoteIPP
* Description    : 获取远端的IP和端口地址，一般在TCP Server模式下使用
* Input          : sockindex Socket索引
                   list 保存IP和端口
*******************************************************************************/
void CH395CMDGetRemoteIPP(st_spi_info * spix, unsigned char sockindex,unsigned char *list)
{
    unsigned char i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD06_GET_REMOT_IPP_SN);
    xWriteCH395Data(spix, sockindex);
    for(i = 0; i < 6; i++)
    {
        *list++ = xReadCH395Data(spix);
    }   
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395SetSocketDesIP
* Description    : 设置socket n的目的IP地址
* Input          : sockindex Socket索引
                   ipaddr 指向IP地址
* Output         : None
* Return         : None
*******************************************************************************/
void CH395SetSocketDesIP(st_spi_info * spix, unsigned char sockindex,unsigned char *ipaddr)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD50_SET_IP_ADDR_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, ipaddr[0]);
    xWriteCH395Data(spix, ipaddr[1]);
    xWriteCH395Data(spix, ipaddr[2]);
    xWriteCH395Data(spix, ipaddr[3]);
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395SetSocketProtType
* Description    : 设置socket 的协议类型
* Input          : sockindex Socket索引
                   prottype 协议类型，请参考 socket协议类型定义(CH395INC.H)
*******************************************************************************/
void CH395SetSocketProtType(st_spi_info * spix, unsigned char sockindex,unsigned char prottype)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD20_SET_PROTO_TYPE_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, prottype);
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************

* Function Name  : CH395SetSocketDesPort
* Description    : 设置socket n的协议类型
* Input          : sockindex Socket索引
                   desprot 2字节目的端口
*******************************************************************************/
void CH395SetSocketDesPort(st_spi_info * spix, unsigned char sockindex,unsigned short desprot)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD30_SET_DES_PORT_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, (unsigned char)desprot);
    xWriteCH395Data(spix, (unsigned char)(desprot >> 8));
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395SetSocketSourPort
* Description    : 设置socket n的协议类型
* Input          : sockindex Socket索引
                   desprot 2字节源端口
* Output         : None
* Return         : None
*******************************************************************************/
void CH395SetSocketSourPort(st_spi_info * spix, unsigned char sockindex,unsigned short surprot)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD30_SET_SOUR_PORT_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, (unsigned char)surprot);
    xWriteCH395Data(spix, (unsigned char)(surprot>>8));
    xEndCH395Cmd();
	release_spi();
}

/******************************************************************************
* Function Name  : CH395SetSocketIPRAWProto
* Description    : IP模式下，socket IP包协议字段
* Input          : sockindex Socket索引
                   prototype IPRAW模式1字节协议字段
*******************************************************************************/
void CH395SetSocketIPRAWProto(st_spi_info * spix, unsigned char sockindex,unsigned char prototype)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD20_SET_IPRAW_PRO_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, prototype);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395EnablePing
* Description    : 开启/关闭 PING
* Input          : enable : 1  开启PING
                          ：0  关闭PING
*******************************************************************************/
void CH395EnablePing(st_spi_info * spix, unsigned char enable)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD01_PING_ENABLE);
    xWriteCH395Data(spix, enable);
    xEndCH395Cmd();
	release_spi();
}


/********************************************************************************
* Function Name  : CH395SendData
* Description    : 向发送缓冲区写数据
* Input          : sockindex Socket索引
                   databuf  数据缓冲区
                   len   长度
*******************************************************************************/
void CH395SendData(st_spi_info * spix, unsigned char sockindex,unsigned char *databuf,unsigned short len)
{
    unsigned short i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD30_WRITE_SEND_BUF_SN);
    xWriteCH395Data(spix, (unsigned char)sockindex);
    xWriteCH395Data(spix, (unsigned char)len);
    xWriteCH395Data(spix, (unsigned char)(len>>8));
   
    for(i = 0; i < len; i++)
    {
        xWriteCH395Data(spix, *databuf++);
    }
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395GetRecvLength
* Description    : 获取接收缓冲区长度
* Input          : sockindex Socket索引
* Output         : None
* Return         : 返回接收缓冲区有效长度
*******************************************************************************/
unsigned short CH395GetRecvLength(st_spi_info * spix, unsigned char sockindex)
{
    unsigned short i;

	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD12_GET_RECV_LEN_SN);
    xWriteCH395Data(spix, (unsigned char)sockindex);
    i = xReadCH395Data(spix);
    i = (unsigned short)(xReadCH395Data(spix)<<8) + i;
    xEndCH395Cmd();
	release_spi();
    return i;
}

/*******************************************************************************
* Function Name  : CH395ClearRecvBuf
* Description    : 清除接收缓冲区
* Input          : sockindex Socket索引
*******************************************************************************/
void CH395ClearRecvBuf(st_spi_info * spix, unsigned char sockindex)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD10_CLEAR_RECV_BUF_SN);
    xWriteCH395Data(spix, (unsigned char)sockindex);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395GetRecvLength
* Description    : 读取接收缓冲区数据
* Input          : sockindex Socket索引
                   len   长度
                   pbuf  缓冲区
*******************************************************************************/
int CH395GetRecvData(st_spi_info * spix, unsigned char sockindex,unsigned short len,unsigned char *pbuf)
{
    unsigned short i;
    if(!len)return 0;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD30_READ_RECV_BUF_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, (unsigned char)len);
    xWriteCH395Data(spix, (unsigned char)(len>>8));
    sys_delay_us(1);
    for(i = 0; i < len; i++)
    {
       *pbuf = xReadCH395Data(spix);
       pbuf++;
    }   
    xEndCH395Cmd();
	release_spi();
	return len;
}

/********************************************************************************
* Function Name  : CH395CMDSetRetryCount
* Description    : 设置重试次数
* Input          : count 重试值，最大为20次
********************************************************************************/
void CH395CMDSetRetryCount(st_spi_info * spix, unsigned char count)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD10_SET_RETRAN_COUNT);
    xWriteCH395Data(spix, count);
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDSetRetryPeriod
* Description    : 设置重试周期
* Input          : period 重试周期单位为毫秒，最大1000ms
* Output         : None
* Return         : None
*******************************************************************************/
void CH395CMDSetRetryPeriod(st_spi_info * spix, unsigned short period)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD10_SET_RETRAN_COUNT);
    xWriteCH395Data(spix, (unsigned char)period);
    xWriteCH395Data(spix, (unsigned char)(period>>8));
    xEndCH395Cmd();
	release_spi();
}

/********************************************************************************
* Function Name  : CH395CMDGetSocketStatus
* Description    : 获取socket
* Input          : None
* Output         : socket n的状态信息，第1字节为socket 打开或者关闭
                   第2字节为TCP状态
* Return         : None
*******************************************************************************/
void CH395CMDGetSocketStatus(st_spi_info * spix, unsigned char sockindex,unsigned char *status)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD12_GET_SOCKET_STATUS_SN);
    xWriteCH395Data(spix, sockindex);
    *status++ = xReadCH395Data(spix);
    *status++ = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395OpenSocket
* Description    : 打开socket，此命令需要等待执行成功
* Input          : sockindex Socket索引
* Output         : None
* Return         : 返回执行结果
*******************************************************************************/
unsigned char  CH395OpenSocket(st_spi_info * spix, unsigned char sockindex)
{
    unsigned char i = 0;
    unsigned char s = 0;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD1W_OPEN_SOCKET_SN);
    xWriteCH395Data(spix, sockindex);
    xEndCH395Cmd();
	release_spi();
    while(1)
    {
        sys_delay_ms(5);                                                 /* 延时查询，建议2MS以上*/
        s = CH395GetCmdStatus(spix);                                     /* 不能过于频繁查询*/
        if(s !=CH395_ERR_BUSY)break;                                 /* 如果CH395芯片返回忙状态*/
        if(i++ > 200)return CH395_ERR_UNKNOW;                        /* 超时退出*/
    }
    return s;
}

/*******************************************************************************
* Function Name  : CH395CloseSocket
* Description    : 关闭socket，
* Input          : sockindex Socket索引
* Output         : None
* Return         : 返回执行结果
*******************************************************************************/
unsigned char  CH395CloseSocket(st_spi_info * spix, unsigned char sockindex)
{
    unsigned char i = 0;
    unsigned char s = 0;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD1W_CLOSE_SOCKET_SN);
    xWriteCH395Data(spix, sockindex);
    xEndCH395Cmd();
	release_spi();
    while(1)
    {
        sys_delay_ms(5);                                                 /* 延时查询，建议2MS以上*/
        s = CH395GetCmdStatus(spix);                                     /* 不能过于频繁查询*/
        if(s !=CH395_ERR_BUSY)break;                                 /* 如果CH395芯片返回忙状态*/
        if(i++ > 200)return CH395_ERR_UNKNOW;                        /* 超时退出*/
    }
    return s;
}

/********************************************************************************
* Function Name  : CH395TCPConnect
* Description    : TCP连接，仅在TCP模式下有效，此命令需要等待执行成功
* Input          : sockindex Socket索引
* Output         : None
* Return         : 返回执行结果
*******************************************************************************/
unsigned char CH395TCPConnect(st_spi_info * spix, unsigned char sockindex)
{
    unsigned char i = 0;
    unsigned char s = 0;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD1W_TCP_CONNECT_SN);
    xWriteCH395Data(spix, sockindex);
    xEndCH395Cmd();
	release_spi();
    while(1)
    {
        sys_delay_ms(5);                                                 /* 延时查询，建议2MS以上*/
        s = CH395GetCmdStatus(spix);                                     /* 不能过于频繁查询*/
        if(s !=CH395_ERR_BUSY)break;                                 /* 如果CH395芯片返回忙状态*/
        if(i++ > 200)return CH395_ERR_UNKNOW;                        /* 超时退出*/
    }
    return s;
}

/******************************************************************************
* Function Name  : CH395TCPListen
* Description    : TCP监听，仅在TCP模式下有效，此命令需要等待执行成功
* Input          : sockindex Socket索引
* Output         : None
* Return         : 返回执行结果
*******************************************************************************/
unsigned char CH395TCPListen(st_spi_info * spix, unsigned char sockindex)
{
    unsigned char i = 0;
    unsigned char s = 0;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD1W_TCP_LISTEN_SN);
    xWriteCH395Data(spix, sockindex);
    xEndCH395Cmd();
	release_spi();
    while(1)
    {
        sys_delay_ms(5);                                                 /* 延时查询，建议2MS以上*/
        s = CH395GetCmdStatus(spix);                                     /* 不能过于频繁查询*/
        if(s !=CH395_ERR_BUSY)break;                                 /* 如果CH395芯片返回忙状态*/
        if(i++ > 200)return CH395_ERR_UNKNOW;                        /* 超时退出*/
    }
    return s;
}

/********************************************************************************
* Function Name  : CH395TCPDisconnect
* Description    : TCP断开，仅在TCP模式下有效，此命令需要等待执行成功
* Input          : sockindex Socket索引
* Output         : None
* Return         : None
*******************************************************************************/
unsigned char CH395TCPDisconnect(st_spi_info * spix, unsigned char sockindex)
{
    unsigned char i = 0;
    unsigned char s = 0;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD1W_TCP_DISNCONNECT_SN);
    xWriteCH395Data(spix, sockindex);
    xEndCH395Cmd();
	release_spi();
    while(1)
    {
        sys_delay_ms(5);                                                 /* 延时查询，建议2MS以上*/
        s = CH395GetCmdStatus(spix);                                     /* 不能过于频繁查询*/
        if(s !=CH395_ERR_BUSY)break;                                 /* 如果CH395芯片返回忙状态*/
        if(i++ > 200)return CH395_ERR_UNKNOW;                        /* 超时退出*/
    }
    return s;
}

/*******************************************************************************
* Function Name  : CH395GetSocketInt
* Description    : 获取socket n的中断状态
* Input          : sockindex   socket索引
* Output         : None
* Return         : 中断状态
*******************************************************************************/
unsigned char CH395GetSocketInt(st_spi_info * spix, unsigned char sockindex)
{
    unsigned char intstatus;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD11_GET_INT_STATUS_SN);
    xWriteCH395Data(spix, sockindex);
    sys_delay_us(2);
    intstatus = xReadCH395Data(spix);
    xEndCH395Cmd();
	release_spi();
    return intstatus;
}

/*******************************************************************************
* Function Name  : CH395CRCRet6Bit
* Description    : 对多播地址进行CRC运算，并取高6位。
* Input          : mac_addr   MAC地址
* Output         : None
* Return         : 返回CRC32的高6位
*******************************************************************************/
unsigned char CH395CRCRet6Bit(unsigned char *mac_addr)
{
    unsigned long perByte;
    unsigned long perBit;
    const unsigned long poly = 0x04C11DB7;
    unsigned long crc_value = 0xFFFFFFFF;
    unsigned char c;
    for ( perByte = 0; perByte < 6; perByte ++ ) 
    {
        c = *(mac_addr++);
        for ( perBit = 0; perBit < 8; perBit++ ) 
        {
            crc_value = (crc_value<<1)^((((crc_value>>31)^c)&0x01)?poly:0);
            c >>= 1;
        }
    }
    crc_value=crc_value>>26;                                      
    return ((unsigned char)crc_value);
}


/*******************************************************************************
* Function Name  : CH395GetIPInf
* Description    : 获取IP，子网掩码和网关地址
* Input          : None
* Output         : 12个字节的IP,子网掩码和网关地址
* Return         : None
*******************************************************************************/
void CH395GetIPInf(st_spi_info * spix, unsigned char *addr)
{
    unsigned char i;
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD014_GET_IP_INF);
    for(i = 0; i < 20; i++)
    {
     *addr++ = xReadCH395Data(spix);
    }
    xEndCH395Cmd();
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395SetSocketRecvBuf
* Description    : 设置Socket接收缓冲区
* Input          : sockindex  socket索引
                 ：startblk   起始地址
                 ：blknum     单位缓冲区个数 ，单位为512字节
*******************************************************************************/
void CH395SetSocketRecvBuf(st_spi_info * spix, unsigned char sockindex,unsigned char startblk,unsigned char blknum)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD30_SET_RECV_BUF);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, startblk);
    xWriteCH395Data(spix, blknum);
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395SetSocketSendBuf
* Description    : 设置Socket发送缓冲区
* Input          : sockindex  socket索引
                 ：startblk   起始地址
                 ：blknum     单位缓冲区个数
* Output         : None
* Return         : None
*******************************************************************************/
void CH395SetSocketSendBuf(st_spi_info * spix, unsigned char sockindex,unsigned char startblk,unsigned char blknum)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD30_SET_SEND_BUF);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, startblk);
    xWriteCH395Data(spix, blknum);
	release_spi();
}


/*******************************************************************************
* Function Name  : CH395SetStartPara
* Description    : 设置CH395启动参数
* Input          : mdata
* Output         : None
* Return         : None
*******************************************************************************/
void CH395SetStartPara(st_spi_info * spix, unsigned long mdata)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD40_SET_FUN_PARA);
    xWriteCH395Data(spix, (unsigned char)mdata);
    xWriteCH395Data(spix, (unsigned char)((unsigned short)mdata>>8));
    xWriteCH395Data(spix, (unsigned char)(mdata >> 16));
    xWriteCH395Data(spix, (unsigned char)(mdata >> 24));
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395CMDGetGlobIntStatus
* Description    : 获取全局中断状态，收到此命令CH395自动取消中断,0x44及以上版本使用
* Input          : None
* Output         : None
* Return         : 返回当前的全局中断状态
*******************************************************************************/
unsigned short CH395CMDGetGlobIntStatus_ALL(st_spi_info * spix)
{
		unsigned short init_status;
		waiting_spi_release();
		lock_spi();
		xWriteCH395Cmd(spix, CMD02_GET_GLOB_INT_STATUS_ALL);
		sys_delay_us(2);
		init_status = xReadCH395Data(spix);
		init_status = (unsigned short)(xReadCH395Data(spix)<<8) + init_status;
		xEndCH395Cmd();
		release_spi();
		return init_status;
}

/*******************************************************************************
* Function Name  : CH395SetKeepLive
* Description    : 设置keepalive功能
* Input          : sockindex Socket号
*                  cmd 0：关闭 1：开启
* Output         : None
* Return         : None
*******************************************************************************/
void CH395SetKeepLive(st_spi_info * spix, unsigned char sockindex,unsigned char cmd)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD20_SET_KEEP_LIVE_SN);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, cmd);
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395KeepLiveCNT
* Description    : 设置keepalive重试次数
* Input          : cnt 重试次数（）
* Output         : None
* Return         : None
*******************************************************************************/
void CH395KeepLiveCNT(st_spi_info * spix, unsigned char cnt)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD10_SET_KEEP_LIVE_CNT);
    xWriteCH395Data(spix, cnt);
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395KeepLiveIDLE
* Description    : 设置KEEPLIVE空闲
* Input          : idle 空闲时间（单位：ms）
* Output         : None
* Return         : None
*******************************************************************************/
void CH395KeepLiveIDLE(st_spi_info * spix, unsigned long idle)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD40_SET_KEEP_LIVE_IDLE);
    xWriteCH395Data(spix, (unsigned char)idle);
    xWriteCH395Data(spix, (unsigned char)((unsigned short)idle>>8));
    xWriteCH395Data(spix, (unsigned char)(idle >> 16));
    xWriteCH395Data(spix, (unsigned char)(idle >> 24));
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395KeepLiveINTVL
* Description    : 设置KeepLive间隔时间 
* Input          : intvl 间隔时间（单位：ms）
* Output         : None
* Return         : None
*******************************************************************************/
void CH395KeepLiveINTVL(st_spi_info * spix, unsigned long intvl)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD40_SET_KEEP_LIVE_INTVL);
    xWriteCH395Data(spix, (unsigned char)intvl);
    xWriteCH395Data(spix, (unsigned char)((unsigned short)intvl>>8));
    xWriteCH395Data(spix, (unsigned char)(intvl >> 16));
    xWriteCH395Data(spix, (unsigned char)(intvl >> 24));
	release_spi();
}

/*******************************************************************************
* Function Name  : CH395SetTTLNum
* Description    : 设置TTL
* Input          : sockindex Socket号
*                  TTLnum:TTL数
* Output         : None
* Return         : None
*******************************************************************************/
void CH395SetTTLNum(st_spi_info * spix, unsigned char sockindex,unsigned char TTLnum)
{
	waiting_spi_release();
	lock_spi();
    xWriteCH395Cmd(spix, CMD20_SET_TTL);
    xWriteCH395Data(spix, sockindex);
    xWriteCH395Data(spix, TTLnum);
	release_spi();
}

unsigned char CH395A_Init(st_spi_info * spix)
{	//设置CH395A的IP、网关、子网掩码等参数后然后初始化CH395A
	unsigned char ucData;
	ucData = CH395CMDCheckExist(spix, 0x65);                      
    if(ucData != 0x9a) return CH395_ERR_UNKNOW;

	ucData = CH395CMDGetVer(spix);

	CH395CMDSetIPAddr(spix, (unsigned char *)Local_IP);        /* 设置CH395的IP地址 */
	CH395CMDSetGWIPAddr(spix, (unsigned char *)Local_GW);      /* 设置网关地址 */
	CH395CMDSetMASKAddr(spix, (unsigned char *)Local_SN);      /* 设置子网掩码，默认为255.255.255.0*/   
	sys_delay_ms(10);
	return CH395CMDInitCH395(spix);
}

unsigned char CH395SocketInitOpen(st_spi_info * spix, st_socket_info * socket)
{ 
    CH395SetSocketDesIP(spix, socket->Index,socket->IPAddr);               /* 设置socket n目标IP地址 */
    CH395SetSocketProtType(spix, socket->Index,socket->ProtoType);         /* 设置socket n协议类型 */
    CH395SetSocketDesPort(spix, socket->Index,socket->DesPort);            /* 设置socket n目的端口 */
    CH395SetSocketSourPort(spix, socket->Index,socket->SourPort);          /* 设置socket n源端口 */
    if(CH395OpenSocket(spix, socket->Index) == CH395_ERR_UNKNOW) return CH395_ERR_UNKNOW;     /* 打开socket n */
    return CH395TCPConnect(spix, socket->Index);                          /* 开始连接 */
}

void CH395SocketInterrupt(st_spi_info * spix, st_socket_info * socket)
{
    unsigned char sock_int_socket;
    unsigned short len;

    sock_int_socket = CH395GetSocketInt(spix, socket->Index);         /* 获取socket 的中断状态 */
    if(sock_int_socket & SINT_STAT_SENBUF_FREE)                      /* 发送缓冲区空闲，可以继续写入要发送的数据 */
    {

    }
    if(sock_int_socket & SINT_STAT_SEND_OK)                          /* 发送完成中断 */
    {
    	socket->SendSta = 1;
    }
    if(sock_int_socket & SINT_STAT_RECV)                             /* 接收中断 */
    {
    	if(socket->RecvSta == 1) return;							/*等待上一个数据处理完成*/
		//if(gucMqttBackupFlag && (socket->Index == 0)) return;
        len = CH395GetRecvLength(spix, socket->Index);               /* 获取当前缓冲区内数据长度 */
        if(len == 0)return;
		memset(socket->pRecvBuf, 0x00, socket->usRecvMaxLen);
        if(len > (socket->usRecvMaxLen - 2)) len = (socket->usRecvMaxLen - 2);           /* 发送缓冲区最大为1024 */
        len = CH395GetRecvData(spix, socket->Index,len,socket->pRecvBuf);         /* 读取数据 */
		//debug_printf("L1:%u\r\n", len);
		socket->pRecvBuf[len] = 0;								//结束符
		socket->RecvSta = 1;
		socket->RecvLen = len;
		socket->RecvTick = sys_get_ms();
   }
   if(sock_int_socket & SINT_STAT_CONNECT)                            /* 连接成功，仅在TCP模式下有效*/
   {
   		if(socket->ScokStatus & SOCKET_WAIT_SUCCESS)				
			socket->ScokStatus = SOCKET_CONNECTED;
   }
   if(sock_int_socket & SINT_STAT_DISCONNECT)                        /* 断开中断，仅在TCP模式下有效 */
   {
		 socket->TimeOutFlag = 1;
		 socket->SendTick = sys_get_ms();
		 debug_printf("socket %u is disconnect!\r\n", socket->Index);
		 if((socket->Index == 1) && ((gNvidiaDevInfo.ucNVIDIASta != 9) ||(gNvidiaDevInfo.usErrcode != 1)))
		 {
		 	gNvidiaDevInfo.ucNVIDIASta = 9;	
		 	gNvidiaDevInfo.usErrcode = 1;
		 	set_cloud_push_flag(PROP_NVIDIA);
		 }
		 if(socket->Index == 2)
		 {
			CH395CloseSocket(spix, socket->Index);
			sys_delay_ms(2);
			CH395OpenSocket(spix, socket->Index);
			sys_delay_ms(2);
			CH395TCPListen(spix, socket->Index);
			socket->ScokStatus = SOCKET_WAIT_SUCCESS;
		 }
//		 printf("socket %u is disconnect!\r\n", socket->Index);
   }
   if(sock_int_socket & SINT_STAT_TIM_OUT)                           /* 超时中断，仅在TCP模式下有效 */
   {
       if(socket->TcpMode == TCP_CLIENT_MODE)             
       {
       		if((socket->Index == 1) && ((gNvidiaDevInfo.ucNVIDIASta != 9) ||(gNvidiaDevInfo.usErrcode != 1)))
			 {
			 	gNvidiaDevInfo.ucNVIDIASta = 9;	
			 	gNvidiaDevInfo.usErrcode = 1;
			 	set_cloud_push_flag(PROP_NVIDIA);
			 }
       		debug_printf("socket %u is timeout!\r\n", socket->Index);
//			len = sprintf(pucBuf, "socket %u is timeout!\r\n", socket->Index);
//			debug_aurt_send(pucBuf, len);
//			printf("socket %u is timeout!\r\n", socket->Index);
			socket->SendTick = sys_get_ms();
			socket->TimeOutFlag = 1;
       }
	   	if(socket->Index == 2)
		 {
			CH395CloseSocket(spix, socket->Index);
			sys_delay_ms(2);
			CH395OpenSocket(spix, socket->Index);
			sys_delay_ms(2);
			CH395TCPListen(spix, socket->Index);
			socket->ScokStatus = SOCKET_WAIT_SUCCESS;
		 }
    }
}

void CH395GlobalInterrupt(st_spi_info * spix, st_socket_info * socket)
{
   unsigned short init_status;
   unsigned char buf[10]; 

    init_status = CH395CMDGetGlobIntStatus_ALL(spix);
    if(init_status & GINT_STAT_UNREACH)                              /* 不可达中断，读取不可达信息 */
    {
        CH395CMDGetUnreachIPPT(spix, buf);           
    }
    if(init_status & GINT_STAT_IP_CONFLI)                            /* 产生IP冲突中断，建议重新修改CH395的 IP，并初始化CH395*/
    {
    }
    if(init_status & GINT_STAT_PHY_CHANGE)                           /* 产生PHY改变中断*/
    {
	}
    if(init_status & GINT_STAT_SOCK0)
    {
        CH395SocketInterrupt(spix, &socket[0]);                         /* 处理socket 0中断*/
    }
    if(init_status & GINT_STAT_SOCK1)                               
    {
        CH395SocketInterrupt(spix, &socket[1]);                         /* 处理socket 1中断*/
    }
	if(init_status & GINT_STAT_SOCK2)                               
    {
        CH395SocketInterrupt(spix, &socket[2]);                         /* 处理socket 1中断*/
    }
}




/**************************** endfile *************************************/



