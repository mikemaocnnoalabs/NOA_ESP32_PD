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
#include "..\LIB\PUB\NOA_syspara.h"
#include "..\LIB\PUB\NOA_parameter_table.h"

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
    NOA_Parametr_Set(nTemp_i, (char *)strTemp.c_str());
    if (nTemp_i == 36) {
      NOA_Parametr_Set(nTemp_i + 1, (char *)strTemp.c_str());
    }
  }
//  for (JsonObject::iterator itPara=stringPara.begin(); itPara!= stringPara.end(); ++ itPara) {    
//    Serial.println(itPara->key().c_str()); // is a JsonString
//    Serial.println(itPara->value().as<const char *>()); // is a JsonValue
//  }
  docPara_Temp.clear();
  DBGLOG(Info, "docPara_Temp is %d after Response para", !(docPara_Temp.isNull()));

//  char lenPara[32] = {0};
//  memset(lenPara, 0, 32);
//  snprintf(lenPara, 32, "%d", strlen(system_para_page_file));
//  NOAServer.sendHeader("Content-length:", lenPara);
  NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
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
        NOA_Parametr_Set(26, (char *)localtime);
        NOA_UpdateParaJson();
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
  char strValue[NOA_PARAM_LEN] = {0};
//  Serial.printf("Sink cap cnt %d index %d\r\n", pd_src_cap_cnt[0], pd_source_cap_current_index);
//  Serial.printf("Sink default mv %d ma %d\r\n", pd_sink_port_default_valtage, pd_sink_port_default_current);
  for(nTemp_i = 0; nTemp_i < NOA_PARA_SIZE; nTemp_i++) {
    memset(res, 0, 8);
    memset(strValue, 0, NOA_PARAM_LEN);
    snprintf(res, 8, "%d", nTemp_i);
    NOA_Parametr_Get(nTemp_i, (char *)&strValue);
    if (strlen(strValue) > 0) {
      stringPara[res] = strValue;
    } else {
      stringPara[res] = res;
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
  Serial.println(strPara);
  memset(system_para_page_file, 0, 4096);
  memcpy(system_para_page_file, strPara, measureJson(docPara) + 1);
  docPara.clear();
  DBGLOG(Info, "docPara is %d", !(docPara.isNull()));
}

void NOA_UpdateParaJson() {
  JsonObject rootPara = docPara.to<JsonObject>();
  JsonObject stringPara = rootPara.createNestedObject("string");
  char res[8] = {0};
  int nTemp_i = 0;

  char strValue[NOA_PARAM_LEN] = {0};
  for(nTemp_i = 0; nTemp_i < NOA_PARA_SIZE; nTemp_i++) {
    memset(res, 0, 8);
    memset(strValue, 0, NOA_PARAM_LEN);
    snprintf(res, 8, "%d", nTemp_i);
    NOA_Parametr_Get(nTemp_i, (char *)&strValue);
    if (strlen(strValue) > 0) {
      stringPara[res] = strValue;
    } else {
      stringPara[res] = res;
    }
  }  
  char strPara[measureJson(docPara) + 1] = {0};
  serializeJson(docPara, strPara, measureJson(docPara) + 1);
//  Serial.println(strPara);
  memset(system_para_page_file, 0, 4096);
  memcpy(system_para_page_file, strPara, measureJson(docPara) + 1);
  docPara.clear();
//  DBGLOG(Info, "docPara is %d again", !(docPara.isNull()));
//  Serial.println(system_para_page_file);
//  NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file); 
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

//    NOAServer.on("/cjson/system_para.json", HTTP_POST, [&]() {
//      NOAServer.sendHeader("Cache-Control", "no-cache");
//      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
//      NOAServer.send(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
//    });
//    NOAServer.on("/update", HTTP_POST, handleResponse, handleFileupload);

    NOAServer.on("/cjson/system_para.json", HTTP_GET, [&]() {
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
//      char lenPara[32] = {0};
//      memset(lenPara, 0, 32);
//      snprintf(lenPara, 32, "%d", strlen(system_para_page_file));
//      NOAServer.sendHeader("Content-length:", lenPara);
      NOAServer.setContentLength(strlen(system_para_page_file));
//      NOAServer.sendContent_P(system_para_page_file);
      NOAServer.send_P(200, PSTR("application/json;charset=utf-8"), system_para_page_file);
    });
    NOAServer.on("/lang/cn.json", HTTP_GET, [&]() {
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.setContentLength(strlen(cn_page_file));
      NOAServer.send_P(200, PSTR("application/json;charset=utf-8"), cn_page_file);
    });

    NOAServer.on("/lang/en.json", HTTP_GET, [&]() {
      NOAServer.sendHeader("Cache-Control", "no-cache");
      NOAServer.sendHeader("X-Content-Type-Options", "nosniff");
      NOAServer.setContentLength(strlen(en_page_file));
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
    NOAServer.on("/cjson/system_para.json", HTTP_POST, handleResponse_para, handleUpload_para);

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
