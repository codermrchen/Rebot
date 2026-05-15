/******************************************************************************
 * @brief    命令行处理
 *
 * Copyright (c) 2015-2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
* 2015-06-09      Morro        Initial version
 *
 * 2017-07-04      Morro        优化字段分割处理
 *
 * 2020-07-05      Morro        使用cli_obj_t对象, 支持多个命令源处理
 * 2020-08-29      Morro        支持AT指令解析及回显控制,解决缓冲区满之后收不到新数据的问题
 * 2020-02-16      Morro        添加命令行守卫处理程序
 ******************************************************************************/
#ifndef _CMDLINE_H_
#define _CMDLINE_H_

#include <stdlib.h>
#include "comdef.h"

/////////////////////////////////////////////////////////////////////////////
#define CLI_MAX_CMD_LEN                 256            /*命令行长度*/
#define CLI_MAX_ARGS                    64             /*最大参数个数*/
#define CLI_MAX_CMDS                    64             /*最大允许定义的命令个数*/

/*命令类型 */
#define CLI_CMD_TYPE_EXEC               0              /* 普通执行命令*/
#define CLI_CMD_TYPE_QUERY              1              /* 查询命令 (XXX?)*/
#define CLI_CMD_TYPE_SET                2              /* 设置命令 (XXX=YY)*/

#define __cmd_register(name, handler, brief)  \
            USED ANONY_TYPE(const cmd_item_t,__cli_cmd_##handler)\
            SECTION("cli.cmd.1") = {name, brief, handler}

/*******************************************************************************
 * @brief     命令注册
 * @params    name      - 命令名
 * @params    handler   - 命令处理程序
 *            类型:int (*handler)(struct cli_obj *s, int argc, char *argv[]);
 * @params    brief     - 使用说明
 */
#define cmd_register(name,handler,brief)    \
            __cmd_register(name,handler,brief)

/*命令项定义*/
typedef struct {
	const char *name;		                    /*命令名*/
    const char *brief;                          /*命令简介*/
    /**
     * @brief     命令处理程序,类型
     * @params    o      - cli 对象cli_obj
     * @params    argc   - 命令参数个数
     * @params    argv   - 命令参数表
     * @return    命令执行结果, 对于AT指令, 返回true时会自动响应OK,返回false时则
     *            响应ERROR
     */
	int (*handler)(void *pobj, int argc, char *argv[]);
} cmd_item_t;

/*cli 接口定义 -------------------------------------------------------------*/
typedef struct {
    /**
     * @brief 通信数据(串口)写接口
     */
    int (*write)(void *pobj, void *buf, unsigned short len);
    /**
     * @brief 通信数据(串口)读接口
     */
    int (*read) (void *pobj, void *buf, unsigned short len);
    /**
     * @brief 命令行守卫程序(不需要则填写NULL,允许所有命令执行)
     * @retval true - 允许执行, false - 忽略此命令
     */
    int (*cmd_guard)(char *cmdline);

} cli_port_t;

/*命令行对象,该结构与st_MODBUS_dev兼容*/
typedef struct cli_obj {
    int  (*get_val)(char *pdata);
    int  (*guard)(char *cmdline);/* 命令拦截 */
    int (*read) (void *pobj, void *buf, unsigned short len);
    int (*write)(void *pobj, void *buf, unsigned short len);
    unsigned char type   : 3;    /* 命令类型*/
    unsigned char enable : 1;    /* CLI 开关控制*/
    unsigned char echo   : 1;    /* 回显设置*/
} cli_obj_t;

void cli_enable(void);

void cli_disable (void);

void cli_echo_ctrl (int echo);

void cli_print(void *pobj, const char *format, ...);

int cli_process(void *pobj, char *pdata);

int cli_init(const cli_port_t *p);

size_t strsplit(char *s, const char *separator, char *list[], size_t len);

const cmd_item_t* find_cmd(const char *keyword, int n);


#endif	/* __CMDLINE_H */
