/*
  NOA_public.h - NOA public library head file
  Copyright 2012 NOA Labs
  Copyright 2021 Mike Mao
  Released under an MIT license. See LICENSE file. 
*/

#ifndef __NOA_PUBLIC_H
#define __NOA_PUBLIC_H

#define ESP32DEBUGGING
#include <ESP32Logger.h>

#ifdef __cplusplus
extern "C" {
#endif

void NOA_PUB_ESP32DebugInit(void);
void NOA_PUB_I2C_Scanner(void);
void NOA_PUB_I2C_PD_RreadAllRegs(uint8_t PD_ADDR);
void NOA_PUB_I2C_PD_Testing(uint8_t PD_ADDR);

#ifdef __cplusplus
}
#endif

#endif
