#ifndef _AP3216C_LINUX_H
#define _AP3216C_LINUX_H

#include <stdio.h>
#include <stdlib.h>

#define AP3216C_DEVICE_PATH "/sys/class/misc/ap3216c"
#define AP3216C_ALS_FILE                                                       \
  "/sys/class/misc/ap3216c/als" // ambient light intensity data
#define AP3216C_IR_FILE                                                        \
  "/sys/class/misc/ap3216c/ps" // approaching distance data
#define AP3216C_PS_FILE "/sys/class/misc/ap3216c/ir" // infraed intensity data

int ap3216c_linux_getAlsData();
int ap3216c_linux_getPsData();
int ap3216c_linux_getIrData();
#endif // !AP3216C_LINUX_H
