#ifndef __NET_TASK_UART_H_
#define __NET_TASK_UART_H_

extern uint8_t NVIDIA_send_is_idle(void);
extern uint8_t cloud_send_is_idle(void);
extern void net_debug_printf(char * pcBuf, int wLen);

#endif

