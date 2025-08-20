#ifndef _DEVICE_MONITOR_H
#define _DEVICE_MONITOR_H

#include <stdio.h>
#include <sys/types.h>

#define CPU_INFO_FILE "/proc/cpuinfo"
#define CPU_TEMP_FILE "/sys/class/thermal/thermal_zone0/temp"
#define CPU_TIME_FILE "/proc/stat"
#define MEM_USAGE_FILE "/proc/meminfo"

/*  The time slice(jiffies) spent by the CPU in different states since its
 * startup */
typedef struct {
  unsigned long long user;   // user mode time
  unsigned long long nice;   // low-priority user-space time
  unsigned long long system; // kernel mode time
  unsigned long long idle;   // idel time
  unsigned long long iowait; // wait for the io completion time
  unsigned long long irq;
  unsigned long long softirq;
  unsigned long long steal;
  unsigned long long total;
} CpuTimes;

float getCpuTemperature();
void readCpuTimes(CpuTimes *times);
double getCpuLoad();
long getMemValue(const char *fileContent, const char *key);
float getMemUsage(void);

#endif // !_DEVICE_MONITOR_H
