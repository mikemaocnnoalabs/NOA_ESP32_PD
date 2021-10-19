#include <Wire.h>

#include "tcpm_driver.h"
#include "usb_pd.h"
#include "NCP81239.h"

#include "NOA_public.h"

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

void setup() {
  Serial.begin(115200);
  delay(50);

  NOA_PUB_ESP32DebugInit();

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
//  Wire1.setClock(1000000);

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
  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
//  delay(2);
  delayMicroseconds(100);
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  // digitalWrite(debug_led_pin, HIGH);
  DBGLOG(Info, "Port %d HIGH", port);
  if (port == 0) {
//    digitalWrite(ncp_bb_con_en_pin, HIGH);
    DBGLOG(Info, "ncp_bb_con_en_pin %d", digitalRead(ncp_bb_con_en_pin));
    pd_sink_port_ready = 1;
//    ncp81239_pmic_set_tatus();
  }
  pd_source_cap_max_index = cnt - 1;
}
