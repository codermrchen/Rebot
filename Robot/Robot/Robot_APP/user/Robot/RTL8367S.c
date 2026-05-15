#include "platform.h"
#include "bsp.h"
#include "rtl8367s.h"

SMI_TYPE SMI_DEV[] = {
		{GPIOH, GPIO_Pin_4, GPIOH, GPIO_Pin_5},
		{GPIOH, GPIO_Pin_7, GPIOH, GPIO_Pin_8},
	};

static void RTL8367_Delay(uint32_t us)
{
	uint32_t ulCnt = us * 42;
	while(ulCnt--) __NOP();
}

static void SDA_OUT(uint8_t ucIndex)
{   
	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SMI_DEV[ucIndex].usMDIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(SMI_DEV[ucIndex].MDIO_PORT, &GPIO_InitStructure);
}
static void SDA_IN(uint8_t ucIndex)
{   
	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SMI_DEV[ucIndex].usMDIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;   
    GPIO_Init(SMI_DEV[ucIndex].MDIO_PORT, &GPIO_InitStructure);
}

static void SMI_Clock(uint8_t ucIndex)
{
	MDC_L(ucIndex);
	RTL8367_Delay(100);
	MDC_H(ucIndex);
	RTL8367_Delay(100);
}

static void SMI_WriteBit(uint8_t ucIndex, uint8_t bit)
{
	if(bit) MDIO_H(ucIndex);
	else MDIO_L(ucIndex);
	SMI_Clock(ucIndex);
}

static uint8_t SMI_ReadBit(uint8_t ucIndex)
{
	uint8_t bit;
	MDC_L(ucIndex);
	RTL8367_Delay(100);
	bit = MDIO_READ(ucIndex);
	MDC_H(ucIndex);
	RTL8367_Delay(100);
	return bit;
}

static void SMI_SendPreamble(uint8_t ucIndex)
{	//SMIÇ°µ¼Âë
	uint8_t i;
	SDA_OUT(ucIndex);
	for(i = 0; i< 32; i++)
		SMI_WriteBit(ucIndex, 1);
}

uint16_t SMI_Read(uint8_t ucIndex, uint8_t ucPhyAddr, uint8_t ucRegAddr)
{
	uint16_t usData = 0;
	uint8_t i;

	taskENTER_CRITICAL();
	SMI_SendPreamble(ucIndex);

	SMI_WriteBit(ucIndex, 0); SMI_WriteBit(ucIndex, 1);	//01Æô¶¯
	SMI_WriteBit(ucIndex, 1); SMI_WriteBit(ucIndex, 0);	//10¶ÁÈ¡

	for(i = 0; i < 5; i++)								//PHY addr
		SMI_WriteBit(ucIndex, (ucPhyAddr >> (4 - i)) & 0x01);

	for(i = 0; i < 5; i++)								//¼Ä´æÆ÷µØÖ·
		SMI_WriteBit(ucIndex, (ucRegAddr >> (4 - i)) & 0x01);

	SDA_IN(ucIndex);

	SMI_ReadBit(ucIndex);
	if(SMI_ReadBit(ucIndex) != 0) 
	{
		taskEXIT_CRITICAL();
		return 0xFFFF;
	}

	for(i = 0; i < 16; i++)
		usData = (usData << 1) | SMI_ReadBit(ucIndex);

	SDA_OUT(ucIndex);
	MDIO_H(ucIndex);
	taskEXIT_CRITICAL();
	return usData;
}

void SMI_Write(uint8_t ucIndex, uint8_t ucPhyAddr, uint8_t ucRegAddr, uint16_t usData)
{
	uint8_t i;

	taskENTER_CRITICAL();
	SMI_SendPreamble(ucIndex);

	SMI_WriteBit(ucIndex, 0); SMI_WriteBit(ucIndex, 1);	//01Æô¶¯
	SMI_WriteBit(ucIndex, 0); SMI_WriteBit(ucIndex, 1);	//01Ð´Èë

	for(i = 0; i < 5; i++)								//PHY addr
		SMI_WriteBit(ucIndex, (ucPhyAddr >> (4 - i)) & 0x01);

	for(i = 0; i < 5; i++)								//¼Ä´æÆ÷µØÖ·
		SMI_WriteBit(ucIndex, (ucRegAddr >> (4 - i)) & 0x01);

	SMI_WriteBit(ucIndex, 1); SMI_WriteBit(ucIndex, 0);	//TA = 10

	for(i = 0; i < 16; i++)
		SMI_WriteBit(ucIndex, (usData >> (15 - i)) & 0x01);
	taskEXIT_CRITICAL();
}


