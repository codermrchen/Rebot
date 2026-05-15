/******************************************************************************
 * @brief    ST MCU 通用外设配置
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
#include "bsp.h"
#include "platform.h"
#include "net_task_uart.h"

#define TGT_obj_set(obj, val, flag)     (*(uint32_t *)(obj) = (uint32_t)(val) | (flag))
#define TGT_PERI_GRPNO(obj, base)       (((uint32_t)(obj) - (uint32_t)(base)) / 0x400)

#define isValidObj(obj)                 (0 != *(uint32_t *)&obj)

#define BSP_TIMEOUT_INTERVAL            (1)
#define BSP_TIMEOUT_MAX                 (100)
#define BSP_SLEEP_FLAG                  (0x80)
#define BSP_WKUP_RTC				    (0x01)

typedef uint32_t (*EvtChkFunc1)(void *, uint32_t);
typedef uint32_t (*EvtChkFunc2)(uint32_t);
typedef uint32_t (*EvtChkFunc3)(void);

User_Dma DMA_DEV[] = {
	{DMA1_Stream0, DMA_FLAG_TCIF0 | DMA_FLAG_HTIF0 |DMA_FLAG_TEIF0 | DMA_FLAG_DMEIF0 | DMA_FLAG_FEIF0},
	{DMA1_Stream1, DMA_FLAG_TCIF1 | DMA_FLAG_HTIF1 |DMA_FLAG_TEIF1 | DMA_FLAG_DMEIF1 | DMA_FLAG_FEIF1},
	{DMA1_Stream2, DMA_FLAG_TCIF2 | DMA_FLAG_HTIF2 |DMA_FLAG_TEIF2 | DMA_FLAG_DMEIF2 | DMA_FLAG_FEIF2},
	{DMA1_Stream3, DMA_FLAG_TCIF3 | DMA_FLAG_HTIF3 |DMA_FLAG_TEIF3 | DMA_FLAG_DMEIF3 | DMA_FLAG_FEIF3},
	{DMA1_Stream4, DMA_FLAG_TCIF4 | DMA_FLAG_HTIF4 |DMA_FLAG_TEIF4 | DMA_FLAG_DMEIF4 | DMA_FLAG_FEIF4},
	{DMA1_Stream5, DMA_FLAG_TCIF5 | DMA_FLAG_HTIF5 |DMA_FLAG_TEIF5 | DMA_FLAG_DMEIF5 | DMA_FLAG_FEIF5},
	{DMA1_Stream6, DMA_FLAG_TCIF6 | DMA_FLAG_HTIF6 |DMA_FLAG_TEIF6 | DMA_FLAG_DMEIF6 | DMA_FLAG_FEIF6},
	{DMA1_Stream7, DMA_FLAG_TCIF7 | DMA_FLAG_HTIF7 |DMA_FLAG_TEIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7},
};

/*
 * @brief	   RTC唤醒标志
 */
static volatile uint8_t _rtc_iswakekup = 0;
/*
 * @brief	   系统空闲时间(用于功耗控制)
 */
static uint32_t _sys_idle_time = 0;
static volatile uint32_t s_tick = 0; //系统滴答计时

extern void sys_delay_ms(unsigned int ms);

const unsigned char month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void time_inc_auto(void)
{	//时间自动增加
	if(gCurrentTime.Second < 59)
		gCurrentTime.Second++;
	else
	{
		gCurrentTime.Second = 0;
		if(gCurrentTime.Minute < 59)
			gCurrentTime.Minute++;
		else {
			gCurrentTime.Minute = 0;
			if(gCurrentTime.Hour < 23)
				gCurrentTime.Hour++;
			else
			{	
				gWorkWorker.ReflashWorkTime = 0;	//凌晨重新获取时间
				gCurrentTime.Hour = 0;
				if(gCurrentTime.WeekDay < 7)
					gCurrentTime.WeekDay++;
				else
					gCurrentTime.WeekDay = 1;
				if(gCurrentTime.Day < (month_days[gCurrentTime.Month - 1] - 1))
					gCurrentTime.Day++;
				else {
					gCurrentTime.Day = 1;
					if(gCurrentTime.Month < 11)
						gCurrentTime.Month++;
					else
					{
						gCurrentTime.Month = 0;
						gCurrentTime.Year++;
					}
				}
			}
		}
	}
}

/*
 * @brief   增加系统节拍数(定时器中断中调用,1ms 1次)
 */
#ifndef SYS_OS_TYPE
void BSP_systick_inc(void)
{
	s_tick += BSP_TICK_INTERVAL;
}
#endif

/**
  * @brief  从停止模式中唤醒后,打开HSE，PLL
  */
static void _wkup_sys_cfg(void)
{
    volatile unsigned int retry = 0;

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET && ++retry);

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);
    /* Wait till PLL is ready */
    while (RESET == RCC_GetFlagStatus(RCC_FLAG_PLLRDY));

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    /* Wait till PLL is used as system clock source */
    while (0x08 != RCC_GetSYSCLKSource());
}

static int _BSP_event_wait1(void *chkhandle, void *pobj, uint32_t event, uint32_t status, int32_t timeout)
{
    if (0 != chkhandle && NULL != pobj && 0 != event) {
        EvtChkFunc1 pchkfun = (EvtChkFunc1)chkhandle;

        do {
            sys_delay_ms(BSP_TIMEOUT_INTERVAL);
            if (status == pchkfun(pobj, event)) {
                return 0;
            }
        } while (--timeout > 0);
    }
    return -1;
}

static int _BSP_event_wait2(void *chkhandle, uint32_t event, uint32_t status, int32_t timeout)
{
    if (NULL != chkhandle && 0 != event) {
        EvtChkFunc2 pchkfun = (EvtChkFunc2)chkhandle;

        do {
            sys_delay_ms(BSP_TIMEOUT_INTERVAL);
            if (status == pchkfun(event)) {
                return 0;
            }
        } while (--timeout > 0);
    }
    return -1;
}

static int _BSP_event_wait3(void *chkhandle, uint32_t *pobj, uint32_t event, uint32_t status, int32_t timeout)
{
    if (NULL != chkhandle || 0 != pobj) {
        EvtChkFunc3 pchkfun = (EvtChkFunc3)chkhandle;

        do {
            sys_delay_ms(BSP_TIMEOUT_INTERVAL);
            if (NULL != pchkfun) {
                if (status == pchkfun()) {
                    return 0;
                }
            }
            else if (0 != event) {
                if (event & *pobj) {
                    return 0;
                }
            }
            else if (status == *pobj) {
                    return 0;
            }
        } while (--timeout > 0);
    }
    return -1;
}

int BSP_event_wait(void *chkhandle, void *pobj, uint32_t event, uint32_t status)
{
    int32_t timeout = BSP_TIMEOUT_MAX;

    if (NULL != chkhandle && NULL != pobj) {
        return _BSP_event_wait1(chkhandle, pobj, event, status, timeout);
    }
    else if (NULL == pobj && 0 != event) {
        return _BSP_event_wait2(chkhandle, event, status, timeout);
    }
    return _BSP_event_wait3(chkhandle, pobj, event, status, timeout);
}

/*
 * @brief	   rtc唤醒配置
 * @param[in]  none(1~4000ms)
 */
int BSP_rtc_wkup_cfg(unsigned int ms)
{
    RTC_WakeUpCmd(DISABLE);
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div2);
    RTC_SetWakeUpCounter((unsigned int )(((ms << 14) / 1000.0) + 0.5 ) - 1);

    RTC_ClearITPendingBit(RTC_IT_WUT);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    RTC_WakeUpCmd(ENABLE);
    return 0;
}

/*
 * @brief	   系统休眠接口实现
 * @param[in]  ms  - 休眠时长
 * @note       休眠之后需要考虑两件事情,1个是需要定时起来给喂看门狗,否则会在休眠期
 *             间发送重启.另外一件事情是需要补偿休眠时间给系统滴答时钟,否则会造成
 *             系统时钟不准.
 * @return 	   实际休眠时间
 */
unsigned int BSP_sys_sleep(unsigned int ms)
{
    unsigned int start_time = sys_get_ms();

    while (ms > (sys_get_ms() - start_time)) {
        _rtc_iswakekup = BSP_SLEEP_FLAG;
        PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
        if (!(_rtc_iswakekup & ~BSP_SLEEP_FLAG)) {
            break;
        }
    }

    _rtc_iswakekup = 0;
    _wkup_sys_cfg();

    return (sys_get_ms() - start_time);
}

/*
 * @brief	   rtc唤醒中断
 * @param[in]  none
 * @return 	   none
 */
void BSP_wkup_irqProcess(void)
{
//	time_inc_auto();
    if (_rtc_iswakekup & BSP_SLEEP_FLAG) {
        _rtc_iswakekup |= BSP_WKUP_RTC;
		BSP_systick_inc();
    }
}

/*
 * @brief	    exti interrupt process
 * @param[in]   usart configuration
 * @return 	    none
 */
void BSP_exti_irqProcess(void)
{
    uint32_t line;
    uint8_t i;

    for (i = 0; EXTI_LINE_MAX > i; i++) {
        line = (0x1 << i);
        if (RESET != EXTI_GetITStatus(line)) {
            EXTI_ClearITPendingBit(line);
        }
    }

    _sys_idle_time = sys_get_ms();
}

/*
 * @brief  默认开机或者串口有通信活动, 3S内不允许进入低功耗
 */
bool BSP_sys_isIdle(void)
{
    if (0 == _sys_idle_time) {
        _sys_idle_time = sys_get_ms();
    }if (sys_istimeout(_sys_idle_time, 3000)
        && ring_buf_isallempty()) {
        return true;
    }
    return false;
}

/**=============================
        TIMER interface
==============================**/
//int BSP_timer_cfg(st_DEV_cfg *pCfg)
//{
//    if (NULL != pCfg) {
//        if (NULL != pCfg->rccClkCmd) {
//            pCfg->rccClkCmd(pCfg->rccPeri, ENABLE);
//        }
//
//        if ( 0 > TGT_timer_conf(pCfg->object, pCfg->cfg)) {
//            return -1;
//        }
//        return 0;
//    }
//    return -9;
//}

//void timer6_1s_init(void)
//{
//	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
//	
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
//	
//	TIM_TimeBaseStructure.TIM_Period = 20000 - 1; // 1s
//	TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;      // 84MHz / 84 = 1MHz
//	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
//	TIM_ClearFlag(TIM6, TIM_FLAG_Update);
//	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
//	
//	NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//
//	NVIC_Init(&NVIC_InitStructure);
//	TIM_Cmd(TIM6, ENABLE);
//}


/**=============================
        GPIO interface
==============================**/
int BSP_gpio_cfg(st_DEV_cfg *pCfg)
{
    if (NULL != pCfg && IS_GPIO_ALL_PERIPH(pCfg->object)) {
        if (NULL != pCfg->rccClkCmd) {
            pCfg->rccClkCmd(pCfg->rccPeri, ENABLE);
        }

        if ( 0 > TGT_gpio_conf(pCfg->object, pCfg->cfg)) {
            return -1;
        }
        return 0;
    }
    return -9;
}

/*
 * @brief	    GPIO??
 * @param[in]   gpio object
 * @return if set is successful
 */
int BSP_gpio_set(uint32_t pin, uint8_t state)
{
    if (0 != pin) {
        st_GPIO_pin *gpio = (st_GPIO_pin *)&pin;
        GPIO_TypeDef *grpObj = BSP_Reg2Obj(GPIO_TypeDef, gpio->reg);
        uint16_t  npin = (0x1 << (gpio->pin - PIN0));

        state = (!state ? !gpio->en : gpio->en);
        if (!state) {
            GPIO_ResetBits(grpObj, npin);
        } else {
            GPIO_SetBits(grpObj, npin);
        }

        return BSP_gpio_isEnable(pin, state);
    }
    return -1;
}

/*
 * @brief	    ??GPIO????
 * @param[in]   gpio object
 * @return 	    if gpio is enable is true
 */
int BSP_gpio_isEnable(uint32_t pin, uint8_t state)
{
		uint8_t test;
	
    if (0 != pin) {
        st_GPIO_pin *gpio = (st_GPIO_pin *)&pin;
        GPIO_TypeDef *grpObj = BSP_Reg2Obj(GPIO_TypeDef, gpio->reg);
        uint16_t  npin = (0x1 << (gpio->pin - PIN0));

        if (!gpio->dir) {
			test = GPIO_ReadInputDataBit(grpObj, npin);
            return (state == test);
        }

		test = GPIO_ReadOutputDataBit(grpObj, npin);
        return (state == test);
    }
    return -1;
}

/**=============================
        Uart interface
==============================**/
int BSP_uart_cfg(st_DEV_cfg *pCfg)
{
    int nret = 0;

    if (NULL != pCfg) {
        pCfg->rccClkCmd(pCfg->rccPeri, ENABLE);
        nret = TGT_uart_conf(pCfg->object, pCfg->cfg);
        if (0 <= nret) {
        }
    }
    return nret;
}

int BSP_uart_init(st_uart_info *pUart, uint8_t *pRxBuf, uint16_t rxBufSize)
{
    if (NULL != pUart && NULL != pRxBuf && 0 < rxBufSize) {
        if (0 != pUart->en_pin) {
            BSP_gpio_set(pUart->en_pin, DISABLE);
        }

        if (0 <= ring_buf_init(&pUart->rx_buf, pRxBuf, rxBufSize)) {
            USART_TypeDef *pObj = BSP_Reg2Obj(USART_TypeDef, pUart->port.reg);

            USART_Cmd(pObj, ENABLE);
            TGT_dev_register(pObj, pUart);
            return 0;
        }
    }
    return -1;
}

int BSP_uart_baudrate_set(st_uart_info *pUart, uint32_t baudrate)
{
    if (NULL != pUart && 0 != baudrate) {
        USART_TypeDef *pObj = BSP_Reg2Obj(USART_TypeDef, pUart->port.reg);

        USART_Cmd(pObj, DISABLE);
        USART_SetBaudRate(pObj, baudrate);
        USART_Cmd(pObj, ENABLE);
        return 0;
    }
    return -1;
}

/*
 * @brief	    向串口发送缓冲区内写入数据并启动发送
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    实际写入长度(如果此时缓冲区满,则返回len)
 */
int BSP_uart_write(st_uart_info *pUart, unsigned char *pbuf, uint16_t len)
{
    if (NULL != pUart && isValidObj(pUart->port)) {
        USART_TypeDef *portObj = BSP_Reg2Obj(USART_TypeDef, pUart->port.reg);
        st_ring_buf *ptxBuf = &pUart->tx_buf;
        uint16_t pos, timeout = 20000;
        int nret = 0;

        for (pos = 0; len > pos && 0 < timeout; ) {
            nret = ring_buf_put(ptxBuf, &pbuf[pos], len);
            if (0 >= nret) {
                if (0 >= ring_buf_istimeout(ptxBuf, timeout))
                    continue;
                else
                    break;
            }

            BSP_gpio_set(pUart->en_pin, ENABLE);
            USART_ITConfig(portObj, USART_IT_TXE, ENABLE);
            pos += nret;
        }

        if (0 < ring_buf_istimeout(ptxBuf, timeout)) { //To avoid data is covered.
            BSP_gpio_set(pUart->en_pin, DISABLE);
            USART_ITConfig(portObj, USART_IT_TXE, DISABLE);
            ring_buf_clear(ptxBuf);
        }
    }
    return 0;
}

/*
 * @brief	    读取串口接收缓冲区的数据
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    (实际读取长度)如果接收缓冲区的有效数据大于len则返回len否则返回缓冲
 *              区有效数据的长度
 */
int BSP_uart_read(st_uart_info *pUart, unsigned char **pbuf, uint16_t len)
{
    int nret = 0;

    if (NULL != pbuf && NULL != pUart && isValidObj(pUart->port)) {
        nret = ring_buf_getdata(&pUart->rx_buf, (unsigned char **)pbuf, len);
        if (0 != pUart->rx_buf.lock) {
            ring_buf_clear(&pUart->rx_buf);
        }
    }
    return nret;
}

/*
 * @brief	    usart interrupt process
 * @param[in]   usart configuration
 * @return 	    none
 */
void BSP_uart_irqProcess(st_uart_info *pUart)
{
    if (NULL != pUart && isValidObj(pUart->port)) {
        USART_TypeDef *portObj = BSP_Reg2Obj(USART_TypeDef, pUart->port.reg);
        uint8_t data;

        if (RESET != USART_GetITStatus(portObj, USART_IT_RXNE)) {
            USART_ClearITPendingBit(portObj,USART_IT_RXNE);
            data = USART_ReceiveData(portObj);
            ring_buf_put(&pUart->rx_buf, &data, 1);
        }

        if (RESET != USART_GetITStatus(portObj, USART_IT_TXE)) {
            USART_ClearITPendingBit(portObj, USART_IT_TXE);
            if (0 < ring_buf_get(&pUart->tx_buf, &data, 1)) {
                USART_SendData(portObj, data);               
            }
            else if (RESET != (portObj->SR & USART_FLAG_TC)) {
                USART_ITConfig(portObj, USART_IT_TXE, DISABLE);
                BSP_gpio_set(pUart->en_pin, DISABLE);
            }
        }
#ifdef USART_IT_ORE_RX
        if (RESET != USART_GetITStatus(portObj, USART_IT_ORE_RX)) {
            USART_ClearITPendingBit(portObj, USART_IT_ORE_RX);
            data = USART_ReceiveData(portObj);
        }
#endif
    }
}

/////////////////////////////////////////////////////////////////////////
uint8_t BSP_spi_sendByte(st_spi_info *pSpi, uint8_t data)
{
    if (NULL != pSpi) {
        SPI_TypeDef *pObj = BSP_Reg2Obj(SPI_TypeDef, pSpi->port.reg);

        if (0 > BSP_event_wait(SPI_I2S_GetFlagStatus, pObj, SPI_I2S_FLAG_TXE, SET)) {
            return 0;
        }

        SPI_I2S_SendData(pObj, data);
        if (0 > BSP_event_wait(SPI_I2S_GetFlagStatus, pObj, SPI_I2S_FLAG_RXNE, SET)) {
            return 0;
        }

        data = SPI_I2S_ReceiveData(pObj);
        return data;
    }
    return 0xFF;
}

/*
 * @brief	    向串口发送缓冲区内写入数据并启动发送
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    实际写入长度(如果此时缓冲区满,则返回len)
 */
int BSP_spi_write(st_spi_info *pSpi, uint8_t *pdata, uint16_t len)
{
    uint8_t tmp;
    int i;

    for (i = 0; len > i; i++) {
        tmp = BSP_spi_sendByte(pSpi, pdata[i]);
        if (tmp != pdata[i]) {
            tmp = pdata[i];
        }
    }

    return i;
}

/*
 * @brief	    向串口发送缓冲区内写入数据并启动发送
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    实际写入长度(如果此时缓冲区满,则返回len)
 */
int BSP_spi_read(st_spi_info *pSpi, uint8_t *pdata, uint16_t len)
{
    int32_t i;

    for (i = 0; len > i; i++) {
        pdata[i] = BSP_spi_sendByte(pSpi, 0xFF);
    }

    return i;
}

int BSP_spi_enable(st_spi_info *pSpi, uint8_t state)
{
    if (NULL != pSpi) {
        return BSP_gpio_set(pSpi->cs_pin, state);
    }
    return -1;
}

int BSP_spi_cfg(st_spi_info *pSpi, st_DEV_cfg *pCfg)
{
    int nret = -9;

    if (NULL != pCfg) {
        pCfg->rccClkCmd(pCfg->rccPeri, ENABLE);
        nret = TGT_spi_conf(pCfg->object, pCfg->cfg);
        if (0 <= nret) {
            SPI_Cmd(pCfg->object, ENABLE);
        }

        if (NULL != pSpi) {
            TGT_obj_set(&pSpi->port, pCfg->object, 0);
            TGT_dev_register(pCfg->object, pSpi);
        }
    }
    return nret;
}

/*
 * @brief	    usart interrupt process
 * @param[in]   usart configuration
 * @return 	    none
 */
void BSP_spi_irqProcess(st_spi_info *pSpi)
{
    if (NULL != pSpi && isValidObj(pSpi->port)) {
        SPI_TypeDef *portObj = BSP_Reg2Obj(SPI_TypeDef, pSpi->port.reg);
        //uint8_t data;

        if (RESET != SPI_I2S_GetITStatus(portObj, SPI_I2S_IT_RXNE)) {
            SPI_I2S_ClearITPendingBit(portObj, SPI_I2S_IT_RXNE);
        }

        if (RESET != SPI_I2S_GetITStatus(portObj, SPI_I2S_IT_TXE)) {
            SPI_I2S_ClearITPendingBit(portObj, SPI_I2S_IT_TXE);
        }

        if (RESET != SPI_I2S_GetITStatus(portObj, SPI_I2S_IT_ERR)) {
            SPI_I2S_ClearITPendingBit(portObj, SPI_I2S_IT_ERR);
        }
    }
}

//////////////////////////////////////////////////////
static int BSP_i2c_addr_set(I2C_TypeDef *pObj, uint8_t addr, uint16_t reg, uint8_t flag)
{
    if (0 > BSP_event_wait(I2C_GetFlagStatus, pObj, I2C_FLAG_BUSY, DISABLE)) {
        return -1;
    }

    I2C_GenerateSTART(pObj, ENABLE);
    if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_MODE_SELECT, ENABLE)) {
        return -2;
    }

    I2C_Send7bitAddress(pObj, addr, I2C_Direction_Transmitter);
    if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, ENABLE)) {
        return -3;
    }

    if (flag & 0x80) {
        I2C_Cmd(pObj, ENABLE);
    }

    if (flag & 0x1) {
        I2C_SendData(pObj, (reg >> 8));
        if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_BYTE_TRANSMITTED, ENABLE)) {
            return -4;
        }
    }

    I2C_SendData(pObj, reg);
    if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_BYTE_TRANSMITTED, ENABLE)) {
        return -5;
    }
    return 0;
}

int BSP_i2c_wait_ready(st_i2c_port *pI2c, uint8_t addr)
{
    if (0 != pI2c) {
        I2C_TypeDef *pObj = BSP_Reg2Obj(I2C_TypeDef, pI2c->reg);
        int16_t timeout = BSP_TIMEOUT_MAX;

        do {
            sys_delay_ms(BSP_TIMEOUT_INTERVAL);
            I2C_GenerateSTART(pObj, ENABLE);
            I2C_ReadRegister(pObj, I2C_Register_SR1);
            I2C_Send7bitAddress(pObj, addr, I2C_Direction_Transmitter);
        } while (!(I2C_ReadRegister(pObj, I2C_Register_SR1) & 0x0002) && 0 < timeout--);

        I2C_ClearFlag(pObj, I2C_FLAG_AF);
        I2C_GenerateSTOP(pObj, ENABLE);
        if (0 > timeout) {
            timeout = 0;
        }

        return (0 > timeout ? -1 : 0);
    }
    return -1;
}

int BSP_i2c_write(st_i2c_port *pI2c, uint8_t addr, uint16_t reg, const uint8_t *pdata, uint16_t len)
{
    if (0 != pI2c && NULL != pdata && 0 < len) {
        I2C_TypeDef *pObj = BSP_Reg2Obj(I2C_TypeDef, pI2c->reg);
        uint16_t i;

        if (0 > BSP_i2c_addr_set(pObj, addr, reg, pI2c->wdReg)) {
            return -1;
        }

        for (i = 0; i < len; i++) {
            I2C_SendData(pObj, pdata[i]);
            if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_BYTE_TRANSMITTED, ENABLE)) {
                return -2;
            }
        }

        I2C_GenerateSTOP(pObj, ENABLE);
        return i;
    }
    return -9;
}

int BSP_i2c_read(st_i2c_port *pI2c, uint8_t addr, uint16_t reg, uint8_t *pdata, uint16_t len)
{
    if (0 != pI2c && NULL != pdata && 0 < len) {
        I2C_TypeDef *pObj = BSP_Reg2Obj(I2C_TypeDef, pI2c->reg);
        uint16_t i;

        if (0 != BSP_i2c_addr_set(pObj, addr, reg, pI2c->wdReg | 0x80)) {
            return -1;
        }

        I2C_GenerateSTART(pObj, ENABLE);
        if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_MODE_SELECT, ENABLE)) {
            return -2;
        }

        I2C_Send7bitAddress(pObj, (pI2c->addr << 1), I2C_Direction_Receiver);
        if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, ENABLE)) {
            return -3;
        }

        for (i = 0; i < len; pdata[i++] = I2C_ReceiveData(pObj)) {
            if ((i + 1) == len) {
                I2C_AcknowledgeConfig(pObj, DISABLE);
            }

            if (0 > BSP_event_wait(I2C_CheckEvent, pObj, I2C_EVENT_MASTER_BYTE_RECEIVED, ENABLE)) {
                return -4;
            }
        }

        I2C_GenerateSTOP(pObj, ENABLE);
        I2C_AcknowledgeConfig(pObj, ENABLE);
        return i;
   }
   return -9;
}

int BSP_i2c_cfg(st_i2c_port *pI2c, st_DEV_cfg *pCfg, uint16_t ownAddr)
{
    int8_t nret = -9;

    if (NULL != pCfg) {
        pCfg->rccClkCmd(pCfg->rccPeri, ENABLE);
        nret = TGT_i2c_conf(pCfg->object, pCfg->cfg, ownAddr);
        if (0 <= nret) {
            I2C_Cmd(pCfg->object, ENABLE);
            if (NULL != pI2c) {
                TGT_obj_set(pI2c, pCfg->object, 0);
                TGT_dev_register(pCfg->object, pI2c);
            }
        }
    }
    return nret;
}

/*
 * @brief	    i2c interrupt process
 * @param[in]   usart configuration
 * @return 	    none
 */
void BSP_i2c_irqProcess(st_i2c_port *pI2c)
{
    if (NULL != pI2c && isValidObj(*pI2c)) {
        I2C_TypeDef *portObj = BSP_Reg2Obj(I2C_TypeDef, pI2c->reg);
        //uint8_t data;

        if (RESET != I2C_GetITStatus(portObj, I2C_IT_RXNE)) {
            I2C_ClearITPendingBit(portObj, I2C_IT_RXNE);
        }

        if (RESET != I2C_GetITStatus(portObj, I2C_IT_EVT)) {
            I2C_ClearITPendingBit(portObj, I2C_IT_EVT);
        }

        if (RESET != I2C_GetITStatus(portObj, I2C_IT_ERR)) {
            I2C_ClearITPendingBit(portObj, I2C_IT_ERR);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
int BSP_mco_cfg(st_MCO_Cfg *pmco)
{
    if (NULL != pmco && NULL != pmco->hMcoCfg) {
        if (0 > BSP_gpio_cfg(&pmco->pin)) {
            return -2;
        }
        st_MCO_conf *pobj = (st_MCO_conf *)&pmco->cfg;

        if (RCC_MCO1Config == pmco->hMcoCfg) {
            pmco->hMcoCfg(pobj->out << 21, pobj->div << 24);
        }
        else if (RCC_MCO2Config == pmco->hMcoCfg) {
            pmco->hMcoCfg(pobj->out << 30, pobj->div << 27);
        }
        else {
            return -3;
        }
        return 0;
    }
    return -1;
}

//int BSP_dma_cfg(st_DMA_cfg *pcfg)
//{
//    int nret = 0;
//
//    pcfg->port.rccClkCmd(pcfg->port.rccPeri, ENABLE);
//    nret = TGT_dma_conf(pcfg->port.object, pcfg->port.cfg, pcfg->mem_addr, pcfg->peri_addr);
//    return nret;
//}

//int BSP_adc_cfg(st_DEV_cfg *padc, uint32_t pubcfg, uint8_t isMultiAdc)
//{
//    int nret = 0;
//
//    TGT_adc_pubcfg(pubcfg);
//
//    padc->rccClkCmd(padc->rccPeri, ENABLE);
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
//    nret = TGT_adc_conf(padc->object, padc->cfg, isMultiAdc);
//    if (0 <= nret) {
//        ADC_Cmd(padc->object, ENABLE);
//        if (NONE == nret) {
//            ADC_SoftwareStartConv(padc->object);
//        }
//    }
//
//    return nret;
//}

#ifdef ADC_DOUBLE
void ADC_ULTRAS_Init(void)
{
    // 使能ADC和DMA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); // 启用DMA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;// | GPIO_Pin_1;   // PA0和PA1
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;             // 模拟输入
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;         // 下拉
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 配置DMA
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = &ADC1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)_ultra_adc;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = TGT_ULTRA_ADC_CHNS;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure); // 指定DMA流

	DMA_Cmd(DMA2_Stream0, ENABLE);

	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;	//独立ADC模式
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;	//时钟为4分频
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles; 
	ADC_CommonInit(&ADC_CommonInitStructure);

    // 配置ADC
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 使能扫描模式
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 使用连续转换模式
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_NbrOfConversion = 1; // 2个通道
    ADC_Init(ADC1, &ADC_InitStructure);

    // 配置ADC通道
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_3Cycles);
    //ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_3Cycles);

	// 启用DMA请求
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);

    // 启用ADC
    ADC_Cmd(ADC1, ENABLE);

    // 启动ADC转换
    ADC_SoftwareStartConv(ADC1);
}
#else 
void ADC_ULTRAS_Init(void)
{
    // 使能ADC和DMA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); // 启用DMA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;   // PA0和PA1
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;             // 模拟输入
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;         // 下拉
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 配置DMA
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)_ultra_adc;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = TGT_ULTRA_ADC_CHNS;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure); // 指定DMA流

	DMA_Cmd(DMA2_Stream0, ENABLE);

	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;	//独立ADC模式
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;	//时钟为4分频
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles; 
	ADC_CommonInit(&ADC_CommonInitStructure);

    // 配置ADC
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 使能扫描模式
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 使用连续转换模式
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_NbrOfConversion = 1; // 2个通道
    ADC_Init(ADC1, &ADC_InitStructure);

    // 配置ADC通道
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_28Cycles);

	// 启用DMA请求
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);

    // 启用ADC
    ADC_Cmd(ADC1, ENABLE);

    // 启动ADC转换
    ADC_SoftwareStartConv(ADC1);
}

#endif

void uart_dma_init(USART_TypeDef * uart, uint32_t DMA_CHANNEL, DMA_Stream_TypeDef * DMA_STREAM)
{
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	
    DMA_InitStructure.DMA_Channel = DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&uart->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = NULL;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Cmd(DMA_STREAM, DISABLE);
    DMA_Init(DMA_STREAM, &DMA_InitStructure); // 指定DMA流
    USART_DMACmd(uart, USART_DMAReq_Tx, ENABLE);
}

/* 阻塞式 DMA 发送，返回 0 表示成功 */
uint8_t uart_dma_tx(User_Dma * DMA_DEV, char *buf, uint16_t len)
{
	static unsigned char sucFlag = 0;
	uint32_t ulTick;

	if(sucFlag == 1)
	{
		ulTick = sys_get_ms();
	    while (DMA_GetCmdStatus(DMA_DEV->DMA_Stream) != DISABLE)
	    {	
			if(sys_over_time(ulTick, 1500))
				break;
			sys_delay_ms(5);
		}
	}
	sucFlag = 1;

	DMA_Cmd(DMA_DEV->DMA_Stream, DISABLE);

	if(DMA_DEV->pcSendBuf != NULL)
		sys_free(DMA_DEV->pcSendBuf);

    /* 2. 清所有标志 */
    DMA_ClearFlag(DMA_DEV->DMA_Stream, DMA_DEV->ulDMA_FLAG);

	DMA_DEV->pcSendBuf = sys_malloc(len + 1);

	if(DMA_DEV->pcSendBuf == NULL)
		return 0;

	memset(DMA_DEV->pcSendBuf, 0, sizeof(DMA_DEV->pcSendBuf));
	memcpy(DMA_DEV->pcSendBuf, buf, len);
    /* 3. 重新填地址和长度 */
    DMA_DEV->DMA_Stream->M0AR = (uint32_t)DMA_DEV->pcSendBuf;
    DMA_DEV->DMA_Stream->NDTR = len;

    /* 4. 启动传输 */
    DMA_Cmd(DMA_DEV->DMA_Stream, ENABLE);

    return 0;
}

void debug_printf(const char *fmt, ...) 
{
	char * pcBuf;
	int wLen;
	va_list args;
	static unsigned char sucSta = 0;

	while(sucSta == 1)
		sys_delay_ms(2);
	sucSta = 1;
	
	va_start(args, fmt);
	pcBuf = sys_malloc(512);
	if(pcBuf == NULL) return;
	//wLen = vsnprintf(NULL, 0, fmt, args);
	wLen = vsnprintf(pcBuf, 1024, fmt, args);
	sys_free(pcBuf);
	va_end(args);

	if(wLen < 0) return;
	pcBuf = sys_malloc(wLen + 1);
	if(pcBuf == NULL) return;

	va_start(args, fmt);
	vsnprintf(pcBuf, wLen + 1, fmt, args);
	va_end(args);
#if (NET_DEBUG == 1)
	net_debug_printf(pcBuf, wLen);
#else
	debug_send(pcBuf);
#endif
	sys_free(pcBuf);
	sucSta = 0; 
}


/**
  * @brief  设置/获取时间和日期
  * @param  timeinfo
  * @retval timesample
  */
void BSP_rtc_datetime(struct tm *timeinfo)
{
    #define RTC_BKP_DATA        (0x32F2)
    #define RTC_YEAR_START      (2000)

    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

	if (!timeinfo->tm_mday) {
        RTC_GetTime(RTC_Format_BIN, &rtc_time);
        RTC_GetDate(RTC_Format_BIN, &rtc_date);

        timeinfo->tm_hour    = rtc_time.RTC_Hours;
        timeinfo->tm_min     = rtc_time.RTC_Minutes;
        timeinfo->tm_sec     = rtc_time.RTC_Seconds;
        timeinfo->tm_wday    = rtc_date.RTC_WeekDay - 1;
        timeinfo->tm_mday    = rtc_date.RTC_Date;
        timeinfo->tm_mon     = rtc_date.RTC_Month - 1;
        timeinfo->tm_year    = rtc_date.RTC_Year;
    }
    else {
        unsigned int year = timeinfo->tm_year + RTC_YEAR_START;
        unsigned char month = timeinfo->tm_mon;
        unsigned char week;

        if (2 > month) {
            month += 12;
            year --;
        }

        month ++;
	    week = (timeinfo->tm_mday + 2 * month + 3 * (month + 1) / 5
	            + year + year / 4 - year / 100 + year / 400) % 7;
        timeinfo->tm_wday = (week + 1) % 7;

    	rtc_date.RTC_WeekDay = timeinfo->tm_wday + 1;
    	rtc_date.RTC_Date    = timeinfo->tm_mday;
    	rtc_date.RTC_Month   = timeinfo->tm_mon + 1;
    	rtc_date.RTC_Year    = timeinfo->tm_year;
    	RTC_SetDate(RTC_Format_BIN, &rtc_date);
    	RTC_WriteBackupRegister(RTC_BKP_DR0, RTC_BKP_DATA);

        rtc_time.RTC_H12     = RTC_H12_AM;
        rtc_time.RTC_Hours   = timeinfo->tm_hour;
        rtc_time.RTC_Minutes = timeinfo->tm_min;
        rtc_time.RTC_Seconds = timeinfo->tm_sec;
    	RTC_SetTime(RTC_Format_BIN, &rtc_time);
    	RTC_WriteBackupRegister(RTC_BKP_DR0, RTC_BKP_DATA);
    }
}
