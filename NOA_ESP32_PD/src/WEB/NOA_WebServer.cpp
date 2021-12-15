/*
  NOA_WebServer.cpp - Library for Web Server
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Esp.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoJson.h>

#include "..\DRV\PDM\usb_pd.h"
#include "..\APP\NOA_App.h"
#include "..\LIB\PUB\NOA_public.h"
#include "..\LIB\PUB\NOA_TimeDefs.h"

#include "NOA_WebPage.h"
#include "NOA_WebServer.h"

#define SIZE_OF_WEBSERVER_STACK  SIZE_OF_STACK * 4

TaskHandle_t NOA_WebServer_Task = NULL;
StaticTask_t xTaskBuffer_WebServer;
StackType_t xStack_WebServer[SIZE_OF_WEBSERVER_STACK];
xQueueHandle NOA_WEBSERVER_TASKQUEUE = NULL;
static int nStatus_WebServer = 0;

String _username = "admin";
String _password = "admin";

extern uint32_t pd_src_caps[CONFIG_USB_PD_PORT_COUNT][PDO_MAX_OBJECTS];
extern uint8_t pd_src_cap_cnt[CONFIG_USB_PD_PORT_COUNT];
extern int pd_source_cap_current_index;
extern int pd_sink_port_default_valtage;
extern int pd_sink_port_default_current;

extern uint32_t pd_src_pdo[PDO_MAX_OBJECTS];
extern int pd_src_pdo_cnt;

extern char strReleaseDate[16];
extern char strReleaseTime[16];

WebServer NOAServer(80);

bool shouldreboot = false; // reboot flag

DynamicJsonDocument docPara(4096);
#define NOA_PARA_SIZE 100

void handleNotFound() {
  NOAServer.send(404, "text/plain", "404: Not found");
}

void handleResponse() {      // callback
  NOAServer.sendHeader("Connection", "close");
  NOAServer.sendHeader("Cache-Control", "no-cache");
  NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
  NOAServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
// NOAServer.sendHeader("Cache-Control", "no-cache");
//      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
//      NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
}

void handleFileupload() {  //callback
  HTTPUpload &upload = NOAServer.upload();  // file upload object
  if (upload.status == UPLOAD_FILE_START) {  // Begin file upload
    DBGLOG(Info, "Begin upload file:%s", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    DBGLOG(Info, "uploaded %d bytes file size ", upload.totalSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    DBGLOG(Info, "Write %d bytes to OTA partition", upload.totalSize);
    DBGLOG(Info, "Upgrade is complete. Device will reboot in 5 seconds!");
    shouldreboot = true;
  } else {
    DBGLOG(Info, "Upload firmware fail: status=%d", upload.status);
  }
}

DynamicJsonDocument docPara_Temp(4096);
void handleResponse_para() {      // callback
//  NOAServer.sendHeader("Connection", "close");
//  NOAServer.sendHeader("Cache-Control", "no-cache");
//  NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
  int nArgs_num = NOAServer.args();
  DBGLOG(Info, "Para Response:%d", nArgs_num);
  if (nArgs_num > 0) {
    DBGLOG(Info, "Para Response:%s", NOAServer.arg((nArgs_num - 1)).c_str());
  } else {
//    NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
    NOAServer.send(200);
    return;
  }
  deserializeJson(docPara_Temp, NOAServer.arg((nArgs_num - 1)).c_str());
  JsonObject rootPara = docPara_Temp.as<JsonObject>();
  JsonObject stringPara = rootPara["string"];
  nArgs_num = stringPara.size();
  if (nArgs_num > 0) {
    DBGLOG(Info, "JsonObject string size: %d", nArgs_num);
  } else {
//    NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
    NOAServer.send(200);
    return;
  }

  // Loop through all the key-value pairs in obj
  String strTemp = "";
  int nTemp_i = 0;
  for (JsonPair pPara : stringPara) {
//    Serial.println(pPara.key().c_str());
//    Serial.println(pPara.value().as<const char *>());
    strTemp = pPara.key().c_str();
    nTemp_i = strTemp.toInt();
//    Serial.println(nTemp_i);
    if (nTemp_i < 0 || nTemp_i > 99) {
      DBGLOG(Error, "Para %d Error!", nTemp_i);
      break;
    }
    strTemp = pPara.value().as<const char *>();
//    Serial.println(strTemp);
    NOA_UpdateParaJson(nTemp_i, (char *)strTemp.c_str());
    if (nTemp_i == 36) {
      NOA_UpdateParaJson(nTemp_i + 1, (char *)strTemp.c_str());
    }
  }
//  for (JsonObject::iterator itPara=stringPara.begin(); itPara!= stringPara.end(); ++ itPara) {    
//    Serial.println(itPara->key().c_str()); // is a JsonString
//    Serial.println(itPara->value().as<const char *>()); // is a JsonValue
//  }
/*  char res[8] = {0};
  int nTemp_i = 0;
  String strTemp = "";
  for(nTemp_i = 0; nTemp_i < NOA_PARA_SIZE; nTemp_i++) {
    memset(res, 0, 8);
    snprintf(res, 8, "%d", nTemp_i);
    if (stringPara.containsKey(res) == true) {
      serializeJson(stringPara, Serial);
      Serial.println();
      JsonVariant VarPara = stringPara.getMember(res);
//      Serial.println(VarPara.as<const char*>());
      strTemp = VarPara.as<const char *>();
      break;
    }
  }
  Serial.println(nTemp_i);
  Serial.println((char *)strTemp.c_str());
  NOA_UpdateParaJson(nTemp_i, (char *)strTemp.c_str());
  if (nTemp_i == 36) {
    NOA_UpdateParaJson(nTemp_i + 1, (char *)strTemp.c_str());
  }*/

  docPara_Temp.clear();
  Serial.printf("docPara_Temp is %d after Response para\r\n", !(docPara_Temp.isNull()));

  char lenPara[32] = {0};
  memset(lenPara, 0, 32);
  snprintf(lenPara, 32, "%d", strlen(system_para_page_file));
//  NOAServer.sendHeader("Content-length:", lenPara);
//  NOAServer.send_P(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
}

void handleUpload_para() {  //callback
  DBGLOG(Info, "Para upload:%d", NOAServer.args());
}

//****************************************************************************
// CODE TABLES
//****************************************************************************
// NOA_WebServer_Task_Loop: Wifi Web Server task
void NOA_WebServer_Task_Loop(void * pvParameters) {
  DBGLOG(Info, "NOA_WebServer_Task_Loop running on core %d", xPortGetCoreID());

  uint32_t nTemp_new = millis() / 1000;
  uint32_t nTemp_old = 0;
  char localtime[24] = {0};
  
  while(true){
    if (nStatus_WebServer == 1) {
      NOAServer.handleClient();
      nTemp_new = millis() / 1000;
      if (nTemp_new != nTemp_old) {
        memset(localtime, 0, 24);
//        sprintf(localtime, "%d", nTemp_new);
        sprintf(localtime, "%d %02d:%02d:%02d", nTemp_new/(3600 * 24), (nTemp_new/3600)%24, (nTemp_new%3600)/60, (nTemp_new%3600)%60);
        NOA_UpdateParaJson(26, (char *)localtime);
        nTemp_old = nTemp_new;
      }
    }
    vTaskDelay(1/portTICK_PERIOD_MS);
  }
  DBGLOG(Info, "NOA_WebServer_Task_Loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}

void NOA_ParserParaJson() {
  deserializeJson(docPara, system_para_page_file);
//  serializeJson(docPara, Serial);
//  Serial.println();
//  Serial.println("root");
  JsonObject rootPara = docPara.as<JsonObject>();
//  serializeJson(rootPara, Serial);
//  Serial.println();
//  Serial.println(docPara.memoryUsage());
//  Serial.println(measureJson(docPara));
//  Serial.println(measureJsonPretty(docPara));
  Serial.println("System para root has a JsonObject string:");
  JsonObject stringPara = rootPara["string"];
  serializeJson(stringPara, Serial);
  Serial.println();
//  Serial.println(stringPara.memoryUsage());
//  Serial.println(measureJson(stringPara));
//  Serial.println(measureJsonPretty(stringPara));

  char res[8] = {0};
  int nTemp_i = 0;
  for(nTemp_i = 0; nTemp_i < NOA_PARA_SIZE; nTemp_i++) {
    memset(res, 0, 8);
    snprintf(res, 8, "%d", nTemp_i);
    serializeJson(stringPara[res], Serial);
  }
  docPara.clear();
  Serial.println();
  Serial.printf("docPara is %d\r\n", !(docPara.isNull()));
}

void NOA_GeneratorParaJson() {
  JsonObject rootPara = docPara.to<JsonObject>();
  JsonObject stringPara = rootPara.createNestedObject("string");
  char res[8] = {0};
  int nTemp_i = 0;
  Serial.printf("Sink cap cnt %d index %d\r\n", pd_src_cap_cnt[0], pd_source_cap_current_index);
  Serial.printf("Sink default mv %d ma %d\r\n", pd_sink_port_default_valtage, pd_sink_port_default_current);
  for(nTemp_i = 0; nTemp_i < NOA_PARA_SIZE; nTemp_i++) {
    memset(res, 0, 8);
    snprintf(res, 8, "%d", nTemp_i);
    switch(nTemp_i) {
      case 2: {  // SN
          char deviceid[21] = {0};
          uint64_t chipid;
          chipid = ESP.getEfuseMac();
          sprintf(deviceid, "%" PRIu64, chipid);
          stringPara[res] = deviceid;
        }
        break;
      case 5:  // FW version
        stringPara[res] = NOA_ESP32_PD_VERSION;
        break;
      case 6: {  // HD version
          char devicemode[32] = {0};
          sprintf(devicemode, "%s Revision %d", ESP.getChipModel(), ESP.getChipRevision());
          stringPara[res] = devicemode;
        }
        break;
      case 11: {  // AP load
          char apload[8] = {0};
          uint8_t apstanum = WiFi.softAPgetStationNum() - 1;
          sprintf(apload, "%d", apstanum);
          stringPara[res] = apload;
        }
        break;
      case 16:  // STA IP
        stringPara[res] = WiFi.localIP();
        break;
      case 17:  // STA Submask
        stringPara[res] = WiFi.subnetMask();
        break;
      case 18:  // STA Gateway
        stringPara[res] = WiFi.gatewayIP();
        break;
      case 19:  // STA DNS1
        stringPara[res] = WiFi.dnsIP(0);
        break;
      case 22:  // STA DNS2
        stringPara[res] = WiFi.dnsIP(1);
        break;
      case 27:  // WIFI Load
        stringPara[res] = "0";
        break;
      case 28: {  // PD SINK Voltage
          char pdsnkvoltage[8] = {0};
          sprintf(pdsnkvoltage, "%d", pd_sink_port_default_valtage);
          stringPara[res] = pdsnkvoltage;
        }
        break;
      case 29: {  // PD SINK Current
          char pdsnkvoltage[8] = {0};
          sprintf(pdsnkvoltage, "%d", pd_sink_port_default_current);
          stringPara[res] = pdsnkvoltage;
        }
        break;
      case 30: {  // PD SINK Capabilities
          if (pd_src_cap_cnt[0] != 0) {
            char pdsnkcapabilities[256] = {0};
            char strTemp[24] = {0};
            int nTemp_j = 0;
            for(nTemp_j = 0; nTemp_j < pd_src_cap_cnt[0]; nTemp_j++) {
              Serial.printf("Sink cap %d %d:%d \r\n", nTemp_j, ((pd_src_caps[0][nTemp_j] >> 10) & 0x3ff) * 50, (pd_src_caps[0][nTemp_j] & 0x3ff) * 10);
              memset(strTemp, 0, 16);
              sprintf(strTemp, "[%d:%d]", ((pd_src_caps[0][nTemp_j] >> 10) & 0x3ff) * 50, (pd_src_caps[0][nTemp_j] & 0x3ff) * 10);
              if (nTemp_j == 0) {
                strcpy(pdsnkcapabilities, strTemp);
              } else {
                strcat(pdsnkcapabilities, strTemp);
              }
            }
            stringPara[res] = pdsnkcapabilities;
          } else {
            stringPara[res] = "[-]";
          }
        }
        break;
      case 31:  // PD SRC Voltage
        stringPara[res] = "0";
        break;
      case 32:  // PD SRC Current
        stringPara[res] = "0";
        break;
      case 33:  // PD SRC Capabilities
        stringPara[res] = "0";
        break;
      case 34:  // Wireless charge Voltage
        stringPara[res] = "0";
        break;
      case 35:  // OTA status
        stringPara[res] = "0";
        break;
      case 36:  // OTA status
        stringPara[res] = "0";
        break;
      case 37:  // OTA status
        stringPara[res] = "0";
        break;
      case 41: {  // STA SSID
          if (WiFi.isConnected()) {
            stringPara[res] = WiFi.SSID();
          } else {
            stringPara[res] = "";
          }
        }
        break;
      case 44:  // STA MAC
        stringPara[res] = WiFi.macAddress();
        break;
      case 46:  // AP SSID
        stringPara[res] = WiFi.softAPSSID();
        break;
      case 47:  // AP PASS
        stringPara[res] = "87654321";
        break;
      case 48: {  // RSSI
          int nrssi = 2 * (WiFi.RSSI() + 100);
          if (nrssi > 100) {
            nrssi = 100;
          }
          char wifirssi[16] = {0};
          sprintf(wifirssi, "%d:%d", WiFi.RSSI(),nrssi);
          stringPara[res] = wifirssi;
        }
        break;
      case 51: {  // Release Date
//          char buildingdate[16] = {0};
//          sprintf(buildingdate, "%04d%02d%02d", BUILD_DATE_YEAR_INT,BUILD_DATE_MONTH_INT,BUILD_DATE_DAY_INT);
          stringPara[res] = strReleaseDate;
        }
        break;
      case 52: {  // Release time
//          char buildingtime[8] = {0};
//          sprintf(buildingtime, "%02d%02d%02d", BUILD_TIME_HOURS_INT,BUILD_TIME_MINUTES_INT,BUILD_TIME_SECONDS_INT);
          stringPara[res] = strReleaseTime;
        }
        break;
      case 56:  // AP MAC
        stringPara[res] = WiFi.softAPmacAddress();
        break;
      case 57:  // AP IP
        stringPara[res] = WiFi.softAPIP();
        break;
      default:
        stringPara[res] = res;
        break;
    }
//    serializeJson(stringPara[res], Serial);
  }
//  Serial.println();
//  Serial.println("string:");
//  serializeJson(stringPara, Serial);
//  Serial.println();
//  Serial.println("root:");
//  serializeJson(rootPara, Serial);
//  Serial.println();
//  Serial.println("doc:");
//  serializeJson(docPara, Serial);
  char strPara[measureJson(docPara) + 1] = {0};
  serializeJson(docPara, strPara, measureJson(docPara) + 1);
//  Serial.println(strPara);
  memset(system_para_page_file, 0, 4096);
  memcpy(system_para_page_file, strPara, measureJson(docPara) + 1);
  docPara.clear();
//  Serial.println();
  Serial.printf("docPara is %d again\r\n", !(docPara.isNull()));
}

void NOA_UpdateParaJson(uint8_t nIndex, char *strValue) {
  if (nIndex >= NOA_PARA_SIZE) {
    return;
  }
  deserializeJson(docPara, system_para_page_file);
  JsonObject rootPara = docPara.as<JsonObject>();
//  Serial.println("System para root has a JsonObject string for updating:");
  JsonObject stringPara = rootPara["string"];
//  serializeJson(stringPara, Serial);
//  Serial.println();
  char res[8] = {0};
  if (nIndex == 11 || nIndex == 16 || nIndex == 17 || nIndex == 18 || nIndex == 19 || nIndex == 2 ||\
    nIndex == 27 || nIndex == 28 || nIndex == 29 || nIndex == 30 || nIndex == 31 || nIndex == 32 ||\
    nIndex == 33 || nIndex == 34 || nIndex == 41 || nIndex == 44 || nIndex == 48) { // Skip auto updated paras
    Serial.println("Auto update parameters");
  } else {
    memset(res, 0, 8);
    snprintf(res, 8, "%d", nIndex);
    stringPara[res] =  strValue;
  }

  int nTemp_i = 0;
  char strTemp[16] = {0};
  char strNewValue[128] = {0};
  for(nTemp_i = 0; nTemp_i < NOA_PARA_SIZE; nTemp_i++) {
    memset(res, 0, 8);
    snprintf(res, 8, "%d", nTemp_i);
    memset(strTemp, 0, 16);
    memset(strNewValue, 0, 128);
    switch(nTemp_i) {
      case 11: {  // AP load
          uint8_t apstanum = WiFi.softAPgetStationNum();
          if (apstanum != 0) {
            apstanum = apstanum - 1;
          }
          sprintf(strTemp, "%d", apstanum);
          stringPara[res] = strTemp;
        }
        break;
      case 16:  // STA IP
        stringPara[res] = WiFi.localIP();
        break;
      case 17:  // STA Submask
        stringPara[res] = WiFi.subnetMask();
        break;
      case 18:  // STA Gateway
        stringPara[res] = WiFi.gatewayIP();
        break;
      case 19:  // STA DNS1
        stringPara[res] = WiFi.dnsIP(0);
        break;
      case 22:  // STA DNS2
        stringPara[res] = WiFi.dnsIP(1);
        break;
      case 27:  // WIFI Load
        stringPara[res] = "0";
        break;
      case 28: {  // PD SINK Voltage
          sprintf(strTemp, "%d", pd_sink_port_default_valtage);
          stringPara[res] = strTemp;
        }
        break;
      case 29: {  // PD SINK Current
          sprintf(strTemp, "%d", pd_sink_port_default_current);
          stringPara[res] = strTemp;
        }
        break;
      case 30: {  // PD SINK Capabilities
          if (pd_src_cap_cnt[0] != 0) {
            int nTemp_j = 0;
            for(nTemp_j = 0; nTemp_j < pd_src_cap_cnt[0]; nTemp_j++) {
//              Serial.printf("Sink cap %d %d:%d \r\n", nTemp_j, ((pd_src_caps[0][nTemp_j] >> 10) & 0x3ff) * 50, (pd_src_caps[0][nTemp_j] & 0x3ff) * 10);
              memset(strTemp, 0, 16);
              sprintf(strTemp, "[%d:%d]", ((pd_src_caps[0][nTemp_j] >> 10) & 0x3ff) * 50, (pd_src_caps[0][nTemp_j] & 0x3ff) * 10);
              if (nTemp_j == 0) {
                strcpy(strNewValue, strTemp);
              } else {
                strcat(strNewValue, strTemp);
              }
            }
            stringPara[res] = strNewValue;
          } else {
            stringPara[res] = "[-]";
          }
        }
        break;
      case 31:  // PD SRC Voltage
        stringPara[res] = "0";
        break;
      case 32:  // PD SRC Current
        stringPara[res] = "0";
        break;
      case 33:  // PD SRC Capabilities
        stringPara[res] = "0";
        break;
      case 34:  // Wireless charge Voltage
        stringPara[res] = "0";
        break;
      case 41: {  // STA SSID
          if (WiFi.isConnected()) {
            stringPara[res] = WiFi.SSID();
          } else {
            stringPara[res] = "";
          }
        }
        break;
      case 44:  // STA MAC
        stringPara[res] = WiFi.macAddress();
        break;
      case 48: {  // RSSI
          int nrssi = 2 * (WiFi.RSSI() + 100);
          if (nrssi > 100) {
            nrssi = 100;
          }
          sprintf(strTemp, "%d:%d", WiFi.RSSI(),nrssi);
          stringPara[res] = strTemp;
        }
        break;
      default:
        break;
    }
  }

  int lendoc = measureJson(docPara) + 1;
  char strPara[lendoc] = {0};
  serializeJson(docPara, strPara, lendoc);
  docPara.clear();
//  Serial.printf("docPara is %d after update\r\n", !(docPara.isNull()));
//  Serial.println(strPara);

  memset(system_para_page_file, 0, 4096);
  memcpy(system_para_page_file, strPara, lendoc);
//  Serial.println(system_para_page_file);
  NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
}

void NOA_WebServer_init() {
//  NOA_ParserParaJson();
  NOA_GeneratorParaJson();
  nStatus_WebServer = 0;
  NOA_WebServer_Task = xTaskCreateStaticPinnedToCore(
                   NOA_WebServer_Task_Loop,        // Function that implements the task.
                   "NOAWebServerTask",             // Text name for the task.
                   SIZE_OF_WEBSERVER_STACK,         // Stack size in bytes, not words.
                   NULL,                            // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,            // Priority at which the task is created.
                   xStack_WebServer,                // Array to use as the task's stack.
                   &xTaskBuffer_WebServer,          // Variable to hold the task's data structure.
                   ARDUINO_RUNNING_CORE);
  if (NOA_WebServer_Task == NULL || &xTaskBuffer_WebServer == NULL) {
    DBGLOG(Error, "Create NOA_WebServer_Task fail");
  }
  vTaskDelay(1000/portTICK_PERIOD_MS);
  if (nStatus_WebServer == 0) {
//    NOAServer.on("/", HTTP_GET, []() {
//    NOAServer.sendHeader("Connection", "close");
//    NOAServer.send(200, "text/html", indexhtml); // send webpage
//    });
//    NOAServer.on("/update", HTTP_POST, handleResponse, handleFileupload); // bind callback 
    NOAServer.on("/cjson/system_para.json", HTTP_GET, [&]() {
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      char lenPara[32] = {0};
      memset(lenPara, 0, 32);
      snprintf(lenPara, 32, "%d", strlen(system_para_page_file));
      NOAServer.sendHeader("Content-length:", lenPara);
      NOAServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
//      NOAServer.sendContent_P(system_para_page_file);
      NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
    });

//    NOAServer.on("/cjson/system_para.json", HTTP_POST, [&]() {
//      NOAServer.sendHeader("Cache-Control", "no-cache");
//      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
//      NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
//    });
    NOAServer.on("/cjson/system_para.json", HTTP_POST, handleResponse_para, handleUpload_para);
//    NOAServer.on("/update", HTTP_POST, handleResponse, handleFileupload);

    NOAServer.on("/lang/cn.json", HTTP_GET, [&]() {
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("application/json;charset=utf-8"), cn_page_file);
    });

    NOAServer.on("/lang/en.json", HTTP_GET, [&]() {
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("application/json;charset=utf-8"), en_page_file);
    });

    NOAServer.on("/", HTTP_GET, [&]() {
      if (_username != emptyString && _password != emptyString && !NOAServer.authenticate(_username.c_str(), _password.c_str())) {
        return NOAServer.requestAuthentication();
      } else {
        NOAServer.sendHeader("device", "NOA-Snacker-ESP-32");
        NOAServer.sendHeader("Cache-Control", "no-cache");
        NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
        NOAServer.send_P(200, PSTR("text/html;charset=utf-8"), ServerIndex);
      }
    });

    NOAServer.on("/SystemInfo.html", HTTP_GET, [&]() {
      NOAServer.sendHeader("device", "NOA-Snacker-ESP-32");
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("text/html;charset=utf-8"), SystemInfo_page_file);
    });

    NOAServer.on("/wifiAPSet.html", HTTP_GET, [&]() {
      NOAServer.sendHeader("device", "NOA-Snacker-ESP-32");
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("text/html;charset=utf-8"), wifiAPSet_page_file);
    });

    NOAServer.on("/STASettings.html", HTTP_GET, [&]() {
      NOAServer.sendHeader("device", "NOA-Snacker-ESP-32");
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("text/html;charset=utf-8"), STASettings_page_file);
    });

    NOAServer.on("/DeviceMonitor.html", HTTP_GET, [&]() {
      NOAServer.sendHeader("device", "NOA-Snacker-ESP-32");
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("text/html;charset=utf-8"), DeviceMonitor_page_file);
    });

    NOAServer.on("/firmware.html", HTTP_GET, [&]() {
      NOAServer.sendHeader("device", "NOA-Snacker-ESP-32");
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.send_P(200, PSTR("text/html;charset=utf-8"), firmware_page_file);
    });

    NOAServer.on("/update", HTTP_POST, handleResponse, handleFileupload);

    NOAServer.onNotFound(handleNotFound);

//    NOAServer.on("/get_ota_status", HTTP_POST, [&]() {
//      NOAServer.sendHeader("Cache-Control", "no-cache");
//      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
//      NOAServer.send(200, PSTR("text/plain;charset=utf-8"), get_ota_status);
//    });

    NOAServer.begin(); //enable Webserver
    nStatus_WebServer = 1;
    DBGLOG(Info, "Web Server started");
  }
}
