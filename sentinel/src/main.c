#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 第三方库文件
#include "cJSON/cJSON.h"

// 自定义模块头文件
#include "modules/device_monitor.h"
#include "modules/light_sensor.h"
#include "modules/mqtt_client.h"

// MQTT客户端设置
char *my_BrokerAddress = NULL;
char *my_ClienID = NULL;
char *my_Username = NULL;
char *my_Password = NULL;
int my_reconnectDelaySec = 5;
int my_keepAliveInterval = 60;
int my_maxReconnectAttempts = 99;

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
    sleep(1);
    // usleep(1000 * 10); // 数据刷新率：100Hz

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
             "{\"timestamp_ms\": %ld,\"cpu_temp_c\": %lf,\"cpu_load\": "
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

/* 环境光传感器数据发布线程 */
void *lightSensorThreadFunc(void *arg) {
  char *sensorType = "light_sensor";

  while (!g_exitFlag) {
    sleep(1);
    // usleep(1000 * 1000);

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
  }
  return NULL;
}

/*
 * @brief:  从.json文件读取内容并返回
 *
 * @param:  filename: 文件的储存路径字符串
 *
 * @return: char *: 字符串，存储.json文件内容
 * */
char *readFileToString(const char *filename) {
  FILE *fp = NULL;
  long fileSize = 0;
  char *buffer = NULL;
  size_t readLen = 0;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error: Could not open file %s\n", filename);
    return NULL;
  }

  // 获取文件大小
  fseek(fp, 0, SEEK_END); // 重置光标到文件结束位置
  fileSize = ftell(fp);   // 获取当前光标位置
  rewind(fp);             // 重置光标到文家开头

  // 分配内存
  buffer = (char *)malloc(sizeof(char) * (fileSize + 1)); // +1 for "\0"
  if (buffer == NULL) {
    fprintf(stderr, "Error: Memoy allocation failed for file buffer. \n");
    fclose(fp);
    return NULL;
  }

  // 读取文件内容
  readLen = fread(buffer, 1, fileSize, fp);
  if (readLen != fileSize) {
    fprintf(
        stderr,
        "Error: Failed to read entire file %s. Read %zu bytes, expected %ld.\n",
        filename, readLen, fileSize);
    free(buffer);
    fclose(fp);
    return NULL;
  }

  buffer[fileSize] = '\0';
  fclose(fp);
  return buffer;
}

int main(int argc, char *argv[]) {
  // 打开json文件
  char *config_JsonString = readFileToString("./config/sentinel_config.json");
  if (config_JsonString == NULL) {
    fprintf(stderr, "Error: Read file failed,\n");
    return EXIT_FAILURE;
  }

  // 解析JSON字符串
  cJSON *config_Root = cJSON_Parse(config_JsonString);
  if (config_Root == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    free(config_JsonString);
    return EXIT_FAILURE;
  }

  // 获取JSON数据
  // 访问嵌套对象
  cJSON *config_mqttClient =
      cJSON_GetObjectItemCaseSensitive(config_Root, "mqttClientConfig");
  if (config_mqttClient == NULL || !cJSON_IsObject(config_mqttClient)) {
    fprintf(stderr, "Error: 'mqttClientConfig' object not found or not an "
                    "object in JSON.\n");
    cJSON_Delete(config_Root);
    free(config_JsonString);
    return EXIT_FAILURE;
  }
  cJSON *item = NULL;
  item = cJSON_GetObjectItemCaseSensitive(config_mqttClient, "brokerAddress");
  if (item && cJSON_IsString(item)) {
    my_BrokerAddress = strdup(item->valuestring);
  } else {
    fprintf(
        stderr,
        "Warning: 'brokerAddress' not found or not a string. Using empty.\n");
  }

  item = cJSON_GetObjectItemCaseSensitive(config_mqttClient, "clientID");
  if (item && cJSON_IsString(item)) {
    my_ClienID = strdup(item->valuestring);
  } else {
    fprintf(stderr, "Warning: 'clientID' not found or not a string. Using "
                    "default/empty.\n");
  }

  item = cJSON_GetObjectItemCaseSensitive(config_mqttClient, "username");
  if (item && cJSON_IsString(item)) {
    my_Username = strdup(item->valuestring);
  } else {
    fprintf(stderr, "Warning: 'username' not found or not a string. Using "
                    "default/empty.\n");
  }

  item = cJSON_GetObjectItemCaseSensitive(config_mqttClient, "password");
  if (item && cJSON_IsString(item)) {
    my_Password = strdup(item->valuestring);
  } else {
    fprintf(stderr, "Warning: 'password' not found or not a string. Using "
                    "default/empty.\n");
  }

  item =
      cJSON_GetObjectItemCaseSensitive(config_mqttClient, "keepAliveInterval");
  if (item && cJSON_IsNumber(item)) {
    my_keepAliveInterval = item->valueint;
  } else {
    fprintf(stderr, "Warning: 'keepAliveInterval' not found or not a number. "
                    "Using default/0.\n");
  }

  item =
      cJSON_GetObjectItemCaseSensitive(config_mqttClient, "reconnectDelaySec");
  if (item && cJSON_IsNumber(item)) {
    my_reconnectDelaySec = item->valueint;
  } else {
    fprintf(stderr, "Warning: 'reconnectDelaySec' not found or not a number. "
                    "Using default/0.\n");
  }

  item = cJSON_GetObjectItemCaseSensitive(config_mqttClient,
                                          "maxReconnectAttempts");
  if (item && cJSON_IsNumber(item)) {
    my_maxReconnectAttempts = item->valueint;
  } else {
    fprintf(stderr, "Warning: 'maxReconnectAttempts' not found or not a "
                    "number. Using default/0.\n");
  }

  // 清理资源：释放cJSON对象和从文件读取的字符串
  cJSON_Delete(config_Root);
  free(config_JsonString);

  // 初始化MQTT客户端
  g_mqttConfig.brokerAddress = my_BrokerAddress;
  g_mqttConfig.clientID = my_ClienID;
  g_mqttConfig.userName = my_Username;
  g_mqttConfig.password = my_Password;
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
