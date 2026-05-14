/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   蜂鸣器例程
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32 F429 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
#include "stm32f4xx.h"
#include "stm32f4xx_flash.h"

#define BL_SIZE			0x3000
#define APP1_ADDR		0x08008000
#define APP2_ADDR		0x08100000
#define FLAG_ADDR		0x08004000
#define FLAG_VALID	0x5555AAAA
#define FLAG_DONE   0x00000000

#define BUF_WORDS		(1024/4)

#define ADDR_FLASH_SECTOR_0     0x08000000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_1     0x08004000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_2     0x08008000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_3     0x0800C000U  /* 16 KB */
#define ADDR_FLASH_SECTOR_4     0x08010000U  /* 64 KB */
#define ADDR_FLASH_SECTOR_5     0x08020000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_6     0x08040000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_7     0x08060000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_8     0x08080000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_9     0x080A0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_10    0x080C0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_11    0x080E0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_12    0x08100000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_13    0x08120000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_14    0x08140000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_15    0x08160000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_16    0x08180000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_17    0x081A0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_18    0x081C0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_19    0x081E0000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_20    0x08200000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_21    0x08220000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_22    0x08240000U  /* 128 KB */
#define ADDR_FLASH_SECTOR_23    0x08260000U  /* 128 KB */

static uint32_t copyBuf[BUF_WORDS];

uint32_t GetSector(uint32_t Address)
{
    uint32_t sector = 0;

    if ((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
        sector = FLASH_Sector_0;
    else if ((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
        sector = FLASH_Sector_1;
    else if ((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
        sector = FLASH_Sector_2;
    else if ((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
        sector = FLASH_Sector_3;
    else if ((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
        sector = FLASH_Sector_4;
    else if ((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
        sector = FLASH_Sector_5;
    else if ((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
        sector = FLASH_Sector_6;
    else if ((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
        sector = FLASH_Sector_7;
    else if ((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
        sector = FLASH_Sector_8;
    else if ((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
        sector = FLASH_Sector_9;
    else if ((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
        sector = FLASH_Sector_10;
    else if ((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11))
        sector = FLASH_Sector_11;
    else if ((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12))
        sector = FLASH_Sector_12;
    else if ((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13))
        sector = FLASH_Sector_13;
    else if ((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14))
        sector = FLASH_Sector_14;
    else if ((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15))
        sector = FLASH_Sector_15;
    else if ((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16))
        sector = FLASH_Sector_16;
    else if ((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17))
        sector = FLASH_Sector_17;
    else if ((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18))
        sector = FLASH_Sector_18;
    else if ((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19))
        sector = FLASH_Sector_19;
    else if ((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20))
        sector = FLASH_Sector_20;
    else if ((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21))
        sector = FLASH_Sector_21;
    else if ((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22))
        sector = FLASH_Sector_22;
    else if ((Address < ADDR_FLASH_SECTOR_23 + 0x20000) && (Address >= ADDR_FLASH_SECTOR_23))
        sector = FLASH_Sector_23;
    else
        sector = 0xFFFFFFFF;   /* ???? */

    return sector;
}

FLASH_Status FLASH_EraseApp1(void)
{
    uint16_t secStart = GetSector(APP1_ADDR);
    uint16_t secEnd  = GetSector(APP2_ADDR - 1);
		uint16_t i;
    FLASH_Unlock();
    for (i = (secStart >> 3); i <= (secEnd >> 3); i++) {
        if (FLASH_EraseSector((i << 3), VoltageRange_3) != FLASH_COMPLETE)
            return FLASH_ERROR_PGP;
    }
    FLASH_Lock();
    return FLASH_COMPLETE;
}

FLASH_Status FLASH_WriteWords(uint32_t addr, uint32_t *buf, uint32_t wordCnt)
{
		uint32_t i;
    FLASH_Unlock();
    for (i = 0; i < wordCnt; i++) {
        if (FLASH_ProgramWord(addr + i * 4, buf[i]) != FLASH_COMPLETE)
            return FLASH_ERROR_PGP;
    }
    FLASH_Lock();
    return FLASH_COMPLETE;
}

static FLASH_Status CopyApp2ToApp1(void)
{
    uint32_t src = APP2_ADDR;
    uint32_t dst = APP1_ADDR;
    uint32_t left = APP2_ADDR - APP1_ADDR - 1;
    FLASH_Status stat;

    stat = FLASH_EraseApp1();
    if (stat != FLASH_COMPLETE) return stat;

    while (left) {
        uint32_t w = (left > 1024) ? 1024 : left;
        memcpy((void *)copyBuf, (void *)src, w);
        stat = FLASH_WriteWords(dst, copyBuf, w / 4);
        if (stat != FLASH_COMPLETE) return stat;
        src += w;
        dst += w;
        left -= w;
    }
    return FLASH_COMPLETE;
}

typedef void (*pFunc)(void);

static void JumpToApp(uint32_t addr)
{
    __disable_irq();
    SysTick->CTRL = 0;
    SCB->VTOR = addr;
    __set_MSP(*(__IO uint32_t *)addr);
    ((pFunc)(*(__IO uint32_t *)(addr + 4)))();
}

int main(void)
{
		uint32_t i;
    SystemInit();               // 168 MHz
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    if(*(__IO uint32_t *)FLAG_ADDR == FLAG_VALID) {
				if((*(__IO uint32_t *)APP2_ADDR & 0xFF000000) == 0x20000000)
				{	//make sure APP2 data is ture update data
					if (CopyApp2ToApp1() == FLASH_COMPLETE) {
							FLASH_Unlock();
							FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
							FLASH_ProgramWord(FLAG_ADDR, FLAG_DONE);
							FLASH_Lock();
					}
				}
				else
				{	//or erase update Flag and jump to APP1 to advoid operation wrong
					FLASH_Unlock();
					FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
					FLASH_Lock();
				}
    }
		
		while(1)
		{
			if ((*(__IO uint32_t *)APP1_ADDR & 0xFF000000) == 0x20000000)
					JumpToApp(APP1_ADDR);
			
			for(i = 0; i < 10000; i++)	//delay time and try
				__NOP();
		}
}


