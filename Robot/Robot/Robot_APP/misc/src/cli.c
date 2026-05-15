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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "cli.h"
#include "bsp.h"

static cli_obj_t _cli_obj; /*命令管理器对象 */
static const cmd_item_t _cmd_tbl_start SECTION("cli.cmd.0") = {0};
static const cmd_item_t _cmd_tbl_end SECTION("cli.cmd.4") = {0};

/*******************************************************************************
* @brief	   命令比较器
* @param[in]   none
* @return 	   参考strcmp
*******************************************************************************/
static int cmd_item_comparer(const void *item1, const void *item2)
{
    cmd_item_t *it1 = *((cmd_item_t **)item1);
    cmd_item_t *it2 = *((cmd_item_t **)item2);

    return strcmp(it1->name, it2->name);
}

/*******************************************************************************
 * @brief       查找命令
 * @param[in]   keyword - 命令关键字
 * @return      命令项
*******************************************************************************/
const cmd_item_t* find_cmd(const char *keyword, int n)
{
	const cmd_item_t *it;

    for (it = &_cmd_tbl_start + 1; it < &_cmd_tbl_end; it++) {
        if (!strncasecmp(keyword, it->name, n))
            return it;
    }

	return NULL;
}

/*******************************************************************************
 * @brief	   帮助命令
 * @param[in]
 * @return 	   1
 */
static int do_help (void *pobj, int argc, char *argv[])
{
    const cmd_item_t *item_start = &_cmd_tbl_start + 1;
    const cmd_item_t *item_end   = &_cmd_tbl_end;
	const cmd_item_t *cmdtbl[CLI_MAX_CMDS];
    int i, j, count;

    if (2 == argc) {
        if (NULL != (item_start = find_cmd(argv[1], strlen(argv[1])))) {
            cli_print(pobj, item_start->brief); /*命令使用信息----*/
            cli_print(pobj, "\r\n");
        }

        return 0;
    }

    for (i = 0; (item_end - item_start) > i && CLI_MAX_ARGS > i; i++) {
        cmdtbl[i] = &item_start[i];
    }

    count = i;
    /*对命令进行排序 ---------------------------------------------------------*/
    qsort(cmdtbl, i, sizeof(cmd_item_t *), cmd_item_comparer);
    cli_print(pobj, "\r\n");
    for (i = 0; i < count; i++) {
        cli_print(pobj, cmdtbl[i]->name);                        /*打印命令名------*/
        /*对齐调整*/
        j = strlen(cmdtbl[i]->name);
        if (j < 10) {
            j = 10 - j;
        }

        while (j--) {
            cli_print(pobj, " ");
        }

        cli_print(pobj, "- ");
		cli_print(pobj, cmdtbl[i]->brief);                       /*命令使用信息----*/
        cli_print(pobj, "\r\n");
    }

	return 1;
}
 /*注册帮助命令 ---------------------------------------------------------------*/
cmd_register("help", do_help, "list all command.");
cmd_register("?",    do_help, "alias for 'help'");

/*******************************************************************************
 * @brief      字符串分割  - 在源字符串查找出所有由separator指定的分隔符
 *                            (如',')并替换成字符串结束符'\0'形成子串，同时令list
 *                            指针列表中的每一个指针分别指向一个子串
 * @example
 *             input=> s = "abc,123,456,,fb$"
 *             separator = ",$"
 *
 *             output=>s = abc'\0' 123'\0' 456'\0' '\0' fb'\0''\0'
 *             list[0] = "abc"
 *             list[1] = "123"
 *             list[2] = "456"
 *             list[3] = ""
 *             list[4] = "fb"
 *             list[5] = ""
 *
 * @param[in] str             - 源字符串
 * @param[in] separator       - 分隔字符串
 * @param[in] list            - 字符串指针列表
 * @param[in] len             - 列表长度
 * @return    list指针列表项数，如上例所示则返回6
 ******************************************************************************/
size_t strsplit(char *s, const char *separator, char *list[], size_t len)
{
    size_t count = 0;
    size_t i;

    if (s == NULL || list == NULL || len == 0) {
        return 0;
    }

    list[count++] = s;
    for (i = 0; '\0' != s[i] && len > count; i++) {
        if (NULL != strchr(separator, s[i])) {
            list[count++] = &s[i + 1];                           /*指向下一个子串*/
            s[i] = '\0';
        }
    }

    return count;
}

/*******************************************************************************
 * @brief       获取设置值
 *******************************************************************************/
static int get_val(char *pdata)
{
    char *p;

    p = strchr(pdata, '=');
    if (NULL != p) {
        return atoi(p + 1);
    }
    return 0;
}

/*******************************************************************************
 * @brief       进入cli命令模式(cli此时自动处理用户输入的命令)
 * @param[in]   none
 * @return      none
 *******************************************************************************/
void cli_enable(void)
{
    _cli_obj.enable = true;
}

/*******************************************************************************
 * @brief       退出cli命令模式(cli此时不再处理用户输入的命令)
 * @param[in]   none
 * @return      none
 *******************************************************************************/
void cli_disable (void)
{
    _cli_obj.enable = false;
}

/*******************************************************************************
 * @brief       回显控制
 * @param[in]   echo - 回显开关控制(0/1)
 * @return      none
 **/
void cli_echo_ctrl (int echo)
{
    _cli_obj.echo = echo;
}

/**
 *@brief 打印一个格式化字符串到串口控制台
 *@retval
 */
void cli_print(void *pobj, const char *format, ...)
{
    char buf[CLI_MAX_CMD_LEN + CLI_MAX_CMD_LEN / 2];
    int len;
    va_list args;

    va_start (args, format);
    len = vsnprintf (buf, sizeof(buf), format, args);
    va_end (args);
    if (!pobj || !_cli_obj.write) {
        debug_printf(buf);
    }
    else {
        _cli_obj.write(pobj, buf, len);
    }
}

/*******************************************************************************
 * @brief       处理行
 * @param[in]   line - 命令行
 * @return      none
 *******************************************************************************/
int cli_process(void *pobj, char *pdata)
{
    if (NULL != pdata) {
        const char *start, *end;
        const cmd_item_t *it;
        char *argv[CLI_MAX_ARGS];
        int   argc, ret, isat = 0;

        if (_cli_obj.guard && !_cli_obj.guard(pdata)) { //命令拦截
            return -1;
        }

        if (_cli_obj.echo) { //回显
            cli_print(pobj, "%s\r\n", pdata);
        }

        argc = strsplit(pdata, ":;,", argv, CLI_MAX_ARGS);
        if (NULL == argv[0]) {
            return -1;
        }

        if (!strcasecmp("AT", argv[0])) {
            cli_print(pobj, "+OK\r\n");
            return -1;
        }

#if CLI_AT_ENABLE != 0
        if (0 == strncasecmp(argv[0], "AT+", 3)) {
            isat = 1;
        }
        else if (0 == strncasecmp(argv[0], "AT#", 3)) {
            isat = 1;
        }
#endif

        start= !isat ? argv[0] : argv[0] + 3;
        if ((end = strchr(start, '=')) != NULL) {
            _cli_obj.type = CLI_CMD_TYPE_SET;
        } else if ((end = strchr(start, '?')) != NULL) {
            _cli_obj.type = CLI_CMD_TYPE_QUERY;
        } else {
            _cli_obj.type = CLI_CMD_TYPE_EXEC;
            end = start + strlen(argv[0]);
        }

        if (start == end) {
            return -1;
        }

        if (NULL == (it = find_cmd(start, end - start))) {
            if (0 != isat) {
                cli_print(pobj, "%s\r\n", "+ERROR");
            }

            return -1;
        }

        ret = it->handler(pobj, argc - 1, &argv[1]);
        if (isat) {
            cli_print(pobj, "+%s\r\n", 0 > ret ? "ERROR" : "OK");
        }

        return ret;
    }
    return -1;
}

/*******************************************************************************
 * @brief       cli 初始化
 * @param[in]   p - cli驱动接口
 * @return      none
 *******************************************************************************/
int cli_init(const cli_port_t *p)
{
    _cli_obj.enable = true;
    _cli_obj.get_val= get_val;
    _cli_obj.guard  = p->cmd_guard;
    _cli_obj.write  = p->write;
    _cli_obj.read   = p->read;
    return 0;
}
