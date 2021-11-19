#include <Wire.h>

#include <Esp.h>

#include "usb_pd.h"
#include "NCP81239.h"

#include "NOA_TimeDefs.h"
#include "NOA_public.h"

#ifdef NOA_PD_SNACKER
#define NOA_ESP32_PD_VERSION "0.0.0.6"
#else
#define NOA_ESP32_PD_VERSION "0.1.0.8"
#endif

extern int const usb_pd_snk_sel_pin;

#ifdef NOA_PD_SNACKER
const int usb_pd_snk_int_pin = 32;    // init pin for PD snk
const int usb_pd_snk_sel_pin = 33;    // sel pin for PD snk

const int usb_pd_src1_int_pin = 23;   // init pin for PD src
int usb_pd_src1_sel_pin = 14;         // sel pin for PD src

const int ncp_bb_con1_int_pin = 25;   // init pin for ncp81239
int ncp_bb_con1_en_pin = 12;          // enable pin for ncp81239

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv}, // SNK
  {1, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv}, // SRC
};
// USB-C Specific - TCPM end 1

const uint8_t PD_ADDR = 0x22;
const uint8_t PM_ADDR = 0x74;
#else
const int usb_pd_snk_int_pin = 38;  // init pin for PD snk(P1)
const int usb_pd_snk_sel_pin = 14;  // sel pin for PD snk(P1)

const int usb_pd_src2_int_pin = 39; // init pin for PD src_2(P0)
int usb_pd_src2_sel_pin = 27;       // sel pin for PD src_2(P0)
const int ncp_bb_con2_int_pin = 13; // init pin for src_2 ncp81239(P0)
int ncp_bb_con2_en_pin = 4;         // enable pin for src_2 ncp81239(P0)

const int usb_pd_src1_int_pin = 34; // init pin for PD src_1(P2)
int usb_pd_src1_sel_pin = 19;       // sel pin for PD src_1(P2)
const int ncp_bb_con1_int_pin = 35; // init pin for src_1 ncp81239(P2)
int ncp_bb_con1_en_pin = 2;         // enable pin for src_1 ncp81239(P2)

const int usb_pd_src3_int_pin = 36; // init pin for PD src_3(P3)
int usb_pd_src3_sel_pin = 26;       // sel pin for PD src_3(P3)
const int ncp_bb_con3_int_pin = 37; // init pin for src_3 ncp81239(P3)
int ncp_bb_con3_en_pin = 25;        // enable pin for src_3 ncp81239(P3)

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR_B01, &fusb302_tcpm_drv},   // SNK(C0)(P1) bus0
  {1, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},       // SRC(C1)(P2) bus1
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},       // SRC(C2)(P0 UP) bus0
  {1, fusb302_I2C_SLAVE_ADDR_B01, &fusb302_tcpm_drv},   // SRC(C3)(P3) bus1
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

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.print(__TIME__);
  Serial.println();
//  Serial.print(NOA_Banner);
//  char building_time[17] = {0};
//  sprintf(building_time, "%04d%02d%02d%02d%02d%02d", BUILD_DATE_YEAR_INT,BUILD_DATE_MONTH_INT,BUILD_DATE_DAY_INT,BUILD_TIME_HOURS_INT,BUILD_TIME_MINUTES_INT,BUILD_TIME_SECONDS_INT);
//  Serial.println(building_time);

  NOA_PUB_ESP32DebugInit();
  Serial.println("==========================================");
#ifdef NOA_PD_SNACKER
  Serial.printf(" NOA PD SNACKER Firmware %s\r\n", NOA_ESP32_PD_VERSION);
#else
  Serial.printf(" NOA PD STATION Firmware %s\r\n", NOA_ESP32_PD_VERSION);
#endif
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

  pinMode(usb_pd_snk_int_pin, INPUT_PULLUP);  // snk

  pinMode(usb_pd_src1_int_pin, INPUT_PULLUP);  // src1
  pinMode(ncp_bb_con1_int_pin, INPUT_PULLUP);
  pinMode(ncp_bb_con1_en_pin, OUTPUT);
  digitalWrite(ncp_bb_con1_en_pin, LOW);

  pinMode(usb_pd_snk_sel_pin, OUTPUT);  // SEL for SNK
  digitalWrite(usb_pd_snk_sel_pin, HIGH);

  pinMode(usb_pd_src1_sel_pin, OUTPUT);    // SEL for SRC 1
  digitalWrite(usb_pd_src1_sel_pin, HIGH);

#ifdef NOA_PD_SNACKER
  Wire.begin(26,27);
  Wire.setClock(400000);

  Wire1.begin(21,22);
  Wire1.setClock(400000);

  NOA_PUB_I2C_Scanner(0);
  NOA_PUB_I2C_Scanner(1);
  
  pd_init(0); // init pd snk
  delay(50);

  pd_init(1); // init pd src
  delay(50);

  ncp81239_pmic_init(1);
  ncp81239_pmic_set_tatus(1);
  delay(50);

  NOA_PUB_I2C_PD_RreadAllRegs(0, PD_ADDR);
  NOA_PUB_I2C_PD_RreadAllRegs(1, PD_ADDR);
  NOA_PUB_I2C_PM_RreadAllRegs(1, PM_ADDR);
#else  
  pinMode(usb_pd_src3_int_pin, INPUT_PULLUP);  // SRC 3
  pinMode(ncp_bb_con3_int_pin, INPUT_PULLUP);
  pinMode(ncp_bb_con3_en_pin, OUTPUT);
  digitalWrite(ncp_bb_con3_en_pin, LOW);

  pinMode(usb_pd_src3_sel_pin, OUTPUT); // SEL for SRC 3
  digitalWrite(usb_pd_src3_sel_pin, HIGH);

  pinMode(usb_pd_src2_int_pin, INPUT_PULLUP);  // SRC 2
  pinMode(ncp_bb_con2_int_pin, INPUT_PULLUP);
  pinMode(ncp_bb_con2_en_pin, OUTPUT);
//  digitalWrite(ncp_bb_con2_en_pin, LOW);

  pinMode(usb_pd_src2_sel_pin, OUTPUT); // SEL for SRC 2
  digitalWrite(usb_pd_src2_sel_pin, HIGH);
  
  Wire.begin(23,18);  // P1 SNK(C 0), P3 UP SRC(C 2)
//  Wire.setClock(400000);
  Wire.setClock(600000);

  Wire1.begin(21,22); // P2 SRC(C 1) P3 SRC(C 3)
//  Wire1.setClock(400000);
  Wire1.setClock(600000);

  NOA_PUB_I2C_Scanner(0);
  
  NOA_PUB_I2C_Scanner(1);

  pd_init(0); // init pd snk
  delay(50);

/*  pd_init(1); // init pd src 1
  delay(50);

  ncp81239_pmic_init(1);
  ncp81239_pmic_set_tatus(1);
//  ncp81239_pmic_get_tatus(1);

  pd_init(2); // init pd src 2
  delay(50);

  int cc1 = 0, cc2 = 0;
  tcpm_get_cc(2, &cc1, &cc2);
  DBGLOG(Info, "Port %d CC1 %d CC2 %d", 2, cc1, cc2);
  if (cc1 == 0 && cc2 == 0) {
    digitalWrite(ncp_bb_con2_en_pin, LOW);  // when port2 cc1 cc2 is open
  } else {
    digitalWrite(ncp_bb_con2_en_pin, HIGH);
  }

  ncp81239_pmic_init(2);
  ncp81239_pmic_set_tatus(2);
//  ncp81239_pmic_get_tatus(2);
  
  pd_init(3); // init pd src 3
  delay(50);

  ncp81239_pmic_init(3);
  ncp81239_pmic_set_tatus(3);
//  ncp81239_pmic_get_tatus(3);

  delay(50); */
//  Serial.printf(" P1 SNK C0\r\n");
//  NOA_PUB_I2C_PD_RreadAllRegs(0, P1D_ADDR);
//  Serial.printf(" P0 UP SRC C2\r\n");
//  NOA_PUB_I2C_PD_RreadAllRegs(0, PUPD_ADDR);
//  NOA_PUB_I2C_PM_RreadAllRegs(0, PUPM_ADDR);

//  Serial.printf(" P2 SRC C1\r\n");
//  NOA_PUB_I2C_PD_RreadAllRegs(1, P2D_ADDR);
//  NOA_PUB_I2C_PM_RreadAllRegs(1, P2M_ADDR);
//  Serial.printf(" P3 SRC C3\r\n");
//  NOA_PUB_I2C_PD_RreadAllRegs(1, P3D_ADDR);
//  NOA_PUB_I2C_PM_RreadAllRegs(1, P3M_ADDR);
#endif
}

void loop() {
#ifdef NOA_PD_SNACKER
  int reset = 0;
 
  if (LOW == digitalRead(usb_pd_snk_int_pin)) {
    tcpc_alert(0);
//    DBGLOG(Info, "PD init pin LOW");
  } 
  pd_run_state_machine(0, reset);
 
  if (pd_sink_port_ready == 1) {
    if (LOW == digitalRead(usb_pd_src1_int_pin)) {
      tcpc_alert(1);
//    DBGLOG(Info, "PD init pin LOW");
    }
    pd_run_state_machine(1, 0);
  }

  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
  delay(1);
#else
  int reset = 0;
  if (LOW == digitalRead(usb_pd_snk_int_pin)) {
    tcpc_alert(0);
//    DBGLOG(Info, "PD SNK init pin LOW");
  }  
  pd_run_state_machine(0, reset);
 
  if (pd_sink_port_ready == 1) {
    if (pd_source_port_ready == 0) {
//      delay(1000);
      pd_init(1); // init pd src 1
      delay(50);

      ncp81239_pmic_init(1);
      ncp81239_pmic_set_tatus(1);

      pd_init(2); // init pd src 2
      delay(50);

      int cc1 = 0, cc2 = 0;
      tcpm_get_cc(2, &cc1, &cc2);
      DBGLOG(Info, "Port %d CC1 %d CC2 %d", 2, cc1, cc2);
      if (cc1 == 0 && cc2 == 0) {
        digitalWrite(ncp_bb_con2_en_pin, LOW);  // when port2 cc1 cc2 is open
      } else {
        digitalWrite(ncp_bb_con2_en_pin, HIGH);
      }
      
      ncp81239_pmic_init(2);
      ncp81239_pmic_set_tatus(2);
  
      pd_init(3); // init pd src 3
      delay(50);

      ncp81239_pmic_init(3);
      ncp81239_pmic_set_tatus(3);
//      delay(50);
      pd_source_port_ready = 1;
    } else {
      if (LOW == digitalRead(usb_pd_src1_int_pin)) {
        tcpc_alert(1);
//      DBGLOG(Info, "PD SRC 1 init pin LOW");
      }
      pd_run_state_machine(1, 0);

      if (LOW == digitalRead(usb_pd_src2_int_pin)) {
        tcpc_alert(2);
//      DBGLOG(Info, "PD SRC 2 init pin LOW");
      }
      pd_run_state_machine(2, 0);
    
      if (LOW == digitalRead(usb_pd_src3_int_pin)) {
        tcpc_alert(3);
//      DBGLOG(Info, "PD SRC 3 init pin LOW");
      }
      pd_run_state_machine(3, 0);
    }
  }
  delay(1);
#endif
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  DBGLOG(Info, "Port %d HIGH cnt %d", port, cnt);
  if (port == 0) {
    pd_sink_port_ready = 1;
    pd_source_cap_max_index = cnt - 1;
  }
}
