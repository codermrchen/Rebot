/******************************************************************************
 * @brief    命令行任务
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020/07/11     Morro
 ******************************************************************************/
 #if 1
#include <stdlib.h>
#include "sys_task.h"
#include "platform.h"
#include "modbus.h"
#include "cli.h"

/* Private function prototypes -----------------------------------------------*/
static int _cli_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num);

/* Private variables ---------------------------------------------------------*/
/**
 * @brief
 */
static unsigned char _tty_headbuf[] = {'A', 'T', '#', '#'};
static unsigned char _tty_rxbuf[CMD_RXBUF_SIZE];
//static unsigned char _tty_txbuf[256];
static st_uart_info _tty_port;

static st_MODBUS_packet _tty_data_pkt = {
        .dev_rsp_proc = _cli_rsp_proc,
        .dev_info = {
                .id    = 0x01,
                .pobj  = &_tty_port,
                .read  = MODBUS_read,
                .write = MODBUS_write,
                .state = DEV_STATE_SLAVE,
                .pkt_type = PKT_TYPE_ASCII,
                .phead    = _tty_headbuf,
                .head_len = 3,
                .tail_len = 1,
            },
    };

/* Private functions ---------------------------------------------------------*/
static int _cli_rsp_proc(unsigned char func, unsigned char *pdata, unsigned short num)
{
    if (NULL != pdata && 0 < num) {
        pdata[num] = '\0';
        cli_process(&_tty_port, (char *)pdata);
        return func;
    }
    return -1;
}

/*
 * @brief   命令行任务
 */
static void cli_task_process(void)
{
    MODBUS_process(&_tty_data_pkt);
}

/*
 * @brief	    串口初始化
 * @param[in]   none
 * @return 	    none
 */
static void cli_task_init(void)
{
#ifdef TGT_TTY_CFG
    st_uart_info cfg = TGT_TTY_CFG;

    _tty_port = cfg;
#else
    _tty_port.port.reg = BSP_Obj2Reg(USART1);
#endif

    if (0 <= BSP_uart_init(&_tty_port, _tty_rxbuf, sizeof(_tty_rxbuf))) {
//        ring_buf_init(&_tty_port.tx_buf, _tty_txbuf, sizeof(_tty_txbuf));
//        {
            cli_port_t p = {
                .read = MODBUS_read,
                .write = MODBUS_write,
            };

            cli_init(&p);
//       }
    }
	uart_dma_init(UART8, DMA_Channel_5, DMA1_Stream0);
	cli_process(&_tty_port, "sysinfo");
//	gpucTest = &_tty_data_pkt;
}

/*
 * @brief    cli任务
 */
//module_init("cli", cli_task_init);
task_define(cli, cli_task_init, cli_task_process, 200, 1024, 4);



/*
 * @brief    cli任务
 */
//module_init("cli", cli_task_init);
//task_define(cli, cli_task_init, cli_task_process, 100, 2048, 1);

/*
 * @brief	   重定向printf
 */
int fputc(int c, FILE *f)
{
#if DEBUG_EN
    BSP_uart_write(&_tty_port, (unsigned char *)&c, 1);
#endif
    return c;
}
#endif

