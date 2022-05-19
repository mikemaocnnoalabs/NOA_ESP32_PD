/*
  NOA_App.cpp - Library for Main APP..
  Copyright 2022 NOA
  Copyright 2022 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <stdio.h>
#include <string.h>

#include "esp_types.h"
#include <driver/timer.h>
#include <driver/gpio.h>

#include "..\DRV\PDM\usb_pd.h"
#include "..\DRV\PDM\NCP81239.h"
#include "..\LIB\PUB\NOA_debug.h"
#include "..\LIB\PUB\NOA_public.h"

#include "NOA_App.h"
#include "..\PD\NOA_Pd.h"

//****************************************************************************
// APP task
#define SIZE_OF_APP_STACK  SIZE_OF_STACK * 2

TaskHandle_t NOA_App_Task = NULL;
xQueueHandle NOA_APP_TASKQUEUE = NULL;

StaticTask_t xTaskBuffer_App;
StackType_t xStack_App[SIZE_OF_APP_STACK];

//GPIO key task
#define SIZE_OF_GPIO_STACK  SIZE_OF_STACK

TaskHandle_t GPIO_Input_Task = NULL;
static xQueueHandle gpio_evt_queue = NULL;

StaticTask_t xTaskBuffer_GPIOInput;
StackType_t xStack_GPIOInput[SIZE_OF_GPIO_STACK];

static int bpress_key = 0;
static int blift_key = 0;

static int nclick = 0;
static int blong_click = 0;
static int bshort_click = 0;

static int nhold_key = 0;

static int nboot_lattepanda = 0;
static int nauto_powersave = 0;

int bpower_save = 0;
static int bpower_direct_save = 0;

static int nwake_time = 0;
//****************************************************************************
void station_powersave() {
  if (gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
    NOA_Pd_Sink_PowerSave_init();
  }
}

void station_wakeup() {
  if (gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
    gpio_set_level((gpio_num_t)station_db_pin, 1);          // Hold DB pin
    gpio_set_level((gpio_num_t)station_en5v_pin, 0);
    gpio_set_level((gpio_num_t)ncp_bb_con1_en_pin, 1);
    gpio_set_level((gpio_num_t)ncp_bb_con2_en_pin, 1);
    gpio_set_level((gpio_num_t)ncp_bb_con3_en_pin, 1);
    NOA_Pd_init();
  }
}

static void wdg_timer_callback(void* arg) {
//  int64_t time_since_boot = esp_timer_get_time();
//  APP_DEBUG("wdg timer called, time since boot: %lld us", time_since_boot);
  NOA_PUB_MSG msg;
  memset(&msg, 0, sizeof(NOA_PUB_MSG));
  msg.message = APP_MSG_WDG_ID;
  if (NOA_APP_TASKQUEUE != NULL) {
    xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
  }

  static bool toggle0 = false;
  toggle0 = !toggle0;
  if (toggle0 == false) {
    memset(&msg, 0, sizeof(NOA_PUB_MSG));
    msg.message = APP_MSG_TIMER_ID;
    if (NOA_APP_TASKQUEUE != NULL) {
      xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
    }
  }
}

void IRAM_ATTR isr_gpio(void* arg) {
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void gpio_task_loop(void *arg) {
  APP_DEBUG("Start GPIO task gpio_task_loop...");
  uint32_t io_num = 0;
  uint32_t button_time = 0;
  for (;;) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      switch(io_num) {
        case station_button_pin:
          if (nwake_time != 15) {
            APP_DEBUG("Ignore GPIO[%d] intr val:%d in first %d sec when wake up from sleep", io_num, gpio_get_level((gpio_num_t)io_num), nwake_time);
            break;
          }
          if (gpio_get_level((gpio_num_t)io_num) == 0) {
            if (bpress_key == 0) {
              bpress_key = 1;
              button_time = millis();
              gpio_set_level((gpio_num_t)panda_power_pin, 1);
              APP_DEBUG("GPIO[%d] intr, val:%d Button down", io_num, gpio_get_level((gpio_num_t)io_num));
            }
          } else if(bpress_key == 1) {
            APP_DEBUG("S3 %d S4 %d S0 %d autosave %d wakereason %d", gpio_get_level((gpio_num_t)panda_s3_pin), gpio_get_level((gpio_num_t)panda_s4_pin), gpio_get_level((gpio_num_t)panda_s0_pin), nauto_powersave, nwake_reason);
            if (blift_key == 0) {
              blift_key = 1;
              button_time = millis() - button_time;
              gpio_set_level((gpio_num_t)panda_power_pin, 0);
              APP_DEBUG("GPIO[%d] intr, val:%d Button up", io_num, gpio_get_level((gpio_num_t)io_num));
              nhold_key = 0;
            }
          }
          if (bpress_key & blift_key) {
            bpress_key = 0;
            blift_key = 0;
            if (button_time > 1000) {
              APP_DEBUG("Long Click Button, time:%d", button_time);
              blong_click = 1;
            } else {
//              APP_DEBUG("Short Click Button, time:%d", button_time);
              bshort_click = 1;
            }
            NOA_PUB_MSG msg;
            memset(&msg, 0, sizeof(NOA_PUB_MSG));
            msg.message = APP_MSG_KEYCLICK;
            if (bshort_click) {
              msg.param1 = 1;
            }
            if (blong_click) {
              msg.param1 = 2;
            }
            if (NOA_APP_TASKQUEUE != NULL) {
              xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
            }
            blong_click = 0;
            bshort_click = 0;
          }
          break;
        default:
          APP_DEBUG("Error GPIO[%d] intr, val:%d", io_num, gpio_get_level((gpio_num_t)io_num));
          break;
      }
    }
  }
  APP_DEBUG("APP_DEBUG Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}

//****************************************************************************
// CODE TABLES
//****************************************************************************
// MAIN_APP_Task_Loop: Main APP task
void MAIN_APP_Task_Loop(void * pvParameters) {
  APP_DEBUG("MAIN_APP_Task_Loop running on core %d", xPortGetCoreID());
  BaseType_t xStatus = 0;
  NOA_PUB_MSG msg;
  static int nled_blink = 0;  // blink power LED
  static int nled_breath = 0;  // breath power LED

  while(true){
    memset(&msg, 0, sizeof(NOA_PUB_MSG));
    xStatus = xQueueReceive(NOA_APP_TASKQUEUE, &msg, portMAX_DELAY);
    if (xStatus == pdPASS) {
//      APP_DEBUG("MAIN_APP_Task_Loop get a message %d QueueSpaces %d", msg.message, uxQueueSpacesAvailable(NOA_APP_TASK));
    }
    switch(msg.message) {
      case APP_MSG_SRCREADY:
        break;
      case APP_MSG_SRCNOTREADY:
        break;
      case APP_MSG_APREADY:
        break;
      case APP_MSG_APNOTREADY:
        break;
      case APP_MSG_WIRELESSREADY:
        break;
      case APP_MSG_WIRELESSNOTREADY:
        break;
      case APP_MSG_WIFIREADY:
        break;
      case APP_MSG_WIFINOTREADY:
        break;
      case APP_MSG_NFCREADY:
        break;
      case APP_MSG_NFCNOTREADY:
        break;
      case APP_MSG_POWERSAVE:
//        APP_DEBUG("App task APP_MSG_POWERSAVE");
        bpower_save = 1;
        station_powersave();
        break;
      case APP_MSG_WAKEUP:
        break;
      case APP_MSG_WDG_ID:
        if (nwake_reason == 3) { // auto power up lattepanda when wake up from sleep
          if (nwake_time < 15) {
            nwake_time++;
          }
        } else {
          nwake_time = 15;
        }
        if (bpower_save == 0 && bpower_direct_save == 0) {
//          APP_DEBUG("S3 %d S4 %d nboot_lattepanda %d nwake_time %d", gpio_get_level((gpio_num_t)panda_s3_pin), gpio_get_level((gpio_num_t)panda_s4_pin), nboot_lattepanda, nwake_time);
          if (gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {  // Panda is not power up
            if ((nboot_lattepanda >= 6 || nboot_lattepanda == 0) && (nwake_time == 15)) { // skip first 6 secs after panda is boot up, for some board s4 will down in booting step, and Skip 7.5 sec after station2 is wake up, wait it auto boots up panda
              nled_blink = !nled_blink;
              if (nled_blink == 1) {
                gpio_set_level((gpio_num_t)station_powerled_pin, 1);
              } else {
                gpio_set_level((gpio_num_t)station_powerled_pin, 0);
              }
            } else {
              gpio_set_level((gpio_num_t)station_powerled_pin, 1);
            }
          } else { // Panda is power up, LED is long light
            if (gpio_get_level((gpio_num_t)panda_s3_pin) == 1) { // no OS sleep signal
              gpio_set_level((gpio_num_t)station_powerled_pin, 1);
            } else {  // OS sleep
              switch(nled_breath) {
                case 0:
                case 1:
                case 2:
                  gpio_set_level((gpio_num_t)station_powerled_pin, 1);
                  break;
                case 3:
                  gpio_set_level((gpio_num_t)station_powerled_pin, 0);
                  break;
              }
              nled_breath++;
              if (nled_breath == 4) {
                nled_breath = 0;
              }
            }
            if (nboot_lattepanda == 0) {
              nboot_lattepanda = 1;
            }
          }
        } else {
          if (gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
            gpio_set_level((gpio_num_t)station_powerled_pin, 0);
          }
        }
        switch(nwake_time) {
          case 0:
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:  // wait 3.5 secs for P2 board PD connection
            break;
          case 7:  // send 1.5 sec sw signal
            gpio_set_level((gpio_num_t)panda_power_pin, 1);
            break;
          case 8:
          case 9:
            break;
          case 10:
            gpio_set_level((gpio_num_t)panda_power_pin, 0);
            break;
          case 11:
          case 12:
          case 13:
          case 14:
            break;
          default:
            nwake_time = 15;
            break;
        }
        break;
      case APP_MSG_TIMER_ID: {
//        APP_DEBUG("App task APP_MSG_TIMER_ID: %d", nclick);
          if (nclick == 1) {
            nclick = 0;
          }
          if (gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
            if (nboot_lattepanda != 0) {  // if get s4 low signal after panda is booting up
              nboot_lattepanda++;
            }
            if (nboot_lattepanda > 6) {
              nboot_lattepanda = 0;
            }
            nauto_powersave++;
            if (nauto_powersave >= 300 && bpower_save == 0) { // when panda is power off, auto sleep after 5 minutes
              nauto_powersave = 0;
              memset(&msg, 0, sizeof(NOA_PUB_MSG));
              msg.message = APP_MSG_POWERSAVE;
              if (NOA_APP_TASKQUEUE != NULL) {
                xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
              }
            }
          } else {
            nauto_powersave = 0;
          }
          if ((bpress_key & !blift_key)) { // long hold button to sleep
            if (nhold_key != 0) {
              APP_DEBUG("App task APP_MSG_TIMER_ID hold button %d sec", nhold_key);
              if (nhold_key >= 12) {
                if (gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
                  bpower_direct_save = 1;
                  gpio_set_level((gpio_num_t)station_powerled_pin, 0);
                }
              }
            }
            nhold_key++;
          } else {
            nhold_key = 0;
          }
          if (bpower_direct_save == 1) {
            APP_DEBUG("bpower_direct_save %d sec", nhold_key);
            if (gpio_get_level((gpio_num_t)station_button_pin) == 1) {  // Sleep after released button
              nauto_powersave = 0;
              memset(&msg, 0, sizeof(NOA_PUB_MSG));
              msg.message = APP_MSG_POWERSAVE;
              if (NOA_APP_TASKQUEUE != NULL) {
                xQueueSend(NOA_APP_TASKQUEUE, (void *)&msg, (TickType_t)0);
              }
            }
          }
          int nvoltage_p1 = ncp81239_pmic_get_voltage(1);
//          int nvoltage_p2 = ncp81239_pmic_get_voltage(2);
          int nvoltage_p3 = ncp81239_pmic_get_voltage(3);
//          APP_DEBUG("P1 voltage %d P2 voltage %d P3 voltage %d wakefrom %d waketime %d", nvoltage_p1, nvoltage_p2, nvoltage_p3, nwake_reason, nwake_time);
          if (nvoltage_p1 != 50 || nvoltage_p3 != 50) {
            nauto_powersave = 0;
          }
        }
        break;
      case APP_MSG_KEYCLICK:
        nauto_powersave = 0;
        switch(msg.param1) {
          case 1: {
              nclick++;
            }
            break;
          case 2:
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
  APP_DEBUG("MAIN_APP_Task_Loop Exit from core %d", xPortGetCoreID());
  gpio_isr_handler_remove((gpio_num_t)station_button_pin);
  gpio_uninstall_isr_service();
  vTaskDelete(NULL);
}

void NOA_App_init() {
  /* Create one timers:
  * 1. a periodic timer which will run every 0.1s, and print a message
  */
  const esp_timer_create_args_t wdg_timer_args = {
    .callback = &wdg_timer_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "wdgtimer",
    .skip_unhandled_events = true
  };

  esp_timer_handle_t wdg_timer;
  ESP_ERROR_CHECK(esp_timer_create(&wdg_timer_args, &wdg_timer));
  /* The timer has been created but is not running yet */

  /* gpio key button */
//  gpio_wakeup_enable(GPIO_NUM_40, GPIO_INTR_LOW_LEVEL);
//  esp_deep_sleep_enable_gpio_wakeup(GPIO_NUM_40, GPIO_INTR_LOW_LEVEL);

  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add((gpio_num_t)station_button_pin, isr_gpio, (void*) station_button_pin);

  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  if (gpio_evt_queue == NULL) {
    APP_DEBUG("Create gpio_evt_queue fail");
  }

  GPIO_Input_Task = xTaskCreateStatic(
                 gpio_task_loop,                // Function that implements the task.
                 "GPIOInputTask",               // Text name for the task.
                 SIZE_OF_GPIO_STACK,            // Stack size in bytes, not words.
                 NULL,                          // Parameter passed into the task.
                 tskIDLE_PRIORITY + 5,          // Priority at which the task is created.
                 xStack_GPIOInput,              // Array to use as the task's stack.
                 &xTaskBuffer_GPIOInput);       // Variable to hold the task's data structure.
  if (GPIO_Input_Task == NULL) {
    APP_DEBUG("Create gpio_task_loop fail");
  }

  /* APP main task */
  NOA_APP_TASKQUEUE = xQueueCreate(SIZE_OF_TASK_QUEUE, sizeof(NOA_PUB_MSG));
  if (NOA_APP_TASKQUEUE == NULL) {
    APP_DEBUG("Create NOA_APP_TASKQUEUE fail");
  }

  NOA_App_Task = xTaskCreateStatic(
                   MAIN_APP_Task_Loop,        // Function that implements the task.
                   "MAINAPPTask",             // Text name for the task.
                   SIZE_OF_APP_STACK,         // Stack size in bytes, not words.
                   NULL,                      // Parameter passed into the task.
                   tskIDLE_PRIORITY + 1,      // Priority at which the task is created.
                   xStack_App,                // Array to use as the task's stack.
                   &xTaskBuffer_App);         // Variable to hold the task's data structure.
  if (NOA_App_Task == NULL) {
    APP_DEBUG("Create Main_App_Task fail");
  }

  /* Start the timers */
  ESP_ERROR_CHECK(esp_timer_start_periodic(wdg_timer, 500000));  // 0.5S
  APP_DEBUG("Started timers, time since boot: %lld us", esp_timer_get_time());
}
/*********************************FILE END*************************************/

