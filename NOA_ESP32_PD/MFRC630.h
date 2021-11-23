/*
  MFRC630.h - Library for interacting MFRC630 chip with the Adafruit_MFRC630 library.
  Copyright 2012 NOA Labs
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/

#ifndef MFRC630_H
#define MFRC630_H

#ifdef __cplusplus
extern "C" {
#endif

#define  MSG_ID_USER_NFC_START    0x4000

#define  NFC_MSG_READY             MSG_ID_USER_NFC_START+0xFD1
#define  NFC_MSG_NOTREADY          MSG_ID_USER_NFC_START+0xFD2
#define  NFC_MSG_CLEAR             MSG_ID_USER_NFC_START+0xFD3

void MFRC630_NFC_init();

bool radio_mifare1K_dump_minimal(void);
void MFRC630_fifo_read_test(void);
void status_test(void);

#ifdef __cplusplus
}
#endif

#endif /* MFRC630_H */
