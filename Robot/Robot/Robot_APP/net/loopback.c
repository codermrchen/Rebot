#include <stdio.h>
#include "loopback.h"
//#include "hal_w5500.h"
#include "hal_wiz_socket.h"
//#include "hal_wiz_cfg.h"

//extern uint32_t GlobalCounter;
#if _WIZCHIP_SOCK_NUM_ == 8
uint8_t connection_status[_WIZCHIP_SOCK_NUM_] = {0,0,0,0,0,0,0,0};
#else
uint8_t connection_status[_WIZCHIP_SOCK_NUM_] = {0,0,0,0};
#endif

#if LOOPBACK_MODE == LOOPBACK_MAIN_NOBLCOK
int loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port)
{
   uint16_t sentsize = 0;
   uint16_t size = 0;
   int ret;

   switch(getSn_SR(sn))
   {
      case SOCK_ESTABLISHED :
         if(getSn_IR(sn) & Sn_IR_CON)
         {
            printf("%d:Connected\r\n",sn);
            setSn_IR(sn,Sn_IR_CON);
         }
         if((size = getSn_RX_RSR(sn)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
         {
            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;
            ret = recv(sn,buf,size);
            if(ret <= 0) return ret;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
            sentsize = 0;
            while(size != sentsize)
            {
               ret = send(sn,buf+sentsize,size-sentsize);
               if(ret < 0)
               {
                  wizchip_close(sn);
                  return ret;
               }
               sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
            }
         }
         break;
      case SOCK_CLOSE_WAIT :
         printf("%d: CloseWait\r\n",sn);
         if((ret = wizchip_disconnect(sn)) != SOCK_OK) return ret;
         printf("%d:Closed\r\n",sn);
         break;
      case SOCK_INIT :
         printf("%d:Listen\r\n",sn);
         if( (ret = wizchip_listen(sn)) != SOCK_OK) return ret;
         break;
      case SOCK_CLOSED:
         printf("%d:LBTStart\r\n",sn);
         ret = wizchip_socket(sn,Sn_MR_TCP,port,0x00);
         if (ret != sn) {
            printf("%d: TCP isn't opened\r\n", sn);
            return ret;
         }
         printf("%d: TCP is opened\r\n", sn);
         break;
      default:
         break;
   }
   return 1;
}

int loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port)
{
   uint16_t size, sentsize;
   uint16_t destport;
   uint8_t  destip[4];
   uint8_t  packinfo = 0;
   int  ret;

   switch(getSn_SR(sn))
   {
      case SOCK_UDP :
         if((size = getSn_RX_RSR(sn)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
         {
            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;
            ret = wizchip_recvfrom(sn,buf,size,destip,(uint16_t*)&destport,&packinfo);
            if(ret <= 0)   // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
            {
               printf("%d: recvfrom error. %ld\r\n",sn,ret);
               return ret;
            }
            size = (uint16_t) ret;
            sentsize = 0;
            while(sentsize != size)
            {
               ret = wizchip_sendto(sn,buf+sentsize,size-sentsize,destip,destport);
               if(ret < 0)
               {
                  printf("%d: sendto error. %ld\r\n",sn,ret);
                  return ret;
               }
               sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
            }
         }
         break;
      case SOCK_CLOSED:
         printf("%d:LBUStart\r\n",sn);
         ret = wizchip_socket(sn, Sn_MR_UDP, port, 0x00);
         if (ret != sn) {
            printf("%d: UDP isn't opened\r\n", sn);
            return ret;
         }
         printf("%d: UDP is opened\r\n");
         break;
      default :
         break;
   }
   return 1;
}
#endif

#if LOOPBACK_MODE == LOOPBACK_BLOCK_API
int loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t size)
{
   int ret = 0;

   ret = wizchip_recv(sn, buf, size);
   if(ret != size)
   {
      if(ret < 0)
      {
         printf("%d:recv() error:%ld\r\n",sn,ret);
         wizchip_close(sn);
         return ret;
      }
   }
   size = ret;
   ret = wizchip_send(sn,buf,size);
   if(ret != size)
   {
      if(ret < 0)
      {
         printf("%d:send() error:%ld\r\n",sn,ret);
         wizchip_close(sn);
      }
   }
   return ret;
}

int loopback_udps(uint8_t sn, uint8_t* buf, uint16_t size)
{
   static uint8_t  addr[4] = {0,};
   static uint16_t port = 0;

   uint8_t  packinfo;
   int  ret = 0;

   if((ret = wizchip_recvfrom(sn,buf,size, addr,&port,&packinfo)) < 0)
   {
      printf("%d:recvfrom error:%ld\r\n",sn,ret);
      return ret;
   }
   if(packinfo & 0x80)
   {
      printf("%d:recvfrom %d.%d.%d.%d(%d), size=%ld.\r\n",sn,addr[0],addr[1],addr[2],addr[3],port, ret);
   }
   if(packinfo & 0x01)
   {
      printf("%d:recvfrom remained packet.\r\n",sn);
   }
   else
   {
      printf("%d:recvfrom completed.\r\n",sn);
   }
   if( (ret = wizchip_sendto(sn, buf, ret, addr, port)) < 0)
   {
      printf("%d:sendto error:%ld\r\n",sn,ret);
      return ret;
   }
   printf("%d:sendto %d.%d.%d.%d(%d), size=%ld\r\n",sn,addr[0],addr[1],addr[2],addr[3],port, ret);
   return ret;
}
#endif


#if LOOPBACK_MODE == LOOPBACK_NONBLOCK_API
int rcvonly_tcps(uint8_t sn, uint8_t *pbuf, uint16_t port)
{
    uint16_t size;
    int ret = 0;

    switch(getSn_SR(sn)) {
        case SOCK_ESTABLISHED :
            if (0 == connection_status[sn]) {
                printf("%d: Connected\r\n", sn);
                connection_status[sn] = 1;
            }

            size = getSn_RX_RSR(sn);
            if (0 < size)  {// Don't need to check SOCKERR_BUSY because it doesn't not occur.
                //printf("size: %d\r\n", size);
                if (DATA_BUF_SIZE < size) {
                    size = DATA_BUF_SIZE;
                }

                //printf("timer: %d\r\n", GlobalCounter);
                ret = wizchip_recv(sn, pbuf, size);
                //printf("recv count: %d\r\n", ret);
                if (ret != size) {
                    if (SOCK_BUSY == ret) {
                        return 0;
                    }

                    if (0 > ret) {
                        printf("%d:recv() error: %d\r\n", sn, ret);
                        wizchip_close(sn);
                        connection_status[sn] = 0;
                        return ret;
                    }
                }
                return size;
            }
            break;
        case SOCK_CLOSE_WAIT :
            printf("%d:CloseWait\r\n", sn);
            ret = wizchip_disconnect(sn));
            if (SOCK_OK == ret) {
                connection_status[sn] = 0;
                printf("%d:Closed\r\n", sn);
            }
            break;
        case SOCK_CLOSED :
            printf("%d:ROTStart\r\n",sn);
            ret = wizchip_socket(sn, Sn_MR_TCP, port, SF_TCP_NODELAY);
            if (sn != ret) {
                printf("%d:socket() error:%d\r\n", sn, ret);
                connection_status[sn] = 0;
                wizchip_close(sn);
            }
            break;
        case SOCK_INIT :
            printf("%d:Opened\r\n", sn);
            ret = wizchip_listen(sn);
            if (SOCK_OK ! = ret) {
                printf("%d:Listen error\r\n",sn);
            }
            else {
                printf("%d:Listen ok\r\n",sn);
            }
            break;
        default :
            break;
    }

    return ret;
}

int loopback_tcps(uint8_t sn, uint8_t *pbuf, uint16_t port)
{
    uint16_t size;
    int sentsize = 0;
    int ret = 0;

    switch (getSn_SR(sn)) {
        case SOCK_ESTABLISHED :
            if (0 == connection_status[sn]) {
                printf("%d: Connected\r\n", sn);
                connection_status[sn] = 1;
            }

            size = getSn_RX_RSR(sn);
            if (0 < size) { // Don't need to check SOCKERR_BUSY because it doesn't not occur.
                //printf("size: %d\r\n", size);
                if (DATA_BUF_SIZE < size) size = DATA_BUF_SIZE;
                //printf("timer: %d\r\n", GlobalCounter);
                ret = wizchip_recv(sn, pbuf, size);
                //printf("recv count: %d\r\n", ret);
                if (ret == size) {
                    // Don't care SOCKERR_BUSY, because it is zero.
                    for (size = ret, sentsize = 0; sentsize != size; sentsize += ret) {
                        ret = wizchip_send(sn, &pbuf[sentsize], size - sentsize);
                        if (ret != (size - sentsize)) {
                            if (0 > ret) {
                                printf("%d:send() error:%d\r\n", sn, ret);
                                wizchip_close(sn);
                                connection_status[sn] = 0;
                                return ret;
                            }
                        }
                    }
                }
                else if (SOCK_BUSY == ret) {
                    return 0;
                }
                else if (0 > ret) {
                    printf("%d:recv() error:%d\r\n", sn, ret);
                    wizchip_close(sn);
                    connection_status[sn] = 0;
                    return ret;
                }
                return size;
            }
            break;
        case SOCK_CLOSE_WAIT :
            printf("%d:CloseWait\r\n", sn);
            ret = wizchip_disconnect(sn);
            if (SOCK_OK == ret) {
                connection_status[sn] = 0;
                printf("%d:Closed\r\n", sn);
            }
            break;
        case SOCK_CLOSED :
            printf("%d:LBTStart\r\n",sn);
            ret = wizchip_socket(sn, Sn_MR_TCP, port, SF_TCP_NODELAY);
            if (sn != ret) {
                printf("%d:socket() error:%d\r\n", sn, ret);
                connection_status[sn] = 0;
                wizchip_close(sn);
            }
            break;
        case SOCK_INIT :
            printf("%d: TCP is opened\r\n", sn);
            ret = wizchip_listen(sn);
            if (SOCK_OK != ret) {
                printf("%d: Listen error\r\n", sn);
            }
            else {
                printf("%d: Listen ok\r\n", sn);
            }
            break;
        default :
            break;
    }

    return ret;
}

int loopback_udps(uint8_t sn, uint8_t *pbuf, uint16_t port)
{
    uint16_t destport, sentsize, size;
    uint8_t  destip[4];
    int ret;

    switch (getSn_SR(sn)) {
        case SOCK_UDP :
            size = getSn_RX_RSR(sn));
            if (0 < size) { // Don't need to check SOCKERR_BUSY because it doesn't not occur.
                if (DATA_BUF_SIZE < size) size = DATA_BUF_SIZE;
                ret = wizchip_recvfrom(sn, pbuf, size, destip, (uint16_t *)&destport);
                if (0 < ret) { // Don't care SOCKERR_BUSY, because it is zero.
                    for (size = ret, sentsize = 0; sentsize != size; sentsize += ret) {
                        ret = wizchip_sendto(sn, &pbuf[sentsize], size - sentsize, destip, destport);
                        if (0 > ret) {
                            printf("%d: sendto error. %d\r\n", sn, ret);
                            break;
                        }
                    }
                }
                else { // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                    printf("%d: recvfrom error. %d\r\n", sn, ret);
                }
            }
            break;
        case SOCK_CLOSED:
            printf("%d:LBUStart\r\n",sn);
            ret = wizchip_socket(sn, Sn_MR_UDP, port, 0x0);
            if (sn == ret) {
                printf("%d: UDP is opened\r\n", sn);
            }
            break;
        default :
            break;
    }
    return ret;
}
#endif
