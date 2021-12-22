/*
  NOA_Net.cpp - Library for Net APP..
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Esp.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "..\LIB\PUB\NOA_public.h"
#include "..\LIB\PUB\NOA_syspara.h"

#include "..\DRV\PDM\usb_pd.h"
#include "..\APP\NOA_App.h"
#include "..\WEB\NOA_WebServer.h"

#include "NOA_Net.h"

#define SIZE_OF_NET_STACK  SIZE_OF_STACK * 4

TaskHandle_t NOA_Net_Task = NULL;
StaticTask_t xTaskBuffer_Net;
StackType_t xStack_Net[SIZE_OF_NET_STACK];
xQueueHandle NOA_NET_TASKQUEUE = NULL;
static int nStatus_WiFiAP = 0;
static int nStatus_WiFiSTA = 0;
static int nStatus_WiFiWebServer = 0;

TaskHandle_t WIFI_Test_Task = NULL;
StaticTask_t xTaskBuffer_WIFI_Test;
StackType_t xStack_WIFI_Test[SIZE_OF_NET_STACK];
static int nStatus_WiFiTesting = 0;

TaskHandle_t WIFI_Http_Task = NULL;
StaticTask_t xTaskBuffer_WIFI_Http;
StackType_t xStack_WIFI_Http[SIZE_OF_NET_STACK];
static int nStatus_WiFiHttp = 0;
// const char* http_url="https://blob.wi-whisper.com/videos/video0_480_272.avi";
// const char* http_url="https://downloads.arduino.cc/arduino-1.8.16-windows.zip";
// const char* http_url="https://mirrors.gigenet.com/ubuntu/20.04.3/ubuntu-20.04.3-live-server-amd64.iso";
// const char* http_url="https://mirrors.tuna.tsinghua.edu.cn/ubuntu-releases/21.04/ubuntu-21.04-live-server-amd64.iso";

const char* http_url="http://192.168.1.102/video0_480_272.avi";

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
//const char* ssid="KSK_Eden";         //default SSID for Sta
//const char* password="EdenTest123";  //passwd for default SSID

// const char* ssid="NOA Labs (2.4GHz)";     //default SSID for Sta
// const char* password="noa-labs.com";  //passwd for default SSID

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
    case SYSTEM_EVENT_AP_START: {
        Serial.println("AP Started");
        WiFi.softAPConfig(lxip, lxip1, lxip2);  // Set AP Net parameters
//      WiFi.softAPsetHostname(NOA_ESP32_HOST_NAME);
//      WiFi.setHostname(NOA_ESP32_HOST_NAME);
        String strAPSSID = WiFi.softAPSSID();
        WiFi.softAPsetHostname(strAPSSID.c_str());
        WiFi.setHostname(strAPSSID.c_str());
        msg.message = APNET_MSG_READY;
        if (NOA_APP_TASKQUEUE != NULL) {
          xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
        }
        nStatus_WiFiAP = 1;
      }
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("AP Stopped");
      msg.message = APNET_MSG_NOTREADY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      nStatus_WiFiAP = 0;
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("AP Sta Connected");
      nAP_Sta_Numbers = WiFi.softAPgetStationNum();
//      if (nAP_Sta_Numbers > 0) {
//        nAP_Sta_Numbers = nAP_Sta_Numbers - 1;
//      }
      Serial.print("AP Sta Num:");
      Serial.println(nAP_Sta_Numbers);
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("AP Sta Disconnected");
      nAP_Sta_Numbers = WiFi.softAPgetStationNum();
//      if (nAP_Sta_Numbers > 0) {
//        nAP_Sta_Numbers = nAP_Sta_Numbers - 1;
//      }
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
      Serial.println("STA IPv4: ");
//      Serial.println(WiFi.localIP());
//      Serial.println(WiFi.subnetMask());
//      Serial.println(WiFi.gatewayIP());
//      Serial.println(WiFi.dnsIP(0));
//      Serial.println(WiFi.dnsIP(1));
      msg.message = NET_MSG_READY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      nStatus_WiFiSTA = 1;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("STA Disconnected");
      msg.message = NET_MSG_NOTREADY;
      if (NOA_APP_TASKQUEUE != NULL) {
        xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
      }
      nStatus_WiFiSTA = 0;
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("STA Stopped");
      break;
    case SYSTEM_EVENT_WIFI_READY:
      Serial.println("WIFI Ready");
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
  
  Serial.println("Please wait for Iperf client...");
//  Serial.println(WiFi.getTxPower());
//  Serial.println(WiFi.softAPSSID().c_str());
//  Serial.println(WiFi.SSID().c_str());
//  Serial.println(WiFi.getHostname());
//  Serial.println(WiFi.softAPgetHostname());
//  Serial.printf("STA mac: %s\r\n", WiFi.macAddress().c_str());
//  Serial.printf(" AP mac: %s\r\n", WiFi.softAPmacAddress().c_str());
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

#define NOA_HTTP_BUFFER_SIZE  512
uint8_t http_buff[NOA_HTTP_BUFFER_SIZE] = { 0 };
// WIFI_Http_Task_Loop: Wifi http download task
void WIFI_Http_Task_Loop(void * pvParameters) {
  DBGLOG(Info, "WIFI_Http_Task_Loop running on core %d", xPortGetCoreID());
  HTTPClient http_down;
  int32_t  http_down_size = -1;
  WiFiClient *http_stream = NULL;
  while(true){
    if (nStatus_WiFiSTA == 1 && nStatus_WiFiHttp == 1) {
      DBGLOG(Info, "Http Download begin %d", millis()/1000);
      http_down.begin(http_url);
      http_down.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      DBGLOG(Info, "Http Download GET %d", millis()/1000);
      int httpCode = http_down.GET();
      if(httpCode > 0) {
        DBGLOG(Info, "Http Download GET code %d", httpCode);
        if(httpCode == HTTP_CODE_OK) {
          // get lenght of document (is -1 when Server sends no Content-Length header)
          http_down_size = http_down.getSize();
          DBGLOG(Info, "Http Download file size %d", http_down_size);
          // get tcp stream
          http_stream = http_down.getStreamPtr();
          uint32_t nTemp_new = millis() / 1000;
          uint32_t nTemp_old = 0;
          uint32_t nDownload_size = 0;
          uint32_t nDownload_size_old = 0;
          uint32_t stream_size = 0;
          uint32_t nReadnumbers = 0;
          // read all data from server
          while(http_down.connected() && (http_down_size > 0 || http_down_size == -1)) {
            stream_size = http_stream->available();
            if(stream_size) {
              nTemp_new = millis() / 1000;
              nReadnumbers = http_stream->readBytes(http_buff, ((stream_size > sizeof(http_buff)) ? sizeof(http_buff) : stream_size));;
              nDownload_size = nDownload_size + nReadnumbers;
              if (nTemp_new >= (nTemp_old + 5)) {
                DBGLOG(Info, "%d %d %d %d/s %d-%d",nTemp_new, stream_size, nReadnumbers, (nDownload_size - nDownload_size_old)/5, nDownload_size, http_down_size);
                nTemp_old = nTemp_new;
                nDownload_size_old = nDownload_size;
              }
              if (http_down_size > 0) {
                http_down_size = http_down_size - nReadnumbers;
              }
            }
          }
          DBGLOG(Info, "Http Download closed or file end %d.", nDownload_size);
        }
      } else {
        DBGLOG(Error, "Http Download GET failed, error: %s", http_down.errorToString(httpCode).c_str());
      }
      http_down.end();
    }
    vTaskDelay(1/portTICK_PERIOD_MS);
  }
  DBGLOG(Info, "WIFI_Http_Task_Loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}

// Net_APP_Task_Loop: Net APP task
void Net_APP_Task_Loop(void * pvParameters) {
  DBGLOG(Info, "Net_APP_Task_Loop running on core %d", xPortGetCoreID());
  BaseType_t xStatus = 0;
  NOA_PUB_MSG msg;

  while(true){
    memset(&msg, 0, sizeof(NOA_PUB_MSG));
    xStatus = xQueueReceive(NOA_NET_TASKQUEUE, &msg, portMAX_DELAY);
    if (xStatus == pdPASS) {
//    DBGLOG(Info, "Net_APP_Task_Loop get a message %d QueueSpaces %d", msg.message, uxQueueSpacesAvailable(NOA_NET_TASK));
    }
    switch(msg.message) {
      case APP_MSG_TIMER_ID:
//        DBGLOG(Info, "Net task APP_MSG_TIMER_ID, StackSize %ld", uxTaskGetStackHighWaterMark(NULL));
        if (nStatus_WiFiAP == 1 || nStatus_WiFiSTA == 1) {
          if (nStatus_WiFiWebServer == 0) {
            nStatus_WiFiWebServer = 1;
//            NOA_SysPara_init();
            NOA_Get_All_Data();
            NOA_WebServer_init();
          }
        }
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
  vTaskDelay(1000/portTICK_PERIOD_MS);
  nStatus_WiFiHttp = 0;
  WIFI_Http_Task = xTaskCreateStaticPinnedToCore(
                   WIFI_Http_Task_Loop,       // Function that implements the task.
                   "WIFIHttpTask",          // Text name for the task.
                   SIZE_OF_NET_STACK,       // Stack size in bytes, not words.
                   NULL,                    // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,        // Priority at which the task is created.
                   xStack_WIFI_Http,          // Array to use as the task's stack.
                   &xTaskBuffer_WIFI_Http,   // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);
  if (WIFI_Http_Task == NULL || &xTaskBuffer_WIFI_Http == NULL) {
    DBGLOG(Error, "Create WIFI_Http_Task fail");
  }
  vTaskDelay(1000/portTICK_PERIOD_MS);
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
  nStatus_WiFiWebServer = 0;
}
