#include "modules/device_monitor.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * @brief Get the CPU temperature,
 *
 * @return float: The CPU temperature ('C)
 * */
float getCpuTemperature(void) {
  const char *temp_file = CPU_TEMP_FILE;
  FILE *fp;
  int temp_raw;
  float temp_current;

  // open the cpu temperature data file
  fp = fopen(temp_file, "r");
  if (fp == NULL) {
    perror("Error opening temperature file");
    return -1;
  }

  // get the data
  if (fscanf(fp, "%d", &temp_raw) != 1) {
    fprintf(stderr, "Error reading temperature value.\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  temp_current = (float)temp_raw / 1000.0;
  return temp_current;
}

/*
 * brief Opending the CPU time file and read data
 *
 * param times: The pointer of the CpuTimes structure, which is used to store
 * CPU time slice data
 *
 * */
void readCpuTimes(CpuTimes *times) {
  FILE *fp = fopen(CPU_TIME_FILE, "r");
  if (fp == NULL) {
    perror("Error opening CPU_TIME_FILE");
    memset(times, 0, sizeof(CpuTimes));
    return;
  }

  char line[256];
  // Read first line
  if (fgets(line, sizeof(line), fp) == NULL) {
    fprintf(stderr, "Error reading from CPU_TIME_FILE\n");
    memset(times, 0, sizeof(CpuTimes));
    fclose(fp);
    return;
  }

  sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &times->user,
         &times->nice, &times->system, &times->idle, &times->iowait,
         &times->irq, &times->softirq, &times->steal);

  times->total = times->user + times->nice + times->system + times->idle +
                 times->iowait + times->irq + times->softirq + times->steal;

  fclose(fp);
}

/*
 * brief Read the CPU time slice data twice in successtion, calculate the
 * difference to obtian the CPU load during that period of time.
 *
 * return double: The CPU load(utilization rate)
 * */
double getCpuLoad(void) {
  CpuTimes t1, t2;
  readCpuTimes(&t1);
  sleep(1);
  readCpuTimes(&t2);

  unsigned long long totalDelta = t2.total - t1.total;
  unsigned long long idelDelta = t2.idle - t1.idle;

  if (totalDelta == 0) {
    fprintf(stderr, "CPU is not run.\n");
    return -1;
  }

  double cpuUsage =
      100.0 * (double)(totalDelta - idelDelta) / (double)totalDelta;
  return cpuUsage;
}

/*
 * brief  Helper function to parse a value from MEM_USAGE_FILE
 *
 * param  fileContent: Key-value pair file content.
 *        key: The key.
 *
 * return long: The value of the key
 * */
long getMemValue(const char *fileContent, const char *key) {
  // Find the start position of the Key-value pair
  char *linePtr = strstr(fileContent, key);

  if (linePtr) {
    long value = 0;
    sscanf(linePtr + strlen(key), "%ld", &value);
    return value;
  }

  return -1;
}

/*
 * brief  Serch for the corresponding value through the key and finally
 * calculate the memory usage rate.
 *
 * return float: The memory usage rate.
 * */
float getMemUsage(void) {
  FILE *fp = fopen(MEM_USAGE_FILE, "r");
  if (fp == NULL) {
    perror("Error opening MEM_USAGE_FILE");
    return -1;
  }

  char buffer[2048];
  size_t bytesRead = fread(buffer, 1, sizeof(buffer), fp);
  fclose(fp);
  buffer[bytesRead] = '\0';

  long totalMemory = getMemValue(buffer, "MemTotal:");
  long availableMemory = getMemValue(buffer, "MemAvailable:");

  if (totalMemory == -1 || availableMemory == -1) {
    fprintf(stderr, "Cloud not parse memory info.\n");
    return -1;
  }

  long usedMemory = totalMemory - availableMemory;
  float memoryUsage = 100.0 * (float)usedMemory / (float)totalMemory;
  return memoryUsage;
}
