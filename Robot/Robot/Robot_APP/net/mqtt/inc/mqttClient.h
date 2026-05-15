/******************************************************************************
 * @brief    MQTT 客户端管理
 *
 * Copyright (c) 2024 <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-07-16     Morro        Initial version
 * 2021-11-28     Morro        优化自动重连管理 
 *******************************************************************************
 * @details
 *
 * MQTT组件使用流程
 *
 * 1. 定义 mqtt_config_t 配置参数结构体,至少填充host,port,client_id,event_handler
 *    ,recvbuf_size这几个域,如:
 *    static mqtt_config_t config = {
 *        .host = "www.xxx.com",                       //服务器地址
 *        .port = 1883,                                //服务器端口号
 *        .client_id = "MQTT-Demo",
 *        .event_handler = event_func,
 *        .recvbuf_size  = 256
 *       //可选参数(按表{1, 3, 10, 20, 60, 120, 300}s间隔进行重连)
 *        .reconnect_enable   = 1,                     //断开后自动重连
 *        .heartbeat_interval = 300,                   //5min 1个心跳包
 *        .clean_session      = true
 *    };
 *
 * 2. 使用mqtt_client_create 创建MQTT客户端实例,如果成功则返回非NULL值
 
 * 4. 启动一个任务, 并间歇调用 mqtt_client_process 函数
 *
 * 5. 启动一个任务, 并持续调用 mqtt_client_recv 函数

 * 6. 调用mqtt_client_connect连接服务器(如何使能了自动重连则可以省略此步骤)
 * 
 * 7. 完成以上步骤就可以跟服务器进行数据交互了(订阅主题/发布消息)
 ******************************************************************************/
#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_ 
     
#define MQTT_CONN_TIMEOUT   30           /* 连接超时时间(s)*/     
#define MQTT_SEND_TIMEOUT   30           /* 发送超时时间(s)*/     
     
typedef int mqtt_client_t;

/**
 * @brief MQTT 服务质量
 */
typedef enum {
    QOS0 = 0, 
    QOS1, 
    QOS2, 
    SUBFAIL = 0x80
}mqtt_qos;

/**
 * @brief MQTT 事件类型
 */
typedef enum {
    MQTT_EVENT_ERROR = 0,          //未知错误
    MQTT_EVENT_OFFLINE,            //已断开连接
    MQTT_EVENT_RECONNECT,          //重连成功
    MQTT_EVENT_ONLINE,             //连接成功
    MQTT_EVENT_DATA,               //来自服务器的数据包
}mqtt_event_type; 

/**
 * @brief    mqtt事件参数
 */
typedef struct {
    mqtt_event_type type;                         /* 事件类型*/
    
    /*下面是服务器推送的MQTT_EVENT_DATA信息 */                          
    mqtt_qos       qos;
    unsigned char  retain;
    unsigned char  dup;
    unsigned char  lock;
    const char    *topic;                         /* 主题*/
    int            topic_size;                    /* 主题长度*/
    unsigned char *payload;                       /* 载荷 */
    int            payload_size;                  /* 载荷长度 */
}mqtt_event_args_t;

/**
 * @brief    mqtt client 配置
 */
typedef struct {
    /* 事件处理程序 */
    void (*event_handler)(mqtt_client_t *, mqtt_event_args_t *args);
    const char    *client_id;                     /* 客户端id */
    const char    *host;                          /* 远程服务器主机名 */
    const char    *username;                      /* 用户名称*/
    const char    *userpwd;                       /* 用户密码*/
    unsigned short recvbuf_size;                  /* 接收缓冲区大小(取决于playload)*/
    unsigned short port;                          /* 服务器端口号 */
    unsigned short heartbeat_interval;            /* 心跳间隔 (unit:s)*/
    unsigned char  reconnect_enable;              /* 自动重连使能 */
    unsigned char  clean_session;                 /* 离线包处理方式*/
    /** 
     * @brief 遗属信息
     */
    struct {                                      
        unsigned char will_flag;                  
        unsigned char retain;
        mqtt_qos      qos;                         
        const char   *topic;
        const char   *msg;
    } will_options;
} mqtt_config_t;

/**
 * @brief      创建mqtt客户端
 * @param[in]  config - 客户端配置参数
 * @return     NULL   - 创建失败, 其它值 - mqtt客户端
 */
//mqtt_client_t *mqtt_client_create(const mqtt_config_t *);
/**
 * @brief    销毁mqtt客户端
 */
//void mqtt_client_destroy(mqtt_client_t *);

/**
 * @brief	   连接服务器
 * @return     RIL_OK  -  连接成功, 其它值 - 异常
 */
int mqtt_client_connect(st_socket_info * socket, char * dev_sn);

   
/**
 * @brief	   发布消息
 * @params[in] mc    - mqtt_client
 * @params[in] topic - 主题
 * @params[in] payload   - 数据
 * @params[in] payload_size   - 数据长度
 * @params[in] qos   - 消息质量
 * @retval     RIL_OK  -  发布成功, 其它值 - 异常
 */
int mqtt_client_publish(st_socket_info * socket, const char *topic, void *payload, 
                        int payload_size, mqtt_qos qos);
/**
 * @brief	   订阅主题
 * @params[in] mc    - mqtt_client
 * @params[in] topic - 主题
 * @params[in] qos   - 服务质量
 * @params[out] grantedQoS   -  服务器保证的质量
 * @retval     RIL_OK  -  订阅成功, 其它值 - 异常
 */
int mqtt_client_subscribe(st_socket_info * socket, const char *topic, mqtt_qos qos, mqtt_qos *grantedQoS);

/**
 * @brief	   解除订阅
 * @params[in] mc     - mqtt_client
 * @params[in] topic  - 主题
 * @retval     RIL_OK -  操作成功, 其它值 - 异常
 */
//int mqtt_client_unsubscribe(st_socket_info * socket, const char *topic);

extern void keepalive(st_socket_info * socket);

extern void keepalive_cloud(st_socket_info * socket);
extern void request_curtime(st_socket_info * socket);

extern bool publish_packet_process(st_socket_info * socket);

/**
 * @brief	   MQTT任务处理程序, 管理心跳发送及重连
 * @params[in] mc - mqtt_client
 * @note       该函数不允许与mqtt_client_recv放到同一个任务或者/线程中进行轮询
 * @return     none
 */
//void mqtt_client_process(st_socket_info * socket);

/**
 * @brief	   MQTT数据接收处理程序
 * @params[in] mc - mqtt_client
 * @note       该函数应单独建立一个任务进行轮询调用
 * @return     none
 */
void mqtt_client_recv(mqtt_client_t *mc);

#endif
