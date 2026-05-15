/******************************************************************************
 * @brief    ST MCU 通用外设配置
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
#ifndef _TARGET_H_
#define _TARGET_H_

#include "stm32f4xx.h"

#define EXTI_LINE_MAX   22
typedef enum {
	PERI_COM1 = 0,
	PERI_COM2,
	PERI_COM3,
	PERI_COM4,
	PERI_COM5,
	PERI_COM6,
	PERI_COM7,
	PERI_COM8,
    PERI_I2C1,
    PERI_I2C2,
    PERI_I2C3,
    PERI_SPI1,
    PERI_SPI2,
    PERI_SPI4,
    PERI_DMA1,
    PERI_DMA2,
    PERI_ADC,
    PERI_ADC1,
    PERI_ADC2,
    PERI_ADC3,
	PERI_MAX
} e_PERI_Type;

void TGT_dev_register(void *pbus, void *pobj);

/////////////////////////////////////////////////////////////////
#define NONE                 0x0
#define BOTH                 0x3

typedef enum {
    CH0 = 0,
    CH1,
    CH2,
    CH3,
    CH4,
    CH5,
    CH6,
    CH7,
    CH8,
    CH9,
    CH10,
    CH11,
    CH12,
    CH13,
    CH14,
    CH15,
    CH_MAX
} e_DEV_channel;

typedef enum {
    PERI2MEM = 0,
    MEM2PERI,
    MEM2MEM
} e_DATA_dir;

typedef enum {
    PRI_LOW = 0,
    PRI_MID,
    PRI_HIGH,
    PRI_TOP
} e_PRIORITY_lvl;

typedef enum {
    NORNAL = 0,
    CIRCUL
} e_DATA_mode;

typedef enum {
    THRESH_QUARTER = 0,
    THRESH_HALF,
    THRESH_3QUARTER,
    THRESH_FULL
} e_THRESHOLD_TYPE;

typedef enum {
    BURST_SINGLE = 0,
    BURST_INC4,
    BURST_INC8,
    BURST_INC16
} e_BURST_TYPE;

typedef enum {
    STREAM0 = 0,
    STREAM1,
    STREAM2,
    STREAM3,
    STREAM4,
    STREAM5,
    STREAM6,
    STREAM7
} e_STREAM_TYPE;

typedef enum {
    SZBYTE = 0,
    SZWORD,
    SZDWORD
} e_DATA_SIZE;

typedef enum {
    BIT12 = 0,
    BIT10,
    BIT8,
    BIT6
} e_DATA_BITS;

typedef enum {
    RIGHT = 0,
    LEFT = 1
} e_DATA_align;

typedef enum {
    RISING = 1,
    FALLING = 2,
} e_TRIG_edge;

typedef enum {
    TRIG_T1_CC1 = 0,
    TRIG_T1_CC2,
    TRIG_T1_CC3,
    TRIG_T2_CC2,
    TRIG_T2_CC3,
    TRIG_T2_CC4,
    TRIG_T2_TRGO,
    TRIG_T3_CC1,
    TRIG_T4_TRGO,
    TRIG_T4_CC4,
    TRIG_T5_CC1,
    TRIG_T5_CC2,
    TRIG_T5_CC3,
    TRIG_T8_CC1,
    TRIG_T8_TRGO,
    TRIG_EXT_IT11
} e_TRIG_source;

typedef enum {
    ADC_3CYCLES = 0,
    ADC_15CYCLES,
    ADC_28CYCLES,
    ADC_56CYCLES,
    ADC_84CYCLES,
    ADC_112CYCLES,
    ADC_144CYCLES,
    ADC_480CYCLES
} e_SAMPLE_time;

typedef enum {
    PERIOD_5CYCLES = 0,
    PERIOD_6CYCLES,
    PERIOD_7CYCLES,
    PERIOD_8CYCLES,
    PERIOD_9CYCLES,
    PERIOD_10CYCLES,
    PERIOD_11CYCLES,
    PERIOD_12CYCLES,
    PERIOD_13CYCLES,
    PERIOD_14CYCLES,
    PERIOD_15CYCLES,
    PERIOD_16CYCLES,
    PERIOD_17CYCLES,
    PERIOD_18CYCLES,
    PERIOD_19CYCLES,
    PERIOD_20CYCLES
} e_SAMPLE_period;

typedef enum {
    DMA_MODE1 = 1,
    DMA_MODE2,
    DMA_MODE3
} e_DMA_accessMode;

typedef enum {
    INDEPENDENT = 0,  //独立ADC
    REGINJECSIMULT = 0x01,
    REGALTERTRIG = 0x02,
    INJECSIMULT = 0x05,
    REGSIMULT = 0x06, //双重ADC
    INTERL = 0x07,
    ALTERTRIG = 0x09,
    TRIPLEMODE = 0x10 //三重ADC
} e_ADC_Mode;

typedef enum {
    ADC_DIV2 = 0,
    ADC_DIV4,
    ADC_DIV6,
    ADC_DIV8
} e_ADC_Div;

#define ADC_DMA_MODE(d)     ((d) << 29)
#define ADC_REF_VOL(t)      ((t) << 30)
typedef enum {
    REFVOL_BAT = 0x1,
    REFVOL_TMP = 0x2,
} e_ADC_REFVOL;

#define ADC_PRESALER(p)     ((p) << 0)
#define ADC_SAMPLEPERIOD(p) ((p) << 2)
#define ADC_MODE(m)         ((m) << 6)
#define ADC_DMAMODE(d)      ((d) << 11)
typedef struct {
    uint16_t prescaler:2;    //e_ADC_Div
    uint16_t sample_period:4;//e_SAMPLE_period
    uint16_t adc_mode:5;     //e_ADC_Mode
    uint16_t dma_mode:2;     //e_DMA_accessMode
} st_ADC_commonCfg;

#define ADC_USEDMA(u)       ((u) << 0)
#define ADC_USEBAT(u)       ((u) << 1)
#define ADC_USETEMP(u)      ((u) << 2)
#define ADC_CONTINUOUS(c)   ((c) << 3)
#define ADC_SAMPLETIME(t)   ((t) << 4)
#define ADC_DATAALIGN(a)    ((a) << 7)
#define ADC_DATABITS(b)     ((b) << 8)
#define ADC_TRIGEDGE(e)     ((e) << 10)
#define ADC_TRIGSRC(s)      ((s) << 12)
#define ADC_CHN(ch)         (0x1 << (16 + ch))
typedef struct {
    uint32_t use_dma:1;      //enable or disable
    uint32_t use_bat:1;      //enable or disable
    uint32_t use_temp:1;     //enable or disable
    uint32_t continuous:1;   //enable or disable
    uint32_t sample_time:3;  //e_SAMPLE_time
    uint32_t data_align:1;   //e_DATA_align
    uint32_t data_bits:2;    //e_DATA_BITS
    uint32_t trig_edge:2;    //e_TRIG_edge
    uint32_t trig_src:4;     //e_TRIG_source
    uint32_t channel:16;     //e_DEV_channel(0 ~ 15)
} st_ADC_conf;

#define DMA_CHN(ch)         ((ch)  << 0)
#define DMA_DIR(dir)        ((dir) << 4)
#define DMA_PRIORITY(pri)   ((pri) << 6)
#define DMA_MODE(m)         ((m)   << 8)
#define DMA_STREAMTYPE(t)   ((t)   << 9)
#define DMA_FIFOMODE(m)     ((m)   << 12)
#define DMA_FIFOTHRESH(t)   ((t)   << 13)
#define DMA_MEMWSIZE(s)     ((s)   << 15)
#define DMA_MEMBURST(b)     ((b)   << 17)
#define DMA_MEMINC(inc)     ((inc) << 19)
#define DMA_PERIINC(inc)    ((inc) << 20)
#define DMA_PERIBURST(b)    ((b)   << 21)
#define DMA_PERIWSIZE(s)    ((s)   << 23)
#define DMA_BUFSIZE(b)      ((b)   << 25)
typedef struct {
    uint32_t chn:4;          //e_CHANNEL_TYPE
    uint32_t dir:2;          //e_DATA_dir
    uint32_t pri:2;          //e_PRIORITY_lvl
    uint32_t mode:1;         //e_DATA_mode
    uint32_t stream_type:3;  //e_STREAM_TYPE
    uint32_t fifo_mode:1;    //enable or disable
    uint32_t fifo_thresh:2;  //e_THRESHOLD_TYPE
    uint32_t mem_datasize:2; //e_DATA_SIZE
    uint32_t mem_burst:2;    //e_BURST_TYPE
    uint32_t mem_inc:1;      //enable or disable
    uint32_t peri_inc:1;     //enable or disable
    uint32_t peri_burst:2;   //e_BURST_TYPE
    uint32_t peri_datasize:2;//e_DATA_SIZE
    uint32_t buf_size:7;     //
} st_DMA_conf;

#define GPIO_Pins(pin)       (0x1 << (pin))
typedef enum {
    PIN0  = 0x0,
    PIN1,
    PIN2,
    PIN3,
    PIN4,
    PIN5,
    PIN6,
    PIN7,
    PIN8,
    PIN9,
    PIN10,
    PIN11,
    PIN12,
    PIN13,
    PIN14,
    PIN15,
    PIN_MAX
} e_GPIO_Pin;

#define GPIO_AF(t)          ((t) << 16)
typedef enum {
    AF_MCO = 0x1,
    AF_TIM1 = 0x2,
    AF_TIM3 = 0x3,
    AF_I2C1 = 0x5,
    AF_I2C2 = 0x5,
    AF_I2C3 = 0x5,
    AF_SPI1 = 0x6,
    AF_SPI2 = 0x6,
    AF_SPI3 = 0x7,
    AF_SPI4 = 0x6,
    AF_ETH = 0x8,
    AF_UART1 = 0x8,
    AF_UART2 = 0x8,
    AF_UART3 = 0x8,
    AF_UART4 = 0x9,
    AF_UART5 = 0x9,
    AF_UART6 = 0x9,
    AF_UART7 = 0x9,
    AF_UART8 = 0x9,
} e_GPIO_Af;

#define GPIO_MODE(m)        ((m) << 22)
typedef enum {
    IN  = 0x0,
    OUT,
    AF,
    AN,
} e_GPIO_Mode;

#define GPIO_PUPD(p)        ((p) << 24)
typedef enum {
	NOPULL = 0,
    UP,
    DW
} e_GPIO_PuPd;

#define GPIO_SPEED(s)       ((s) << 26)
typedef enum {
    SLOW  = 0x0,
    MID,
    FAST,
    TOP
} e_GPIO_Speed;

#define GPIO_OTYPE(t)      ((t) << 28)
typedef enum {
    PP = 0x0,
    OD
} e_GPIO_Otype;

#define GPIO_EN(e)         ((e) << 29)
typedef enum {
    LOW  = 0x0,
    HIGH
} e_GPIO_Enable;

typedef struct {
    uint32_t pins:16;    //begin from 1
    uint32_t af:6;
    uint32_t mode:2;
    uint32_t pupd:2;
    uint32_t speed:2;
    uint32_t otype:1;
    uint32_t en:1;
} st_GPIO_conf;

#define MCO_OUT(o)         ((o) << 0)
typedef enum {
    HSI = 0,
    LSE,
    HSE,
    PLLCLK,
    PLLI2SCLK = 1,
    SYSCLK = 0,
} e_MCO_Out;

#define MCO_DIV(d)          ((d) << 3)
typedef enum {
    DIV1 = 0,
    DIV2 = 4,
    DIV3,
    DIV4,
    DIV5,
    DIV6,
    DIV7,
    DIV8
} e_DIV_ratio;

typedef struct {
    uint32_t out:3;
    uint32_t div:3;
    uint32_t type:1;
    uint32_t res:25;
} st_MCO_conf;

typedef enum {
    LINE0 = 0,
    LINE1,
    LINE2,
    LINE3,
    LINE4,
    LINE5,
    LINE6,
    LINE7,
    LINE8,
    LINE9,
    LINE10,
    LINE11,
    LINE12,
    LINE13,
    LINE14,
    LINE15,
    LINE16,
    LINE17,
    LINE18,
    LINE19,
    LINE20,
    LINE21,
    LINE22,
} e_EXTI_Line;

typedef enum {
    INTI = (0x0),
    EVTI
} e_EXTI_Mode;

typedef enum {
    RISE = 1,
    FALL
} e_EXTI_Trig;

#define EXTI_TRIG(t)        ((t) << 0)
#define EXTI_MODE(m)        ((m) << 2)
#define EXTI_LINE(l)        ((l) << 3)
#define EXTI_PORT(p)        ((EXTI_PortSourceGPIO##p) << 8)
#define INT_CHN(c)          ((c) << 13)
#define INT_MAINPRI(m)      ((m) << 20)
#define INT_SUBPRI(s)       ((s) << 24)
typedef struct {
    uint32_t trigType:2;//e_EXTI_Trig
    uint32_t mode:1;    //e_EXTI_Mode
    uint32_t line:5;    //EXTI_Line0
    //uint32_t pinSrc:4;//EXTI_PinSource0
    uint32_t portSrc:5; //EXTI_PortSourceGPIOC
    uint32_t chn:7;     //IRQn_Type
    uint32_t mainPri:4; //0 ~ 15
    uint32_t subPri:4;  //0 ~ 15
    uint32_t res:4;
} st_EXTI_conf;

#define UART_MODE(m)    ((m) << 16)
typedef enum {
    UART_MODE_Rx = 1,
    UART_MODE_TX,
    UART_MODE_ALL
} e_UART_Mode;

#define UART_PARITY(p)   ((p) << 18)
typedef enum {
    EVEN = 0x2,
    ODD
} e_UART_Parity;

#define UART_STOPBITS(s)   ((s) << 20)
typedef enum {
    BITS1 = 0x0,
    BITS0P5,
    BITS2,
    BITS1P5
} e_UART_StopBits;

#define UART_FLOWCTRL(f)   ((f) << 22)
typedef enum {
    RTS = 1,
    CTS
} e_UART_FlowCtrl;

#define UART_WORDLEN(w)   ((w) << 24)
typedef enum {
    BITS8 = 0x0,
    BITS9
} e_UART_WordLen;

#define UART_INTI(i)       ((i) << 30)
#define UART_DEVTYPE(t)    ((t) << 25)
#define UART_BAUDRATE(r)   (((r)/100) << 0)
typedef struct {
    uint32_t baudrate:16;
    uint32_t mode:2;
    uint32_t parity:2;
    uint32_t stopBits:2;
    uint32_t flowCtrl:2;
    uint32_t wordLen:1;
    uint32_t devtype:5;
    uint32_t inti:1;
    uint32_t res:1;
} st_UART_conf;

#define SPI_MODE(m)        ((m) << 0)
typedef enum {
    MASTER = 0,
    SLAVE
} e_SPI_Mode;

#define SPI_FSTBIT(f)     ((f) << 1)
typedef enum {
    MSB = 0,
    LSB
} e_SPI_FirsBit;

#define SPI_DATABIT(d)      ((d) << 2)
typedef enum {
    _8B = 0,
    _16B
} e_SPI_DataBits;

#define SPI_CPHA(cp)        ((cp) << 3)
typedef enum {
    UNIEDGE = 0,
    BIEDGE
} e_SPI_Cpha;

/*typedef enum {
    LOW = 0,
    HIGH
} e_SPI_CPOL;*/
//refer to e_GPIO_Enable
#define SPI_CPOL(cp)        ((cp) << 4)
#define SPI_NSS(n)          ((n) << 5)
typedef enum {
    HARD = 0,
    SOFT
} e_SPI_NSS;

#define SPI_DIR(d)          ((d) << 6)
typedef enum {
    DUPLEX = 0,
    RXONLY,
    RX,
    TX
} e_SPI_Dir;

#define SPI_PRESC(p)        ((p) << 8)
typedef enum {
    PRESC2 = 0,
    PRESC4,
    PRESC8,
    PRESC16,
    PRESC32,
    PRESC64,
    PRESC128,
    PRESC256
} e_SPI_Presc;

#define SPI_CRCPOLY(c)      ((c) << 11)
#define SPI_DEVTYPE(t)      ((t) << 27)
typedef struct {
    uint32_t mode:1;    //SPI_Mode_Master
    uint32_t firtBit:1; //SPI_FirstBit_MSB
    uint32_t dataBits:1;//SPI_DataSize_8b
    uint32_t cpha:1;    //SPI_CPHA_2Edge
    uint32_t cpol:1;    //SPI_CPOL_High
    uint32_t nss:1;     //SPI_NSS_Soft
    uint32_t dir:2;     //SPI_Direction_2Lines_FullDuplex
    uint32_t presc:3;   //SPI_BaudRatePrescaler_2
    uint32_t crcPoly:16;
    uint32_t devtype:5;
} st_SPI_conf;

#define I2C_MODE(m)        ((m) << 0)
typedef enum {
    I2C = 0,
    SMBUS_DEV,
    SMBUS_HOST = 5
} e_I2C_Mode;

#define I2C_ACK(a)          ((a) << 3)
#define I2C_DUTYCYCLE(c)    ((c) << 4)
typedef enum {
    CYCLE2 = 0,
    CYCLE16_9
} e_I2C_DutyCycle;

#define I2C_ADDRBITS(b)     ((b) << 5)
typedef enum {
    BITS7 = 1,
    BITS10 = 3
} e_I2C_AddrBits;

#define I2C_SPEED(s)        (((s)/1000) << 11)
#define I2C_DEVTYPE(t)      ((t) << 27)
typedef struct {
    uint32_t mode:3;     //I2C_Mode_I2C
    uint32_t isack:1;    //I2C_Ack_Enable
    uint32_t dutyCycle:1;//I2C_DutyCycle_2
    uint32_t addrBits:2; //I2C_AcknowledgedAddress_7bit
    uint32_t res:4;
    uint32_t speed:16;   //kHz
    uint32_t devtype:5;
} st_I2C_conf;

#define TIM_CLKDIV(d)       ((d) << 18)
typedef enum {
    TIM_DIV1 = 0, // TIM_CKD_DIV1
    TIM_DIV2,     // TIM_CKD_DIV2
    TIM_DIV4      // TIM_CKD_DIV4
} e_TIMER_clkDiv;

#define TIM_CNTMODE(m)      ((m) << 16)
typedef enum {
    INC = 0, //Incremental, default
    DEC = 1, //Decrement
    ALIGN1 = 2,
    ALIGN2 = 4,
    ALIGN3 = 6
} e_TIMER_CntMode;

#define TIM_FREQ(f)        ((f) << 0)
typedef struct {
    uint32_t freq:16;   //
    uint32_t clkDiv:2;  //Sample clock division: TDTS = Tck_tim
    uint32_t cntMode:3; //TIM向上计数模式TIM_CounterMode_Up
    uint32_t res:11;
} st_TIMER_conf;

#define PLL_Q(m)        ((m) << 20)
#define PLL_P(m)        ((m) << 16)
#define PLL_N(m)        ((m) << 7)
#define PLL_M(m)        ((m) << 1)
#define RCC_OSC(osc)    ((osc) << 0)
typedef enum {
    INTERNAL = (0x0),
    EXTERNAL
} e_RCC_OSC;

//uint32_t m, uint32_t n, uint32_t p, uint32_t q
typedef struct {
    uint32_t osc:1; // e_RCC_OSC
    uint32_t m:6;   //主PLL和音频PLL分频系数(PLL之前的分频),取值范围:2~63.
    uint32_t n:9;   //主PLL倍频系数(PLL倍频),取值范围:64~432
    uint32_t p:4;   //系统时钟的主PLL分频系数(PLL之后的分频),取值范围:2,4,6,8.(仅限这4个值!)
    uint32_t q:4;   //USB/SDIO/随机数产生器等的主PLL分频系数(PLL之后的分频),取值范围:2~15
    uint32_t res:8;
} st_RCC_conf;

#define IWDG_PRESC(p)    ((p) << 5)
#define IWDG_OSC(c)      ((c) << 0)
#define IWDG_TIMEOUT(t)  (((t) / 100) << 16)
typedef struct {
    uint8_t osc:5;   //4MHz~26MHz
    uint8_t presc:3; //喂狗超时时间(ms)
    uint8_t res;
    uint16_t timeout;//unit 100ms
} st_IWDG_conf;

int TGT_gpio_conf(GPIO_TypeDef *pgpio, uint32_t cfg);
int TGT_uart_conf(USART_TypeDef *puart, uint32_t cfg);
int TGT_spi_conf(SPI_TypeDef *pspi, uint32_t cfg);
int TGT_i2c_conf(I2C_TypeDef *pi2c, uint32_t cfg, uint16_t ownAddr);

//int TGT_dma_conf(DMA_TypeDef *pdma, uint32_t cfg, uint32_t memBaseAddr, uint32_t periBaseAddr);
//int TGT_adc_conf(ADC_TypeDef *padc, uint32_t cfg, uint8_t isMultiAdc);
int TGT_adc_pubcfg(uint32_t cfg);

int TGT_timer_conf(TIM_TypeDef *pTmr, uint32_t cfg);
int TGT_exti_conf(uint32_t cfg, FunctionalState cmd);
int TGT_nvic_conf(uint32_t cfg, FunctionalState cmd);
int TGT_iwdg_conf(uint32_t cfg);
int TGT_rcc_conf(uint32_t cfg);

int TGT_rtc_conf(void);
int TGT_rtc_wkupcfg(unsigned int ms);

#endif
