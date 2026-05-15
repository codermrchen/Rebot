/********************************** (C) COPYRIGHT *******************************
* File Name          : CH395SPI_HW.C
* Author             : WCH
* Version            : V1.1
* Date               : 2014/8/1
* Description        : CH395芯片 硬件SPI串行连接的硬件抽象层 V1.0
*                     
*******************************************************************************/

#include "bsp.h"
#include "ch395spi_hw.h"
#include "sys_util.h"

/*******************************************************************************
* Function Name  : Spi395Exchange
* Description    : 硬件SPI输出且输入8个位数据
* Input          : d---将要送入到CH395的数据
* Output         : None
* Return         : None
*******************************************************************************/
unsigned char Spi395Exchange(st_spi_info * spix, unsigned char d )  
{  
    return BSP_spi_sendByte(spix, d);
}

/******************************************************************************
* Function Name  : xWriteCH395Cmd
* Description    : 向CH395写命令
* Input          : cmd 8位的命令码
* Output         : None
* Return         : None
*******************************************************************************/
void xWriteCH395Cmd(st_spi_info * spix, unsigned char cmd)                                          
{               
    xEndCH395Cmd();                                                  /* 防止CS原来为低，先将CD置高 */
    xCH395CmdStart( );                                               /* 命令开始，CS拉低 */
    Spi395Exchange(spix, cmd);                                       /* SPI发送命令码 */
    sys_delay_us(2);                                                /* 必要延时,延时1.5uS确保读写周期不小于1.5uS */
}

/******************************************************************************
* Function Name  : xWriteCH395Data
* Description    : 向CH395写数据
* Input          : mdata 8位数据
* Output         : None
* Return         : None
*******************************************************************************/
void  xWriteCH395Data(st_spi_info * spix, unsigned char mdata)
{   
    Spi395Exchange(spix, mdata);                                           /* SPI发送数据 */
}

/*******************************************************************************
* Function Name  : xReadCH395Data
* Description    : 从CH395读数据
* Input          : None
* Output         : None
* Return         : 8位数据
*******************************************************************************/
unsigned char xReadCH395Data(st_spi_info * spix)                                                  
{
    unsigned char i;
    i = Spi395Exchange(spix, 0xff);                                        /* SPI读数据 */
    return i;
}

/**************************** endfile *************************************/


