#ifndef _LOOPBACK_H_
#define _LOOPBACK_H_

#include <stdint.h>

#define DATA_BUF_SIZE   1024
/************************/
/* Select LOOPBACK_MODE */
/************************/
#define LOOPBACK_MAIN_NOBLOCK    0
#define LOOPBACK_NONBLOCK_API    1
#define LOOPBACK_BLOCK_API       2

#define LOOPBACK_MODE   LOOPBACK_NONBLOCK_API

int loopback_tcps(uint8_t sn, uint8_t *pbuf, uint16_t port);

int loopback_udps(uint8_t sn, uint8_t *pbuf, uint16_t port);

int rcvonly_tcps(uint8_t sn, uint8_t *pbuf, uint16_t port);

#endif
