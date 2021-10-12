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
void NOA_PUB_I2C_Scanner(uint8_t nIndex);
void NOA_PUB_I2C_ReceiveBytes(uint8_t nIndex, uint8_t PD_ADDR, uint8_t addr, uint8_t *data, uint16_t length);
void NOA_PUB_I2C_SendBytes(uint8_t nIndex, uint8_t PD_ADDR, uint8_t addr, uint8_t *data, uint16_t length);

void NOA_PUB_I2C_PD_RreadAllRegs(uint8_t nIndex, uint8_t PD_ADDR);
void NOA_PUB_I2C_PM_RreadAllRegs(uint8_t nIndex, uint8_t PD_ADDR);
void NOA_PUB_I2C_PD_Testing(uint8_t nIndex, uint8_t PD_ADDR);

#ifdef __cplusplus
}
#endif

#endif
