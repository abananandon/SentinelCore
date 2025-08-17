#include "../src/modules/device_monitor/device_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  float cpuTemperature = 0.0;
  double cpuLoad = 0.0;
  float memoryUsageRate = 0.0;

  while (1) {
    cpuTemperature = getCpuTemperature();
    cpuLoad = getCpuLoad();
    memoryUsageRate = getMemUsage();

    printf("cpuTemperature = %f \ncpuLoad = %lf \nmemoryUsageRate = %f \n",
           cpuTemperature, cpuLoad, memoryUsageRate);
  }
  return EXIT_SUCCESS;
}
