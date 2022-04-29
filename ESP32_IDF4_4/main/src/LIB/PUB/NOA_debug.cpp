/*
  NOA_public.h - NOA debug library head file
  Copyright 2012 NOA Labs
  Copyright 2022 Mike Mao
  Released under an MIT license. See LICENSE file. 
*/
#include <stddef.h>
#include <esp_attr.h>

#include "NOA_debug.h"

//used by hal log
const char * IRAM_ATTR pathToFileName(const char * path) {
  size_t i = 0;
  size_t pos = 0;
  char * p = (char *)path;
  while (*p) {
    i++;
    if (*p == '/' || *p == '\\') {
      pos = i;
    }
    p++;
  }
  return path+pos;
}

void Debug_init(void) {
#ifdef  NOA_DEBUG_ENABLE
//  Calling this function restores all Info-level logging at runtime (as "Log Maximum Verbosity" set to "Info")
//  esp_log_level_set("*", ESP_LOG_INFO);
//  ESP_LOGI(TAG, "%d:%s App started!", __LINE__, __FUNCTION__);
  esp_log_level_set("*", ESP_LOG_INFO);
#else
#endif
}

/*********************************FILE END*************************************/

