/******************************************************************************
 * @brief    ST MCU 通用外设配置
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
#ifndef _BSP_H_
#define _BSP_H_

#include <string.h>
#include <time.h>
#include "comdef.h"
#include "target.h"
#include "sys_cfg.h"
#include "ringbuffer.h"

typedef void (*data_chkvalid)(void);

//public define ========================
#define DEVICE_INITED                   (1)

#define BSP_CMD_INVALID                 (0xFF)
#ifndef SYS_OS_TYPE
#define BSP_TICK_INTERVAL               (1000) //系统滴答时间(ms)

void BSP_systick_inc(void);
#else
#include "FreeRTOS.h"
#include "task.h"

#define BSP_TICK_INTERVAL               configTICK_RATE_HZ
#define BSP_systick_inc                 xTaskIncrementTick

#endif

extern unsigned int  sys_idle_time;

#define BSP_Obj2Reg(obj)                ((uint32_t)obj >> 8)
#define BSP_Reg2Obj(type, reg)          (type *)((reg) << 8)
#define BSP_DEV_obj(obj, flag)          ((uint32_t)(obj) | (flag))
#define BSP_DEV_type(name, id)          PERI_##name#id
#define BSP_DEV_RCCPERI(p)              ((p) << 0)
#define BSP_DEV_PARAM(p)                ((p) << 5)

#define BSP_rtc_wkupcfg(ms)             TGT_rtc_wkupcfg(ms)
#define BSP_rtc_cfg()                   TGT_rtc_conf()
#define BSP_rcc_cfg(cfg)                TGT_rcc_conf(cfg)
#define BSP_wdog_cfg(cfg)               TGT_iwdg_conf(cfg)
#define BSP_inti_cfg(cfg, cmd)          TGT_nvic_conf(cfg, cmd)
#define BSP_exti_cfg(cfg, cmd)          TGT_exti_conf(cfg, cmd)

typedef int (*BSP_bus_opt)(void *pbus, void *pbuf, unsigned short len);

typedef struct {
    void *pbus;
    BSP_bus_opt read;
    BSP_bus_opt write;
    void (*delay)(uint32_t ms);
} st_BSP_bus;

typedef struct {
    uint8_t type; //e_PERI_Type
    void   *pobj;
    void   *next;
} st_BSP_dev;

typedef struct {
    uint32_t cfg;
    uint32_t rccPeri;
    void (*rccClkCmd)(uint32_t, FunctionalState);
    void *object;
} st_DEV_cfg;

typedef struct {
    void (*hMcoCfg)(uint32_t, uint32_t);
    st_DEV_cfg pin;
    uint32_t cfg;
} st_MCO_Cfg;

typedef struct {
    st_DEV_cfg port;
    uint32_t mem_addr;
    uint32_t peri_addr;
} st_DMA_cfg;

typedef struct {
    st_DEV_cfg gpio;
    uint32_t exti;
} st_GPIO_Cfg;

typedef struct {
    st_DEV_cfg port; //e_PERI_Type
    uint32_t en_pin;
} st_UART_cfg;

typedef struct {
    st_DEV_cfg port;
    st_DEV_cfg scl;
    st_DEV_cfg sda;
    st_DEV_cfg wp;
    uint32_t  inti;
} st_I2C_Cfg;

typedef struct {
    st_DEV_cfg port;
    uint32_t cs_pin;
} st_SPI_cfg;

//GPIO interface ========================
#define BSP_GPIO_DIR_OUT    (0x1 << 6)
#define BSP_GPIO_EN_HIGH    (0x1 << 5)
typedef struct {
    uint32_t pin:5; //begin from 1
    uint32_t en :1; //0 - low, 1 - high
    uint32_t dir:1; //0 - in, 1 - out
    uint32_t res:1; //reserved bit
    uint32_t reg:24;
} st_GPIO_pin;

int BSP_event_wait(void *chkhandle, void *pobj, uint32_t event, uint32_t status);
int BSP_gpio_cfg(st_DEV_cfg *pCfg);
int BSP_gpio_set(uint32_t pin, uint8_t state);
int BSP_gpio_isEnable(uint32_t pin, uint8_t state);

//UART interface ========================
typedef struct {
    uint32_t res:8;
    uint32_t reg:24;
} st_UART_port;

typedef struct {
    st_UART_port   port;
    uint32_t       en_pin;
    st_ring_buf    rx_buf;
    st_ring_buf    tx_buf;
    data_chkvalid *check;
} st_uart_info;

int  BSP_uart_cfg(st_DEV_cfg *pCfg);
int  BSP_uart_init(st_uart_info *pUart, uint8_t *pRxBuf, uint16_t rxBufSize);
int  BSP_uart_baudrate_set(st_uart_info *pUart, uint32_t baudrate);
int  BSP_uart_write(st_uart_info *pUart, unsigned char *pbuf, uint16_t len);
int  BSP_uart_read(st_uart_info *pUart, unsigned char **pbuf, uint16_t len);
void BSP_uart_irqProcess(st_uart_info *pUart);

//SPI interface  ========================
typedef struct {
    uint32_t   cs:7;
    uint32_t  init:1;
    uint32_t  reg:24;
} st_spi_port;

typedef struct {
    st_spi_port port;
    uint32_t cs_pin;
} st_spi_info;

void BSP_spi_irqProcess(st_spi_info *pSpi);
int BSP_spi_cfg(st_spi_info *pSpi, st_DEV_cfg *pCfg);
int BSP_spi_write(st_spi_info *pSpi, uint8_t *pdata, uint16_t len);
int BSP_spi_read(st_spi_info *pSpi, uint8_t *pdata, uint16_t len);
int BSP_spi_enable(st_spi_info *pSpi, uint8_t state);
uint8_t BSP_spi_sendByte(st_spi_info *pSpi, uint8_t data);
void time_inc_auto(void);

//I2C interface  ========================

typedef struct {
    uint32_t addr:7;
    uint32_t wdReg:1; /* word address */
    uint32_t reg:24;
} st_i2c_port;

typedef struct {
	DMA_Stream_TypeDef *  DMA_Stream;
	uint32_t ulDMA_FLAG;
	char * pcSendBuf;
}User_Dma;

extern User_Dma DMA_DEV[];

int BSP_i2c_write(st_i2c_port *pI2c, uint8_t addr, uint16_t reg, const uint8_t *pdata, uint16_t len);
int BSP_i2c_read(st_i2c_port *pI2c, uint8_t addr, uint16_t reg, uint8_t *pdata, uint16_t len);
int BSP_i2c_wait_ready(st_i2c_port *pI2c, uint8_t addr);
int BSP_i2c_cfg(st_i2c_port *pI2c, st_DEV_cfg *pCfg, uint16_t ownAddr);
void BSP_i2c_irqProcess(st_i2c_port *pI2c);

//int BSP_adc_cfg(st_DEV_cfg *pcfg, uint32_t pubcfg, uint8_t isMultiAdc);
void ADC_ULTRAS_Init(void);
int BSP_mco_cfg(st_MCO_Cfg *pcfg);
//int BSP_dma_cfg(st_DMA_cfg *pcfg);
//int BSP_timer_cfg(st_DEV_cfg *pcfg);

void BSP_rtc_datetime(struct tm *timeinfo);

void BSP_wkup_irqProcess(void);
bool BSP_sys_isIdle(void);
//void timer6_1s_init(void);
unsigned int BSP_idletime_get(void);
unsigned int BSP_sys_sleep(unsigned int ms);

extern void uart_dma_init(USART_TypeDef * uart, uint32_t DMA_CHANNEL, DMA_Stream_TypeDef * DMA_STREAM);

void BSP_exti_irqProcess(void);
extern uint8_t uart_dma_tx(User_Dma * DMA_DEV, char *buf, uint16_t len);
extern void debug_printf(const char *fmt, ...);

#define debug_send(buf)				uart_dma_tx(&DMA_DEV[0], buf, strlen(buf))	

#endif
