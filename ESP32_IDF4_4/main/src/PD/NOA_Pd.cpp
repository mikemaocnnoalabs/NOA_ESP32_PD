/*
  NOA_Pd.cpp - Library for PD Discovery
  Copyright 2022 NOA
  Copyright 2022 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <stdio.h>
#include <string.h>

#include "esp_types.h"
#include <esp_sleep.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
// #include <driver/adc.h>
#include <esp_pm.h>

#include "..\DRV\PDM\usb_pd.h"
#include "..\DRV\PDM\FUSB302.h"
#include "..\DRV\PDM\NCP81239.h"

#include "..\LIB\PUB\NOA_debug.h"
#include "..\LIB\PUB\NOA_public.h"

#include "NOA_Pd.h"
//****************************************************************************
#ifdef NOA_PD_STATION2
// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {1, fusb302_I2C_SLAVE_ADDR_B01, &fusb302_tcpm_drv, TCPC_ALERT_ACTIVE_LOW},   // SNK(C0)(P1) bus1
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv, TCPC_ALERT_ACTIVE_LOW},       // SRC(C1)(P2) bus0
  {1, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv, TCPC_ALERT_ACTIVE_LOW},       // SRC(C2)(P0 UP) bus1
  {0, fusb302_I2C_SLAVE_ADDR_B01, &fusb302_tcpm_drv, TCPC_ALERT_ACTIVE_LOW},   // SRC(C3)(P3) bus0
};
// USB-C Specific - TCPM end 1

const uint8_t P1D_ADDR = 0x23;

const uint8_t PUPD_ADDR = 0x22;
const uint8_t PUPM_ADDR = 0x74;

const uint8_t P2D_ADDR = 0x22;
const uint8_t P2M_ADDR = 0x74;

const uint8_t P3D_ADDR = 0x23;
const uint8_t P3M_ADDR = 0x75;
#endif

int pd_source_cap_current_index = 0, pd_source_cap_max_index = 0;
static int pd_sink_port_ready = 0;
static int pd_source_port_ready = 0;
int pd_sink_port_default_valtage = 0;
int pd_sink_port_default_current = 0;

int pd_source_port_default_valtage = 0;
int pd_source_port_default_current = 0;
//****************************************************************************
#define SIZE_OF_PD_STACK  SIZE_OF_STACK

TaskHandle_t pd_Task = NULL;
static xQueueHandle pd_evt_queue = NULL;
StaticTask_t xTaskBuffer_pd;
StackType_t xStack_pd[SIZE_OF_PD_STACK];

void reset_all_io() {
//  NOA_PUB_I2C_master_driver_deinitialize(0);
//  NOA_PUB_I2C_master_driver_deinitialize(1);
//  gpio_reset_pin((gpio_num_t)i2c0_scl_pin);  // open gpio_reset_pin will up current of sleep
//  gpio_reset_pin((gpio_num_t)i2c0_sda_pin);
//  gpio_reset_pin((gpio_num_t)i2c1_scl_pin);
//  gpio_reset_pin((gpio_num_t)i2c1_sda_pin);
//
//  gpio_reset_pin((gpio_num_t)station_db_pin);
//  gpio_reset_pin((gpio_num_t)station_button_pin);
//  gpio_reset_pin((gpio_num_t)station_powersave_pin);
//  gpio_reset_pin((gpio_num_t)station_powerled_pin);
//  gpio_reset_pin((gpio_num_t)station_en5v_pin);
//
//  gpio_reset_pin((gpio_num_t)panda_power_pin);
//  gpio_reset_pin((gpio_num_t)panda_s0_pin);
//  gpio_reset_pin((gpio_num_t)panda_s4_pin);
//  gpio_reset_pin((gpio_num_t)panda_s5_pin);
//
//  gpio_reset_pin((gpio_num_t)usb_pd_snk_int_pin);
//  gpio_reset_pin((gpio_num_t)usb_pd_snk_sel_pin);
//
//  gpio_reset_pin((gpio_num_t)usb_pd_src2_int_pin);
//  gpio_reset_pin((gpio_num_t)usb_pd_src2_sel_pin);
//  gpio_reset_pin((gpio_num_t)ncp_bb_con2_int_pin);
//  gpio_reset_pin((gpio_num_t)ncp_bb_con2_en_pin);
//
//  gpio_reset_pin((gpio_num_t)usb_pd_src1_int_pin);
//  gpio_reset_pin((gpio_num_t)usb_pd_src1_sel_pin);
//  gpio_reset_pin((gpio_num_t)ncp_bb_con1_int_pin);
//  gpio_reset_pin((gpio_num_t)ncp_bb_con1_en_pin);
//
//  gpio_reset_pin((gpio_num_t)usb_pd_src3_int_pin);
//  gpio_reset_pin((gpio_num_t)usb_pd_src3_sel_pin);
//  gpio_reset_pin((gpio_num_t)ncp_bb_con3_int_pin);
//  gpio_reset_pin((gpio_num_t)ncp_bb_con3_en_pin);

//  rtc_gpio_isolate((gpio_num_t)i2c0_scl_pin);
//  rtc_gpio_isolate((gpio_num_t)i2c0_sda_pin);
//
//  rtc_gpio_isolate((gpio_num_t)station_en5v_pin);
//
//  rtc_gpio_isolate((gpio_num_t)panda_power_pin);
//  rtc_gpio_isolate((gpio_num_t)panda_s0_pin);
//  rtc_gpio_isolate((gpio_num_t)panda_s4_pin);
//  rtc_gpio_isolate((gpio_num_t)panda_s5_pin);
//
//  rtc_gpio_isolate((gpio_num_t)usb_pd_src2_int_pin);
//  rtc_gpio_isolate((gpio_num_t)usb_pd_src2_sel_pin);
//  rtc_gpio_isolate((gpio_num_t)ncp_bb_con2_int_pin);
//  rtc_gpio_isolate((gpio_num_t)ncp_bb_con2_en_pin);
//
//  rtc_gpio_isolate((gpio_num_t)usb_pd_src1_sel_pin);
//  rtc_gpio_isolate((gpio_num_t)ncp_bb_con1_int_pin);
//  rtc_gpio_isolate((gpio_num_t)ncp_bb_con1_en_pin);
//
//  rtc_gpio_isolate((gpio_num_t)usb_pd_src3_int_pin);
//  rtc_gpio_isolate((gpio_num_t)usb_pd_src3_sel_pin);
//  rtc_gpio_isolate((gpio_num_t)ncp_bb_con3_int_pin);
//  rtc_gpio_isolate((gpio_num_t)ncp_bb_con3_en_pin);
//
//  rtc_gpio_isolate((gpio_num_t)GPIO_NUM_12);
//  rtc_gpio_isolate((gpio_num_t)GPIO_NUM_15);
//  rtc_gpio_isolate((gpio_num_t)GPIO_NUM_16);
}

void pd_task_loop(void *arg) {
  APP_DEBUG("Start PD task pd_task_loop...");
  while(!bpower_save){
    int reset = 0;
    if (0 == gpio_get_level((gpio_num_t)usb_pd_snk_int_pin)) {
      tcpc_alert(0);
//      APP_DEBUG("PD SNK init pin LOW");
    }  
    pd_run_state_machine(0, reset);

    if (pd_sink_port_ready == 1) {
      if (pd_source_port_ready == 0) {
        pd_init(1); // init pd src 1
        vTaskDelay(50/portTICK_PERIOD_MS);

        ncp81239_pmic_init(1);
        ncp81239_pmic_set_tatus(1);

        pd_init(2); // init pd src 2
        vTaskDelay(50/portTICK_PERIOD_MS);

        int cc1 = 0, cc2 = 0;
        tcpm_get_cc(2, &cc1, &cc2);
        APP_DEBUG("C%d CC1 %d CC2 %d", 2, cc1, cc2);
//        if (cc1 == 0 && cc2 == 0) {
        if ((cc1 == 0 && cc2 == 0) || gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
          gpio_set_level((gpio_num_t)ncp_bb_con2_en_pin, 0);  // when port2 cc1 cc2 is open
        } else {
          gpio_set_level((gpio_num_t)ncp_bb_con2_en_pin, 1);
        }

        ncp81239_pmic_init(2);
        ncp81239_pmic_set_tatus(2);

        pd_init(3); // init pd src 3
        vTaskDelay(50/portTICK_PERIOD_MS);

        ncp81239_pmic_init(3);
        ncp81239_pmic_set_tatus(3);
        pd_source_port_ready = 1;
      } else {
        if (0 == gpio_get_level((gpio_num_t)usb_pd_src1_int_pin)) {
          tcpc_alert(1);
//        APP_DEBUG("PD SRC 1 init pin LOW");
        }
        pd_run_state_machine(1, 0);

        if (0 == gpio_get_level((gpio_num_t)usb_pd_src2_int_pin)) {
          tcpc_alert(2);
//        APP_DEBUG("PD SRC 2 init pin LOW");
        }
        pd_run_state_machine(2, 0);

        if (0 == gpio_get_level((gpio_num_t)usb_pd_src3_int_pin)) {
          tcpc_alert(3);
//        APP_DEBUG("PD SRC 3 init pin LOW");
        }
        pd_run_state_machine(3, 0);
      }
    }
    vTaskDelay(10/portTICK_PERIOD_MS);
  }
  pd_sink_port_ready = 0;
  APP_DEBUG("pd_task_loop Exit from core %d", xPortGetCoreID());
  vTaskDelete(NULL);
}

void pd_powersave_task_loop(void *arg) {
  APP_DEBUG("Start PD power save task pd_powersave_task_loop...");
  while(bpower_save){
    int reset = 0;
    if (0 == gpio_get_level((gpio_num_t)usb_pd_snk_int_pin)) {
      tcpc_alert(0);
//      APP_DEBUG("PD SNK init pin LOW");
    }  
    pd_run_state_machine(0, reset);
    if (pd_sink_port_ready == 1) {
      break;
    }
    vTaskDelay(10/portTICK_PERIOD_MS);
  }
  pd_sink_port_ready = 0;
  vTaskDelay(50/portTICK_PERIOD_MS);
  gpio_set_level((gpio_num_t)ncp_bb_con1_en_pin, 0);
  gpio_set_level((gpio_num_t)ncp_bb_con2_en_pin, 0);
  gpio_set_level((gpio_num_t)ncp_bb_con3_en_pin, 0);
//  gpio_set_level((gpio_num_t)station_en5v_pin, 1);
//  gpio_set_level((gpio_num_t)station_powersave_pin, 1);
//  gpio_set_level((gpio_num_t)station_db_pin, 0);
  APP_DEBUG("pd_powersave_task_loop Exit from core %d", xPortGetCoreID());
  uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
  reset_all_io();
  esp_deep_sleep_start();
//  esp_light_sleep_start();
  vTaskDelete(NULL);
}

void NOA_Pd_init() {
  pd_init(0); // init pd snk
  vTaskDelay(50/portTICK_PERIOD_MS);

  if (pd_evt_queue == NULL) {
    pd_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (pd_evt_queue == NULL) {
      APP_DEBUG("Create pd_evt_queue fail");
    }
  }

  pd_Task = xTaskCreateStatic(
                 pd_task_loop,           // Function that implements the task.
                 "PDTask",               // Text name for the task.
                 SIZE_OF_PD_STACK,       // Stack size in bytes, not words.
                 NULL,                   // Parameter passed into the task.
                 tskIDLE_PRIORITY + 5,   // Priority at which the task is created.
                 xStack_pd,              // Array to use as the task's stack.
                 &xTaskBuffer_pd);       // Variable to hold the task's data structure.
  if (pd_Task == NULL) {
    APP_DEBUG("Create pd_task_loop fail");
  }
}

void NOA_Pd_Sink_PowerSave_init() {
  pd_init(0); // init pd snk
  vTaskDelay(50/portTICK_PERIOD_MS);

  pd_Task = xTaskCreateStatic(
                 pd_powersave_task_loop,    // Function that implements the task.
                 "PDSinkPowerSaveTask",     // Text name for the task.
                 SIZE_OF_PD_STACK,          // Stack size in bytes, not words.
                 NULL,                      // Parameter passed into the task.
                 tskIDLE_PRIORITY + 5,      // Priority at which the task is created.
                 xStack_pd,                 // Array to use as the task's stack.
                 &xTaskBuffer_pd);          // Variable to hold the task's data structure.
  if (pd_Task == NULL) {
    APP_DEBUG("Create pd_powersave_task_loop fail");
  }
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps) {
  APP_DEBUG("C%d HIGH cnt %d %04X: %d mv %d ma", port, cnt, *src_caps, ((*src_caps >> 10) & 0x3ff) * 50, (*src_caps & 0x3ff) * 10);
  if (port == 0) {
    pd_sink_port_ready = 1;
    pd_source_cap_max_index = cnt - 1;
    pd_sink_port_default_valtage = ((*src_caps >> 10) & 0x3ff) * 50;
    pd_sink_port_default_current = (*src_caps & 0x3ff) * 10;
  }
}
/*********************************FILE END*************************************/

