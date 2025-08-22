#include "modules/mqtt_client.h"
#include <MQTTClient.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* 回调函数实现 */

/*
 * @brief 当连接丢失时被paho库调用
 *
 * @param context: 客户端上下文
 *        cause：原因
 * */
void paho_conn_lost(void *context, char *cause) {
  mqttClientContext_t *ctx = (mqttClientContext_t *)context;
  pthread_mutex_lock(&ctx->lock);
  ctx->isConnected = false;
  // log日志

  if (ctx->onConnStatusCb) {
    ctx->onConnStatusCb(false, ctx->onConnStatusUserData);
  }
  pthread_mutex_unlock(&ctx->lock);
}

/*
 * @brief 当收到消息时被paho库调用
 *
 * @param context: 客户端上下文
 *        topicName: 话题名
 *        topicLen: 话题长度
 *        message: 客户端消息结构体
 * */
int paho_msg_arrived(void *context, char *topicName, int topicLen,
                     MQTTClient_message *message) {
  mqttClientContext_t *ctx = (mqttClientContext_t *)context;
  // log日志

  if (ctx->onCommandCb) {
    // 复制topic和payload，因为paho提供的指针生命周期只在回调函数内
    char *topicCopy = strndup(topicName, topicLen);
    char *payloadCopy = (char *)malloc(message->payloadlen + 1);
    if (payloadCopy) {
      memcpy(payloadCopy, message->payload, message->payloadlen);
      payloadCopy[message->payloadlen] = '\0';
      ctx->onCommandCb(topicCopy, payloadCopy, message->payloadlen,
                       ctx->onCommandUserData);
      free(payloadCopy);
    } else {
    }
    free(topicCopy);
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);
  return 1;
}

/*
 * @brief 消息发送完毕时被paho库调用
 *
 * @param
 * */
void paho_delivery_complete(void *context, MQTTClient_deliveryToken dt) {
  mqttClientContext_t *ctx = (mqttClientContext_t *)context;
  // log日志
}

/* 内部辅助函数 */
/*
 * @brief 尝试连接MQTT Broker
 *
 * @param
 * */
static int connectToBroker(mqttClientContext_t *ctx) {
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  int rc;

  conn_opts.cleansession = ctx->config.cleanSession;
  conn_opts.keepAliveInterval = ctx->config.keepAliveInterval;

  // 存在用户名和密码
  if (ctx->config.userName && ctx->config.password) {
    conn_opts.username = ctx->config.userName;
    conn_opts.password = ctx->config.password;
  }

  // 配置遗嘱消息（LWT）
  if (ctx->lwtTopic && ctx->lwtPayload) {
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    will_opts.topicName = ctx->lwtTopic;
    will_opts.payload.data = ctx->lwtPayload;
    will_opts.payload.len = strlen(ctx->lwtPayload);
    will_opts.qos = ctx->lwtQos;
    will_opts.retained = true; // 遗嘱消息通常保留，以便于订阅者立即获得离线状态
    conn_opts.will = &will_opts;
  }

  // log日志

  // 连接Broker
  pthread_mutex_lock(&ctx->lock); // 保护客户端操作
  rc = MQTTClient_connect(ctx->client, &conn_opts);
  if (rc != MQTTCLIENT_SUCCESS) {
    pthread_mutex_unlock(&ctx->lock);
    return -1;
  }

  ctx->isConnected = true;
  pthread_mutex_unlock(&ctx->lock);

  // 连接成功后，发布上线消息（如果配置了LWT，通常LWT的topic就是online topic）
  // 确保与LWT topic一直，并带上Reatain标志，让所有订阅者知道设备上线了
  // 注意：LWT topic 通常是 "sentinel/{device_id}/online"
  if (ctx->lwtTopic) {
    // 构建上线消息payload
    char onlinePayload[128];
    snprintf(onlinePayload, sizeof(onlinePayload),
             "{\"status\":\"online\",\"timestamp_ms\":%ld}",
             (long)time(NULL) * 1000);

    // 使用publish函数，Qos 1，Ratain为true
    // 注意：这里要确保LWT的topic和online status topic
    // 一致，否则需要单独的lwt_topic_online
    mqttClient_Publish(ctx, ctx->lwtTopic, onlinePayload, strlen(onlinePayload),
                       1, true);
    // log日志
  }

  // 连接成功后订阅命令topic: app/{app_id}/control
  char controlTopic[256];
  snprintf(controlTopic, sizeof(controlTopic), "app/%s/control",
           ctx->config.clientID);

  rc = mqttClient_Subscribe(ctx, controlTopic, 1);
  if (rc != 0) {
    fprintf(stderr, "Faild to subscribe to control topic.\n");
  }

  // 通知上层模块连接成功
  if (ctx->onConnStatusCb) {
    ctx->onConnStatusCb(true, ctx->onConnStatusUserData);
  }

  return 0;
}

/*
 * @brief 自动重联线程函数
 *
 *
 * */
static void *reConnectThreadFunc(void *arg) {
  mqttClientContext_t *ctx = (mqttClientContext_t *)arg;
  int reconnectAttempts = 0;
  int currentDelay = ctx->config.reconnectDelaySec;

  while (!ctx->shouldExit) {
    pthread_mutex_lock(&ctx->lock);
    bool connected = ctx->isConnected;
    pthread_mutex_unlock(&ctx->lock);

    if (connected) {
      // 如果已经连接，定期调用paho的yield来处理网络IO和keep-alive
      MQTTClient_yield();    // yield 1s
      reconnectAttempts = 0; // 重置重联次数
      currentDelay = ctx->config.reconnectDelaySec;
      usleep(100 * 100); // 避免空转，短时间休眠
      continue;
    }

    if (ctx->config.maxReconnectAttempts > 0 &&
        reconnectAttempts >= ctx->config.maxReconnectAttempts) {
      fprintf(stderr, "Exceeded max MQTT reconnect attempts (%d). Aborting.\n",
              ctx->config.maxReconnectAttempts);
      exit(EXIT_FAILURE);
    }

    sleep(currentDelay);

    if (connectToBroker(ctx) == 0) {
      reconnectAttempts = 0;
      currentDelay = ctx->config.reconnectDelaySec;
    } else {
      reconnectAttempts++;
      // 指数退避，但限制最大重联间隔
      currentDelay = currentDelay * 2 > 300 ? 300 : currentDelay * 2;
    }
  }
  return NULL;
}

/* 公共API实现 */
/* @brief 初始化MQTT客户端上下文和配置，创建客户端实例
 *
 * @param ctx: 指向MQTT客户端上下文的结构体指针
 *        config: 指向MQTT客户端配置结构体的指针
 *
 * @return 0 成功
 * */
int mqttClient_Init(mqttClientContext_t *ctx,
                    const mqttClientConfig_t *config) {
  if (!ctx || !config) {
    return -1;
  }

  memset(ctx, 0, sizeof(mqttClientContext_t));

  // 复制配置信息
  ctx->config.brokerAddress = strdup(config->brokerAddress);
  ctx->config.clientID = strdup(config->clientID);
  if (config->userName)
    ctx->config.userName = strdup(config->userName);
  if (config->password)
    ctx->config.password = strdup(config->password);
  ctx->config.keepAliveInterval = config->keepAliveInterval;
  ctx->config.reconnectDelaySec = config->reconnectDelaySec;
  ctx->config.maxReconnectAttempts = config->maxReconnectAttempts;
  ctx->config.cleanSession = config->cleanSession;

  if (!ctx->config.brokerAddress || !ctx->config.clientID ||
      (ctx->config.userName && !ctx->config.password) ||
      (!ctx->config.userName && ctx->config.password)) {
    // log日志:初始化失败
    free(ctx->config.brokerAddress);
    free(ctx->config.clientID);
    free(ctx->config.userName);
    free(ctx->config.password);
    return -1;
  }

  // 初始化MQTT客户端实例
  int rc = MQTTClient_create(&ctx->client, ctx->config.brokerAddress,
                             ctx->config.clientID, MQTTCLIENT_PERSISTENCE_NONE,
                             NULL);
  if (rc != MQTTCLIENT_SUCCESS) {
    // log日志：创建客户端实例失败
    return -1;
  }

  // 设置回调函数
  MQTTClient_setCallbacks(ctx->client, ctx, paho_conn_lost, paho_msg_arrived,
                          paho_delivery_complete);

  // 初始化互斥锁
  pthread_mutex_init(&ctx->lock, NULL);
  pthread_cond_init(&ctx->cond, NULL);

  return 0;
}

/*
 * @brief 注册受到控制命令消息的回调函数
 * */
void mqttClient_RegisterCommandCallback(mqttClientContext_t *ctx,
                                        mqttOnCommandCallback_t callback,
                                        void *userData) {
  if (ctx) {
    ctx->onCommandCb = callback;
    ctx->onCommandUserData = userData;
  }
}

/*
 * @brief 注册连接状态变化的回调函数
 * */
void mqttClient_RegisterConnectionStatusCallback(
    mqttClientContext_t *ctx, mqttOnConnectionStatusCallback_t callback,
    void *userData) {
  if (ctx) {
    ctx->onConnStatusCb = callback;
    ctx->onConnStatusUserData = userData;
  }
}

/*
 * @brief 设置遗嘱消息（LWT）
 *
 * @param ctx: MQTT客户端上下文指针
 *        topic: 遗嘱消息的topic
 *        payload: 遗嘱消息的payload
 *        qos: 遗嘱消息的qos
 * */
void mqttClient_SetLWT(mqttClientContext_t *ctx, const char *topic,
                       const char *payload, int qos) {
  if (ctx) {
    free(ctx->lwtTopic);
    free(ctx->lwtPayload); // 释放旧的
    ctx->lwtTopic = topic;
    ctx->lwtPayload = payload;
    ctx->lwtQos = qos;

    if (!ctx->lwtTopic || !ctx->lwtPayload) {
      // 设置失败
      fprintf(stderr, "Fail to set LWT.\n");
    }
  }
}

/*  */
static pthread_t reconnectThreadID;

/*
 * @brief 启动MQTT客户端的连接和后台处理线程。
 *        这是一个阻塞函数，应在单独的线程中调用。
 *
 * @param ctx: MQTT客户端上下文指针
 * */
void mqttClient_Start(mqttClientContext_t *ctx) {
  if (!ctx) {
    return;
  }

  ctx->shouldExit = false;

  // 启动一个独立的线程用来处理连接和重联逻辑
  if (pthread_create(&reconnectThreadID, NULL, reConnectThreadFunc, ctx) != 0) {
    fprintf(stderr, "Fail to create MQTT reconnect thread \n");
    exit(EXIT_FAILURE);
  }
}

/*
 * @brief 停止MQTT客户端并清理资源
 *
 * @param ctx: MQTT客户端上下文指针
 * */
void mqttClient_Stop(mqttClientContext_t *ctx) {
  if (!ctx) {
    return;
  }

  ctx->shouldExit = true;
  // 等待后台线程结束
  if (reconnectThreadID != 0) {
    pthread_join(reconnectThreadID, NULL);
    reconnectThreadID = 0; // 重置
  }

  pthread_mutex_lock(&ctx->lock);
  if (ctx->isConnected) {
    MQTTClient_disconnect(ctx->client, 1000);
    ctx->isConnected = false;
    if (ctx->onConnStatusCb) {
      ctx->onConnStatusCb(false, ctx->onConnStatusUserData);
    }
  }
  pthread_mutex_unlock(&ctx->lock);

  // 清理客户端资源
  MQTTClient_destroy(&ctx->client);

  free(ctx->config.brokerAddress);
  free(ctx->config.clientID);
  free(ctx->config.userName);
  free(ctx->config.password);
  free(ctx->lwtPayload);
  free(ctx->lwtTopic);

  // 摧毁互斥锁和条件变量
  pthread_mutex_destroy(&ctx->lock);
  pthread_cond_destroy(&ctx->cond);
}

/*
 * @brief MQTT客户端发送消息
 *
 * @param ctx: 指向MQTT客户端上下文的结构体指针
 *        topic: 消息topic
 *        payload: 消息载荷
 *        payloadLen: 载荷长度
 *        qos: QoS级别
 *        retained: 是否保留消息标志
 *
 * @return 0 成功
 * */
int mqttClient_Publish(mqttClientContext_t *ctx, const char *topic,
                       const char *payload, int payloadLen, int qos,
                       bool retained) {
  if (!ctx || !topic || !payload) {
    return -1;
  }

  // 保护客户端操作
  pthread_mutex_lock(&ctx->lock);
  if (!ctx->isConnected) {
    pthread_mutex_unlock(&ctx->lock);
    // log日志
    return -1;
  }

  // 构建信息
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = (void *)payload;
  pubmsg.payloadlen = payloadLen;
  pubmsg.qos = qos;
  pubmsg.retained = retained;
  MQTTClient_deliveryToken token;

  // 发送信息
  int rc = MQTTClient_publishMessage(ctx->client, topic, &pubmsg, &token);
  pthread_mutex_unlock(&ctx->lock);

  if (rc != MQTTCLIENT_SUCCESS) {
    // log日志
    return -1;
  }

  // log日志
  return 0;
}

/*
 * @brief 订阅MQTT topic
 *
 * @param ctx: MQTT客户端上下文指针
 *        topic: 要订阅的话题Topic
 *        qos: 订阅的QOS级别
 *
 * @return 0 成功
 * */
int mqttClient_Subscribe(mqttClientContext_t *ctx, const char *topic, int qos) {
  if (!ctx || !topic) {
    return -1;
  }

  pthread_mutex_lock(&ctx->lock);
  if (!ctx->isConnected) {
    pthread_mutex_unlock(&ctx->lock);
    return -1;
  }

  int rc = MQTTClient_subscribe(ctx->client, topic, qos);
  pthread_mutex_unlock(&ctx->lock);

  if (rc != MQTTCLIENT_SUCCESS) {
    return -1;
  }

  return 0;
}
