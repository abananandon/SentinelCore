#include "light_sensor.h"
#include "ap3216c_linux.h"

int getAlsData(void) { return ap3216c_linux_getAlsData(); }

int getPsData(void) { return ap3216c_linux_getPsData(); }

int getIrData(void) { return ap3216c_linux_getIrData(); }
