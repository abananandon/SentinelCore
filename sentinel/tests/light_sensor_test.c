#include "../src/modules/light_sensor/light_sensor.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int als, ir, ps;

  while (1) {
    als = getAlsData();
    ir = getIrData();
    ps = getPsData();

    printf("ALS data = %d, IR data = %d, PS data = %d\n", als, ir, ps);
  }

  return EXIT_SUCCESS;
}
