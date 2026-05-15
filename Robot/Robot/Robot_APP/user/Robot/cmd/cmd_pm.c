/******************************************************************************
 * @brief     低功耗控制命令
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021/03/07     Morro
 ******************************************************************************/
#include <stdlib.h>
#include "sys_util.h"
#include "sys_pm.h"
#include "cli.h"

/*
 * @brief   低功耗控制命令
 * @example pm 0 - 禁用低功耗
 * @example pm 1 - 启用低功耗
 */
static int _do_cmd_pm(void *pobj, int argc, char *argv[])
{
	gulUnWorkTick = sys_get_ms();
    if (argc != 1) {
        cli_print(pobj, "Command format error\r\n");
    }

    if (NULL != sys_strstr(argv[0], "on")) {
        sys_pm_enable();
        cli_print(pobj, "Lowpower enable...\r\n");
        return 1;
    }
    else if (NULL != sys_strstr(argv[0], "off")) {
        sys_pm_disable();
        cli_print(pobj, "Lowpower disable...\r\n");
        return 1;
    }
    return 0;
}cmd_register("pm", _do_cmd_pm, "Low power control command");
