/*
  NOA_Net.cpp - Library for Net APP..
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Esp.h>
#include <WiFi.h>

#include "..\DRV\PDM\usb_pd.h"
#include "..\APP\NOA_App.h"
#include "..\LIB\PUB\NOA_public.h"

#include "NOA_Net.h"

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

WiFiServer Iperf_Server(5001);

unsigned int localPort=5001;      //Local port
unsigned int remoteport=80;       //Remote port
const char* ssid="NOARDTest";     //default SSID for Sta
const char* password="12345678";  //passwd for default SSID
// const char* ssid1="NOASNA_0000000000";        //default AP SSID
const char* password1="87654321"; //passwd for default AP SSID
#define NOA_ESP32_HOST_NAME  "NOA_SNACKER_ESP32"
uint8_t nAP_Sta_Numbers = 0;
//****************************************************************************
// CODE TABLES
//****************************************************************************
void WiFiEvent(WiFiEvent_t event){
  NOA_PUB_MSG msg;
  memset(&msg, 0, sizeof(NOA_PUB_MSG));
  switch(event) {
    case SYSTEM_EVENT_AP_START:
      Serial.println("AP Started");
      WiFi.softAPConfig(lxip, lxip1, lxip2);  // Set AP Net parameters
      WiFi.softAPsetHostname(NOA_ESP32_HOST_NAME);
      WiFi.setHostname(NOA_ESP32_HOST_NAME);
      Serial.print("AP IPv4:");
      Serial.println(WiFi.softAPIP());
      msg.message = APNET_MSG_READY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("AP Stopped");
      msg.message = APNET_MSG_NOTREADY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("AP Sta Connected");
      nAP_Sta_Numbers = WiFi.softAPgetStationNum() - 1;
      Serial.print("AP Sta Num:");
      Serial.println(nAP_Sta_Numbers);
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("AP Sta Disconnected");
      nAP_Sta_Numbers = WiFi.softAPgetStationNum() - 1;
      Serial.print("AP Sta Num:");
      Serial.println(nAP_Sta_Numbers);
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("AP Sta Passigned");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("STA Started");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("STA Connected");
//      WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      Serial.print("STA IPv6: ");
      Serial.println(WiFi.localIPv6());
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("STA IPv4: ");
      Serial.println(WiFi.localIP());

      msg.message = NET_MSG_READY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("STA Disconnected");
      msg.message = NET_MSG_NOTREADY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("STA Stopped");
      break;
    default:
      Serial.println(event, HEX);
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
    Serial.printf("%d WIFI(2.4G)Networks Found\r\n", n);
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      int nrssi = 2 * (WiFi.RSSI(i) + 100);
      if (nrssi > 100) {
        nrssi = 100;
      }
      Serial.printf("%2d:%-24.24s %s (%d:%-3d) CH%02d", i + 1, WiFi.SSID(i).c_str(), WiFi.BSSIDstr(i).c_str(), WiFi.RSSI(i), nrssi, WiFi.channel(i));
      switch(WiFi.encryptionType(i)){
        case WIFI_AUTH_OPEN:
          Serial.println(" OPEN");
          break;
        case WIFI_AUTH_WEP:
          Serial.println(" WEP");
          break;
        case WIFI_AUTH_WPA_PSK:
          Serial.println(" WPA");
          break;
        case WIFI_AUTH_WPA2_PSK:
          Serial.println(" WPA2");
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          Serial.println(" WPA/WPA2");
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          Serial.println(" WPA2_EN");
          break;
        default:
          break;
      }
    }
  }

  WiFi.scanDelete();
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);

  char deviceid[21] = {0};
  uint64_t chipid;
  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  uint16_t idlen = strlen(deviceid);
//  Serial.printf(" ESP Device ID %s %d\r\n", deviceid, idlen);
  char strWIFIap[32] = {0};
  memset(strWIFIap,'\0', 32);
  snprintf(strWIFIap, 32, "NOA_SNACKER_%-6.6s", &deviceid[idlen - 6]);
//  DBGLOG(Info, "AP_SSID %s", strWIFIap);
  const char* ssid1 = strWIFIap;

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid1, password1, 1);  // Set AP SSID and Passwd
//  WiFi.softAPConfig(lxip, lxip1, lxip2);  // Set AP Net parameters

  WiFi.begin(ssid, password);  // Set STA SSID and Passwd
//  WiFi.config(sip, sip1, sip2);  // Set STA Net parameters
  
  Serial.println("Please wait");
//  Serial.println(WiFi.getTxPower());
//  Serial.println(WiFi.softAPSSID().c_str());
//  Serial.println(WiFi.SSID().c_str());
//  Serial.println(WiFi.getHostname());
//  Serial.println(WiFi.softAPgetHostname());
  Serial.printf("STA mac: %s\r\n", WiFi.macAddress().c_str());
  Serial.printf(" AP mac: %s\r\n", WiFi.softAPmacAddress().c_str());
  nStatus_WiFiTesting = 1;

  Iperf_Server.begin(5001);
  uint8_t data_buffer[1024] = {0};
  while(true) {
    // listen for incoming clients
    WiFiClient Iperf_Client = Iperf_Server.available();
    if (Iperf_Client) {
      Serial.printf("%ld Get a New Iperf Client\r\n", millis()/1000);
      while (Iperf_Client.connected()) {
        if (Iperf_Client.available()) {
          Iperf_Client.read(data_buffer, 1024);
//          memset(&data_buffer, 0, SIZE_OF_STACK);
        }
      }
      // close the connection:
      Iperf_Client.stop();
      Serial.printf("%ld Iperf Client disconnected\r\n", millis()/1000);
    }
//    vTaskDelay(1/portTICK_PERIOD_MS);
  }

  DBGLOG(Info, "WIFI_Test_Task_Loop Exit from core %d", xPortGetCoreID());
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
//        DBGLOG(Info, "Net task APP_MSG_TIMER_ID, StackSize %ld", uxTaskGetStackHighWaterMark(NULL));
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
                   tskIDLE_PRIORITY + 1,        // Priority at which the task is created.
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
