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

//////////////////////////////////////////
static volatile uint32_t s_tick; //系统滴答计时
static void *s_pDevObj[PERI_MAX] = {NULL};

/////////////////////////////////////////
/*
 * @brief	    串口收发中断
 * @param[in]   none
 * @return 	    none
 */
void USART1_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM1]);
}

void USART2_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM2]);
}

void USART3_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM3]);
}

void UART4_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM4]);
}

void UART5_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM5]);
}

void USART6_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM6]);
}

void UART7_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM7]);
}

void UART8_IRQHandler(void)
{
    BSP_uart_irqProcess(s_pDevObj[PERI_COM8]);
}

/*
 * @brief	    I2C1 irq process
 * @param[in]   none
 * @return 	    none
 */
void I2C1_EV_IRQHandler(void)
{
    BSP_i2c_irqProcess(s_pDevObj[PERI_I2C1]);
}

void I2C2_ER_IRQHandler(void)
{
    BSP_i2c_irqProcess(s_pDevObj[PERI_I2C2]);
}

void I2C3_ER_IRQHandler(void)
{
    BSP_i2c_irqProcess(s_pDevObj[PERI_I2C3]);
}

/*
 * @brief	    SPI2 irq process
 * @param[in]   none
 * @return 	    none
 */
void SPI1_IRQHandler(void)
{
    BSP_spi_irqProcess(s_pDevObj[PERI_SPI1]);
}

void SPI2_IRQHandler(void)
{
    BSP_spi_irqProcess(s_pDevObj[PERI_SPI2]);
}

// ADC 转换完成中断服务程序
//void ADC_IRQHandler(void) {
//    static uint8_t channel_idx = 0;
//
//	if(ADC_GetITStatus(ADC1, ADC_IT_OVR))
//		ADC_ClearITPendingBit(ADC1, ADC_IT_OVR);
//    if (ADC_GetITStatus(ADC1, ADC_IT_EOC)) { // 每个通道转换完成时触发
//        // 读取当前通道数据
//		if(!(gucADCGetFinish & (0x01 << channel_idx)))
//		{	//等待上一次数据处理完成
//			_ultra_adc[channel_idx] = ADC_GetConversionValue(ADC1);
//			gucADCGetFinish |= (0x01 << channel_idx);
//		}
//		channel_idx = (channel_idx + 1) % 2; // 切换通道索引0/1
//
//		// 清除中断标志
//		ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
//    }
//}

/*
 * @brief	    Timer3 irq process
 * @param[in]   none
 * @return 	    none
 */
void TIM3_IRQHandler(void)
{
    if (RESET != TIM_GetITStatus(TIM3, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

/*
 * @brief	    Timer4 irq process
 * @param[in]   none
 * @return 	    none
 */
void TIM4_IRQHandler(void)
{
    if (RESET != TIM_GetITStatus(TIM4, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}

/*
 * @brief 7个中断通道在NVRC中使用, 7个中断服务函数
 */
void EXTI0_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

void EXTI1_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

void EXTI2_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

void EXTI3_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

void EXTI4_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

void EXTI9_5_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

void EXTI15_10_IRQHandler(void)
{
    BSP_exti_irqProcess();
}

/*
 * @brief	   rtc唤醒中断
 * @param[in]  none
 * @return 	   none
 */
void RTC_WKUP_IRQHandler(void)
{
    if (RESET != RTC_GetITStatus(RTC_IT_WUT)) {
        RTC_ClearITPendingBit(RTC_IT_WUT);
        EXTI_ClearITPendingBit(EXTI_Line22);
        BSP_wkup_irqProcess();
		//BSP_rtc_wkupcfg(1000);
    }
}

/*
 * @brief       外部中断配置
 * @param[in]   - Line       中断号
 * @param[in]   - Trigger    触发器(上升沿/下降沿)
 * @return      none
 */
int TGT_exti_conf(uint32_t cfg, FunctionalState cmd)
{
    if (0 != cfg) {
        st_EXTI_conf *pobj = (st_EXTI_conf *)&cfg;
        EXTI_InitTypeDef  config = {
                .EXTI_Trigger = (EXTITrigger_TypeDef)((pobj->trigType + 1) << 2),
                .EXTI_Mode = (EXTIMode_TypeDef)(pobj->mode << 2),
                .EXTI_Line = (0x1 << pobj->line),
                .EXTI_LineCmd = cmd,
            };

        if (EXTI_PinSource15 >= pobj->line) {
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
            SYSCFG_EXTILineConfig(pobj->portSrc, pobj->line);
        }

        EXTI_Init(&config);
        EXTI_ClearITPendingBit(pobj->line);
        TGT_nvic_conf(cfg, cmd);
        return 0;
    }
    return -1;
}


int TGT_nvic_conf(uint32_t cfg, FunctionalState cmd)
{
    if (0 != cfg) {
        st_EXTI_conf *pobj = (st_EXTI_conf *)&cfg;
        NVIC_InitTypeDef config = {
                .NVIC_IRQChannel = pobj->chn,
                .NVIC_IRQChannelPreemptionPriority = pobj->mainPri,
                .NVIC_IRQChannelSubPriority = pobj->subPri,
                .NVIC_IRQChannelCmd = cmd,
            };

        NVIC_Init(&config);
        return 0;
    }
    return -1;
}

int TGT_rcc_conf(uint32_t cfg)
{
    if (0 != cfg) {
        st_RCC_conf *pobj = (st_RCC_conf *)&cfg;
        __IO uint32_t startUpStatus = 0;
        uint32_t validStatus;

        RCC_DeInit();
        if (0 != pobj->osc) {
            RCC_HSEConfig(RCC_HSE_ON);
            startUpStatus = RCC_WaitForHSEStartUp();
            validStatus = SUCCESS;
        }
        else {
            RCC_HSICmd(ENABLE);
            if (0 > BSP_event_wait(RCC_GetFlagStatus, NULL, RCC_FLAG_HSIRDY, SET)) {
                return -1;
            }

    	    startUpStatus = (RCC->CR & RCC_CR_HSIRDY);
            validStatus = RCC_CR_HSIRDY;
        }

         if (validStatus != startUpStatus) {
            return -2;
         }

        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        PWR->CR |= PWR_CR_VOS;

        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div2);
        RCC_PCLK1Config(RCC_HCLK_Div4);
        RCC_PLLConfig((!pobj->osc ? RCC_PLLSource_HSI : RCC_PLLSource_HSE), pobj->m, pobj->n, pobj->p, pobj->q);

        RCC_PLLCmd(ENABLE);
        if (0 > BSP_event_wait(RCC_GetFlagStatus, NULL, RCC_FLAG_PLLRDY, SET)) {
            return -3;
        }

    /*-----------------------------------------------------*/
        PWR->CR |= PWR_CR_ODEN;
        if (0 > BSP_event_wait(NULL, (void *)&(PWR->CSR), PWR_CSR_ODRDY, 0)) {
            return -4;
        }

        PWR->CR |= PWR_CR_ODSWEN;
        if (0 > BSP_event_wait(NULL, (void *)&(PWR->CSR), PWR_CSR_ODSWRDY, 0)) {
            return -5;
        }

        // 配置FLASH预取指,指令缓存,数据缓存和等待状态
        FLASH->ACR = FLASH_ACR_PRFTEN
                        | FLASH_ACR_ICEN
                        | FLASH_ACR_DCEN
                        | FLASH_ACR_LATENCY_5WS;
    /*-----------------------------------------------------*/

            // 当PLL稳定之后，把PLL时钟切换为系统时钟SYSCLK
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        // 读取时钟切换状态位，确保PLLCLK被选为系统时钟
        if (0 > BSP_event_wait(RCC_GetSYSCLKSource, NULL, 0, 0x08)) {
            return -6;
        }
    }
    return -1;
}

/*
 * @brief	   RTC初始化
 * 外部32.768K的晶振,RTC_SetPrescaler(32767);可以实现1S1次计数
 */
int TGT_rtc_conf(void)
{
    volatile unsigned int retry = 20000;

    /* Allow access to RTC */
    PWR_BackupAccessCmd(ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    if (true) {
        //BKP_DeInit();
        RCC_LSEConfig(RCC_LSE_ON);//选择LSE时钟，因为LSE为外部低速时钟，比内部时钟稳定。

        /* Wait till LSE is ready */
        while (RESET == RCC_GetFlagStatus(RCC_FLAG_LSERDY) && 0 < retry--);

        if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET) {
            /* Select the RTC Clock Source */
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        } else {
            RCC_LSEConfig(RCC_LSE_OFF);
            RCC_LSICmd(ENABLE);
            while (RESET == RCC_GetFlagStatus(RCC_FLAG_LSIRDY));
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        }

        //Enable the RTC Clock
        RCC_RTCCLKCmd(ENABLE);
        RTC_WriteBackupRegister(RTC_BKP_DR0, 0xA0A0);
    }
    return 0;
}

/*
 * @brief	   rtc唤醒配置
 * @param[in]  none(1~4000ms)
 */
int TGT_rtc_wkupcfg(unsigned int ms)
{
    RTC_WakeUpCmd(DISABLE);
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div2);
    RTC_SetWakeUpCounter((unsigned int )((((ms << 14) / 1000.0) - 100.0) - 1) * 6.215 / 6.0);

    RTC_ClearITPendingBit(RTC_IT_WUT);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    RTC_WakeUpCmd(ENABLE);
    return 0;
}

/*
 * @brief       看门狗配置
 * @param[in]   cfg  - configure information
 * @return      none
 */
int TGT_iwdg_conf(uint32_t cfg)
{
    if (0 != cfg) {
        st_IWDG_conf *pobj = (st_IWDG_conf *)&cfg;
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
        IWDG_SetPrescaler(pobj->presc);
        //LSI = 8 Khz, timeout = ((128 × reload) / LSI)
        IWDG_SetReload(pobj->timeout * pobj->osc * 100000 / 128);//*1k * 100
        IWDG_ReloadCounter();
        IWDG_Enable();

        IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
        return 0;
    }
    return -1;
}

/*
 * @brief       定时器配置
 * @param[in]   - TIMx       定时器
 * @param[in]   - hz         计数频率
 * @return      none
 */
//int TGT_timer_conf(TIM_TypeDef *pTmr, uint32_t cfg)
//{
//    if (0 != cfg) {
//        float period = (BSP_TICK_INTERVAL * 1000);
//        st_TIMER_conf *pobj = (st_TIMER_conf *)&cfg;
//        TIM_TimeBaseInitTypeDef  config = {
//            .TIM_CounterMode = (pobj->cntMode << 4),
//            .TIM_ClockDivision = (pobj->clkDiv << 8),
//            .TIM_Period = (pobj->freq * BSP_TICK_INTERVAL) - 1,
//        };
//        RCC_ClocksTypeDef Clocks;
//
//        RCC_GetClocksFreq(&Clocks);
//        if (TIM1 < pTmr && TIM8 > pTmr) {
//             period = (float)Clocks.PCLK1_Frequency / period;
//        }
//        else {
//             period = (float)Clocks.PCLK2_Frequency / period;
//        }
//
//        config.TIM_Prescaler = (uint32_t)(period + 0.5f) - 1;
//        TIM_DeInit(pTmr);
//        TIM_TimeBaseInit(pTmr, &config);
//            TIM_ClearFlag(pTmr, TIM_FLAG_Update);
//            TIM_ITConfig(pTmr, TIM_IT_Update, ENABLE);
//            TIM_Cmd(pTmr, ENABLE);
//        return 0;
//    }
//    return -1;
//}

/*
 * @brief       DMA configuation
 * @param[in]   - cfg      dma config info
 * @return      none
 */
//int TGT_dma_conf(DMA_TypeDef *pdma, uint32_t cfg, uint32_t memBaseAddr, uint32_t periBaseAddr)
//{
//    if (0 != cfg) {
//        st_DMA_conf *pobj = (st_DMA_conf *)&cfg;
//        DMA_InitTypeDef config = {
//            .DMA_Channel = (pobj->chn << 25),                       
//            .DMA_DIR = (pobj->dir << 6),                            
//            .DMA_Mode = (pobj->mode << 8),                          
//            .DMA_Priority = (pobj->pri << 16),                      
//            .DMA_BufferSize = pobj->buf_size,                       
//            .DMA_FIFOMode = (pobj->fifo_mode << 2),                 
//            .DMA_FIFOThreshold = (pobj->fifo_thresh << 0),
//            .DMA_MemoryBurst = (pobj->mem_burst << 23),             
//            .DMA_Memory0BaseAddr = memBaseAddr,                     
//            .DMA_MemoryDataSize = (pobj->mem_datasize << 13),       
//            .DMA_MemoryInc = (pobj->mem_inc << 10),                 
//            .DMA_PeripheralInc = (pobj->peri_inc << 9),             
//            .DMA_PeripheralDataSize = (pobj->peri_datasize << 11),  
//            .DMA_PeripheralBurst = (pobj->peri_burst << 21),        
//            .DMA_PeripheralBaseAddr = periBaseAddr,                 
//        };
//        DMA_Stream_TypeDef *pDmaStream = (DMA_Stream_TypeDef *)((uint32_t)pdma + (pobj->stream_type * 0x18) + 0x10);
//
//        DMA_Init(pDmaStream, &config);
//        DMA_Cmd(pDmaStream, ENABLE);
//        return 0;
//    }
//    return -1;
//}

/*
 * @brief       ADC configuation
 * @param[in]   - port       串口号(USART[1..6])
 * @param[in]   - baudrate   波特率
 * @return      none
 */
int TGT_adc_pubcfg(uint32_t cfg)
{
    if (0 != cfg) {
        st_ADC_commonCfg *pobj = (st_ADC_commonCfg *)&cfg;
        ADC_CommonInitTypeDef config = {
            .ADC_Mode = (pobj->adc_mode << 0),
            .ADC_Prescaler = (pobj->prescaler << 16),
            .ADC_DMAAccessMode = (pobj->dma_mode << 14),
            .ADC_TwoSamplingDelay = (pobj->sample_period << 8),
        };

        ADC_CommonInit(&config);
        return 0;
    }
    return -1;
}

//int TGT_adc_conf(ADC_TypeDef *padc, uint32_t cfg, uint8_t isMultiAdc)
//{
//    if (0 != cfg) {
//        st_ADC_conf *pobj = (st_ADC_conf *)&cfg;
//        uint16_t channels = pobj->channel;
//        uint8_t rank = 0;
//        uint8_t nbr = 0;
//
//        for (channels = pobj->channel; 0 != channels; channels >>= 1) {
//            if (channels & 0x1) {
//                nbr ++;
//            }
//        }
//
//        if (0 != nbr) {
//            ADC_InitTypeDef config;
//
//            // -------------------ADC Init 结构体 参数 初始化--------------------------
//            ADC_StructInit(&config);
//            config.ADC_Resolution = (pobj->data_bits << 24);
//            config.ADC_DataAlign = (pobj->data_align << 11);
//            config.ADC_ExternalTrigConvEdge = (pobj->trig_edge << 28);
//            if (NONE == pobj->trig_edge) {
//                config.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
//            }
//            else {
//                config.ADC_ExternalTrigConv = (pobj->trig_src << 24);
//            }
//
//            config.ADC_ContinuousConvMode = (FunctionalState)pobj->continuous;
//            config.ADC_ScanConvMode = ((1 < nbr || isMultiAdc) ? ENABLE : DISABLE);
//            config.ADC_NbrOfConversion = nbr;
//            ADC_Init(padc, &config);
//
//            for (rank = 0, nbr = 0, channels = pobj->channel; 0 != channels; nbr ++, channels >>= 1) {
//                if (channels & 0x1) {
//                    ADC_RegularChannelConfig(padc, ADC_Channel_0 + nbr, ++rank, pobj->sample_time);
//                }
//            }
//
//            if (ENABLE == pobj->use_dma) {
//                //
//                if (DISABLE == isMultiAdc) {
//                    ADC_DMARequestAfterLastTransferCmd(padc, ENABLE);
//                }
//                else {
//                    ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);
//                }
//
//                ADC_DMACmd(padc, ENABLE);
//            }
//            else {
//                ADC_ITConfig(padc, ADC_IT_EOC, ENABLE);
//            }
//
//            if (ENABLE == pobj->use_temp) {
//                ADC_TempSensorVrefintCmd(ENABLE);
//            }
//            else if (ENABLE == pobj->use_bat) {
//                ADC_VBATCmd(ENABLE);
//            }
//
//            return pobj->trig_edge;
//        }
//    }
//    return -1;
//}

/*
 * @brief       SPI configure
 * @param[in]   - port       端口(SPI[1..2])
 * @param[in]   - ratePrescaler   baud rate prescaler
 * @return      none
 */
int TGT_spi_conf(SPI_TypeDef *pspi, uint32_t cfg)
{
    if (0 != cfg) {
        st_SPI_conf *pobj = (st_SPI_conf *)&cfg;
        SPI_InitTypeDef config = {
            .SPI_Mode = (MASTER == pobj->mode ? SPI_Mode_Master : SPI_Mode_Slave),
            .SPI_Direction = (pobj->dir << (RXONLY == pobj->dir ? 10 : 15)),
            .SPI_BaudRatePrescaler = (pobj->presc << 3),
            .SPI_FirstBit = (pobj->firtBit << 7),
            .SPI_DataSize = (pobj->dataBits << 15),
            .SPI_CPHA = pobj->cpha,
            .SPI_CPOL = (pobj->cpol << 1),
            .SPI_NSS = (pobj->nss << 9),
            .SPI_CRCPolynomial = pobj->crcPoly,  //7
        };

        SPI_I2S_DeInit(pspi);
        SPI_Init(pspi, &config);
        return pobj->devtype;
    }
    return -1;
}

/*
 * @brief       I2c configure
 * @param[in]   - port    端口(I2C[1..3])
 * @param[in]   - speed   波特率
 * @return      none
 */
int TGT_i2c_conf(I2C_TypeDef *pi2c, uint32_t cfg, uint16_t ownAddr)
{
    if (0 != cfg) {
        st_I2C_conf *pobj = (st_I2C_conf *)&cfg;
        I2C_InitTypeDef config = {
            .I2C_DutyCycle = (CYCLE2 == pobj->dutyCycle ? I2C_DutyCycle_2 : I2C_DutyCycle_16_9),
            .I2C_AcknowledgedAddress = (pobj->addrBits << 14),
            .I2C_OwnAddress1 = ownAddr,
            .I2C_ClockSpeed = (pobj->speed * 1000),
            .I2C_Mode = (pobj->mode << 1),
            .I2C_Ack = (pobj->isack << 10),
        };

        I2C_DeInit(pi2c);
        I2C_Init(pi2c, &config);
        I2C_AcknowledgeConfig(pi2c, !pobj->isack ? DISABLE: ENABLE);
        return pobj->devtype;
    }
    return -1;
}

/*
 * @brief       串口配置
 * @param[in]   - port       串口号(USART[1..6])
 * @param[in]   - baudrate   波特率
 * @return      none
 */
int TGT_uart_conf(USART_TypeDef *puart, uint32_t cfg)
{
    if (0 != cfg) {
        st_UART_conf *pobj = (st_UART_conf *)&cfg;
        USART_InitTypeDef config = {
            .USART_BaudRate = (pobj->baudrate * 100),
            .USART_WordLength = (pobj->wordLen << 12),
            .USART_StopBits = (pobj->stopBits << 12),
            .USART_Parity = (pobj->parity << 9),
            .USART_HardwareFlowControl = (pobj->flowCtrl << 8),
            .USART_Mode = (pobj->mode << 2),
        };

        USART_DeInit(puart);
        USART_Init(puart, &config);
        //if (DISABLE != pobj->inti) {
            USART_ITConfig(puart, USART_IT_RXNE, ENABLE);
            USART_ITConfig(puart, USART_IT_ERR, ENABLE);
        //}
        return pobj->devtype;
    }
    return -1;
}

int TGT_gpio_conf(GPIO_TypeDef *pgpio, uint32_t cfg)
{
    //if (0 != cfg) {
        st_GPIO_conf *pobj = (st_GPIO_conf *)&cfg;

        if (0 != pobj->pins) {
            GPIO_InitTypeDef config = {
                    .GPIO_Pin   = pobj->pins,
                    .GPIO_Mode  = (GPIOMode_TypeDef)pobj->mode,
                    .GPIO_OType = (GPIOOType_TypeDef)pobj->otype,
                    .GPIO_Speed = (GPIOSpeed_TypeDef)pobj->speed,
                    .GPIO_PuPd  = (GPIOPuPd_TypeDef)pobj->pupd,
                };

            GPIO_Init(pgpio, &config);
            if (AF == pobj->mode && 0 < pobj->af) {
                uint8_t i;

                for (i = 0; PIN_MAX > i; i++) {
                    if (0 != (pobj->pins & (0x1 << i))) {
                        GPIO_PinAFConfig(pgpio, i, pobj->af - 1);
                    }
                }
            }
            return 0;
        }
    //}
    return -1;
}

void TGT_dev_register(void *pbus, void *pobj)
{
    if (USART1 == pbus) { s_pDevObj[PERI_COM1] = pobj; return; }
    if (USART2 == pbus) { s_pDevObj[PERI_COM2] = pobj; return; }
    if (USART3 == pbus) { s_pDevObj[PERI_COM3] = pobj; return; }
    if (UART4  == pbus) { s_pDevObj[PERI_COM4] = pobj; return; }
    if (UART5  == pbus) { s_pDevObj[PERI_COM5] = pobj; return; }
    if (USART6 == pbus) { s_pDevObj[PERI_COM6] = pobj; return; }
    if (UART7  == pbus) { s_pDevObj[PERI_COM7] = pobj; return; }
    if (UART8  == pbus) { s_pDevObj[PERI_COM8] = pobj; return; }
}
