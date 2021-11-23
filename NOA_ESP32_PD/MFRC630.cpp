/*
  MFRC630.cpp - Library for interacting with the MFRC630 chip.
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Wire.h>
#include <Esp.h>
#include <Adafruit_MFRC630.h>

#include "usb_pd.h"
#include "MFRC630.h"
#include "NOA_App.h"
#include "NOA_public.h"

#define MOJIC_TRICK       (0)
// const uint8_t OM966302_NFC_ADDR = 0x29;

// const int mfrc_630_nfc_int_pin = 5;  // init pin for NFC
// const int mfrc_630_nfc_pdown_pin = 4;  // init pin for NFC
// Adafruit_MFRC630 rfid = Adafruit_MFRC630(MFRC630_I2C_ADDR, mfrc_630_nfc_pdown_pin);
// Adafruit_MFRC630 rfid_nfc = Adafruit_MFRC630(&Wire1, 0x29, mfrc_630_nfc_pdown_pin);
extern Adafruit_MFRC630 rfid_nfc;

TaskHandle_t MFRC630_NFC_Task = NULL;
// xQueueHandle NOA_NFC_TASK = NULL;
StaticTask_t xTaskBuffer_NFC;
StackType_t xStack_NFC[SIZE_OF_STACK] = {0};

TaskHandle_t MFRC630_NFC_Test_Task = NULL;
static int nStatus_NFC_Testing = 0;
StaticTask_t xTaskBuffer_NFC_Test;
StackType_t xStack_NFC_Test[SIZE_OF_STACK] = {0};

/* This functions sends a command to fill the FIFO with random numbers and */
/* then reads the entire FIFO contents in 16 byte chunks */
void MFRC630_fifo_read_test(void) {
  DBGLOG(Debug, "Reading randomly generated numbers from FIFO buffer");

  /* Generate random numbers into the FIFO */
  rfid_nfc.writeCommand(MFRC630_CMD_READRNR);
  /* Note, this command requires a 10ms delay to fill the buffer! */
  vTaskDelay(10);

  /* Dump the FIFO 16 bytes at a time to stay within 32 byte I2C limit */
  uint8_t buff[16];
  int16_t len = rfid_nfc.readFIFOLen();
  while (len) {
    memset(buff, 0, sizeof(buff));
    int16_t readlen = rfid_nfc.readFIFO(len > 16 ? 16 : len, buff);
    len -= readlen;
    /* Display the buffer in 16 byte chunks. */
    NOA_PUB_Print_Buf_Hex(buff, readlen);
  }
}

/* This function writes data to the FIFO and then tries to read it back */
void MFRC630_fifo_write_test(void) {
  uint8_t buff[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

  /* Write data into the FIFO buffer */
  DBGLOG(Debug, "Writing 16 bytes into FIFO buffer");
  int16_t writelen = rfid_nfc.writeFIFO(sizeof(buff), buff);
  (void)writelen;

  /* Read data back and display it*/
  memset(buff, 0, sizeof(buff));
  int16_t readlen = rfid_nfc.readFIFO(sizeof(buff), buff);
  NOA_PUB_Print_Buf_Hex(buff, readlen);
}

/* This function clears the FIFO contents */
void MFRC630_fifo_clear_test(void) {
  uint8_t buff[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

  /* Write data into the FIFO buffer */
  DBGLOG(Debug, "Writing 16 bytes into FIFO buffer");
  int16_t writelen = rfid_nfc.writeFIFO(sizeof(buff), buff);
  (void)writelen;
  int16_t len = rfid_nfc.readFIFOLen();
  DBGLOG(Debug, "FIFO len = %d", len);

  /* Clear the FIFO */
  DBGLOG(Debug, "Clearing FIFO");
  rfid_nfc.clearFIFO();
  len = rfid_nfc.readFIFOLen();
  DBGLOG(Debug, "FIFO len = %d", len);
}

/* This function displays the status of the MFRC's state machine */
void status_test(void) {
  DBGLOG(Debug, "Reading COM Status");
  uint8_t stat = rfid_nfc.getComStatus();

  switch (stat) {
    case MFRC630_COMSTAT_IDLE:
      Serial.println("IDLE");
      break;
    case MFRC630_COMSTAT_TXWAIT:
      Serial.println("TX WAIT");
      break;
    case MFRC630_COMSTAT_TRANSMITTING:
      Serial.println("TRANSMITTING");
      break;
    case MFRC630_COMSTAT_RXWAIT:
      Serial.println("RX WAIT");
      break;
    case MFRC630_COMSTAT_WAITFORDATA :
      Serial.println("WAITING FOR DATA");
      break;
    case MFRC630_COMSTAT_RECEIVING:
      Serial.println("RECEIVING");
      break;
    default:
      Serial.println("UKNOWN!");
      break;
  }
}

void radio_mifare_dump_sector(uint8_t sector_num) {
  uint8_t readbuf[16] = { 0 };
  /* Try to read four blocks inside the sector. */
  for (uint8_t b = 0; b < 4 ; b++) {
    uint8_t len = 0;
    len = rfid_nfc.mifareReadBlock(sector_num * 4 + b, readbuf);
    if (len == 0) {
      /* No data returned! */
      DBGLOG(Debug, "What!?! No data returned for block %d!", sector_num * 4 + b);
      #if MOJIC_TRICK
      Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
      #endif
      return;
    } else {
      /* Display the block contents. */
      DBGLOG(Debug, "%d: ", sector_num * 4 + b);
      NOA_PUB_Print_Buf_Hex(readbuf, len);
    }
  }
}

/* Read the tag contents (assumes scan has been successfully called!). */
void radio_mifare_read(uint8_t *uid, uint8_t uidlen) {
  if (uidlen != 4) {
    DBGLOG(Debug, "Not a Mifare tag!");
    #if MOJIC_TRICK
    Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
    #endif
    return;
  }
  /* Use the default key for fresh Mifare cards. */
  // uint8_t keyb[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  /* Set the key. */
  rfid_nfc.mifareLoadKey(rfid_nfc.mifareKeyGlobal);
  /* Try to authenticate sectors 0..15 (all Mifare cards should have these!). */
  for (uint8_t s = 0; s < 16; s++) {
    DBGLOG(Debug, "Trying to authenticate sector %d with KEYA.", s);
    /* Authenticate. */
    if(rfid_nfc.mifareAuth(MIFARE_CMD_AUTH_A, s*4, uid)) {
      #if MOJIC_TRICK
      Serial.println(" ᕙ(`▽´)ᕗ");
      #endif
      /* Dump the first sector. */
      radio_mifare_dump_sector(s);
    } else {
      Serial.print("AUTH_A failed with ");
      for (uint8_t k = 0; k < 6; k++){
        Serial.print("0x");
        if (HEX < 0x10) {
          Serial.print("0");
        }
        Serial.print(rfid_nfc.mifareKeyGlobal[k], HEX);
        Serial.print(" ");
      }
      Serial.println("");
      #if MOJIC_TRICK
      Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
      #endif
    }
  }
}

/* Configure the radio for ISO14443A-106 type tags and scan for the UID. */
void radio_iso1443A_106_scan() {
  DBGLOG(Debug, "Configuring radio for ISO14443A-106 tags");
  rfid_nfc.configRadio(MFRC630_RADIOCFG_ISO1443A_106);

  /* Request a tag */
  DBGLOG(Debug, "Checking for ISO14443A-106 tags");
  #if MOJIC_TRICK
  Serial.println("┬─┬ ノ( ゜-゜ノ)");
  #endif
  uint16_t atqa = rfid_nfc.iso14443aRequest();
  /* Select the tag if we found something */
  if (atqa) {
    uint8_t uid[10] = { 0 };
    uint8_t uidlen;
    uint8_t sak;
    DBGLOG(Debug, "Found ISO14443A-106 tag with ATQA 0x%02X", atqa);
    #if MOJIC_TRICK
    Serial.println("ᕙ(`▽´)ᕗ");
    #endif

    DBGLOG(Debug, "Selecting the tag");
    #if MOJIC_TRICK
    Serial.println("(ﾉ◕ヮ◕)ﾉ*:・ﾟ✧");
    #endif
    uidlen = rfid_nfc.iso14443aSelect(uid, &sak);
    if (uidlen) {
      Serial.print("Got a UID! --> ");
      for (uint8_t i=0; i<uidlen; i++) {
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
      #if MOJIC_TRICK
      Serial.println("ᕙ(`▽´)ᕗ");
      #endif
      /* Try to read and dump the card's contents. */
      radio_mifare_read(uid, uidlen);
    } else {
      DBGLOG(Debug, "What, no UID found! Did someone steal my tag!!");
      #if MOJIC_TRICK
      Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
      #endif
    }
  } else {
    DBGLOG(Debug, "No ISO14443A-106 tag found!");
    #if MOJIC_TRICK
    Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
    #endif
  }
}

bool radio_ntag156b_write(void) {
  bool rc;
  /* Put the IC in a known-state. */
  rfid_nfc.softReset();
  /* Configure the radio for ISO14443A-106. */
  rfid_nfc.configRadio(MFRC630_RADIOCFG_ISO1443A_106);
  /* Request a tag (activates the near field, etc.). */
  uint16_t atqa = rfid_nfc.iso14443aRequest();
  /* Looks like we found a tag, move on to selection. */
  if (atqa) {
    /* NTAG has a ATQA of 00 44 (Ultralight does as well!). */
    if (atqa == 0x44) {
      uint8_t uid[10] = { 0 };
      uint8_t uidlen;
      uint8_t sak;
      /* Retrieve the UID and SAK values. */
      uidlen = rfid_nfc.iso14443aSelect(uid, &sak);
      Serial.print("Found a tag with UUID ");
      for (uint8_t i = 0; i < uidlen; i++) {
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
      if (uidlen == 7) {
        /* Write something distinct to page 7. */
        DBGLOG(Debug, "Writing data to page 7.");
        uint8_t pagebuf[4] = { 0x12, 0x34, 0x56, 0x78 };
        uint8_t len = rfid_nfc.ntagWritePage(7, pagebuf);
        /* Make sure we successfully wrote 4 bytes! */
        rc = len == 4 ? true : false;
      } else {
        /* Should be 7, not sure what kind of tag we have. */
        DBGLOG(Debug, "Unexpected UID length: %d", uidlen);
        rc = false;
      }
    } else {
      /* Found a tag, but it isn't NTAG */
      DBGLOG(Debug, "Unexpected ATQA value: 0x%02X", atqa);
      rc = false;
    }
  } else {
    /* No tag found! */
    rc = false;
  }
  return rc;
}

/*
 * This more concise loop show the minimim requirements to dump the first 39
 * 4 byte blocks of memory from an NTAG2xx card. No meaningful error-handling
 * or debug output is present here, so this code is intended as a simple
 * starting point for further work.
 */
bool radio_ntag156b_dump_minimal(void) {
  bool rc;
  /* Put the IC in a known-state. */
  rfid_nfc.softReset();
  /* Configure the radio for ISO14443A-106. */
  rfid_nfc.configRadio(MFRC630_RADIOCFG_ISO1443A_106);
  /* Request a tag (activates the near field, etc.). */
  uint16_t atqa = rfid_nfc.iso14443aRequest();
  /* Looks like we found a tag, move on to selection. */
  if (atqa) {
    /* NTAG has a ATQA of 00 44 (Ultralight does as well!). */
    if (atqa == 0x44) {
      uint8_t uid[10] = { 0 };
      uint8_t uidlen;
      uint8_t sak;
      /* Retrieve the UID and SAK values. */
      uidlen = rfid_nfc.iso14443aSelect(uid, &sak);
      Serial.print("Found a tag with UUID ");
      for (uint8_t i = 0; i < uidlen; i++) {
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
      if (uidlen == 7) {
        /* Try to read the first 42 pages from the card. */
        for (uint8_t i = 0; i < 42; i++) {
          /* We should be able to read the page contents now. */
          uint8_t pagebuf[4] = { 0, 0, 0, 0 };
          uint8_t len = rfid_nfc.ntagReadPage(i, pagebuf);
          Serial.print(i);
          Serial.print(": ");
          NOA_PUB_Print_Buf_Hex(pagebuf, len);
        }
        rc = true;
      } else {
        /* Should be 7, not sure what kind of tag we have. */
        DBGLOG(Debug, "Unexpected UID length: %d", uidlen);
        rc = false;
      }
    } else {
      /* Found a tag, but it isn't NTAG */
      DBGLOG(Debug, "Unexpected ATQA value: 0x%02X", atqa);
      rc = false;
    }
  } else {
    /* No tag found! */
    rc = false;
  }
  return rc;
}

/*
 * This more concise loop show the minimim requirements to dump the first 1K
 * of memory from a Mifare 1K or Mifare Plus compatible card. No meaningful
 * error-handling or debug output is present here, so this code is intended
 * as a simple starting point for further work.
 */
bool radio_mifare1K_dump_minimal(void) {
  bool rc = true;
  // Put the IC in a known-state.
  rfid_nfc.softReset();
  // Configure the radio for ISO14443A-106.
  rfid_nfc.configRadio(MFRC630_RADIOCFG_ISO1443A_106);
  // Request a tag (activates the near field, etc.).
  uint16_t atqa = rfid_nfc.iso14443aRequest();
  /* Looks like we found a tag, move on to selection. */
  if (atqa) {
    DBGLOG(Debug, "Found tag %d", atqa);
    uint8_t uid[10] = { 0 };
    uint8_t uidlen;
    uint8_t sak;

    // Retrieve the UID and SAK values.
    uidlen = rfid_nfc.iso14443aSelect(uid, &sak);
    Serial.print("Found a tag with UUID ");
    for (uint8_t i = 0; i < uidlen; i++) {
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
    if (uidlen == 4) {
      // Assume Mifare Classic/Plus and set the global/default key.
      rfid_nfc.mifareLoadKey(rfid_nfc.mifareKeyGlobal);
      // Try to authenticate sectors 0..15.
      for (uint8_t s = 0; s < 16; s++) {
        // Try to authenticate this sector.
        Serial.print("Sector "); Serial.println(s);
        if(rfid_nfc.mifareAuth(MIFARE_CMD_AUTH_A, s*4, uid)) {
          // We should be able to read the sector contents now.
          radio_mifare_dump_sector(s);
        } else {
          Serial.print("AUTH_A failed for sector ");
          Serial.println(s);
        }
      }
      rc = true;
    } else {
      DBGLOG(Debug, "Unexpected UID length: %d", uidlen);
      rc = false;
    }
  } else {
    rc = false;     // No tag found, return false.
  } 
  return rc;
}

//****************************************************************************
// CODE TABLES
//****************************************************************************
void MFRC630_NFC_Test_Task_Loop( void * pvParameters ){
  DBGLOG(Info, "MFRC630_NFC_Test_Task_Loop running on core %d", xPortGetCoreID());
  if (!(rfid_nfc.begin())) {
    DBGLOG(Info,"Unable to initialize the MFRC630. Check wiring?");
    nStatus_NFC_Testing = 0;
  } else {
    nStatus_NFC_Testing = 1;
  }
  /* FIFO tests */
//  fifo_read_test();
//  fifo_write_test();
//  fifo_clear_test();
  /* General HW tests */
//  status_test();
  /* Radio tests */
//  radio_iso1443A_106_scan();  

  DBGLOG(Info, "MFRC630_NFC_Test_Task_Loop Exit from core %d", xPortGetCoreID());

//  uint32_t msg = 0;
  NOA_PUB_MSG msg;
  memset(&msg, 0, sizeof(NOA_PUB_MSG));
  if (nStatus_NFC_Testing == 1) {
    msg.message = NFC_MSG_READY;
  } else {
    msg.message = NFC_MSG_NOTREADY;
  }
  if (NOA_APP_TASKQUEUE != NULL) {
    xQueueSend(NOA_APP_TASKQUEUE, &msg, portMAX_DELAY);
  }
  vTaskDelete(NULL);
}

// MFRC630_NFC_Task_Loop: NFC task loop
/* void MFRC630_NFC_Task_Loop( void * pvParameters ){
  DBGLOG(Info, "MFRC630_NFC_Task_Loop running on core %d", xPortGetCoreID());
  const TickType_t xTicksToWait = pdMS_TO_TICKS(50);
  BaseType_t xStatus = 0;
  NOA_PUB_MSG msg;

  while(true){
    xStatus = xQueueReceive(NOA_NFC_TASK, &msg, xTicksToWait);  //Get message from the queue
//    xStatus = xQueueReceive(NOA_NFC_TASK, &msg, portMAX_DELAY);
    if (xStatus == pdPASS) {
//      DBGLOG(Info, "MFRC630_NFC_Task_Loop get a message %d QueueSpaces %d", msg.message, uxQueueSpacesAvailable(NOA_NFC_TASK));
    }
    switch(msg.message) {
      case NFC_MSG_READY:
        nStatus_NFC_Testing = 1;
        break;
      case APP_MSG_TIMER_ID:
        if (nStatus_NFC_Testing == 0) {
          break;
        }
        DBGLOG(Info, "NFC task APP_MSG_TIMER_ID");
        break;
      default:
        break;
    }
    memset(&msg, 0, sizeof(NOA_NFC_TASK));
  }
  DBGLOG(Info, "MFRC630_NFC_Task_Loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
} */

void MFRC630_NFC_Task_Loop( void * pvParameters ){
  DBGLOG(Info, "MFRC630_NFC_Task_Loop running on core %d", xPortGetCoreID());
  while(true){
//    status_test();
//    radio_iso1443A_106_scan();
//    radio_mifare1K_dump_minimal();
//    radio_ntag156b_dump_minimal();
    if (nStatus_NFC_Testing == 1) {
//      radio_mifare1K_dump_minimal();
//      status_test();
    }
    vTaskDelay(1000);
  }
  DBGLOG(Info, "MFRC630_NFC_Task_Loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}
void MFRC630_NFC_init() {
  nStatus_NFC_Testing = 0;

/*  MFRC630_NFC_Test_Task = xTaskCreateStaticPinnedToCore(
                   MFRC630_NFC_Test_Task_Loop,       // Function that implements the task.
                   "MFRC630NFCTest",            // Text name for the task.
                   SIZE_OF_STACK,               // Stack size in bytes, not words.
                   NULL,                        // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,        // Priority at which the task is created.
                   xStack_NFC_Test,             // Array to use as the task's stack.
                   &xTaskBuffer_NFC_Test,       // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);
                   
  if (MFRC630_NFC_Test_Task == NULL || &xTaskBuffer_NFC_Test == NULL) {
    DBGLOG(Error, "Create MFRC630_NFC_Test_Task fail");
  }

  NOA_NFC_TASK = xQueueCreate(SIZE_OF_TASK_QUEUE, sizeof(NOA_PUB_MSG));
  if (NOA_NFC_TASK == NULL) {
    DBGLOG(Error, "Create NOA_NFC_TASK fail");
  } */

  MFRC630_NFC_Task = xTaskCreateStaticPinnedToCore(
                   MFRC630_NFC_Task_Loop,   // Function that implements the task.
                   "MFRC630NFCTask",        // Text name for the task.
                   SIZE_OF_STACK,           // Stack size in bytes, not words.
                   NULL,                    // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,    // Priority at which the task is created.
                   xStack_NFC,              // Array to use as the task's stack.
                   &xTaskBuffer_NFC,        // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);
                   
  if (MFRC630_NFC_Task == NULL || &xTaskBuffer_NFC == NULL) {
    DBGLOG(Error, "Create MFRC630_NFC_Task fail");
  }
}
