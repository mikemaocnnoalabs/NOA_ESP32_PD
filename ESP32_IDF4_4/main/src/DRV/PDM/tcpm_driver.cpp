/*
 * tcpm_driver.c
 *
 * Created: 11/11/2017 18:42:26
 *  Author: jason
 */ 
#include <driver/i2c.h>

#include "..\..\LIB\PUB\NOA_debug.h"
#include "..\..\LIB\PUB\NOA_public.h"

#include "tcpm_driver.h"

extern const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT];

//extern "C" {
//  void WirebeginTransmission(int bus, int addr) {
//    if (bus == 0) {
//      Wire.beginTransmission(addr);
//    } else {
//      Wire1.beginTransmission(addr);
//    }
//  }
//
//  void Wirewrite(int bus, int value) {
//    if (bus == 0) {
//      Wire.write(value);
//    } else {
//      Wire1.write(value);
//    }
//  }
//  
//  void WireendTransmission(int bus, int stopBit) {
//    if (bus == 0) {
//      Wire.endTransmission(stopBit);
//    } else {
//      Wire1.endTransmission(stopBit);
//    }
//  }
//
//  void WirerequestFrom(int bus, int addr, int quantity, int stopBit) {
//    if (bus == 0) {
//      Wire.requestFrom(addr, quantity, stopBit);
//    } else {
//      Wire1.requestFrom(addr, quantity, stopBit);
//    }
//  }
//
//  int Wireread(int bus) {
//    if (bus == 0) {
//      return Wire.read();
//    } else {
//      return Wire1.read();
//    }
//  }
//}

/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int port, int reg, int val) {
#ifdef NOA_PD_SNACKER
  if(port < 0 || port > 1) {  // support 0 - 1 port only
    return -1;
  }
#else
  if(port < 0 || port > 3) {  // support 0 - 3 port only
    return -1;
  }
#endif
//  WirebeginTransmission(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr);
//  Wirewrite(tcpc_config[port].i2c_host_port, reg & 0xFF);
//  Wirewrite(tcpc_config[port].i2c_host_port, val & 0xFF);
//  WireendTransmission(tcpc_config[port].i2c_host_port, true);
  if (tcpc_config[port].i2c_host_port == 0) {
    // lock
    xSemaphoreTake(wire0_mutex, portMAX_DELAY);
  } else {
    // lock
    xSemaphoreTake(wire1_mutex, portMAX_DELAY);
  }

  uint8_t data_wr[1] = {0};
  data_wr[0] = val;
  NOA_PUB_I2C_master_i2c_write(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, reg, data_wr, 1);

  if (tcpc_config[port].i2c_host_port == 0) {
    // unlock
    xSemaphoreGive(wire0_mutex);
  } else {
    // unlock
    xSemaphoreGive(wire1_mutex);
  }
  return 0;
}

int tcpc_write16(int port, int reg, int val) {
#ifdef NOA_PD_SNACKER
  if(port < 0 || port > 1) {  // support 0 - 1 port only
    return -1;
  }
#else
  if(port < 0 || port > 3) {  // support 0 - 3 port only
    return -1;
  }
#endif
//  WirebeginTransmission(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr);
//  Wirewrite(tcpc_config[port].i2c_host_port, reg & 0xFF);
//  Wirewrite(tcpc_config[port].i2c_host_port, val & 0xFF);
//  Wirewrite(tcpc_config[port].i2c_host_port, (val >> 8) & 0xFF);
//  WireendTransmission(tcpc_config[port].i2c_host_port, true);
  if (tcpc_config[port].i2c_host_port == 0) {
    // lock
    xSemaphoreTake(wire0_mutex, portMAX_DELAY);
  } else {
    // lock
    xSemaphoreTake(wire1_mutex, portMAX_DELAY);
  }

  uint8_t data_wr[2] = {0};
  data_wr[0] = val & 0xFF;
  data_wr[1] = (val >> 8) & 0xFF;
  NOA_PUB_I2C_master_i2c_write(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, reg, data_wr, 2);

  if (tcpc_config[port].i2c_host_port == 0) {
    // unlock
    xSemaphoreGive(wire0_mutex);
  } else {
    // unlock
    xSemaphoreGive(wire1_mutex);
  }
  return 0;
}

int tcpc_read(int port, int reg, int *val) {
#ifdef NOA_PD_SNACKER
  if(port < 0 || port > 1) {  // support 0 - 1 port only
    return -1;
  }
#else
  if(port < 0 || port > 3) {  // support 0 - 3 port only
    return -1;
  }
#endif
//  WirebeginTransmission(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr);
//  Wirewrite(tcpc_config[port].i2c_host_port, reg & 0xFF);
//  WireendTransmission(tcpc_config[port].i2c_host_port, false);
//  WirerequestFrom(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, 1, true);
//  *val = Wireread(tcpc_config[port].i2c_host_port);

  uint8_t data_wr[1] = {0};
  NOA_PUB_I2C_master_i2c_read(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, reg, data_wr, 1);
  *val = data_wr[0];

  return 0;
}

int tcpc_read16(int port, int reg, int *val) {
#ifdef NOA_PD_SNACKER
  if(port < 0 || port > 1) {  // support 0 - 1 port only
    return -1;
  }
#else
  if(port < 0 || port > 3) {  // support 0 - 3 port only
    return -1;
  }
#endif
//  WirebeginTransmission(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr);
//  Wirewrite(tcpc_config[port].i2c_host_port, reg & 0xFF);
//  WireendTransmission(tcpc_config[port].i2c_host_port, false);
//  WirerequestFrom(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, 1, true);
//  *val  = Wireread(tcpc_config[port].i2c_host_port);
//  *val |= (Wireread(tcpc_config[port].i2c_host_port) << 8);

  uint8_t data_wr[2] = {0};
  NOA_PUB_I2C_master_i2c_read(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, reg, data_wr, 2);
  *val = (data_wr[0] | (data_wr[1] << 8));

  return 0;
}

// static i2c_cmd_handle_t cmd = NULL;
int tcpc_xfer(int port, const uint8_t *out, int out_size, uint8_t *in, int in_size, int flags) {
#ifdef NOA_PD_SNACKER
  if(port < 0 || port > 1) {  // support 0 - 1 port only
    return -1;
  }
#else
  if(port < 0 || port > 3) {  // support 0 - 3 port only
    return -1;
  }
#endif

//  WirebeginTransmission(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr);
//  for (; out_size>0; out_size--) {
//    Wirewrite(tcpc_config[port].i2c_host_port, *out);
//    out++;
//  }
//  if (in_size) {
//    WireendTransmission(tcpc_config[port].i2c_host_port, false);
//    WirerequestFrom(tcpc_config[port].i2c_host_port, tcpc_config[port].i2c_slave_addr, (int)in_size, (flags & I2C_XFER_STOP));
//    for (; in_size>0; in_size--) {
//      *in = Wireread(tcpc_config[port].i2c_host_port);
//      in++;
//    }
//  } else {
//    WireendTransmission(tcpc_config[port].i2c_host_port, flags & I2C_XFER_STOP);
//  }
  if (tcpc_config[port].i2c_host_port == 0) {
    // lock
    xSemaphoreTake(wire0_mutex, portMAX_DELAY);
  } else {
    // lock
    xSemaphoreTake(wire1_mutex, portMAX_DELAY);
  }

  i2c_cmd_handle_t cmd = NULL;
  if (out_size) {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, tcpc_config[port].i2c_slave_addr << 1 | WRITE_BIT, ACK_CHECK_EN);

    for (int i = 0; i < out_size; i++) {
      i2c_master_write_byte(cmd, out[i], ACK_CHECK_EN);
    }
  }
  if (in_size) {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, tcpc_config[port].i2c_slave_addr << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, in, in_size - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, in + in_size - 1, I2C_MASTER_NACK);

    if (flags & I2C_XFER_STOP) {
//      APP_DEBUG("Stop! in:%d out:%d", in_size, out_size);
      i2c_master_stop(cmd);
    } else {
//      APP_DEBUG("keep Start! in:%d out:%d", in_size, out_size);
    }
  } else {
    if (flags & I2C_XFER_STOP) {
//      APP_DEBUG("Stop! in:%d out:%d", in_size, out_size);
      i2c_master_stop(cmd);
    }
  }
  esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)tcpc_config[port].i2c_host_port, cmd, 1000 / portTICK_RATE_MS);

//  APP_DEBUG("Delete! in:%d out:%d", in_size, out_size);
  i2c_cmd_link_delete(cmd);

  if (ret == ESP_OK) {
//    APP_DEBUG("xfer buffer OK! in:%d out:%d", in_size, out_size);
  } else if (ret == ESP_ERR_TIMEOUT) {
    APP_DEBUG("Bus is busy");
  } else {
    APP_DEBUG("xfer Failed");
  }
  if (tcpc_config[port].i2c_host_port == 0) {
    // unlock
    xSemaphoreGive(wire0_mutex);
  } else {
    // unlock
    xSemaphoreGive(wire1_mutex);
  }

  return 0;
}


