#include "ap3216c_linux.h"
#include <stdio.h>
#include <unistd.h>

/*
 * @brief   Get the ambient light intensity data
 *
 * @return  int:  ambient light intensity data
 * */
int ap3216c_linux_getAlsData(void) {
  FILE *fd;
  int als;

  // open ALS file
  if ((fd = fopen(AP3216C_ALS_FILE, "r")) == NULL) {
    perror("Error opening ALS file");
    return -1;
  }

  // read ALS data
  if ((fscanf(fd, "%d", &als)) != 1) {
    perror("Error reading ALS data");
    fclose(fd);
    return -1;
  }

  fclose(fd);
  return als;
}

/*
 * @brief   Get the approching distance data
 *
 * @return  int:  approching distance data
 * */
int ap3216c_linux_getPsData(void) {
  FILE *fd;
  int ps;

  // open ALS file
  if ((fd = fopen(AP3216C_PS_FILE, "r")) == NULL) {
    perror("Error opening PS file");
    return -1;
  }

  // read ALS data
  if ((fscanf(fd, "%d", &ps)) != 1) {
    perror("Error reading PS data");
    fclose(fd);
    return -1;
  }

  fclose(fd);
  return ps;
}

/*
 * @brief   Get the infraed intensity data
 *
 * @return  int:  infraed intensity data
 * */
int ap3216c_linux_getIrData(void) {
  FILE *fd;
  int ir;

  // open ALS file
  if ((fd = fopen(AP3216C_IR_FILE, "r")) == NULL) {
    perror("Error opening IR file");
    return -1;
  }

  // read ALS data
  if ((fscanf(fd, "%d", &ir)) != 1) {
    perror("Error reading ALS data");
    fclose(fd);
    return -1;
  }

  fclose(fd);
  return ir;
}
