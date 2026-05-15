/******************************************************************************
 * @brief    EPROM 24CLxx driver interface
 *
 * Copyright (c) 2024, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-10     worker       inited version
 ******************************************************************************/

#ifndef	_HAL_EPROM_24CLXX_H_
#define	_HAL_EPROM_24CLXX_H_

#include "bsp.h"

//For FM24CL16, 2k bytes.
//For FM24CL64, 8k bytes.
/*eeprom model*/
typedef enum {
  	_24CL01_E = 0,
	_24CL02_E,
	_24CL04_E,
	_24CL08_E,
	_24CL16_E,
	_24CL32_E,
	_24CL64_E,
	_24CL128_E,
	_24CL256_E,
	_24CL512_E,
	_24CL1024_E
} e_24CLXX_Mode;

typedef struct {
    e_24CLXX_Mode mode;
    st_i2c_port bus;
    uint32_t wp;
} st_EPROM_info;

/*Interface declaration --------------------------------------------------------------------*/
int32_t EPROM_24CLXX_read(st_EPROM_info *pDev, uint32_t addr, uint8_t *pdata, uint32_t num);
int32_t EPROM_24CLXX_write(st_EPROM_info *pDev, uint32_t addr, const uint8_t *pdata, uint32_t num);
int32_t EPROM_24CLXX_erase(st_EPROM_info *pDev, uint32_t addr, uint8_t data, uint32_t num);
int8_t EPROM_24CLXX_init(st_EPROM_info *pDev);

int8_t EPROM_24CLXX_test(st_EPROM_info *pDev, uint32_t addr, uint32_t num);

#endif
