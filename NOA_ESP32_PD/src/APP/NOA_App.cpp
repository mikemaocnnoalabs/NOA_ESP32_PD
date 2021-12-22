/*
  NOA_App.cpp - Library for Main APP..
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
// #include <ESP32_New_TimerInterrupt.h>
// #include <ESP32TimerInterrupt.h>

#include <Esp.h>
#include <ESP32AnalogRead.h>

#include "..\DRV\PDM\usb_pd.h"
#include "..\DRV\PDM\NCP81239.h"
#include "..\DRV\NFC\NOA_NFC.h"

#include "..\LIB\PUB\NOA_public.h"

#include "..\UI\NEOPixel.h"

#include "..\APP\NOA_App.h"
#include "..\NET\NOA_Net.h"


#define SIZE_OF_APP_STACK  SIZE_OF_STACK * 4
// extern ESP32AnalogRead ncp_bb_con9v_tempadc;
extern int ncp_bb_con1_en_pin;

TaskHandle_t NOA_App_Task = NULL;
xQueueHandle NOA_APP_TASKQUEUE = NULL;

StaticTask_t xTaskBuffer_App;
StackType_t xStack_App[SIZE_OF_APP_STACK];

#define TIMER0_INTERVAL_MS        500
#define TIMER1_INTERVAL_MS        500

hw_timer_t *timer_watchdog = NULL;
hw_timer_t *timer_app = NULL;

static int nStatus_RGB = 0;
static int nStatus_NFC = 0;
static int nStatus_WIFI = 0;
static int nStatus_AP = 0;
static int nStatus_WIRELESS = 0;

//****************************************************************************
// CODE TABLES
//****************************************************************************
// MAIN_APP_Task_Loop: Main APP task
void MAIN_APP_Task_Loop(void * pvParameters) {
  DBGLOG(Info, "MAIN_APP_Task_Loop running on core %d", xPortGetCoreID());
//  const TickType_t xTicksToWait = pdMS_TO_TICKS(50);
  BaseType_t xStatus = 0;
  NOA_PUB_MSG msg;
//  uint32_t msg = 0;

  while(true){
//    xStatus = xQueueReceive(NOA_APP_TASK, &msg, xTicksToWait);  //Get message from the queue
//    msg = 0;
    memset(&msg, 0, sizeof(NOA_PUB_MSG));
    xStatus = xQueueReceive(NOA_APP_TASKQUEUE, &msg, portMAX_DELAY);
    if (xStatus == pdPASS) {
//      DBGLOG(Info, "MAIN_APP_Task_Loop get a message %d QueueSpaces %d", msg.message, uxQueueSpacesAvailable(NOA_APP_TASK));
    }
    switch(msg.message) {
      case RGB_MSG_READY:
        nStatus_RGB = 1;
        break;
      case NFC_MSG_READY:
        DBGLOG(Info, "App task NFC_MSG_READY");
        nStatus_NFC = 1;
        break;
      case NFC_MSG_NOTREADY:
        DBGLOG(Info, "App task NFC_MSG_NOTREADY");
        nStatus_NFC = 0;
        break;
      case NET_MSG_READY:
        nStatus_WIFI = 1;
        break;
      case NET_MSG_NOTREADY:
        nStatus_WIFI = 0;
        break;
      case APNET_MSG_READY:
        nStatus_AP = 1;
        break;
      case APNET_MSG_NOTREADY:
        nStatus_AP = 0;
        break;
      case APP_MSG_SRCREADY:
        break;
      case APP_MSG_SRCNOTREADY:
        break;
      case APP_MSG_APREADY:
        break;
      case APP_MSG_APNOTREADY:
        break;
      case APP_MSG_WIRELESSREADY:
        nStatus_WIRELESS = 1;
        break;
      case APP_MSG_WIRELESSNOTREADY:
        nStatus_WIRELESS = 0;
        break;
      case APP_MSG_WIFIREADY:
        break;
      case APP_MSG_WIFINOTREADY:
        break;
      case APP_MSG_NFCREADY:
        break;
      case APP_MSG_NFCNOTREADY:
        break;
      case APP_MSG_WDG_ID:
//        DBGLOG(Info, "App task APP_MSG_WDG_ID");
//        uint16_t uxArraySize = uxTaskGetNumberOfTasks();
//        DBGLOG(Info, "Temperrature = %d in %ld Heap %d/%d StackSize %ld", ncp_bb_con9v_tempadc.readMiliVolts(), millis()/1000, ESP.getFreeHeap(), ESP.getHeapSize(), uxTaskGetStackHighWaterMark(NULL));
        break;
      case APP_MSG_TIMER_ID:
//        DBGLOG(Info, "App task APP_MSG_TIMER_ID");
//        msg = 0;
        memset(&msg, 0, sizeof(NOA_PUB_MSG));
        if (nStatus_RGB == 1) {
          if(digitalRead(ncp_bb_con1_en_pin) == LOW) {
            msg.message = APP_MSG_SRCNOTREADY;
          } else {
            msg.message = APP_MSG_SRCREADY;
          }
          if (NOA_RGB_TASKQUEUE != NULL) {
            xQueueSend(NOA_RGB_TASKQUEUE, &msg, (TickType_t)0);
          }
          memset(&msg, 0, sizeof(NOA_PUB_MSG));
          if (nStatus_WIFI == 1) {
            msg.message = APP_MSG_WIFIREADY;
          } else {
            msg.message = APP_MSG_WIFINOTREADY;
          }
          if (NOA_RGB_TASKQUEUE != NULL) {
            xQueueSend(NOA_RGB_TASKQUEUE, &msg, (TickType_t)0);
          }
          memset(&msg, 0, sizeof(NOA_PUB_MSG));
          if(nStatus_NFC == 1) {
            msg.message = APP_MSG_NFCREADY;
          } else {
            msg.message = APP_MSG_NFCNOTREADY;
          }
          if (NOA_RGB_TASKQUEUE != NULL) {
            xQueueSend(NOA_RGB_TASKQUEUE, &msg, (TickType_t)0);
          }
          memset(&msg, 0, sizeof(NOA_PUB_MSG));
          if(nStatus_AP == 1) {
            msg.message = APP_MSG_APREADY;
          } else {
            msg.message = APP_MSG_APNOTREADY;
          }
          if (NOA_RGB_TASKQUEUE != NULL) {
            xQueueSend(NOA_RGB_TASKQUEUE, &msg, (TickType_t)0);
          }
          memset(&msg, 0, sizeof(NOA_PUB_MSG));
          if(nStatus_WIRELESS == 1) {
            msg.message = APP_MSG_WIRELESSREADY;
          } else {
            msg.message = APP_MSG_WIRELESSNOTREADY;
          }
          if (NOA_RGB_TASKQUEUE != NULL) {
            xQueueSend(NOA_RGB_TASKQUEUE, &msg, (TickType_t)0);
          }
        }
//        ncp81239_pmic_get_tatus(1);
        break;
      default:
        break;
    }
  }
  DBGLOG(Info, "MAIN_APP_Task_Loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}

void IRAM_ATTR TimerHandler0_Watchdog(void) {
  if (nStatus_RGB != 1) {
    return;
  }
  static bool toggle0 = false;
//  DBGLOG(Info, "ITimer0_Watchdog: millis() = %d", millis());
  toggle0 = !toggle0;
  if (toggle0 == true) {
//    const TickType_t xTicksToWait = pdMS_TO_TICKS(50);
//    uint32_t msg = 0;
    NOA_PUB_MSG msg;
    memset(&msg, 0, sizeof(NOA_PUB_MSG));
    msg.message = APP_MSG_WDG_ID;
    if (NOA_APP_TASKQUEUE != NULL) {
      xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
    }
  }
}

void IRAM_ATTR TimerHandler1_App(void) {
  if (nStatus_RGB != 1) {
    return;
  }
  static bool toggle1 = false;
//  DBGLOG(Info, "ITimer1_App: millis() = %d", millis());
  toggle1 = !toggle1;
//  uint32_t msg = 0;
  NOA_PUB_MSG msg;
  memset(&msg, 0, sizeof(NOA_PUB_MSG));
  msg.message = APP_MSG_TIMER_ID;
  if (toggle1 == true) {
    if (NOA_APP_TASKQUEUE != NULL) {
      xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
    }
    if (NOA_RGB_TASKQUEUE != NULL) {
      xQueueSend(NOA_RGB_TASKQUEUE, (void *)&msg, (TickType_t)0);
    }
  } else {
    if (NOA_NET_TASKQUEUE != NULL) {
      xQueueSend(NOA_NET_TASKQUEUE, (void *)&msg, (TickType_t)0);
    }
  }
}

void NOA_App_init() {
  nStatus_RGB = 0;
  nStatus_NFC = 0;
  nStatus_WIFI = 0;

  timer_watchdog = timerBegin(0,80,true);
  timer_app = timerBegin(1,80,true);
  timerAttachInterrupt(timer_watchdog, TimerHandler0_Watchdog, true);
  timerAttachInterrupt(timer_app, TimerHandler1_App, true);
  timerAlarmWrite(timer_watchdog, TIMER0_INTERVAL_MS * 1000, true);
  timerAlarmWrite(timer_app, TIMER1_INTERVAL_MS * 1000, true);
  timerAlarmEnable(timer_watchdog);
  timerAlarmEnable(timer_app);

  NOA_APP_TASKQUEUE = xQueueCreate(SIZE_OF_TASK_QUEUE, sizeof(NOA_PUB_MSG));
//  NOA_APP_TASKQUEUE = xQueueCreate(SIZE_OF_TASK_QUEUE, sizeof(uint32_t ));
  if (NOA_APP_TASKQUEUE == NULL) {
    DBGLOG(Error, "Create NOA_APP_TASKQUEUE fail");
  }

  NOA_App_Task = xTaskCreateStaticPinnedToCore(
                   MAIN_APP_Task_Loop,        // Function that implements the task.
                   "MAINAPPTask",             // Text name for the task.
                   SIZE_OF_APP_STACK,         // Stack size in bytes, not words.
                   NULL,                      // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,      // Priority at which the task is created.
                   xStack_App,                // Array to use as the task's stack.
                   &xTaskBuffer_App,          // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);   
                   
  if (NOA_App_Task == NULL || &xTaskBuffer_App == NULL) {
    DBGLOG(Error, "Create Main_App_Task fail");
  }
}
