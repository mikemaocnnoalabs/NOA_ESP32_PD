/*
  NOA_public.cpp - NOA arduino public functions
  Copyright 2012 NOA Labs
  Copyright 2022 Mike Mao
  Released under an MIT license. See LICENSE file. */
#include <driver/i2c.h>
#include <unistd.h>

#include "NOA_debug.h"
#include "NOA_public.h"

// Prints out len bytes of hex data in table format.
void NOA_PUB_Print_Buf_Hex(uint8_t *buf, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    printf("0x%02X", buf[i]);
  }
  printf("\r\n");
}

//
//Function:   Swap_hexChar
//Feature:    0x12,0x34,0x56,0x78,0xab,0x0c -> "12 34 56 78 ab 0c"
//Parateters: *buf: char
//            *hex;
//            len:  len
//            fill: char for fill betwen char，0: not fill
//return:     len
//
uint16_t NOA_PUB_Swap_hexChar(char *buf, uint8_t *hex, uint16_t len, char fill) {
  uint8_t i = 0;
  uint16_t l = 0;
  if (fill == 0) {
    l = len * 2;
  } else {
    l = len * 3;
  }
  while (len-- > 0)
  {
    i = (*hex) >> 4;
    *buf++ = i > 9 ? i + ('A' - 10) : i + '0';
    i = (*hex++)&0x0F;
    *buf++ = i > 9 ? i + ('A' - 10) : i + '0';
    if (fill != 0)
    {
      *buf++ = fill;
    }
  }
  *buf = '\0';
  return l;
}

//
//Function:   Swap_charNum
//Feature:    "1024"  -> 0x0400
//Parateters: *buf:  source str
//return:     numbers
//
uint32_t NOA_PUB_Swap_charNum(char *buf) {
  uint32_t num = 0;

  while ('0' <= *buf && *buf <= '9') {
    num *= 10;
    num += *buf++ - '0';
  }

  return num;
}

//
//Function    Swap_charHex
//Feature:    "12 34 56 87 ab 0c" -> 0x12,0x34,0x56,0x78,0xab,0x0c
//Parateters: *hex;   desk number
//            *buf:   source str
//return:     numbers
//
uint16_t NOA_PUB_Swap_charHex(uint8_t *hex, char *buf) {
  char i;
  uint16_t cnt = 0;

  *hex = 0;
  while (1) {
    i = *buf++;
    if ('0' <= i && i <= '9') {
      cnt++;
      i -= '0';
    } else if ('a' <= i && i <= 'f') {
      cnt++;
      i -= 'a' - 10;
    } else if ('A' <= i && i <= 'F') {
      cnt++;
      i -= 'A' - 10;
    } else if (' ' == i) {
      continue;
    } else {
      break;
    }
    *hex <<= 4;
    *hex |= i; 
    if (0 == (cnt&0x01)) {
      *(++hex) = 0;
    }
  }
  return ((cnt+1)>>1);
}


//
//Function:   NOA_PUB_strsplit
//Feature:    "115200-8-1-0"  -> [115200],[8],[1],[0]
//Parateters: subStr: return point head
//            string: source string
//            ch: split ch
//return:     numbers
//
int NOA_PUB_strsplit(ListHandler_t *subStr , char *str, char ch) {
  char *p = str;
  int    i = 0;
  NOA_list_init(subStr);
  while (1) {
    if (*str == ch || *str == '\0') {
      void *payload = NOA_list_nodeApply(sizeof(p));
      if (payload == NULL) {
        return -1;
      }
      NOA_list_bottomInsert(subStr, payload);
      i++;
      *(int*)payload = (int)p; 
      if (*str == '\0') {
        break;
      }
      *str++ = '\0';
      p = str;
    } else {
      str++;
    }
  }
  return i;
}

esp_err_t NOA_PUB_I2C_master_driver_initialize(uint8_t nIndex, int i2c_gpio_sda, int i2c_gpio_scl, uint32_t i2c_frequency) {
  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.scl_io_num = (gpio_num_t)i2c_gpio_scl;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
  conf.sda_io_num = (gpio_num_t)i2c_gpio_sda;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
  conf.master.clk_speed = i2c_frequency;
  esp_err_t ret = i2c_param_config((i2c_port_t)nIndex, &conf);
  if (ret != 0) {
    APP_DEBUG("i2c_param_config Failed");
    return ret;
  } else {
    return i2c_driver_install((i2c_port_t)nIndex, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
  }
}

esp_err_t NOA_PUB_I2C_master_driver_deinitialize(uint8_t nIndex) {
  return i2c_driver_delete((i2c_port_t)nIndex);
}

esp_err_t NOA_PUB_I2C_master_i2c_write(uint8_t nIndex, uint8_t i2c_address, uint8_t cmd_addr, uint8_t *data_wr, uint8_t size) {
  if (size <= 0) {
    APP_DEBUG("data size is less than 0");
    return ESP_FAIL;
  }
//  APP_DEBUG("data len %d size %d", strlen((char *)data_wr), size);
//  for (int i = 0; i < size; i++) {
//    APP_DEBUG("i2 0x%02x cmd 0x%02x data for write 0x%02x", i2c_address, cmd_addr, data_wr[i]);
//  }
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (i2c_address << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, cmd_addr, ACK_CHECK_EN);

  for (int i = 0; i < size; i++) {
    i2c_master_write_byte(cmd, data_wr[i], ACK_CHECK_EN);
  }

  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)nIndex, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_OK) {
//    APP_DEBUG("Write OK");
  } else if (ret == ESP_ERR_TIMEOUT) {
    APP_DEBUG("Bus is busy");
  } else {
    APP_DEBUG("Write Failed");
  }
  return ret;
}

esp_err_t NOA_PUB_I2C_master_i2c_read(uint8_t nIndex, uint8_t i2c_address, uint8_t cmd_addr, uint8_t *data_rd, uint8_t size) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

  i2c_master_write_byte(cmd, (i2c_address << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, cmd_addr, ACK_CHECK_EN);
  i2c_master_start(cmd);

  i2c_master_write_byte(cmd, (i2c_address << 1) | READ_BIT, ACK_CHECK_EN);
  if (size > 1) {
    i2c_master_read(cmd, data_rd, size - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, data_rd + size - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)nIndex, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_OK) {
//    for (int i = 0; i < size; i++) {
//      printf("0x%02x ", data_rd[i]);
//      if ((i + 1) % 16 == 0) {
//        printf("\r\n");
//      }
//    }
//    if (size % 16) {
//      printf("\r\n");
//    }
  } else if (ret == ESP_ERR_TIMEOUT) {
    APP_DEBUG("Bus is busy");
  } else {
    APP_DEBUG("Read failed");
  }
  return ret;
}

void NOA_PUB_I2C_Scanner(uint8_t nIndex){
  uint8_t address;
  printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
  for (int i = 0; i < 128; i += 16) {
    printf("%02x: ", i);
    for (int j = 0; j < 16; j++) {
      if (j == 0) {
        printf("-- ");
        continue;
      }
      fflush(stdout);
      address = i + j;
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
      i2c_master_stop(cmd);
      esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)nIndex, cmd, 1000 / portTICK_RATE_MS);
      i2c_cmd_link_delete(cmd);
      if (ret == ESP_OK) {
        printf("%02x ", address);
      } else if (ret == ESP_ERR_TIMEOUT) {
        printf("UU ");
      } else {
        printf("-- ");
      }
    }
    printf("\r\n");
  }
//  i2c_driver_delete((i2c_port_t)nIndex);
}

void NOA_PUB_PartScan(esp_partition_type_t part_type) {
  esp_partition_iterator_t iterator = NULL;
  const esp_partition_t *next_partition = NULL;
  iterator = esp_partition_find(part_type, ESP_PARTITION_SUBTYPE_ANY, NULL);
  int nIndex = 0;
  while (iterator) {
    next_partition = esp_partition_get(iterator);
    if (next_partition != NULL) {
      if (part_type == ESP_PARTITION_TYPE_APP) {
        printf(" App  Part%d A:0x%06x; S:0x%06x; L:%s\r\n", nIndex, next_partition->address, next_partition->size, next_partition->label);  
      } else {
        printf(" Data Part%d A:0x%06x; S:0x%06x; L:%s\r\n", nIndex, next_partition->address, next_partition->size, next_partition->label);  
      }
      iterator = esp_partition_next(iterator);
      nIndex++;
    }
  }
}

void memory_init(void *memAddr, int memSize) {
}

void *memory_apply(int size) {
  if (size != 0) {
    return malloc(size);
  }

  return NULL;
}
void memory_release(void *addr) {
  free(addr);
  addr = NULL;
}

unsigned long IRAM_ATTR micros() {
  return (unsigned long) (esp_timer_get_time());
}

unsigned long IRAM_ATTR millis() {
  return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

void delay(uint32_t ms) {
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

void IRAM_ATTR delayMicroseconds(uint32_t us) {
  uint32_t m = micros();
  if(us){
    uint32_t e = (m + us);
    if(m > e){ //overflow
      while(micros() > e){
        NOP();
      }
    }
    while(micros() < e){
      NOP();
    }
  }
}

/*********************************FILE END*************************************/

