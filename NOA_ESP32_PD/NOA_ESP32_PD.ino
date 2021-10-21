#include <Wire.h>

#include <Esp.h>

#include "tcpm_driver.h"
#include "usb_pd.h"
#include "NCP81239.h"

#include "NOA_TimeDefs.h"
#include "NOA_public.h"

#define NOA_ESP32_PD_VERSION "0.0.0.1"

const int usb_pd_snk_int_pin = 32;  // init pin for PD snk
const int usb_pd_src_int_pin = 23;  // init pin for PD src
const int ncp_bb_con_int_pin = 25;  // init pin for ncp81239
int ncp_bb_con_en_pin = 12;  // enable pin for ncp81239

const int debug_led_pin  = 30;
const int pushbutton_pin = 0;
int pushbutton_last_state, pushbutton_current_state, pushbutton_last_time, pushbutton_current_time;
int debug_led_current_state = 0;
int pd_source_cap_current_index = 0, pd_source_cap_max_index = 0;

static int pd_sink_port_ready = 0;
static int pd_source_port_ready = 0;

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv_SNK},
  {1, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv_SRC},
};
// USB-C Specific - TCPM end 1

const uint8_t PD_ADDR = 0x22;
const uint8_t PM_ADDR = 0x74;

// This banner is checked the memmory of MCU platform
//const char NOA_Banner[] = {0x7c, 0x20, 0x5c, 0x20, 0x7c, 0x20, 0x7c, 0x20, 0x2f, 0x20, 0x5f, 0x5f, 0x20, 0x5c, 0x20, 0x20, 0x20, 0x20, 0x2f, 0x5c, 0x20, 0x20, 0x20, 0x20, 0x0d, 0x0a,
//                           0x7c, 0x20, 0x20, 0x5c, 0x7c, 0x20, 0x7c, 0x7c, 0x20, 0x7c, 0x20, 0x20, 0x7c, 0x20, 0x7c, 0x20, 0x20, 0x2f, 0x20, 0x20, 0x5c, 0x20, 0x20, 0x20, 0x0d, 0x0a,
//                           0x7c, 0x20, 0x2e, 0x20, 0x60, 0x20, 0x7c, 0x7c, 0x20, 0x7c, 0x20, 0x20, 0x7c, 0x20, 0x7c, 0x20, 0x2f, 0x20, 0x2f, 0x5c, 0x20, 0x5c, 0x20, 0x20, 0x0d, 0x0a,
//                           0x7c, 0x20, 0x7c, 0x5c, 0x20, 0x20, 0x7c, 0x7c, 0x20, 0x7c, 0x5f, 0x5f, 0x7c, 0x20, 0x7c, 0x2f, 0x20, 0x5f, 0x5f, 0x5f, 0x5f, 0x20, 0x5c, 0x20, 0x0d, 0x0a,
//                           0x7c, 0x5f, 0x7c, 0x20, 0x5c, 0x5f, 0x7c, 0x20, 0x5c, 0x5f, 0x5f, 0x5f, 0x5f, 0x2f, 0x2f, 0x5f, 0x2f, 0x20, 0x20, 0x20, 0x20, 0x5c, 0x5f, 0x5c, 0x0d, 0x0a};

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

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.print(__TIME__);
  Serial.println();
  Serial.print(NOA_Banner);
//  char building_time[17] = {0};
//  sprintf(building_time, "%04d%02d%02d%02d%02d%02d", BUILD_DATE_YEAR_INT,BUILD_DATE_MONTH_INT,BUILD_DATE_DAY_INT,BUILD_TIME_HOURS_INT,BUILD_TIME_MINUTES_INT,BUILD_TIME_SECONDS_INT);
//  Serial.println(building_time);

  NOA_PUB_ESP32DebugInit();
  Serial.println("==========================================");
  Serial.printf(" NOA PD Firmware %s\r\n", NOA_ESP32_PD_VERSION);
  Serial.printf(" Building Time %04d%02d%02d%02d%02d%02d\r\n", BUILD_DATE_YEAR_INT,BUILD_DATE_MONTH_INT,BUILD_DATE_DAY_INT,BUILD_TIME_HOURS_INT,BUILD_TIME_MINUTES_INT,BUILD_TIME_SECONDS_INT);
  Serial.printf(" ESP Chip Model %s Revision %d\r\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf(" ESP Chip Cores %d CPUFrea %lu MHz\r\n", ESP.getChipCores(), (unsigned long)ESP.getCpuFreqMHz());
  Serial.printf(" ESP SDK Version %s\r\n", ESP.getSdkVersion());
  Serial.printf(" ESP Heap Free(%d)/Size(%d)Bytes\r\n", ESP.getFreeHeap(), ESP.getHeapSize());
  Serial.printf(" ESP Flash Size %d MB Mode %d Speed %d MHz\r\n", ESP.getFlashChipSize()/1024/1024, ESP.getFlashChipMode(), ESP.getFlashChipSpeed()/1000/1000);

  char deviceid[21] = {0};
  uint64_t chipid;
  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.printf(" ESP Device ID %s\r\n", deviceid);
  Serial.println("==========================================");
//  Serial.println();
    
  pinMode(usb_pd_snk_int_pin, INPUT_PULLUP);
  pinMode(usb_pd_src_int_pin, INPUT_PULLUP);
  
  pinMode(ncp_bb_con_int_pin, INPUT_PULLUP);
  pinMode(ncp_bb_con_en_pin, OUTPUT);
  digitalWrite(ncp_bb_con_en_pin, LOW);
  
  pinMode(debug_led_pin, OUTPUT);
  pinMode(pushbutton_pin, INPUT_PULLUP);
  pushbutton_last_state = 1;
  pushbutton_last_time = 0;
  digitalWrite(debug_led_pin, LOW);
  
//  Wire.begin();
  Wire.begin(26,27);
  Wire.setClock(400000);
//  Wire.setClock(1000000);

  Wire1.begin(21,22);
//  Wire1.begin(26,27);
  Wire1.setClock(400000);

  NOA_PUB_I2C_Scanner(0);
  NOA_PUB_I2C_PD_RreadAllRegs(0, PD_ADDR);
//  NOA_PUB_I2C_PD_Testing(0, PD_ADDR);
//  delay(1000);

  NOA_PUB_I2C_Scanner(1);
  NOA_PUB_I2C_PD_RreadAllRegs(1, PD_ADDR);
  NOA_PUB_I2C_PM_RreadAllRegs(1, PM_ADDR);
  
//  tcpm_init(0);   // init some setting to 0
//  delay(50);
  pd_init(0); // init pd snk
  delay(50);

//  tcpm_init(1);   // init some setting to 0
//  delay(50);
  pd_init(1); // init pd src
  delay(50);
  ncp81239_pmic_init();
  ncp81239_pmic_set_tatus();
//  ncp81239_pmic_get_tatus();
}

void loop() {
  int reset = 0;
  pushbutton_current_state = digitalRead(pushbutton_pin);
  pushbutton_current_time = millis();
  if ((pushbutton_current_state == 0) && (pushbutton_last_state == 1) && ((pushbutton_current_time - pushbutton_last_time) < 100)) {
    if (debug_led_current_state) {
//      digitalWrite(debug_led_pin, LOW);
      DBGLOG(Info, "LED LOW");
      debug_led_current_state = 0;
    } else {
//      digitalWrite(debug_led_pin, HIGH);
      DBGLOG(Info, "LED HIGH");
      debug_led_current_state = 1;
    }
    if (pd_source_cap_current_index < pd_source_cap_max_index) {
      pd_source_cap_current_index++;
    } else {
      pd_source_cap_current_index = 0;
    }
    //pd[0].task_state = PD_STATE_SOFT_RESET);
    reset = 1;
    pd_sink_port_ready = 0;
  }
  pushbutton_last_state = pushbutton_current_state;
  pushbutton_last_time = pushbutton_current_time;
  
  if (LOW == digitalRead(usb_pd_snk_int_pin)) {
    tcpc_alert(0);
//    DBGLOG(Info, "PD init pin LOW");
  } else {
//    DBGLOG(Info, "PD init pin HIGH");
  }  
  pd_run_state_machine(0, reset);
 
  if (LOW == digitalRead(usb_pd_src_int_pin)) {
    tcpc_alert(1);
    DBGLOG(Info, "PD init pin LOW");
  }

  if (pd_sink_port_ready == 1) {
    pd_run_state_machine(1, 0);
  }

//  if (pd_sink_port_ready == 1) {
//    if (LOW == digitalRead(ncp_bb_con_int_pin)) {
//      DBGLOG(Info, "ncp init pin LOW");
//    } 
//  }
  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
//  delay(2);
  delayMicroseconds(100);
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  DBGLOG(Info, "Port %d HIGH", port);
  if (port == 0) {
    pd_sink_port_ready = 1;
    pd_source_cap_max_index = cnt - 1;
  }
}
