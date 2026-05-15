#ifndef __HAL_RTL83XX_H__
#define __HAL_RTL83XX_H__

#include "bsp.h"

//#define MDIO_C45_TEST

typedef int  (*io_get)(unsigned int *pin);
typedef void (*io_set)(unsigned int *pin, unsigned char enable);
typedef void (*io_delay_us)(unsigned int us);

typedef struct {
    unsigned int mdio;
    unsigned int mdc;
} st_RTL_dev;

typedef enum rtk_port_e {
    UTP_PORT0 = 0,
    UTP_PORT1,
    UTP_PORT2,
    UTP_PORT3,
    UTP_PORT4,
    UTP_PORT5,
    UTP_PORT6,
    UTP_PORT7,

    EXT_PORT0 = 16,
    EXT_PORT1,
    EXT_PORT2,

    UNDEFINE_PORT = 30,
    RTK_PORT_MAX = 31
} rtk_port_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int rtl83xx_reg_write(unsigned int phy, unsigned int reg, unsigned int val);

int rtl83xx_reg_read(unsigned int phy, unsigned int reg);

int rtl83xx_init(st_RTL_dev *pdev, io_set hset, io_get hget, io_delay_us delay);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __HAL_RTL83XX_H__ */
