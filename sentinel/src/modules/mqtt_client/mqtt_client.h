#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdbool.h> // for size_t
#include <stddef.h>  // for bool

#include "MQTTClient.h"

/* 预定义日志级别回调函数 */
typedef void (*loggerCallback)(int level, const char *format, ...);

/* 回调函数类型定义 */
/*
 * @brief 收到控制命令消息时的回调
 *
 * @param topic: 消息来源的Topic（已分配内存，回调结束后需要释放）
 *        payload: 消息载荷（已分配内存，回调结束后需要释放）
 *        payloadLen: 载荷长度
 * */
typedef void (*mqttOnCommandCallback_t)(const char *topic, const char *payload,
                                        int payloadLen, void *userData);
/*
 * @brief MQTT连接变化时的回调
 *
 * @param isConnected: 当前连接状态
 *        userData: 用户数据
 * */
typedef void (*mqttOnConnectionStatusCallback_t)(bool isConnected,
                                                 void *userData);

/* MQTT Client config structure */
typedef struct {
  char *brokerAddress;      // Broker 地址 (e.g. "tcp://124.66.66.66:1883")
  char *clientID;           // 客户端ID（唯一标识）
  char *userName;           // 用户名（用于Broker认证）
  char *password;           // 密码
  int keepAliveInterval;    // Keep-alive 心跳间隔（秒）
  int reconnectDelaySec;    // 自动重连间隔起点（秒）
  int maxReconnectAttempts; // 最大尝试重连次数（0表示无限次）
  bool
      cleanSession; // 清理会话（true：每次连接都创建新会话，不保留订阅和离线消息）
} mqttClientConfig_t;

/* MQTT Client context structure */
typedef struct {
  MQTTClient client;         // Paho MQTT 客户端句柄
  mqttClientConfig_t config; // MQTT 配置

  // 线程同步机制
  pthread_mutex_t lock;     // 保持客户端状态和发送队列的互斥锁
  pthread_cond_t cond;      // 条件变量（实现发送队列的非阻塞等待）
  bool isConnected;         // 当前连接状态
  volatile bool shouldExit; // 模块退出标志

  // 注册的回调函数和用户数据
  mqttOnCommandCallback_t onCommandCb;
  void *onCommandUserData;
  mqttOnConnectionStatusCallback_t onConnStatusCb;
  void *onConnStatusUserData;

  // 日志回调函数
  loggerCallback loggerCb;
  void *loggerUserData;

  // 遗嘱消息 (Last Will and Testament) 配置
  char *lwtTopic;
  char *lwtPayload;
  int lwtQos;
} mqttClientContext_t;

int mqttClient_Init(mqttClientContext_t *ctx, const mqttClientConfig_t *config);

void mqttClient_RegisterCommandCallback(mqttClientContext_t *ctx,
                                        mqttOnCommandCallback_t callback,
                                        void *userData);

void mqttClient_RegisterConnectionStatusCallback(
    mqttClientContext_t *ctx, mqttOnConnectionStatusCallback_t callback,
    void *userData);
void mqttClient_SetLWT(mqttClientContext_t *ctx, const char *topic,
                       const char *payload, int qos);

void mqttClient_Start(mqttClientContext_t *ctx);
void mqttClient_Stop(mqttClientContext_t *ctx);
int mqttClient_Publish(mqttClientContext_t *ctx, const char *topic,
                       const char *payload, int payloadLen, int qos,
                       bool retained);

int mqttClient_Subscribe(mqttClientContext_t *ctx, const char *topic, int qos);
#endif // !_MQTT_CLIENT_H
