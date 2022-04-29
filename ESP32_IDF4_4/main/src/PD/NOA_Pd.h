/*
  NOA_Pd.h - Library for PD Discovery.
  Copyright 2012 NOA Labs
  Copyright 2022 Mike mao
  Released under an MIT license. See LICENSE file. 
*/

#ifndef NOA_PD_H
#define NOA_PD_H

#ifdef __cplusplus
extern "C" {
#endif

void NOA_Pd_init();
void NOA_Pd_Sink_PowerSave_init();
void reset_all_io();
#ifdef __cplusplus
}
#endif

#endif /* NOA_PD_H */
