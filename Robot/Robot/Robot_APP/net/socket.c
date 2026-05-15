/******************************************************************************
 * @brief    ril socket
 *
 * Copyright (c) 2020~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-20     Morro        Initial version.
 * 2021-04-23     Morro        Fix the issue of not exiting the critical region
 *                             correctly when allocating Socket id.
 * 2021-05-04     Morro        Fix the issue of repeatedly closing the socket
 *                             when the remote host is disconnected.
 * 2021-12-08     Morro        Fix the problem of receiving abnormal data under
 *                             multitasking system
 ******************************************************************************/
#include <string.h>
#include "linux_list.h"
#include "ringbuffer.h"
#include "async_work.h"
#include "socket.h"

#define SOCK_FD(s)  (int)(s)

/*socket ---------------------------------------------------------------------*/
typedef struct {
    st_socket_base   base;                             /* socket 基本信息*/
    sock_evt_handle  event;                            /* socket事件 */
    st_ring_buf      rb;                               /* 环形缓冲区管理*/
    struct list_head node;                             /* 链表节点*/
    unsigned int     conn_failed_wait;                 /* 连接失败等待时间*/
    unsigned int     conn_timer, send_timer, tick;     /* 定时器*/

    unsigned short   unread_data_size;                 /* 剩余待读取的数据大小*/
    unsigned char    connstat, sendstat;               /* 连接状态,发送状态*/
    unsigned char    conn_failed_cnt;                  /* 连接失败计数*/
    unsigned char    recv_incomming : 1;               /* 数据接收到来标志*/
    unsigned char    recv_event     : 1;               /* 接收事件处理标志*/
    unsigned char    recvbuf[0];                       /* 接收缓冲区*/
} st_socket_obj;

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(socket_list);

//static st_socket_ops _socket_ops;

static unsigned int socket_id_tbl;                   //socket id表

const char *socket_status_desc[SOCK_STAT_MAX] = {
    "Unknow", "Busy", "Completed", "Failed", "Timeout"
};

//static int _socket_send(st_socket_base *s, const void *pbuf, unsigned int len) { return 0; }

//static int _socket_recv(st_socket_base *s, void *pbuf, unsigned int len) { return 0; }

//static int _socket_disconnect(st_socket_base *s) { return 0; }

//static int _socket_connect(st_socket_base *s) { return 0; }

//static int _socket_phy_online(st_socket_base *s) { return 0; }

//static e_socket_status _socket_conn_status(st_socket_base *s) { return SOCK_STAT_UNKNOW; }

//static e_socket_status _socket_send_status(st_socket_base *s) { return SOCK_STAT_UNKNOW; }

/**
 * Returns the smallest power of two >= its argument, with several caveats:
 * If the argument is negative but not Integer.MIN_VALUE, the method returns
 * zero. If the argument is > 2^30 or equal to Integer.MIN_VALUE, the method
 * returns Integer.MIN_VALUE. If the argument is zero, the method returns
 * zero.
 *
 */
static int _roundUpToPowerOfTwo(int i)
{
    i--; // If input is a power of two, shift its high-order bit right.
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;

    return (i + 1);
}

/**
 * @brief      分配SOCKET ID
 */
static bool _socket_id_alloc(int *id)
{
    int i;

    net_enter_critical();
    for (i = 0; i < 32; i++) {
        if ( (socket_id_tbl & (1 << i)) == 0) {
            *id = i;
            socket_id_tbl |= 1 << i;
            net_exit_critical();
            return true;
        }
    }

    net_exit_critical();
    return false;
}

/**
 * @brief      释放SOCKET ID
 */
static void _socket_id_free(int id)
{
    net_enter_critical();
    socket_id_tbl &= ~(1 << id);
    net_exit_critical();
}

/**
 * @brief      事件上报
 */
static void _socket_event_invoke(st_socket_obj *s, e_socket_event type)
{
    if (s->event) {
        s->event(SOCK_FD(s), type);
    }
}
/**
 * @brief       更新socket连接状态
 */
static void _socket_update_connstat(st_socket_obj *s, e_socket_status status)
{
    if (status != SOCK_STAT_DONE && status != SOCK_STAT_BUSY) {
        socket_disconnect(SOCK_FD(s));
    }

    s->connstat = status;
    _socket_event_invoke(s, SOCK_EVENT_CONN); /* 上报连接事件 */
}

/**
 * @brief       更新socket发送状态
 */
static void _socket_update_sendstat(st_socket_obj *s, e_socket_status status)
{
    s->sendstat = status;
    _socket_event_invoke(s, SOCK_EVENT_SEND);         /* 上报发送事件*/
}

/**
 * @brief       注册socket
 */
static  void _socket_register(st_socket_obj *s)
{
    net_enter_critical();
    list_add_tail(&s->node, &socket_list);
    net_exit_critical();
}

/**
 * @brief       移除socket
 */
static void _socket_unregister(st_socket_obj *s)
{
    net_enter_critical();
    list_del(&s->node);
    net_exit_critical();
}

/**
 * @brief  socket数据接收(用于处理主动上报)
 */
static void _socket_data_input(st_socket_obj *s, const void *buf, int size)
{
    for (int i = 0; 5 > i; i++) {
        net_enter_critical();
        size -= ring_buf_put(&s->rb, (unsigned char *)buf, size);
        net_exit_critical();

        if (0 == size) {
            return;
        }

        net_delay(10); //等待上层读取
    }
}

/**
 * @brief  socket数据接收处理
 */
static void _socket_data_proc(st_socket_obj *s)
{
    unsigned int buff_size, read_size;
    void *buff;
	
    if (!socket_online(SOCK_FD(s))) {
        return; /* 未连接网络 */
    }

    //if (s->recv_incomming || s->unread_data_size) {
        if (4 > ring_buf_free_space(&s->rb)) { /* 空间不足,暂时不读取 */
            return;
        }

        buff_size = 1500;
        buff = net_malloc(buff_size);
        if (NULL == buff) {
            return;
        }

        //read_size = _socket_ops.recv(data, buff, buff_size);
        if (read_size) { /* 上报数据 */
            socket_notify(&s->base, SOCK_NOTFI_DATA_REPORT, buff, read_size);

            if (read_size > s->unread_data_size) {
                s->unread_data_size -= read_size;
            }
            else {
                s->unread_data_size = 0;
            }
        } else { /* 数据读取完毕 */
            s->recv_incomming   = 0;
            s->unread_data_size = 0;
        }

        net_free(buff);
    //}
}

/**
 * @brief  当出现连接异常时,插入连接等待时间,避免频繁尝试
 */
//static bool _socket_conn_wait(st_socket_obj *s)
//{
//    unsigned int wait_time;

//    if (s->conn_failed_cnt) {
//        wait_time = s->conn_failed_cnt % 10 * 6000;
//        if (!net_istimeout(s->conn_failed_wait, wait_time)) {
//            return false;
//        }
//    }
//    return true;
//}

/**
 * @brief       socket状态监视
 * @param[in]   c
 * @return      none
 */
static void _sock_status_watch(st_socket_obj *s)
{
    e_socket_status status;

    if (SOCK_STAT_BUSY == s->connstat) { //连接状态查询
        if (net_istimeout(s->conn_timer, MAX_SOCK_CONN_TIME * 1000)) {
            _socket_update_connstat(s, SOCK_STAT_TIMEOUT); //连接超时
        }
        else if (net_istimeout(s->tick, 1000)) {
            s->tick = net_get_ms();
            //status = _socket_ops.conn_status(&s->base);
            if (SOCK_STAT_BUSY != status && SOCK_STAT_UNKNOW != status)
                _socket_update_connstat(s, status);
        }
    }

    if (SOCK_STAT_BUSY == s->sendstat) { //发送状态查询
        if (net_istimeout(s->send_timer, MAX_SOCK_SEND_TIME * 1000)) {
            _socket_update_sendstat(s, SOCK_STAT_TIMEOUT); //发送超时
        }
        else if (net_istimeout(s->tick, 1000)) {
            s->tick = net_get_ms();
            //status = _socket_ops.send_status(&s->base);
            if (SOCK_STAT_BUSY != status && SOCK_STAT_UNKNOW != status) {
                _socket_update_sendstat(s, status);
            }
        }
    }
}

//通知处理
static void _socket_notify_proc(work_async_t *w, void *object, void *params)
{
    e_socket_notify type = (e_socket_notify)((int)(params));
    st_socket_obj *s = (st_socket_obj *)object;

    switch (type) {
        case SOCK_NOTFI_ONLINE:                            //上线事件
            _socket_update_connstat(s, SOCK_STAT_DONE);
            break;
        case SOCK_NOTFI_OFFLINE:                           //掉线事件
            //socket_disconnect(SOCK_FD(s));
            _socket_update_connstat(s, SOCK_STAT_FAILED);
            if (s->sendstat == SOCK_STAT_BUSY) {          //当前正在发送数据
                _socket_update_sendstat(s, SOCK_STAT_FAILED);
            }
            break;
        case SOCK_NOTFI_DATA_REPORT:                       //收到数据
            _socket_event_invoke(s, SOCK_EVENT_RECV);
            s->recv_event = false;
            break;
        case SOCK_NOTFI_SEND_FAILED:                       //发送失败
            _socket_update_sendstat(s, SOCK_STAT_FAILED);
            break;
        case SOCK_NOTFI_SEND_SUCCESS:                      //发送成功
            _socket_update_sendstat(s, SOCK_STAT_DONE);
            break;
        default:
            break;
    }
}

#if 0

/**
 * @brief       socket 异步连接操作
 * @param[in]   sockfd   - socket描述符
 * @param[in]   host     - 远程主机
 * @param[in]   port     - 远程端口
 * @param[in]   type     - socket 类型( 0 -TCP, 1 - UDP)
 * @return      NET_OK   - 执行成功, 其它值 - 异常
 */
int socket_connect_async(uint8_t sockfd, const char *host, unsigned short port, e_socket_type type)
{
    st_socket_obj *s = (st_socket_obj *)sockfd;
    int connect_state;

    if (s->connstat == SOCK_STAT_BUSY || !_socket_ops.isonline(&s->base) || !_socket_conn_wait(s)) {
        return NET_REJECT;
    }

    s->base.host  = host;
    s->base.port  = port;
    s->base.type  = type;
    s->conn_timer = net_get_ms();
    s->sendstat   = SOCK_STAT_UNKNOW;

    connect_state = _socket_ops.connect(&s->base);
    if (NET_OK == connect_state) {
        s->connstat = SOCK_STAT_DONE;
        s->conn_failed_cnt = 0;
        return NET_OK;
    }
    else if (NET_ONGOING != connect_state) {
        s->connstat = SOCK_STAT_FAILED;
        s->conn_failed_cnt ++; /*统计失败次数*/
        s->conn_failed_wait = net_get_ms();
        return NET_ERROR;
    } else {    
        s->connstat = SOCK_STAT_BUSY;
        s->conn_failed_cnt = 0;
        return NET_OK;
    }
}

#endif

/**
 * @brief       socket 数据发送操作(阻塞直到连接结束)
 * @param[in]   sockfd   - socket描述符
 * @param[in]   host     - 远程主机
 * @param[in]   port     - 远程端口
 * @param[in]   type     - 连接类型(NET_SOCK_XXX)
 * @return      NET_OK  - 连接成功, 其它值 - 异常
 */
int socket_connect(uint8_t Index, const char *host, unsigned short port, e_socket_type type)
{
//    st_socket_obj *s = (st_socket_obj *)Index;
//    int ret;

//    ret = socket_connect_async(SOCK_FD(s), host, port, type);
//    if (NET_OK != ret) {
//        return ret;
//    }

//    while (SOCK_STAT_BUSY == s->connstat) {
//        net_delay(1);//yeild
//    }

//    return ((SOCK_STAT_DONE == s->connstat) ? NET_OK : NET_ERROR);
		return 0;
}

/**
 * @brief       断开连接
 */
int socket_disconnect(st_socket_desc sockfd)
{
#if 0
    st_socket_obj *s = (st_socket_obj *)sockfd;

    s->connstat = SOCK_STAT_UNKNOW;
    s->sendstat = SOCK_STAT_UNKNOW;

    return _socket_ops.disconnect(&s->base);
#endif
	return 0;
}

bool socket_phy_online(st_socket_desc sockfd)
{
#if 0
    st_socket_obj *s = (st_socket_obj *)sockfd;

    return (0 != _socket_ops.isonline(&s->base));
#endif
	return true;
}

/**
 * @brief       指示socket已经连接服务器
 */
bool socket_online(st_socket_desc sockfd)
{
    st_socket_obj *s = (st_socket_obj *)sockfd;

    return s->connstat == SOCK_STAT_DONE;
}

/**
 * @brief       指示socket正在进行连接/发送状态
 */
bool socket_busy(st_socket_desc sockfd)
{
    st_socket_obj *s = (st_socket_obj *)sockfd;

    return (s->connstat == SOCK_STAT_BUSY || s->sendstat == SOCK_STAT_BUSY);
}

/**
 * @brief       接收数据(非阻塞)
 * @param[in]   s   - socket
 * @param[in]   buf - 接收缓冲区
 * @param[in]   len - 缓冲区长度
 * @retval      实际接收到数据长度
 */
int socket_recv(st_socket_info * socket)
{
    if(socket->RecvSta == 0) return 0;
	else return socket->RecvLen;
}

/**
 * @brief       socket 数据发送操作(阻塞直到发送结束)
 * @param[in]   s   - socket
 * @param[in]   buf - 数据缓冲区
 * @param[in]   len - 缓冲区长度
 * @return      NET_OK   - 发送成功, 其它值 - 异常
 */
int socket_send(st_socket_info * socket, const void *buf, unsigned int len)
{
	socket_send_async(socket, buf, len);
	socket->SendSta = 1;
	socket->SendSta = 0;
	socket->SendTick = net_get_ms();
    while (!net_istimeout(socket->SendTick, 10000)) {	//发送等待完成10s	
        if (socket->SendSta == 1) {
			socket->RecvWaitFlag = 1;
            return NET_OK;
        }

        sys_delay_ms(10);
    }

    return NET_ERROR;
}

/**
 * @brief       socket 数据发送操作(非阻塞)
 * @param[in]   s   - socket
 * @param[in]   buf - 数据缓冲区
 * @param[in]   len - 缓冲区长度
 * @return      NET_OK   - 执行成功, 其它值 - 异常
 */
int socket_send_async(st_socket_info * socket, const void *buf, unsigned int len)
{
	socket->send(socket->Index, (void *)buf, len);
	return len;
}

/**
 * @brief  socket 监视任务
 */
void socket_status_watch(void)
{
    st_socket_obj *s;
    struct list_head *list;
    struct list_head *n = NULL;

    list_for_each_safe(list, n, &socket_list) {
        s = list_entry(list, st_socket_obj, node);
        _sock_status_watch(s);  /* socket 状态管理*/
        _socket_data_proc(s);   /* socket 数据接收处理*/
    }
}


/**
 * @brief       为socket设置附属数据
 * @param[in]   s       - socket
 * @param[in]   tag     - 附属数据
 */
void socket_set_tag(st_socket_base *s, void *tag)
{
    s->tag = tag;
}

/**
 * @brief  通过socket 附属数据查询socket
 */
st_socket_base *socket_find_by_tag(void *tag)
{
    st_socket_obj *s;
    struct list_head *list ,*n = NULL;

    list_for_each_safe(list, n, &socket_list) {
        s = list_entry(list, st_socket_obj, node);
        if (s->base.tag == tag) {
            return &s->base;
        }
    }
    return NULL;
}

/**
 * @brief  通过id查询socket
 */
st_socket_base *socket_find_by_id(int id)
{
    st_socket_obj *s;
    struct list_head *list ,*n = NULL;

    list_for_each_safe(list, n, &socket_list) {
        s = list_entry(list, st_socket_obj, node);
        if (s->base.id == id) {
            return &s->base;
        }
    }

    return NULL;
}

/**
 * @brief       连接状态
 */
e_socket_status socket_connstat(st_socket_desc sockfd)
{
    st_socket_obj *s = (st_socket_obj *)sockfd;

    return (e_socket_status)s->connstat;
}

/**
 * @brief       发送状态
 */
e_socket_status socket_sendstat(st_socket_desc sockfd)
{
    st_socket_obj *s = (st_socket_obj *)sockfd;

    return (e_socket_status)s->sendstat;
}

/**
 * @brief  强制清理socket资源
 */
void socket_dispose(void)
{
    st_socket_obj *s;
    struct list_head *list, *n = NULL;

    net_enter_critical();

    list_for_each_safe(list, n, &socket_list) {
        s = list_entry(list, st_socket_obj, node);
        s->connstat = SOCK_STAT_UNKNOW;
        s->sendstat = SOCK_STAT_UNKNOW;
        s->conn_failed_cnt = 0;
        s->recv_incomming  = 1;
        s->recv_event      = 0;
    }

    net_exit_critical();
}

/**
 * @brief 销毁socket
 */
void socket_destroy(st_socket_desc sockfd)
{
    st_socket_obj *s = (st_socket_obj *)sockfd;
    if (socket_online(sockfd)) {
        socket_disconnect(sockfd);
    }

    _socket_id_free(s->base.id);
    _socket_unregister(s);
    net_free(s);
}

/**
 * @brief       创建socket
 * @param[in]   s       - socket
 * @param[in]   e       - socket事件回调接口(如果不需要,可以填NULL)
 * @param[in]   bufsize - 接收缓冲区大小(自动对齐2的幂次)，如果填0表示自动分配
 * @return      SOCKET_INVALID -  无效socket, 其它值 - socket 描述符
 */
st_socket_desc socket_create(sock_evt_handle e, unsigned int bufsize)
{
    int id;
    st_socket_obj *s;

    //计算接收缓冲区大小
    bufsize = (0 == bufsize ? DEF_SOCK_RECV_BUFSIZE : bufsize);
    bufsize = _roundUpToPowerOfTwo(bufsize);

    s = (st_socket_obj *)net_malloc(sizeof(st_socket_obj) + bufsize);
    if (NULL == s) {
        return SOCKET_INVALID;
    }

    memset(s, 0, sizeof(st_socket_obj));
    s->event = e;
    if (0 > ring_buf_init(&s->rb, s->recvbuf, bufsize)) { /* 初始化接收缓冲区*/
        goto Error;
    }

    if (!_socket_id_alloc(&id)) {
        goto Error;
    } else {
        s->base.id = id;
        _socket_register(s);
        return (int)s;
    }

Error:
    net_free(s);
    return SOCKET_INVALID;
}

/**
 * @brief  socket 初始化
 */
//void socket_init(st_socket_ops *ops)
//{
//    INIT_LIST_HEAD(&socket_list);
//    _socket_ops.recv        = ((!ops || !ops->recv) ? _socket_recv : ops->recv);
//    _socket_ops.send        = ((!ops || !ops->send) ? _socket_send : ops->send);
//    _socket_ops.connect     = ((!ops || !ops->connect) ? _socket_connect : ops->connect);
//    _socket_ops.disconnect  = ((!ops || !ops->disconnect) ? _socket_disconnect : ops->disconnect);
//    _socket_ops.isonline    = ((!ops || !ops->isonline) ? _socket_phy_online : ops->isonline);
//    _socket_ops.conn_status = ((!ops || !ops->conn_status) ? _socket_conn_status : ops->conn_status);
//    _socket_ops.send_status = ((!ops || !ops->send_status) ? _socket_send_status : ops->send_status);
//}

/*******************************************************************************
 * @brief       产生socket通知
 * @param[in]   s      - socket
 * @param[in]   type   - 通知类型
 * @param[in]   data   - 通用数据,参考e_socket_notify描述
 * @return      none
 ******************************************************************************/
void socket_notify(st_socket_base *base, e_socket_notify type, void *data, int size)
{
    st_socket_obj *s = container_of(base, st_socket_obj, base);

    if (SOCK_NOTFI_DATA_INCOMMING == type) {
        s->unread_data_size = (data ? (int)data : 0);
        s->recv_incomming = 1;
        return;
    }

    if (SOCK_NOTFI_DATA_REPORT != type) {
    }
    else {
        _socket_data_input(s, data, size);
        if (0 != s->recv_event) { //避免频繁产生接收事件
            return;
        }

        s->recv_event = true;
    }

    async_work_add(NULL, s, (void *)type, _socket_notify_proc);
}
