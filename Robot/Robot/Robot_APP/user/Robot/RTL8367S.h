#ifndef _ROBOT_IIC_H_
#define _ROBOT_IIC_H_

#include "platform.h"
#include "bsp.h"

typedef struct {
	GPIO_TypeDef * MDC_PORT;
	uint16_t usMDC_Pin;
	GPIO_TypeDef * MDIO_PORT;
	uint16_t usMDIO_Pin;
}SMI_TYPE;

#define RTL8367S_DEV	0x04

extern SMI_TYPE SMI_DEV[];

#define MDC_H(ucIndex)		GPIO_SetBits(SMI_DEV[ucIndex].MDC_PORT, SMI_DEV[ucIndex].usMDC_Pin)
#define MDC_L(ucIndex)		GPIO_ResetBits(SMI_DEV[ucIndex].MDC_PORT, SMI_DEV[ucIndex].usMDC_Pin)
#define MDIO_H(ucIndex)		GPIO_SetBits(SMI_DEV[ucIndex].MDIO_PORT, SMI_DEV[ucIndex].usMDIO_Pin)
#define MDIO_L(ucIndex)		GPIO_ResetBits(SMI_DEV[ucIndex].MDIO_PORT, SMI_DEV[ucIndex].usMDIO_Pin)
#define MDIO_READ(ucIndex)  GPIO_ReadInputDataBit(SMI_DEV[ucIndex].MDIO_PORT, SMI_DEV[ucIndex].usMDIO_Pin)

extern uint16_t SMI_Read(uint8_t ucIndex, uint8_t ucPhyAddr, uint8_t ucRegAddr);
extern void SMI_Write(uint8_t ucIndex, uint8_t ucPhyAddr, uint8_t ucRegAddr, uint16_t usData);

#define RTLA_Read(ucPhyAddr, ucRegAddr)				SMI_Read(0, ucPhyAddr, ucRegAddr)
#define RTLA_Write(ucPhyAddr, ucRegAddr, usData)	SMI_Write(0, ucPhyAddr, ucRegAddr, usData)
#define RTLB_Read(ucPhyAddr, ucRegAddr)				SMI_Read(0, ucPhyAddr, ucRegAddr)
#define RTLB_Write(ucPhyAddr, ucRegAddr, usData)	SMI_Write(0, ucPhyAddr, ucRegAddr, usData)


#endif



