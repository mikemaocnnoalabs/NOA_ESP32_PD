/*
  NOA_public.h - NOA debug library head file
  Copyright 2012 NOA Labs
  Copyright 2022 Mike Mao
  Released under an MIT license. See LICENSE file. 
*/
#ifndef __NOA_DEBUG_H_
#define __NOA_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "esp_log.h"

#define NOA_DEBUG_ENABLE    // Open DEBUG log
const char * pathToFileName(const char * path);
void Debug_init(void);

#ifndef NOA_DEBUG_ENABLE
// #define EYBOND_TRACE_ENABLE    // Change DEBUG log Output to TRACE mode
#endif

#ifdef NOA_DEBUG_ENABLE
#define DEBUG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%u) %s:%s:%d " format LOG_RESET_COLOR "\n"
#define APP_DEBUG(format, ... )  { esp_log_write(ESP_LOG_INFO,   pathToFileName(__FILE__), DEBUG_FORMAT(D, format), esp_log_timestamp(), pathToFileName(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__); }
#else
#define APP_DEBUG(FORMAT,...)
#endif

#ifdef __cplusplus
}
#endif

#endif //__NOA_DEBUG_H_
/******************************************************************************/
