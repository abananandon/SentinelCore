#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "modules/device_monitor.h"
#include "modules/light_sensor.h"
#include "modules/mqtt_client.h"

#define my_BrokerAddress "tcp://47.97.69.180:1883"
#define my_ClienID "ATK-IMX6U-01"
#define my_reconnectDelaySec 5
#define my_keepAliveInterval 60
#define my_maxReconnectAttempts 99

// 定义全局MQTT客户端上下文
static mqttClientContext_t g_mqttContex;
static mqttClientConfig_t g_mqttConfig;
bool g_exitFlag = false; // 全局退出标志，所有线程共享

// 定义线程ID变量
pthread_t deviceStatusThreadID;
pthread_t lightSensorThreadID;

/* 回调函数 */
void mqttCommandHandle(const char *topic, const char *payload, int payloadLen,
                       void *userData) {
  fprintf(stdout, "Receice command.\n");
}

void mqttConnectionStatusHandle(bool isConnected, void *userData) {
  if (isConnected) {
    fprintf(stdout, "Client is connected.\n");
  } else {
    fprintf(stdout, "Connect error.\n");
  }
}

/* 子线程函数 */
// 信号处理函数
void signalHandle(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    g_exitFlag = true;
  }
}

// 设备状态采集和发送线程
void *deviceStatusThreadFunc(void *arg) {
  while (!g_exitFlag) {
    usleep(10000);

    // 检查MQTT是否连接
    if (!g_mqttContex.isConnected) {
      fprintf(stderr, "MQTT Client does not connected.\n");
      continue;
    }

    // 开始采集设备状态
    float cpuTemp = getCpuTemperature();
    float memUsage = getMemUsage();
    double cpuLoad = getCpuLoad();

    // 设置消息载荷
    char deviceStatusPaylod[256];
    snprintf(deviceStatusPaylod, sizeof(deviceStatusPaylod),
             "{\"timestamp_ms\": %ld,\"cup_load\": %lf,\"cpu_temp_c\": "
             "%f,\"mem_usage_percent\": %f}",
             (long)time(NULL), cpuTemp, cpuLoad, memUsage);

    // 设置消息话题
    char deviceStatusTopic[256];
    snprintf(deviceStatusTopic, sizeof(deviceStatusTopic), "sentinel/%s/status",
             g_mqttConfig.clientID);

    // 发布消息
    int rc =
        mqttClient_Publish(&g_mqttContex, deviceStatusTopic, deviceStatusPaylod,
                           strlen(deviceStatusPaylod), 0, true);
    if (rc != 0) {
      fprintf(stderr, "Failed publish device status.\n");
    }
  }

  return NULL;
}

// 环境光传感器数据发布线程
void *lightSensorThreadFunc(void *arg) {
  char *sensorType = "light_sensor";

  while (!g_exitFlag) {
    usleep(5000);

    // 检查MQTT是否连接
    if (!g_mqttContex.isConnected) {
      fprintf(stderr, "MQTT Client does not connected.\n");
      continue;
    }

    // 采集数据
    int als = getAlsData();
    int ps = getPsData();
    int ir = getIrData();

    // 构建payload
    char lightSensorPayload[256];
    snprintf(lightSensorPayload, sizeof(lightSensorPayload),
             "{\"timestamp_ms\": %ld,\"light_lux\": %d,\"infrared_cd\": %d, "
             "\"sensor_id\": %s}",
             (long)time(NULL), als, ir, sensorType);

    // 构建topic
    char lightSensorTopic[256];
    snprintf(lightSensorTopic, sizeof(lightSensorTopic), "sentinel/%s/light",
             g_mqttConfig.clientID);

    int rc =
        mqttClient_Publish(&g_mqttContex, lightSensorTopic, lightSensorPayload,
                           strlen(lightSensorPayload), 0, true);
    if (rc != 0) {
      fprintf(stderr, "Fialed to publish light senser data.\n");
    }

    return NULL;
  }
}

int main(int argc, char *argv[]) {
  // 初始化MQTT客户端
  g_mqttConfig.brokerAddress = my_BrokerAddress;
  g_mqttConfig.clientID = my_ClienID;
  g_mqttConfig.keepAliveInterval = my_keepAliveInterval;
  g_mqttConfig.reconnectDelaySec = my_reconnectDelaySec;
  g_mqttConfig.maxReconnectAttempts = my_maxReconnectAttempts;

  if (mqttClient_Init(&g_mqttContex, &g_mqttConfig) != 0) {
    fprintf(stderr, "Client initial failed.\n");
    return EXIT_FAILURE;
  }

  // 设置遗嘱消息
  char lwtTopic[256];
  char lwtPayload[256];
  snprintf(lwtTopic, sizeof(lwtTopic), "sentinel/%s/online",
           g_mqttConfig.clientID);
  snprintf(lwtPayload, sizeof(lwtPayload),
           "\"status\": \"online\",\"timestamp_ms\": %ld",
           (long)time(NULL) * 1000);
  mqttClient_SetLWT(&g_mqttContex, lwtTopic, lwtPayload, 1);

  // 注册回调函数
  mqttClient_RegisterCommandCallback(&g_mqttContex, mqttCommandHandle, NULL);
  mqttClient_RegisterConnectionStatusCallback(&g_mqttContex,
                                              mqttConnectionStatusHandle, NULL);

  // 启动客户端
  mqttClient_Start(&g_mqttContex);

  // 设置信号处理，用于退出
  signal(SIGINT, signalHandle);
  signal(SIGTERM, signalHandle);

  /* 创建并启动应用层工作线程 */
  if (pthread_create(&deviceStatusThreadID, NULL, deviceStatusThreadFunc,
                     NULL) != 0) {
    fprintf(stderr, "Creat device ststus thread failed.\n");
    mqttClient_Stop(&g_mqttContex);
    return EXIT_FAILURE;
  }

  if (pthread_create(&lightSensorThreadID, NULL, lightSensorThreadFunc, NULL) !=
      0) {
    fprintf(stderr, "Creat light sensor thread failed.\n");
    mqttClient_Stop(&g_mqttContex);
    return EXIT_FAILURE;
  }

  /* 主线程进入等待状态，直到收到退出信号 */
  while (!g_exitFlag) {
    sleep(1);
  }

  return EXIT_SUCCESS;
}
