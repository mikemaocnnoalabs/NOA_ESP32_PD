#include <Wire.h>

#include "tcpm_driver.h"
#include "usb_pd.h"

#include "NOA_public.h"

const int usb_pd_int_pin = 32;
const int debug_led_pin  = 21;
const int pushbutton_pin = 0;
int pushbutton_last_state, pushbutton_current_state, pushbutton_last_time, pushbutton_current_time;
int debug_led_current_state = 0;
int pd_source_cap_current_index = 0, pd_source_cap_max_index = 0;

// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
// USB-C Specific - TCPM end 1

const uint8_t PD_ADDR = 0x22;

void setup() {
  Serial.begin(115200);
  delay(50);

  NOA_PUB_ESP32DebugInit();

  pinMode(usb_pd_int_pin, INPUT_PULLUP);
//  digitalWrite(usb_pd_int_pin, HIGH);
  pinMode(debug_led_pin, OUTPUT);
  pinMode(pushbutton_pin, INPUT_PULLUP);
  pushbutton_last_state = 1;
  pushbutton_last_time = 0;
  digitalWrite(debug_led_pin, LOW);
  
  Wire.begin();
//  Wire.begin(26,27);
//  Wire.setClock(400000);
  Wire.setClock(1000000);

  NOA_PUB_I2C_Scanner();
  NOA_PUB_I2C_PD_RreadAllRegs(PD_ADDR);
//  NOA_PUB_I2C_PD_Testing(PD_ADDR);

  tcpm_init(0);
  delay(50);
  pd_init(0);
  delay(50);
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
  }
  pushbutton_last_state = pushbutton_current_state;
  pushbutton_last_time = pushbutton_current_time;
  
  if (LOW == digitalRead(usb_pd_int_pin)) {
    tcpc_alert(0);
//    DBGLOG(Info, "PD init pin LOW");
  } else {
//    DBGLOG(Info, "PD init pin HIGH");
  }
  
  pd_run_state_machine(0, reset);
  // For some reason, a delay of 4 ms seems to be best
  // My guess is that spamming the I2C bus too fast causes problems
  delay(1);
}

void pd_process_source_cap_callback(int port, int cnt, uint32_t *src_caps)
{
  // digitalWrite(debug_led_pin, HIGH);
  DBGLOG(Info, "HIGH")
  pd_source_cap_max_index = cnt - 1;
}
