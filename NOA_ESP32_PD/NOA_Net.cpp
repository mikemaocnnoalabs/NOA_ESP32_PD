/*
  NOA_Net.cpp - Library for Net APP..
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Esp.h>
#include "WiFi.h"

#include "usb_pd.h"
#include "NOA_App.h"
#include "NOA_Net.h"
#include "NOA_public.h"

#define SIZE_OF_NET_STACK  SIZE_OF_STACK * 4

TaskHandle_t NOA_Net_Task = NULL;
StaticTask_t xTaskBuffer_Net;
StackType_t xStack_Net[SIZE_OF_NET_STACK];
xQueueHandle NOA_NET_TASKQUEUE = NULL;

TaskHandle_t WIFI_Test_Task = NULL;
StaticTask_t xTaskBuffer_WIFI_Test;
StackType_t xStack_WIFI_Test[SIZE_OF_NET_STACK];
static int nStatus_WiFiTesting = 0;

IPAddress sip(192,168,1,29);      //Sta IP
IPAddress sip1(192,168,1,1);      //Sta gateway
IPAddress sip2(255,255,255,0);    //Sta netmask
IPAddress serverip(192,168,1,4);  //Remote IP for Local network

IPAddress lxip(192,168,88,1);     //AP IP
IPAddress lxip1(192,168,88,1);    //AP gateway
IPAddress lxip2(255,255,255,0);   //AP netmask
IPAddress xip(192,168,88,2);      //Remote IP for AP network

#define STA_SSID "NOARDTest"
#define STA_PASS "123456789"
#define AP_SSID  "esp32"

unsigned int localPort=9999;      //Local port
unsigned int remoteport=9999;     //Remote port
const char* ssid="NOARDTest";     //default SSID for Sta
const char* password="123456789"; //passwd for default SSID
const char* ssid1="WIFI1";        //default AP SSID
const char* password1="987654321"; //passwd for default AP SSID
char buff[1024];    // buffer for network

//****************************************************************************
// CODE TABLES
//****************************************************************************
void WiFiEvent(WiFiEvent_t event){
  switch(event) {
    case SYSTEM_EVENT_AP_START:
      Serial.println("AP Started");
      WiFi.softAPsetHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("AP Stopped");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("STA Started");
      WiFi.setHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("STA Connected");
      WiFi.enableIpV6();
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      Serial.print("STA IPv6: ");
      Serial.println(WiFi.localIPv6());
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("STA IPv4: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("STA Disconnected");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("STA Stopped");
      break;
    default:
      break;
  }
}

void WIFI_Test_Task_Loop( void * pvParameters ){
  DBGLOG(Info, "WIFI_Test_Task_Loop running on core %d", xPortGetCoreID());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  Serial.printf("Scan Done\r\n");
  if (n == 0) {
    Serial.printf("No networks found\r\n");
  } else {
    Serial.printf("%d networks found\r\n", n);
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
//      Serial.print(i + 1);
//      Serial.print(": ");
//      Serial.print(WiFi.SSID(i));
//      Serial.print(" (");
//      Serial.print(WiFi.RSSI(i));
//      Serial.print(")");
//      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
//      delay(10);
        Serial.printf("%d:  %s \t\t (%d)\r\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
/*        switch(WiFi.encryptionType(i)) {
          case WIFI_AUTH_OPEN:
            Serial.println();
            break;
          case WIFI_AUTH_WEP:
            break;
          case WIFI_AUTH_WPA_PSK:
            break;
          case WIFI_AUTH_WPA2_PSK:
            break;
          case WIFI_AUTH_WPA_WPA2_PSK:
            break;
          case WIFI_AUTH_WPA2_ENTERPRISE:
            break;
        } */
    }
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.onEvent(WiFiEvent);
  Serial.println(WiFi.macAddress());  
  DBGLOG(Info, "WIFI_Test_Task_Loop Exit from core %d", xPortGetCoreID());

  NOA_PUB_MSG msg;
  memset(&msg, 0, sizeof(NOA_PUB_MSG));
  msg.message = NET_MSG_READY;
  xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
  nStatus_WiFiTesting = 1;
  vTaskDelete(NULL);
}

// Net_APP_Task_Loop: Net APP task
void Net_APP_Task_Loop(void * pvParameters) {
  DBGLOG(Info, "Net_APP_Task_Loop running on core %d", xPortGetCoreID());
  BaseType_t xStatus = 0;
//  uint32_t msg = 0;
  NOA_PUB_MSG msg;

  while(true){
//    msg = 0;
    memset(&msg, 0, sizeof(NOA_PUB_MSG));
    xStatus = xQueueReceive(NOA_NET_TASKQUEUE, &msg, portMAX_DELAY);
//    xStatus = xQueueReceive(NOA_NET_TASKQUEUE, &msg, xTicksToWait);
    if (xStatus == pdPASS) {
//      DBGLOG(Info, "Net_APP_Task_Loop get a message %d QueueSpaces %d", msg.message, uxQueueSpacesAvailable(NOA_NET_TASK));
    }
    switch(msg.message) {
      case APP_MSG_TIMER_ID:
        DBGLOG(Info, "Net task APP_MSG_TIMER_ID, StackSize %ld", uxTaskGetStackHighWaterMark(NULL));
/*        switch(WiFi.status()) {
          case WL_CONNECTED:
            break;
          default:
            DBGLOG(Info, "WiFi.status  %d", WiFi.status());
            break;
        }  */
        break;
      default:
//        DBGLOG(Info, "Net task runing");
        break;
    }
  }
  DBGLOG(Info, "Net_APP_Task_Loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}

void NOA_Net_init() {
  nStatus_WiFiTesting = 0;
  WIFI_Test_Task = xTaskCreateStaticPinnedToCore(
                   WIFI_Test_Task_Loop,       // Function that implements the task.
                   "WIFITestTask",          // Text name for the task.
                   SIZE_OF_NET_STACK,       // Stack size in bytes, not words.
                   NULL,                    // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,    // Priority at which the task is created.
                   xStack_WIFI_Test,          // Array to use as the task's stack.
                   &xTaskBuffer_WIFI_Test,   // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);
  if (WIFI_Test_Task == NULL || &xTaskBuffer_WIFI_Test == NULL) {
    DBGLOG(Error, "Create WIFI_Test_Task fail");
  }

  NOA_NET_TASKQUEUE = xQueueCreate(SIZE_OF_TASK_QUEUE, sizeof(NOA_PUB_MSG));
  if (NOA_NET_TASKQUEUE == NULL) {
    DBGLOG(Error, "Create NOA_NET_TASKQUEUE fail");
  }

  NOA_Net_Task = xTaskCreateStaticPinnedToCore(
                   Net_APP_Task_Loop,       // Function that implements the task.
                   "NetAPPTask",            // Text name for the task.
                   SIZE_OF_NET_STACK,       // Stack size in bytes, not words.
                   NULL,                    // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,    // Priority at which the task is created.
                   xStack_Net,          // Array to use as the task's stack.
                   &xTaskBuffer_Net,    // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);
  if (NOA_Net_Task == NULL || &xTaskBuffer_Net == NULL) {
    DBGLOG(Error, "Create Net_App_Task fail");
  }
}
