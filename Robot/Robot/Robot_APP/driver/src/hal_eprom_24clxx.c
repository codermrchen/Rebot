/******************************************************************************
 * @brief    EPROM 24CLxx driver implement
 *  refer to: https://www.cnblogs.com/acuity/p/12154108.html
 *          https://blog.csdn.net/shoutday/article/details/12992529
 * Copyright (c) 2024, <worker@junlei.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-10     worker       inited version
 ******************************************************************************/
#include <stdlib.h>
#include "sys_util.h"
#include "hal_eprom_24clxx.h"

/*
#if (TTY_RXBUF_SIZE & (TTY_RXBUF_SIZE - 1)) != 0
    #error "TTY_RXBUF_SIZE must be power of 2!"
#endif

#if (TTY_TXBUF_SIZE & (TTY_TXBUF_SIZE - 1)) != 0
    #error "TTY_RXBUF_SIZE must be power of 2!"
#endif
*/
#define _24CLXX_ERASE_FLAG              (0x80000000)
#define _24CLXX_slaveaddr_get(mode, devAddr, memOff) { \
                    if (_24CL16_E >= (mode)) { /* page address*/ \
                        devAddr |= ((memOff) >> 8) & 0x7; \
                    } \
                    devAddr <<= 1; \
                }

 /**
 * @brief  get page size
 * @param  model:eeprom model
 * @retval page size
 */
static uint8_t _24CLXX_pagesize(uint8_t mode)
{
    switch(mode) {
        case _24CL01_E:
        case _24CL02_E:
            return 8;
        case _24CL04_E:
        case _24CL08_E:
        case _24CL16_E:
            return 16;
        case _24CL32_E:
        case _24CL64_E:
        case _24CL128_E:
        case _24CL256_E:
        case _24CL512_E:
        case _24CL1024_E:
            return 32;
        default: break;
    }
    return 0;
}

  /**
 * @brief  get chip size
 * @param  model:eeprom model
 * @retval chip size,uinit:byte
 */
static uint32_t _24CLXX_chipsize(uint8_t mode)
{
    switch(mode) {
        case _24CL01_E:
        case _24CL02_E:
        case _24CL04_E:
        case _24CL08_E:
        case _24CL16_E:
        case _24CL32_E:
        case _24CL64_E:
        case _24CL128_E:
        case _24CL256_E:
        case _24CL512_E:
        case _24CL1024_E:
            return (0x1u << (mode + 7));
         default: break;
    }
    return 0;
}

/**
 * @brief  write one page.
 * @param  pDev pointer to the eeprom device struct.
 * @param  addr the address of write to.
 * @param  pdata the data to write.
 * @param  num number of bytes to write..
 * @retval return 0 if 0k,anything else is considered an error.
 */
static int32_t _24CLXX_write_data(st_EPROM_info *pDev, uint32_t addr, const uint8_t *pdata, uint32_t num)
{
    uint16_t page_size = _24CLXX_pagesize(pDev->mode);

    if (0 < page_size) {
        st_i2c_port *pi2c = &pDev->bus;
        uint8_t *page_buf = NULL;
        uint8_t  dev_addr;
        uint16_t snd_num;
        uint32_t left_num;
        int32_t  ret;

        if (num & _24CLXX_ERASE_FLAG) { // fill fix value
            page_buf = (uint8_t *)sys_malloc(page_size);
            if (!page_buf) {
                return -100;
            }

            sys_memset(page_buf, *pdata, page_size);
            num &= ~_24CLXX_ERASE_FLAG;
            pdata = page_buf;
        }

        for (left_num = num, snd_num = page_size - (addr % page_size);
            0 < left_num;
            left_num -= snd_num, addr += snd_num, snd_num = page_size) {

            if (left_num < snd_num) {
                snd_num = left_num;
            }

            dev_addr = pi2c->addr;
            _24CLXX_slaveaddr_get(pDev->mode, dev_addr, addr);
            ret = BSP_i2c_write(pi2c, dev_addr, addr, pdata, snd_num);
            if (0 > ret) {
                return ret;
            }

            ret = BSP_i2c_wait_ready(pi2c, dev_addr);
            if (0 > ret) {
                return ret;
            }

            if (!page_buf) {
                pdata += snd_num;
            }
        }

        if (NULL != page_buf) {
            sys_free(page_buf);
        }

        return (num - left_num);
    }
    return -9;
}

/**
 * @brief  write data to eeprom.
 * @param  pdev pointer to the eeprom device struct.
 * @param  addr the address of write to.
 * @param  pdata the data to write.
 * @param  num number of bytes to write..
 * @retval return 0 if 0k,anything else is considered an error.
 */
int32_t EPROM_24CLXX_write(st_EPROM_info *pDev, uint32_t addr, const uint8_t *pdata, uint32_t num)
{
    if (NULL != pDev && _24CLXX_chipsize(pDev->mode) >= (addr + num)) {
        int32_t ret;

        BSP_gpio_set(pDev->wp, DISABLE);//release write protect
        ret = _24CLXX_write_data(pDev, addr, pdata, num);
        BSP_gpio_set(pDev->wp, ENABLE); //write protect
        return ret;
    }
    return -9;
}

/**
 * @brief  erase the eeprom.
 * @param  pdev pointer to the eeprom device struct.
 * @param  addr the address to erase from.
 * @param  pdata padding data.
 * @param  num number of bytes to erase.
 * @retval return 0 if 0k,anything else is considered an error.
 */
int32_t EPROM_24CLXX_erase(st_EPROM_info *pDev, uint32_t addr, uint8_t data, uint32_t num)
{
    if (NULL != pDev) {
        uint32_t capacity = _24CLXX_chipsize(pDev->mode);
        int32_t ret;

        if (!num || capacity < (addr + num)) {
            num = capacity - addr;
        }

        BSP_gpio_set(pDev->wp, DISABLE);//release write protect
        ret = _24CLXX_write_data(pDev, addr, &data, (num | _24CLXX_ERASE_FLAG));
        BSP_gpio_set(pDev->wp, ENABLE);
        return ret;
    }
    return -9;
}

/**
 * @brief  read from the eeprom.
 * @param  pDev pointer to the eeprom device struct.
 * @param  addr the address to read from.
 * @param  pdata where to put read data.
 * @param  num number of bytes to read.
 * @retval return if less than 0 is considered an error, anything else is Ok.
 */
int32_t EPROM_24CLXX_read(st_EPROM_info *pDev, uint32_t addr, uint8_t *pdata, uint32_t num)
{
    if (NULL != pDev && _24CLXX_chipsize(pDev->mode) >= (addr + num)) {
        st_i2c_port *pi2c = &pDev->bus;
        uint8_t dev_addr = pi2c->addr;
        int ret;

        _24CLXX_slaveaddr_get(pDev->mode, dev_addr, addr);
        ret = BSP_i2c_read(pi2c, dev_addr, addr, pdata, num);
        return ret;
    }
    return -9;
}

/*
 * @brief       device≥ı ºªØ
 * @param[in]   pCfg - config infomation
 * @return      < 0 error, else is ok
 */
int8_t EPROM_24CLXX_init(st_EPROM_info *pDev)
{
    if (NULL != pDev && 0 != pDev->bus.reg && 0 != pDev->bus.addr) {
        BSP_gpio_set(pDev->wp, ENABLE); //write protect
        if (_24CL16_E < pDev->mode) {
            st_i2c_port *pi2c = &pDev->bus;

            pi2c->wdReg = 0x1;
        }
        return 0;
    }
    return -9;
}
