/******************************************************************************
 * @brief    MQTT 客户端管理
 *
 * Copyright (c) 2021 <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-16     Morro        Initial version
 * 2021-11-28     Morro        优化自动重连管理
 ******************************************************************************/
#include <stdbool.h>
#include <string.h>
#include "comdef.h"
#include "socket.h"
#include "mqttPacket.h"
#include "mqttClient.h"
#include "config.h"
#include "platform.h"
#include "cli.h"
#include "lift_task.h"
#include "work_task.h"


#define MAX_PACKET_ID 0xFFFF

/**
 * @brief    MQTT 请求状态
 */
typedef enum {
    MQTT_REQ_IDLE,
    MQTT_REQ_BUSY,
    MQTT_REQ_DONE,
    MQTT_REQ_FAILED,
    MQTT_REQ_TIMEOUT
} mqtt_request_state;

/**
 * @brief    MQTT 客户端信息
 */
typedef struct mqtt_info {
    mqtt_client_t  client;
    mqtt_config_t  config;                             /* 配置参数*/
    net_sem_t      mutex;                              /* 互斥锁*/
    net_sem_t      sem_ready;                          /* 就绪信号*/
    st_socket_desc   sockfd;
    mqtt_request_state conn_state;                     /* 连接请求状态 */
    /* 发送请求状态(发布,订阅,解除订阅)*/
    mqtt_request_state send_state;
    unsigned char  state;                              /* 接收解析器状态 */
    unsigned char  reconnect_retry;                    /* 重连重次*/
    unsigned short packet_id;                          /* 当前包ID*/
    unsigned int   reconnect_timer;                    /* 重连定时器*/
    unsigned int   recv_timer;                         /* 接收定时器*/
    unsigned int   keep_alive_timer;                   /* 心跳定时器 */
    unsigned int   last_sent, last_recv;               /* 记录最后1次发送/接收时刻*/
    unsigned       ping: 1;                            /* 心跳发送标志*/
    unsigned       connected: 1;                       /* 连接状态*/
    unsigned       reconnect: 1;                       /* 重连使能*/
    unsigned       error_cnt: 8;                       /* 异常计数*/
    unsigned short total;                              /* 总字节数*/
    unsigned short recvcnt;                            /* 接收计数器*/
    unsigned short bufsize;
    unsigned char  buf[0];                             /* 接收缓冲区*/
} mqtt_info_t;


/**
 * @brief      连接状态判断
 */
//static bool is_connected(mqtt_info_t *mi)
//{
//    return mi->connected;
//}

//下一包的包ID
static int getNextPacketId(st_socket_info * socket)
{
    return socket->PacketID++;
}

/**
 * @brief	   解析包长度
 * @retval     NET_OK - 成功获取, NET_ERROR - 异常, NET_ONGOING - 进行中
 */
//static int parse_packet_size(mqtt_info_t *mi)
//{
//    int i, multiplier;
//    int byte;
//    unsigned int total = 0;
//    multiplier = 1;
//    for (i = 1; i < mi->recvcnt; i++) {
//        byte = mi->buf[i];
//        total +=  (byte & 127) * multiplier;
//        multiplier *= 128;
//        if ((byte & 128) == 0) {
//            mi->total = total;
//            return NET_OK;
//        }
//        if (i >= 4)
//            return NET_ERROR;
//    }
//    return NET_ONGOING;
//}

/**
 * @brief	   发送数据包
 */
static int send_parket(st_socket_info * socket, void *buf, unsigned int size)
{
    return socket_send(socket, buf, size);
}

/*
 * @brief	   事件处理
 */
//static void event_invoke(mqtt_info_t *mi,  mqtt_event_args_t *args)
//{
//    if (mi->config.event_handler) {
//        if (0 < args->payload_size) {
//            char ch = args->payload[args->payload_size];
//
//            args->payload[args->payload_size] = '\0';
//            mi->config.event_handler(&mi->client, args);          //递交到上层处理
//            args->payload[args->payload_size] = ch;
//        }
//        else {
//            mi->config.event_handler(&mi->client, args);          //递交到上层处理
//        }
//    }
//}


/**
 * @brief    心跳包
 */
void keepalive(st_socket_info * socket)
{
    int len;
		static uint32_t sulTick;

    if ((sys_get_ms() - sulTick) > MQTT_HEARTBEAT * 70){//(net_istimeout(sulTick, MQTT_HEARTBEAT / 2 * 1000)) {
        len = MQTTSerialize_pingreq(socket->pSend, socket->usSendMaxLen);
        if (len > 0 && send_parket(socket, socket->pSend, len) == NET_OK) {
            socket->RecvWaitFlag = 1;
			socket->RecvSta = 1;
					sulTick = sys_get_ms();
        }

		while(!net_istimeout(sulTick, 3000))
		{
			if(socket->RecvSta == 1)
			{
				socket->RecvSta = 0;
				
				if((socket->pRecvBuf[0] >> 4) == PINGRESP) {
					sulTick = sys_get_ms();
					return;
				}
				else break;
			}

			sys_delay_ms(10);
		}
		sulTick = sys_get_ms();
		//不对英伟达进行判断
//		if(socket->Index == 0)
//		{
//			printf("Unreceive pingreq reply\r\n");
//			socket->ScokStatus = SOCKET_CONNECTED;
//		}
    }
}

int get_last_len(unsigned char * Buf)
{
	int len = 0, i;
	for(i = 0; i < 4; i++)
	{
		if(Buf[i] >= '0' && Buf[i] <= '9')
			len = len * 16 + (Buf[i] - '0');
		else if(Buf[i] >= 'A' && Buf[i] <= 'F')
			len = len * 16 + (Buf[i] - 'A' + 10);
		else if(Buf[i] >= 'a' && Buf[i] <= 'f')
			len = len * 16 + (Buf[i] - 'a' + 10);
		else
			return -1;
	}
	return len * 2;
}

int mqtt_data_handler(unsigned char * Buf)
{
	int argc;
	char *argv[CLI_MAX_ARGS];
	const char *start, *end;
	const cmd_item_t *it;

	argc = strsplit((char *)Buf, ":;,", argv, CLI_MAX_ARGS);
	start= argv[0];
	end = start + strlen(argv[0]);
	if (start == end) {
        return -1;
    }
	if (NULL == (it = find_cmd(start, end - start))) {
        debug_printf("Not find command");

        return -1;
    }
	gulPauseTime = sys_get_ms();
	it->handler(NULL, argc - 1, &argv[1]);
	return 0;
}

static void _cloud_rece_handler(unsigned char * pucBuff, st_socket_info * socket)
{
	memset(socket->pSend, 0, socket->usSendMaxLen);
	if(gOTAFlag.ucUpdateFlag == 1) return;
	if(memcmp(pucBuff, "SetPlan:", 8) == 0)
	{
		memcpy(socket->pSend, "SetPlan:01", 10);
		socket->SendLen = 10;
	}
	else if(memcmp(pucBuff, "SetStatus:", 10) == 0)
	{
		if((gWorkWorker.WorkSta == 1) || IS_WALK_WORK())
			memcpy(socket->pSend, "SetStatus:09", 12);
		else
			memcpy(socket->pSend, "SetStatus:01", 12);
		socket->SendLen = 12;
	}
	else if(memcmp(pucBuff, "Setpara:", 8) == 0)
	{
		memcpy(socket->pSend, "Setpara:01", 10);
		socket->SendLen = 10;
	}
	else if(memcmp(pucBuff, "SetLed:", 7) == 0)
	{
		memcpy(socket->pSend, "SetLed:01", 9);
		socket->SendLen = 9;
	}
	else if(memcmp(pucBuff, "SetMode:", 8) == 0)
	{
		if((gWorkWorker.WorkSta == 1) || IS_WALK_WORK())
			memcpy(socket->pSend, "SetMode:09", 10);
		else
			memcpy(socket->pSend, "SetMode:01", 10);
		socket->SendLen = 10;
	}
}

static void _NVIDIA_rece_handler(unsigned char * pucBuff, int wLen)
{
	if(sys_strstr((void *)pucBuff, _NET_NVIDIA_OPT) != NULL)
	{
		if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '1')
		{	//英伟达正常
			if((gNvidiaDevInfo.ucNVIDIASta != 0) || (gNvidiaDevInfo.usErrcode != 0))
			{
				gNvidiaDevInfo.ucNVIDIASta = 0;
				gNvidiaDevInfo.usErrcode = 0;
				set_cloud_push_flag(PROP_NVIDIA);
			}
		}
		else if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '9')
		{
			if((gNvidiaDevInfo.ucNVIDIASta != 9) || (gNvidiaDevInfo.usErrcode != 3))
			{
				gNvidiaDevInfo.ucNVIDIASta = 9;
				gNvidiaDevInfo.usErrcode = 3;
				set_cloud_push_flag(PROP_NVIDIA);
			}
		}
	}
	if(JUDGE_WORK(WORK_ASK_PHOTO))
	{	//等待巡检开始点成功
		if(sys_strstr((void *)pucBuff, _NET_INSPECT_SET) != NULL)
		{
			if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '1')
				SET_WORK_STA(WORK_ASK_GETHIGH);
			else if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '9')
			{
				lift_high_pos(0);
				SET_WORK_STA(WORK_VIEW_FINISH);
				gucRequeEndFlag = 0;
			}
		}
	}
	else if(JUDGE_WORK(WORK_ASK_GETHIGH))
	{	//等待英伟达反馈上升高度
		if(sys_strstr((void *)pucBuff, "SetHeight:") != NULL)
		{	//执行上升高度
			mqtt_data_handler(pucBuff);
			SET_WORK_STA(WAITLIFTREACH);
		}
	}
	else if(JUDGE_WORK(WORKASKEXE))
	{	//等待英伟达执行巡检项
		if(sys_strstr((void *)pucBuff, _NET_INSPECT_EXE) != NULL)
		{
			if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '1')
			{
				SET_WORK_STA(WAITEXETIONBACK);
				gucRequeEndFlag = 1;
			}
			if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '9')
			{
				lift_high_pos(0);
				SET_WORK_STA(WORK_VIEW_FINISH);
				gucRequeEndFlag = 0;
			}
		}
	}
	else if(JUDGE_WORK(WAITEXETIONBACK))
	{	//等待英伟达执行结果
		if(sys_strstr((void *)pucBuff, _NET_INSPECT_END) != NULL)
		{
			if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '1')
				SET_WORK_STA(WORK_ASK_GETHIGH);
			else if(pucBuff[wLen - 2] == '0' && pucBuff[wLen - 1] == '9')
			{
				lift_high_pos(0);
			}
		}
	}
	return;
}

/**
 * @brief	   PUBLISH包处理
 * @retval     true | false
 */
bool publish_packet_process(st_socket_info * socket)
{
	int len;
	bool result = false;
	char * dataIndex, * datanext;
	unsigned char dataBuf[800];
	unsigned long num = 0;

	if(socket->Index == 0)
	{
		dataIndex = strstr((const char *)socket->pRecvBuf, "pub_reply");
		if(dataIndex != NULL)
		{
			if(socket->Ping)
			{
				debug_printf("receive pinqback!\r\n");
				socket->Ping = 0;
				gucMqttPinqTry = 0;
				gulCloudAliveTick = sys_get_ms();
			}
			dataIndex = NULL;
		}
		dataIndex = strstr((const char *)socket->pRecvBuf, "pub0");
		if(dataIndex != NULL)
			datanext = strstr((const char *)(dataIndex + 10), "pub0");
	}
	else
		dataIndex = strstr((const char *)socket->pRecvBuf, "message");

Look_Coop:
	if(dataIndex != NULL)
	{
		dataIndex += (socket->Index == 0) ? 3 : 7;
		result = true;
		len = get_last_len((unsigned char *)dataIndex);		//获取剩余字节的个数
		if(socket->Index == 0)
		{
			dataIndex += 14;				//获取当前序列号
			for(int i = 0; i < 4; i++)
				num = num * 16 + sys_ascii2num(dataIndex[i]);
			gusPacketID = num;

			dataIndex += 6;					//移动到实际数据的位置
		}
		else
			dataIndex += 20;	
		len -= 20;	

		if(sys_hex2ascii(dataBuf, (unsigned char *)dataIndex, len))
		{
			if(socket->Index == 0)
			{
				_cloud_rece_handler(dataBuf, socket);
				debug_printf("Cloud:%s  Recv Len:%u\r\n", dataBuf, socket->RecvLen);
				mqtt_data_handler(dataBuf);
			}
			else
			{
				len = len / 2;
				debug_printf("Nvidia:%s\r\n", dataBuf);
				_NVIDIA_rece_handler(dataBuf, len);
			}
		}
	}
	if((socket->Index == 0) && (dataIndex != NULL) && (datanext != NULL))
	{
		dataIndex = datanext;
		datanext = strstr((const char *)(dataIndex + 10), "pub0");
		goto Look_Coop;
	}

    return result;
}
/**
 * @brief	   发布释放包处理
 * @retval     true | false
 */
//static bool pubrel_packet_process(st_socket_info * socket)
//{
//    int len;
//    unsigned short id;
//    unsigned char  dup, type;
//    MQTTHeader header;
//
//    header.byte = socket->pRecvBuf[0];
//    if (!MQTTDeserialize_ack(&type, &dup, &id, socket->pSend, MQTT_SEND_SIZE))
//        return false;
//
//    if ((len = MQTTSerialize_ack(socket->pSend, MQTT_SEND_SIZE,
//              (header.byte == PUBREC) ? PUBREL : PUBCOMP, 0, id)) <= 0)
//        return false;
//
//    if (send_parket(socket, socket->pSend, len) != NET_OK)
//        return false;
//
//    return true;
//}

/**
 * @brief    重连处理
 */
//static void reconnect_process(mqtt_info_t *mi)
//{
//	st_socket_info sockettest;
//    //重连间隔表(S)
//    static const unsigned short interval_tbl[] = {1, 3, 10, 20, 60, 120, 300};
//    unsigned int interval;
//    mqtt_event_args_t args;
//
//    interval = interval_tbl[mi->reconnect_retry % ARRAY_COUNT(interval_tbl)];
//    if (net_istimeout(mi->reconnect_timer, interval * 1000)) {
//        mi->reconnect_timer = net_get_ms();
//        if (NET_OK == mqtt_client_connect(&sockettest, (char * )NULL)) {
//            args.type = MQTT_EVENT_RECONNECT;
//            args.payload_size = 0;
//            mi->reconnect_retry = 0;
//            event_invoke(mi, &args);
//        } else {
//            /* 重连异常统计 */
//            if (mi->reconnect_retry < ARRAY_COUNT(interval_tbl) - 1) {
//                mi->reconnect_retry++;
//            }
//        }
//    }
//}

/**
 * @brief	   mqtt 包解析
 * @retval     none
 */
//static void mqtt_packet_parse(st_socket_info * socket)
//{
//    MQTTHeader header;
//
//    header.byte = socket->pRecvBuf[0];
//    switch (header.bits.type) {
//    case CONNACK:
//        break;
//    case PUBACK:
//    case SUBACK:
//    case UNSUBACK:
//    case PUBCOMP:
//        break;
//
//    case PUBLISH: {
//        if (!publish_packet_process(socket)) {
//            // error
//        }
//        break;
//    }
//    case PUBREC:                           //QoS2第2个包(server->client)
//    case PUBREL: {                         //QoS2第3个包(client->server)
//        if (!pubrel_packet_process(socket)) {
//        }
//        break;
//    }
//    case PINGRESP:
//        socket->Ping = 0;         //ping 响应
//        break;
//    }
//}

/**
 * @brief	   mqtt 数据解析
 * @retval     none
 */
//static void mqtt_data_parse(mqtt_info_t *mi)
//{
//    int len, ret;
//
//    //len = socket_recv(mi->sockfd, &mi->buf[mi->recvcnt], mi->bufsize - mi->recvcnt);
//    if (0 < len) {
//        mi->recvcnt += len;
//    }
//    else if (0 < mi->recvcnt) {
//        //Packet type
//        if (mi->state == 0 && mi->recvcnt > 1) {
//            mi->recv_timer = net_get_ms();
//            mi->state ++;
//        }
//		else if (net_istimeout(mi->recv_timer, 3000)) { // 超时处理
//			mi->recvcnt = 0;
//		}
//
//        //Remaining Length
//        if (1 == mi->state) {
//            ret = parse_packet_size(mi); //解析包大小
//            if (NET_OK == ret) {
//                mi->state++;
//            }
//            else if (ret == NET_ERROR) {
//                mi->state = 0;
//            }
//        }
//
//        if (mi->state == 2 && mi->recvcnt >= mi->total) {
//            mqtt_packet_parse((st_socket_info *)&mi);
//            mi->last_recv = net_get_ms(); //记录最后一包接收时刻
//            mi->state     = 0;            //解析完一帧,回到初始状态
//            //mi->recvcnt = 0;
//            len = mi->total + 2;
//            if (128 < len) len ++;
//            if (mi->recvcnt <= len) {
//                mi->recvcnt  = 0;
//            }
//            else {
//                mi->recvcnt -= len;
//                memcpy(mi->buf, &mi->buf[len], mi->recvcnt);
//            }
//        }
//    }
//    else if (mi->state && net_istimeout(mi->recv_timer, 3000)) {  //接收超时处理
//        mi->recvcnt = 0;
//        mi->state   = 0;
//    }
//}

/**
 * @brief      创建mqtt客户端
 * @param[in]  e    - 事件处理接口
 * @param[in]  host - 主机地址(www.xxx.com)
 * @param[in]  port - 端口(一般填1883)
 * @return     NULL - 创建失败, 其它值 - mqtt客户端
 */
//mqtt_client_t *mqtt_client_create(const mqtt_config_t *config)
//{
//    mqtt_info_t *info;
//    info = (mqtt_info_t *)net_malloc(sizeof(mqtt_info_t) + config->recvbuf_size);
//    if (info == NULL)
//        return NULL;
//
//    memset(info, 0, sizeof(mqtt_info_t));
//    info->config    = *config;        //配置参数
//    info->mutex     = net_sem_new(1); //互斥量
//    info->sem_ready = net_sem_new(0); //完成量
//
//    info->sockfd    =  socket_create(NULL, 1024); //创建socket
//    if (SOCKET_INVALID == info->sockfd){
//        net_free(info);
//        return NULL;
//    }
//
//    info->bufsize = config->recvbuf_size;
//    info->recvcnt = 0;
//    return &info->client;
//}

/**
 * @brief    销毁mqtt客户端
 */
//void mqtt_client_destroy(mqtt_client_t *mc)
//{
//    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
//
//    socket_destroy(mi->sockfd);
//    net_sem_free(mi->mutex);
//    net_sem_free(mi->sem_ready);
//    net_free(mi);
//}

/**
 * @brief	   连接服务器
 * @params[in] mc      - mqtt_client
 * @return     NET_OK  -  连接成功, 其它值 - 异常
 */
int mqtt_client_connect(st_socket_info * socket, char *dev_sn)
{
    MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
    unsigned char sendbuf[128];
    unsigned char sessionPresent;
    unsigned char rc;
    int len, ret;

    //构造连接参数
    options.cleansession     = true;
    options.keepAliveInterval= MQTT_HEARTBEAT;
    options.username.cstring = ((socket->Index == 0) ? (char *)MQTT_USR_NAME : (char *)NVIDIA_USR_NAME);
    options.password.cstring = ((socket->Index == 0) ? (char *)MQTT_USR_PWD : (char *)NVIDIA_USR_PWD);
    options.clientID.cstring = (char *)dev_sn;
    options.willFlag         = false;		//不使用遗嘱消息，避免误操作
//    options.will.qos         = QOS1;
//    options.will.retained    = false;
//    options.will.topicName.cstring = NULL;
//    options.will.message.cstring = NULL;

    len = MQTTSerialize_connect(sendbuf, sizeof(sendbuf), &options);
    if (0 >= len) {
        return NET_INVALID;
    }

    /* 发送连接包*/
    ret = send_parket(socket, sendbuf, len);
    if (NET_OK != ret) {
        return NET_ERROR;
    }
	while(!net_istimeout(socket->SendTick, MQTT_CONN_TIMEOUT * 1000))
	{
		if(socket->RecvSta == 1)
		{
			if (MQTTDeserialize_connack(&sessionPresent, &rc, socket->pRecvBuf, socket->RecvLen) == 1 && rc == 0)
			{
				socket->RecvSta = 0;
				return NET_OK;
			}
			else
			{
				socket->RecvSta = 0;
				return NET_FAILED;
			}
		}

		sys_delay_ms(10);
	}

	return NET_TIMEOUT;
}

/**
 * @brief	   发布消息
 * @params[in] mc    - mqtt_client
 * @params[in] topic - 主题
 * @params[in] payload   - 数据
 * @params[in] payload_size   - 数据长度
 * @params[in] qos   - 消息质量
 * @retval     NET_OK  -  发布成功, 其它值 - 异常
 */
int mqtt_client_publish(st_socket_info * socket, const char *topic,
                        void *payload, int payload_size, mqtt_qos qos)
{
    MQTTString topic_name = MQTTString_initializer;
    unsigned char *sendbuf; //发送缓冲区
    int send_bufsize = payload_size + 128;
    int len, id, ret = NET_ERROR;

    topic_name.cstring = (char *)topic;

    sendbuf = net_malloc(send_bufsize);
    if (NULL == sendbuf) {
        return NET_NOMEM;
    }
		memset(sendbuf, 0, send_bufsize);

    id = getNextPacketId(socket); //获取新包号
    len = MQTTSerialize_publish(sendbuf, send_bufsize, 0,  qos, 0, id,
            topic_name, (unsigned char *)payload, payload_size);
    if (0 >= len) {
        goto exit;
    }

//	if((socket->Index == 0) && gucPrintFlag)
//		debug_printf(&sendbuf[4]);
    //更新发送状态
    ret = send_parket(socket, sendbuf, len);
    if (NET_OK != ret || QOS0 == qos) {
        goto exit;
    }

	//QOS1, QOS2
    /* 等待发送确认*/
	while(!net_istimeout(socket->SendTick, MQTT_SEND_TIMEOUT * 1000))
	{
		if(socket->RecvSta == 1)
		{
			unsigned short pid;
	        unsigned char dup, type;
			if (MQTTDeserialize_ack(&type, &dup, &pid, socket->pRecvBuf, socket->RecvLen) == 1)
				ret = NET_OK;
			goto exit;
		}

		sys_delay_ms(10);
	}

exit:
	socket->SendSta = 0;
    net_free(sendbuf);
    return ret;
}


/**
 * @brief	   订阅主题
 * @params[in] mc    - mqtt_client
 * @params[in] topic - 主题
 * @params[in] qos   - 服务质量
 * @params[out] grantedQoS   -  服务器保证质量
 * @retval     NET_OK  -  订阅成功, 其它值 - 异常
 */
int mqtt_client_subscribe(st_socket_info * socket, const char *topic, mqtt_qos qos, mqtt_qos *grantedQoS)
{
    MQTTString topic_name = MQTTString_initializer;
    unsigned char sendbuf[128];
    int len, ret = NET_REJECT;

    topic_name.cstring = (char *)topic;

    len = MQTTSerialize_subscribe(sendbuf, sizeof(sendbuf), 0, getNextPacketId(socket), 1, &topic_name, (int *)&qos);
    if (0 >= len) {
        goto exit;
    }

    //更新发送状态
    ret = send_parket(socket, sendbuf, len);
    if (NET_OK != ret) {
        goto exit;
    }

	ret = NET_ERROR;

	while(!net_istimeout(socket->SendTick, MQTT_SEND_TIMEOUT * 1000))
	{
		if(socket->RecvSta == 1)
		{
			int count = 0;
        	unsigned short id;
			if (MQTTDeserialize_suback(&id, 1, &count, (int *)grantedQoS, socket->pRecvBuf, socket->RecvLen) == 1)
			{
				if (SUBFAIL != *grantedQoS)
		            ret = NET_OK;
			}
			goto exit;
		}

		sys_delay_ms(10);
	}

exit:
	socket->SendSta = 0;
	socket->RecvSta = 0;
    return ret;
}

/**
 * @brief	   解除主题订阅
 * @params[in] mc     - mqtt_client
 * @params[in] topic  - 主题
 * @retval     NET_OK -  操作成功, 其它值 - 异常
 */
//int mqtt_client_unsubscribe(st_socket_info * socket, const char *topic)
//{
//    MQTTString topic_name = MQTTString_initializer;
//    unsigned char sendbuf[128];
//    int len, ret = NET_REJECT;
//
//    topic_name.cstring = (char *)topic;
//
//    len = MQTTSerialize_unsubscribe(sendbuf, sizeof(sendbuf), 0, getNextPacketId(socket), 1, &topic_name);
//    if (len <= 0)
//        goto exit;
//
//    ret = send_parket(socket, sendbuf, len);
//    if (ret != NET_OK)
//        goto exit;
//
//	while(!net_istimeout(socket->SendTick, MQTT_SEND_TIMEOUT * 1000))
//	{
//		if(socket->RecvSta == 1)
//		{
//        	unsigned short id;
//			if (MQTTDeserialize_unsuback(&id, socket->pRecvBuf, socket->RecvLen) == 1)
//				ret = NET_OK;
//			goto exit;
//		}
//		sys_delay_ms(10);
//	}
//
//exit:
//	socket->SendSta = 0;
//	socket->RecvSta = 0;
//    return ret;
//}

/**
 * @brief	   MQTT任务处理程序, 管理心跳发送及重连
 * @params[in] mc - mqtt_client
 * @note       该函数不允许与mqtt_client_recv放到同一个任务或者/线程中进行轮询
 * @return     none
 */
//void mqtt_client_process(st_socket_info * socket)
//{
//    mqtt_event_args_t args;
//    mqtt_info_t *mi;
//
//    if (NULL != socket) {
//        keepalive(socket);
//        reconnect_process(mi);
//        if (is_connected(mi) && !socket_online(mi->sockfd)) {
//            mi->connected = false;
//            args.payload_size = 0;
//            args.type = MQTT_EVENT_OFFLINE;
//            event_invoke(mi, &args);
//        }
//    }
//}

/**
 * @brief	   MQTT数据接收处理程序
 * @params[in] mc - mqtt_client
 * @note       该函数应单独建立一个任务进行轮询调用
 * @return     none
 */
//void mqtt_client_recv(mqtt_client_t *mc)
//{
//    if (NULL != mc) {
//        mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
//
//        mqtt_data_parse(mi);
//
//        /**
//         * @breif 网络中断且有数据请求
//         */
//        if (is_connected(mi) && !socket_online(mi->sockfd)) {
//            if (mi->conn_state == MQTT_REQ_BUSY || mi->send_state == MQTT_REQ_BUSY) {
//                net_sem_post(mi->sem_ready);
//            }
//        }
//    }
//}
