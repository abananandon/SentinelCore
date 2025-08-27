#include "cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  cJSON *json1 = cJSON_CreateObject();
  cJSON_AddStringToObject(json1, "name", "json1");
  cJSON_AddStringToObject(json1, "cJSON_Version", cJSON_Version());
  cJSON_AddNumberToObject(json1, "NO", 1);

  char *out1 = cJSON_Print(json1);

  printf("%s\n", out1);

  return EXIT_SUCCESS;
}
