/*
  NOA_public.h - NOA public library head file
  Copyright 2012 NOA Labs
  Copyright 2021 Mike Mao
  Released under an MIT license. See LICENSE file. 
*/
#ifndef __NOA_PUBLIC_H
#define __NOA_PUBLIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_partition.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
// #include <freertos/event_groups.h>
// #include <freertos/xtensa_api.h>
// #include <freertos/portmacro.h>
// #include <freertos/xtensa_api.h>

#include <driver/gpio.h>

#include "NOA_list.h"

#ifdef NOA_PD_SNACKER
#ifdef NOA_PD_SNACKER2
#define NOA_ESP32_PD_VERSION "1.0.1.4"  // 1.0.x.x support new Snacker hardware
#define NOA_PD_DEVICENAME "NOA-Snacker2-ESP-32"
#else
#define NOA_ESP32_PD_VERSION "0.0.1.3"  // 0.0.xx support old snacker hardware
#define NOA_PD_DEVICENAME "NOA-Snacker-ESP-32"
#endif
#else
#ifdef NOA_PD_STATION
#define NOA_ESP32_PD_VERSION "0.1.1.2"  // 0.1.x.x support ESP32
#define NOA_PD_DEVICENAME "NOA-Station-ESP-32"
#endif
#ifdef NOA_PD_STATION2
#define NOA_ESP32_PD_VERSION __NOA_APP_VERSION__  // 1.1.x.x support ESP32-S2
#define NOA_PD_DEVICENAME "NOA-Station-ESP-32-S2"
#endif
#endif

#define SIZE_OF_TASK_QUEUE  16
#define SIZE_OF_STACK       2048
#define ESP_INTR_FLAG_DEFAULT   0

// #define ARDUINO_RUNNING_CORE xPortGetCoreID()

// something from pgmspace.h file of arduino system
#define PROGMEM
#define PGM_P         const char *
#define PGM_VOID_P    const void *
#define PSTR(s)       (s)
#define _SFR_BYTE(n)  (n)

#define pgm_read_byte(addr)   (*(const unsigned char *)(addr))
#define pgm_read_word(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(const unsigned short *)(_addr); \
})
#define pgm_read_dword(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(const unsigned long *)(_addr); \
})
#define pgm_read_float(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(const float *)(_addr); \
})
#define pgm_read_ptr(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(void * const *)(_addr); \
})

#define pgm_get_far_address(x) ((uint32_t)(&(x)))

#define pgm_read_byte_near(addr)  pgm_read_byte(addr)
#define pgm_read_word_near(addr)  pgm_read_word(addr)
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_float_near(addr) pgm_read_float(addr)
#define pgm_read_ptr_near(addr)   pgm_read_ptr(addr)
#define pgm_read_byte_far(addr)   pgm_read_byte(addr)
#define pgm_read_word_far(addr)   pgm_read_word(addr)
#define pgm_read_dword_far(addr)  pgm_read_dword(addr)
#define pgm_read_float_far(addr)  pgm_read_float(addr)
#define pgm_read_ptr_far(addr)    pgm_read_ptr(addr)
//------------------------------------------------

#ifdef NOA_PD_STATION2
const int i2c0_scl_pin = GPIO_NUM_5;  // i2c 0 scl
const int i2c0_sda_pin = GPIO_NUM_3;  // i2c 0 sda

const int i2c1_scl_pin = GPIO_NUM_34;  // i2c 0 scl
const int i2c1_sda_pin = GPIO_NUM_33;  // i2c 0 sda

const int station_db_pin = GPIO_NUM_39;         // station DB port, must output HIGH
const int station_button_pin = GPIO_NUM_4;      // power click button
const int station_powerled_pin = GPIO_NUM_35;   // power led
const int station_en5v_pin = GPIO_NUM_21;       // station 5v enable/power save pin

const int panda_power_pin = GPIO_NUM_10;    // Panda switch
const int panda_s0_pin = GPIO_NUM_11;       // Panda S0
const int panda_s4_pin = GPIO_NUM_13;       // Panda S4
const int panda_s5_pin = GPIO_NUM_14;       // Panda S5

const int usb_pd_snk_int_pin = GPIO_NUM_38;  // init pin for PD snk(P1) (port 1 C0)
const int usb_pd_snk_sel_pin = GPIO_NUM_41;  // sel pin for PD snk(P1) (port 1 C0)

const int usb_pd_src2_int_pin = GPIO_NUM_20;  // init pin for PD src_2(P0) (UPport C2)
const int usb_pd_src2_sel_pin = GPIO_NUM_19;  // sel pin for PD src_2(P0) (UPport C2)
const int ncp_bb_con2_int_pin = GPIO_NUM_18;  // init pin for src_2 ncp81239(P0) (UPport C2)
const int ncp_bb_con2_en_pin = GPIO_NUM_17;   // enable pin for src_2 ncp81239(P0) (UPport C2)

const int usb_pd_src1_int_pin = GPIO_NUM_37;  // init pin for PD src_1(P2) (port 2 C1)
const int usb_pd_src1_sel_pin = GPIO_NUM_1;  // sel pin for PD src_1(P2) (port 2 C1)
const int ncp_bb_con1_int_pin = GPIO_NUM_36;  // init pin for src_1 ncp81239(P2) (port 2 C1)
const int ncp_bb_con1_en_pin = GPIO_NUM_2;    // enable pin for src_1 ncp81239(P2)(port 2 C1) 

const int usb_pd_src3_int_pin = GPIO_NUM_6;  // init pin for PD src_3(P3) (port 3 C3)
const int usb_pd_src3_sel_pin = GPIO_NUM_9;  // sel pin for PD src_3(P3) (port 3 C3)
const int ncp_bb_con3_int_pin = GPIO_NUM_7;  // init pin for src_3 ncp81239(P3) (port 3 C3)
const int ncp_bb_con3_en_pin = GPIO_NUM_8;   // enable pin for src_3 ncp81239(P3) (port 3 C3)

extern xQueueHandle NOA_APP_TASKQUEUE;

extern int bpower_save;
extern int nwake_reason;
#endif

extern xSemaphoreHandle wire0_mutex;
extern xSemaphoreHandle wire1_mutex;

#define I2C_MASTER_TX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define WRITE_BIT   I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT    I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN    0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS   0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL     0x0               /*!< I2C ack value */
#define NACK_VAL    0x1               /*!< I2C nack value */

#ifdef NOA_PD_SNACKER
// Task Queue Mesage struct
extern xQueueHandle NOA_APP_TASKQUEUE;
extern xQueueHandle NOA_RGB_TASKQUEUE;
extern xQueueHandle NOA_NET_TASKQUEUE;
#endif

typedef struct {
  uint32_t  message;
  uint32_t  param1;
  uint32_t  param2;
  uint32_t  srcTaskId;
} NOA_PUB_MSG;

void NOA_PUB_Print_Buf_Hex(uint8_t *buf, uint16_t len);
uint16_t NOA_PUB_Swap_hexChar(char *buf, uint8_t *hex, uint16_t len, char fill);
uint32_t NOA_PUB_Swap_charNum(char *buf);
int NOA_PUB_strsplit(ListHandler_t *subStr , char *str, char ch);

esp_err_t NOA_PUB_I2C_master_driver_initialize(uint8_t nIndex, int i2c_gpio_sda, int i2c_gpio_scl, uint32_t i2c_frequency);
esp_err_t NOA_PUB_I2C_master_driver_deinitialize(uint8_t nIndex);
esp_err_t NOA_PUB_I2C_master_i2c_write(uint8_t nIndex, uint8_t ic_address, uint8_t cmd_addr, uint8_t *data_wr, uint8_t size);
esp_err_t NOA_PUB_I2C_master_i2c_read(uint8_t nIndex, uint8_t ic_address, uint8_t cmd_addr, uint8_t *data_rd, uint8_t size);

esp_err_t NOA_PUB_I2C_master_tcpc_xfer(uint8_t nIndex, const uint8_t *out, int out_size, uint8_t *in, int in_size, int flags);

void NOA_PUB_I2C_Scanner(uint8_t nIndex);

void NOA_PUB_PartScan(esp_partition_type_t part_type);

void memory_init(void *memAddr, int memSize);
void *memory_apply(int size);
void memory_release(void *addr);

#define NOP() asm volatile ("nop")
unsigned long IRAM_ATTR micros();
unsigned long IRAM_ATTR millis();
void delay(uint32_t ms);
void IRAM_ATTR delayMicroseconds(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif
