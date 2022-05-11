#include <stdio.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <esp_efuse.h>
#include <soc/efuse_reg.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <soc/rtc.h>
#include <esp_spi_flash.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>

#include <soc/soc_caps.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

#include "src\DRV\PDM\usb_pd.h"

#include "src\LIB\PUB\NOA_debug.h"
#include "src\LIB\PUB\NOA_timedefs.h"
#include "src\LIB\PUB\NOA_public.h"

#include "src\APP\NOA_App.h"
#include "src\PD\NOA_Pd.h"

xSemaphoreHandle wire0_mutex = NULL;
xSemaphoreHandle wire1_mutex = NULL;

//char strReleaseDate[16] = {0};
//char strReleaseTime[16] = {0};

// This banner is checked the memmory of MCU platform
const char NOA_Banner[] = {0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,\
                           0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,\
                           0xe2, 0x96, 0x88, 0x20, 0x0d, 0x0a, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,\
                           0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20,\
                           0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x0d, 0x0a, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2,\
                           0x96, 0x88, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,\
                           0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x0d, 0x0a, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88,\
                           0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,\
                           0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x0d, 0x0a, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20,\
                           0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,\
                           0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x0d, 0x0a};

uint32_t getPsramSize(void) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
  return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t getHeapSize(void) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t getDmaSize(void) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DMA);
  return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t getCpuFrequencyMhz(){
  rtc_cpu_freq_config_t conf;
  rtc_clk_cpu_freq_get_config(&conf);
  return conf.freq_mhz;
}

void Uart_Print_Info() {
  printf(__DATE__);
  printf(" ");
  printf(__TIME__);
  printf("\n");
//  printf(NOA_Banner);
  printf("==========================================\n");
#ifdef NOA_RT7_MAINBOARD
#ifdef NOA_PD_SNACKER2
  printf(" NOA PD SNACKER2 Firmware %s\r\n", NOA_ESP32_PD_VERSION);
#else
  printf(" NOA PD SNACKER Firmware %s\r\n", NOA_ESP32_PD_VERSION);
#endif
#else
#ifdef NOA_PD_STATION
  printf(" NOA PD STATION Firmware %s\r\n", NOA_ESP32_PD_VERSION);
#endif
#ifdef NOA_PD_STATION2
  printf(" NOA PD STATION2 Firmware %s\r\n", NOA_ESP32_PD_VERSION);
#endif
#endif
//  memset(strReleaseDate, 0, 16);
//  memset(strReleaseTime, 0, 16);
//  sprintf(strReleaseDate, "%04d%02d%02d", BUILD_DATE_YEAR_INT,BUILD_DATE_MONTH_INT,BUILD_DATE_DAY_INT);
//  sprintf(strReleaseTime, "%02d%02d%02d", BUILD_TIME_HOURS_INT,BUILD_TIME_MINUTES_INT,BUILD_TIME_SECONDS_INT);
//  printf(" Building Time %s%s\r\n", strReleaseDate, strReleaseTime);
  printf(" Building Time %s\r\n", __NOA_APP_BUILD_TIME__);
//  printf(__NOA_APP_BUILD_TIME__);
//  printf(__NOA_APP_VERSION__);
//  printf(__NOA_APP_NAME__);
  esp_chip_info_t info;
  esp_chip_info(&info);

  uint32_t package = esp_efuse_get_pkg_ver();
  uint8_t strChipMode[32] = {0};
  memset(strChipMode, '\0', sizeof(strChipMode));

  switch(info.model) {
    case CHIP_ESP32:
      memcpy(strChipMode, "ESP32", sizeof("ESP32"));
      break;
    case CHIP_ESP32S2:  // ESP-IDF V4.2
      memcpy(strChipMode, "ESP32S2", sizeof("ESP32S2"));
      break;
    case CHIP_ESP32S3:  // ESP-IDF V4.3
      memcpy(strChipMode, "ESP32S3", sizeof("ESP32S3"));
      break;
    case CHIP_ESP32C3:
      memcpy(strChipMode, "ESP32C3", sizeof("ESP32C3"));
      break;
    case CHIP_ESP32H2:    // ESP-IDF V4.4
      memcpy(strChipMode, "ESP32H2", sizeof("ESP32H2"));
      break;
    default:
      memcpy(strChipMode, "Unknow", sizeof("Unknow"));
      break;
  }
#ifndef NOA_PD_STATION2
  switch(package) {
    case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6:
      strcat((char *)strChipMode, "-D0WDQ6");
      break;
    case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ5:
      strcat((char *)strChipMode, "-D0WDQ5");
      break;
    case EFUSE_RD_CHIP_VER_PKG_ESP32D2WDQ5:
      strcat((char *)strChipMode, "-D2WDQ5");
      break;
    case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD2:
        strcat((char *)strChipMode, "-PICOD2");
        break;
    case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4:
      strcat((char *)strChipMode, "-PICOD4");
      break;
    case EFUSE_RD_CHIP_VER_PKG_ESP32PICOV302:  // ESP-IDF V3.3.6
      strcat((char *)strChipMode, "-PICOV302");
      break;
    default:
      strcat((char *)strChipMode, "-Unknow");
      break;
  }
#else
  switch (package) {
    case 0: // ESP32-S2
      strcat((char *)strChipMode, "");
      break;
    case 1: // ESP32-S2FH16
      strcat((char *)strChipMode, "-FH16");
      break;
    case 2: // ESP32-S2FH32
      strcat((char *)strChipMode, "-FH32");
      break;
    default: // New package, features unknown
      break;
  }
#endif
  printf(" ESP Chip Model %s Revision %d\r\n", strChipMode, info.revision);
  printf(" ESP Chip Cores %d CPUFrea %dMHz\r\n", info.cores, getCpuFrequencyMhz());
  printf(" ESP SDK Version %s\r\n", esp_get_idf_version());
  printf(" ESP Heap MINFree(%d)MAX(%d)Free(%d)/Size(%d)Bytes\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_INTERNAL), getHeapSize());
  printf(" ESP DMA MINFree(%d)MAX(%d)Free(%d)/Size(%d)Bytes\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DMA), heap_caps_get_largest_free_block(MALLOC_CAP_DMA), heap_caps_get_free_size(MALLOC_CAP_DMA), getDmaSize());

  esp_image_header_t fhdr;
  if(spi_flash_read(0x1000, (uint32_t*)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC) {
    APP_DEBUG("SPI flash Read Magic Header Fail!");
    return;
  }
  int spi_size = 0, spi_speed = 0;
  switch(fhdr.spi_size & 0x0F) {
    case 0x0: // 8 MBit (1MB)
      spi_size = 1;
      break;
    case 0x1: // 16 MBit (2MB)
      spi_size = 2;
      break;
    case 0x2: // 32 MBit (4MB)
      spi_size = 4;
      break;
    case 0x3: // 64 MBit (8MB)
      spi_size = 8;
      break;
    case 0x4: // 128 MBit (16MB)
      spi_size = 16;
      break;
    default:  // fail?
      break;
  }

  switch(fhdr.spi_speed & 0x0F) {
    case 0x0: // 40 MHz
      spi_speed = 40;
       break;
    case 0x1: // 26 MHz
      spi_speed = 26;
      break;
    case 0x2: // 20 MHz
      spi_speed = 20;
      break;
    case 0xf: // 80 MHz
      spi_speed = 80;
      break;
    default: // fail?
      break;
  }
  uint8_t strSpiMode[16] = {0};
  memset(strSpiMode, '\0', sizeof(strSpiMode));
  switch(fhdr.spi_mode & 0x0F) {
    case 0x0: // FM_QIO
      memcpy(strSpiMode, "QIO", sizeof("QIO"));
      break;
    case 0x1: // FM_QOUT
      memcpy(strSpiMode, "QOUT", sizeof("QOUT"));
      break;
    case 0x2: // FM_DIO
      memcpy(strSpiMode, "DIO", sizeof("DIO"));
      break;
    case 0x3: // FM_DOUT
      memcpy(strSpiMode, "DOUT", sizeof("DOUT"));
      break;
    case 0x4: // FM_FAST_READ
      memcpy(strSpiMode, "FAST_READ", sizeof("FAST_READ"));
      break;
    case 0x5: // FM_SLOW_READ
      memcpy(strSpiMode, "SLOW_READ", sizeof("SLOW_READ"));
      break;
    default: // FM_UNKNOWN
      break;
  }
  printf(" ESP Flash Size %dMB Mode %s Speed %dMHz\r\n", spi_size, strSpiMode, spi_speed); 

  uint64_t _chipmacid = 0LL;
  esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
  char deviceid[21] = {0};
  sprintf(deviceid, "%llu", _chipmacid);
  printf(" ESP Device ID %s\r\n", deviceid);  // C8 C3 21 F7 C6 30 = 220740414064176
  uint8_t base_mac_addr[6] = {0};
  esp_efuse_mac_get_default(base_mac_addr);  // 30:C6:F7:21:C3:C8
  printf(" ESP Base MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n", base_mac_addr[0], base_mac_addr[1], base_mac_addr[2],
                                                       base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);

  char buf[17] = {0};
  esp_ota_get_app_elf_sha256(buf, sizeof(buf));
  printf(" NOA APP SHA256 %s\r\n", buf);

// Display information about the current running image.
  NOA_PUB_PartScan(ESP_PARTITION_TYPE_APP);
  NOA_PUB_PartScan(ESP_PARTITION_TYPE_DATA);
  printf("==========================================\n");  
}

esp_sleep_wakeup_cause_t print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0 : printf("Wakeup caused by external signal using RTC_IO\r\n"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : printf("Wakeup caused by external signal using RTC_CNTL\r\n"); break;
    case ESP_SLEEP_WAKEUP_TIMER : printf("Wakeup caused by timer\r\n"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : printf("Wakeup caused by touchpad\r\n"); break;
    case ESP_SLEEP_WAKEUP_ULP : printf("Wakeup caused by ULP program\r\n"); break;
    case ESP_SLEEP_WAKEUP_GPIO : printf("Wakeup caused by GPIO program\r\n"); break;
    case ESP_SLEEP_WAKEUP_UART : printf("Wakeup caused by UART program\r\n"); break;
    default : printf("Wakeup was not caused by deep sleep: %d\r\n",wakeup_reason); break;
  }
  return wakeup_reason;
}

// #define NOA_LIGHT_SLEEP 1
// #define NOA_DEEP_SLEEP  1

extern "C" void app_main(void) {
  rtc_gpio_hold_dis((gpio_num_t)station_en5v_pin);
  print_wakeup_reason();
  bpower_save = 0;
//  esp_sleep_enable_timer_wakeup(60000000);  // auto wake up after 60 sec, for testing only
  esp_sleep_enable_ext1_wakeup((1ULL << (gpio_num_t)station_button_pin), ESP_EXT1_WAKEUP_ALL_LOW);
  Debug_init();
  Uart_Print_Info();
//  esp_wifi_stop();
// ESP32-S2 Dev board link to PD adapter, 5.13V 0.0040A 0.0206W
// ESP32-S2 Dev board link to USB hub, 5.13V 0.0006A 0.0035W
// Eden board link to PD adpater, 5.12V 0.0612A 0.30xxW
#ifdef NOA_DEEP_SLEEP
  printf("Enabling EXT1 wakeup on pins GPIO%d\n", GPIO_NUM_0);
  esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_0), ESP_EXT1_WAKEUP_ALL_LOW);

  uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
//  reset_all_io();
  esp_deep_sleep_start();
#endif
// ESP32-S2 Dev board link to PD adapter, 5.12V 0.0071A 0.0368W
// ESP32-S2 Dev board link to USB hub, 5.13V 0.0036A 0.0189W
// Eden board link to PD adpater, 5.12V 0.065XA 0.32XXW
#ifdef NOA_LIGHT_SLEEP
  const int wakeup_level = 0;
  gpio_config_t config = {
    .pin_bit_mask = BIT64(GPIO_NUM_0),
    .mode = GPIO_MODE_INPUT
  };
  ESP_ERROR_CHECK(gpio_config(&config));
  gpio_wakeup_enable(GPIO_NUM_0, wakeup_level == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);

  esp_sleep_enable_gpio_wakeup();
  uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
  esp_light_sleep_start();
#endif

  gpio_config_t io_conf_outputpin_db;
  io_conf_outputpin_db.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_outputpin_db.pin_bit_mask = (1ULL << station_db_pin) | (1ULL << station_powerled_pin) | (1ULL << panda_power_pin);
  io_conf_outputpin_db.mode = GPIO_MODE_OUTPUT;  // output
  io_conf_outputpin_db.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf_outputpin_db.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf_outputpin_db);

  gpio_set_level((gpio_num_t)station_db_pin, 1);  // Hold DB pin
  gpio_set_level((gpio_num_t)station_powerled_pin, 1);  // light power led pin
  gpio_set_level((gpio_num_t)panda_power_pin, 0);  // Hold panda power pin

  gpio_config_t io_conf_outputpin_40;
  io_conf_outputpin_40.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_outputpin_40.pin_bit_mask = (1ULL << GPIO_NUM_40);
  io_conf_outputpin_40.mode = GPIO_MODE_INPUT;  // input
  io_conf_outputpin_40.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf_outputpin_40.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf_outputpin_40);

  gpio_config_t io_conf_outputpin_5v;
  io_conf_outputpin_5v.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_outputpin_5v.pin_bit_mask = (1ULL << station_en5v_pin);
  io_conf_outputpin_5v.mode = GPIO_MODE_OUTPUT;  // output
  io_conf_outputpin_5v.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf_outputpin_5v.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf_outputpin_5v);

  gpio_set_level((gpio_num_t)station_en5v_pin, 0);  // Hold station 5V pin

  vTaskDelay(50/portTICK_PERIOD_MS);

  gpio_config_t io_conf_inputpin_pd;  // Init all USB302 PD chip Int pin, pull up
  io_conf_inputpin_pd.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_inputpin_pd.pin_bit_mask = (1ULL << usb_pd_snk_int_pin) | (1ULL << usb_pd_src1_int_pin) | (1ULL << usb_pd_src2_int_pin) | (1ULL << usb_pd_src3_int_pin);
  io_conf_inputpin_pd.mode = GPIO_MODE_INPUT;  // input
  io_conf_inputpin_pd.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf_inputpin_pd.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf_inputpin_pd);

  gpio_config_t io_conf_outpupin_pd_sel;  // Init all USB302 PD chip sel pin
  io_conf_outpupin_pd_sel.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_outpupin_pd_sel.pin_bit_mask = (1ULL << usb_pd_snk_sel_pin) | (1ULL << usb_pd_src1_sel_pin) | (1ULL << usb_pd_src2_sel_pin) | (1ULL << usb_pd_src3_sel_pin);
  io_conf_outpupin_pd_sel.mode = GPIO_MODE_OUTPUT;  // output
  io_conf_outpupin_pd_sel.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf_outpupin_pd_sel.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf_outpupin_pd_sel);

  gpio_set_level((gpio_num_t)usb_pd_snk_sel_pin, 1);
  gpio_set_level((gpio_num_t)usb_pd_src1_sel_pin, 1);
  gpio_set_level((gpio_num_t)usb_pd_src2_sel_pin, 1);
  gpio_set_level((gpio_num_t)usb_pd_src3_sel_pin, 1);

  gpio_config_t io_conf_inputpin_panda;  // Init all panda status pin, pull down
  io_conf_inputpin_panda.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_inputpin_panda.pin_bit_mask = (1ULL << panda_s0_pin) | (1ULL << panda_s4_pin) | (1ULL << panda_s5_pin);
  io_conf_inputpin_panda.mode = GPIO_MODE_INPUT;  // input
  io_conf_inputpin_panda.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf_inputpin_panda.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf_inputpin_panda);

  if (NOA_PUB_I2C_master_driver_initialize(0, i2c0_sda_pin, i2c0_scl_pin, 400000) != ESP_OK) { // Iint I2C bus 0
    APP_DEBUG("Initialize I2C bus %d(sda:%d slc:%d) frequency %d fail", 0, i2c0_sda_pin, i2c0_scl_pin, 400000);
    return;
  }
  NOA_PUB_I2C_Scanner(0);

  wire0_mutex = xSemaphoreCreateMutex();
  if (!wire0_mutex) {
    APP_DEBUG("Create mutex for I2C0 bus fail");
    return;
  }
  // lock
  xSemaphoreTake(wire0_mutex, portMAX_DELAY);
  // unlock
  xSemaphoreGive(wire0_mutex);

  if (NOA_PUB_I2C_master_driver_initialize(1, i2c1_sda_pin, i2c1_scl_pin, 400000) != ESP_OK) { // Iint I2C bus 1
    APP_DEBUG("Initialize I2C bus %d(sda:%d slc:%d) frequency %d fail", 1, i2c1_sda_pin, i2c1_scl_pin, 400000);
    return;
  }
  NOA_PUB_I2C_Scanner(1);

  wire1_mutex = xSemaphoreCreateMutex();
  if (!wire1_mutex) {
    APP_DEBUG("Create mutex for I2C0 bus fail");
    return;
  }
  // lock
  xSemaphoreTake(wire1_mutex, portMAX_DELAY);
  // unlock
  xSemaphoreGive(wire1_mutex);

  gpio_config_t io_conf_inputpin_button;
  io_conf_inputpin_button.intr_type = (gpio_int_type_t)GPIO_INTR_ANYEDGE;
  io_conf_inputpin_button.mode = GPIO_MODE_INPUT;
  io_conf_inputpin_button.pin_bit_mask = (1ULL << station_button_pin);
  io_conf_inputpin_button.pull_down_en = GPIO_PULLDOWN_DISABLE;  // set default to low
  io_conf_inputpin_button.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf_inputpin_button);

  gpio_config_t io_conf_inputpin_ncp;  // Init all NCP81239 chip Int pin, pull up
  io_conf_inputpin_ncp.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_inputpin_ncp.pin_bit_mask = (1ULL << ncp_bb_con1_int_pin) | (1ULL << ncp_bb_con2_int_pin) | (1ULL << ncp_bb_con3_int_pin);
  io_conf_inputpin_ncp.mode = GPIO_MODE_INPUT;  // input
  io_conf_inputpin_ncp.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf_inputpin_ncp.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf_inputpin_ncp);

  gpio_config_t io_conf_inputpin_ncp_en;  // Init all NCP81239 chip En pin
  io_conf_inputpin_ncp_en.intr_type = (gpio_int_type_t)GPIO_INTR_DISABLE;
  io_conf_inputpin_ncp_en.pin_bit_mask = (1ULL << ncp_bb_con1_en_pin) | (1ULL << ncp_bb_con2_en_pin) | (1ULL << ncp_bb_con3_en_pin);
  io_conf_inputpin_ncp_en.mode = GPIO_MODE_OUTPUT;  // output
  io_conf_inputpin_ncp_en.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf_inputpin_ncp_en.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf_inputpin_ncp_en);
  gpio_set_level((gpio_num_t)ncp_bb_con1_en_pin, 0);
  gpio_set_level((gpio_num_t)ncp_bb_con3_en_pin, 0);

  int cc1 = 0, cc2 = 0;
  tcpm_get_cc(2, &cc1, &cc2);
  APP_DEBUG("C%d CC1 %d CC2 %d", 2, cc1, cc2);
  if ((cc1 == 0 && cc2 == 0) || gpio_get_level((gpio_num_t)panda_s4_pin) == 0) {
    gpio_set_level((gpio_num_t)ncp_bb_con2_en_pin, 0);  // when src2 cc1 cc2 is open
  } else {
    gpio_set_level((gpio_num_t)ncp_bb_con2_en_pin, 1);
  }

  NOA_App_init();

  vTaskDelay(50/portTICK_PERIOD_MS);
  APP_DEBUG("In %ld Heap %d/%d StackSize %d", millis()/1000, heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_INTERNAL), uxTaskGetStackHighWaterMark(NULL));  
  NOA_Pd_init();

  static uint32_t loop_time_new = 0;
  static uint32_t loop_time_old = 0;

  while(1) {
    loop_time_new = millis();
    if (loop_time_new - loop_time_old > 500) {
      loop_time_old = loop_time_new;
    }
    vTaskDelay(100 / portTICK_RATE_MS);
  }

  NOA_PUB_I2C_master_driver_deinitialize(0);
  NOA_PUB_I2C_master_driver_deinitialize(1);
}

