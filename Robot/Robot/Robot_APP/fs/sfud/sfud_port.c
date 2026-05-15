/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>
#ifdef SFUD_DEMO_MODE
#include "bsp_spi_flash.h"
//

typedef struct {
    SPI_TypeDef  *spix;
    GPIO_TypeDef *cs_gpiox;
    uint16_t      cs_gpio_pin;
} spi_user_data, *spi_user_data_t;
#endif

/**
  * @brief  使用SPI发送一个字节的数据
  * @param  byte：要发送的数据
  * @retval 返回接收到的数据
  */
extern uint8_t SPI_FLASH_SendByte(uint8_t byte);

#ifndef SFUD_DEMO_MODE
static spi_user_data spi = TGT_FLASH_CFG;
#else
static spi_user_data spi = { .spix = SPI2, .cs_gpiox = GPIOB, .cs_gpio_pin = GPIO_Pin_12 };
#endif
static char log_buf[256];

void sfud_log_debug(const char *file, const long line, const char *format, ...);

static void spi_lock(const sfud_spi *spi) {
    __disable_irq();
}

static void spi_unlock(const sfud_spi *spi) {
    __enable_irq();
}

/* about 100 microsecond delay */
static void retry_delay_100us(void) {
    uint32_t delay = 120;
    while(delay--);
}

static void FLASH_Transmit(const spi_user_data_t pdev, uint8_t *pbuf, size_t size)
{
    while (0 < size--) {
#ifndef SFUD_DEMO_MODE
        BSP_spi_sendByte(&pdev->bus, *pbuf);
#else
        SPI_FLASH_SendByte(*pbuf);
#endif
        pbuf ++;
    }
}

static void FLASH_Receive(const spi_user_data_t pdev, uint8_t *pbuf, size_t size)
{
    while (0 < size--) {
#ifndef SFUD_DEMO_MODE
        *pbuf = BSP_spi_sendByte(&pdev->bus, pdev->dummy);
#else
        *pbuf = SPI_FLASH_ReadByte();
#endif
        pbuf ++;
    }
}

static void FLASH_Enable(const spi_user_data_t pdev, uint8_t state)
{
#ifndef SFUD_DEMO_MODE
    BSP_gpio_set(pdev->hold, !state);
    BSP_gpio_set(pdev->wp, !state);
    BSP_gpio_set(pdev->cs, state);
#else
    if (0 == state) {
        GPIO_SetBits(pdev->cs_gpiox, pdev->cs_gpio_pin);
    }
    else {
        GPIO_ResetBits(pdev->cs_gpiox, pdev->cs_gpio_pin);
    }
#endif
}

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
        size_t read_size) {
    sfud_err result = SFUD_SUCCESS;
    spi_user_data_t spi_dev = (spi_user_data_t) spi->user_data;

    if (write_size) {
        SFUD_ASSERT(write_buf);
    }
    if (read_size) {
        SFUD_ASSERT(read_buf);
    }

    FLASH_Enable(spi_dev, true);
	FLASH_Transmit(spi_dev, (uint8_t *)write_buf, write_size);
	FLASH_Receive(spi_dev, read_buf, read_size);
    FLASH_Enable(spi_dev, false);

    return result;
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
        uint8_t *read_buf, size_t read_size) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your qspi read flash data code
     */

    return result;
}
#endif /* SFUD_USING_QSPI */

sfud_err sfud_spi_port_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your port spi bus and device object initialize code like this:
     * 1. rcc initialize
     * 2. gpio initialize
     * 3. spi device initialize
     * 4. flash->spi and flash->retry item initialize
     *    flash->spi.wr = spi_write_read; //Required
     *    flash->spi.qspi_read = qspi_read; //Required when QSPI mode enable
     *    flash->spi.lock = spi_lock;
     *    flash->spi.unlock = spi_unlock;
     *    flash->spi.user_data = &spix;
     *    flash->retry.delay = null;
     *    flash->retry.times = 10000; //Required
     */

	    switch (flash->index) {
    case SFUD_W25Q256FV_DEVICE_INDEX: {
//        /* RCC 初始化 */
//        rcc_configuration(&spi1);
//        /* GPIO 初始化 */
//        gpio_configuration(&spi1);
//        /* SPI 外设初始化 */
//        spi_configuration(&spi1);

        /* 同步 Flash 移植所需的接口及数据 */
        flash->spi.wr = spi_write_read;
        flash->spi.lock = spi_lock;
        flash->spi.unlock = spi_unlock;
        flash->spi.user_data = &spi;
        /* about 100 microsecond delay */
        flash->retry.delay = retry_delay_100us;
        /* adout 60 seconds timeout */
        flash->retry.times = 60 * 10000;

        break;
    }
    }


    return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\r\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD]");
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\r\n", log_buf);
    va_end(args);
}
