/*
 * tcpm_driver.c
 *
 * Created: 11/11/2017 18:42:26
 *  Author: jason
 */ 

#include "tcpm_driver.h"
#include "Arduino.h"
#include <Wire.h>

// extern const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT];

extern "C" {
  //#include <Wire.h>

  void WirebeginTransmission(int addr)
  {
    Wire.beginTransmission(addr);
  }

  void Wirewrite(int value)
  {
    Wire.write(value);
  }
  
  void WireendTransmission(int stopBit)
  {
    Wire.endTransmission(stopBit);
  }

  void WirerequestFrom(int addr, int quantity, int stopBit)
  {
    Wire.requestFrom(addr, quantity, stopBit);
  }

  int Wireread()
  {
    return Wire.read();
  }

  void Wire1beginTransmission(int addr)
  {
    Wire1.beginTransmission(addr);
  }

  void Wire1write(int value)
  {
    Wire1.write(value);
  }
  
  void Wire1endTransmission(int stopBit)
  {
    Wire1.endTransmission(stopBit);
  }

  void Wire1requestFrom(int addr, int quantity, int stopBit)
  {
    Wire1.requestFrom(addr, quantity, stopBit);
  }

  int Wire1read()
  {
    return Wire1.read();
  }
}

#ifdef NOA_PD_SNACKER
/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int port, int reg, int val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      Wirewrite(val & 0xFF);
      WireendTransmission(true);
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1write(val & 0xFF);
      Wire1endTransmission(true);
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_write16(int port, int reg, int val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      Wirewrite(val & 0xFF);
      Wirewrite((val >> 8) & 0xFF);
      WireendTransmission(true);
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1write(val & 0xFF);
      Wire1write((val >> 8) & 0xFF);
      Wire1endTransmission(true);
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_read(int port, int reg, int *val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val = Wireread();
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1endTransmission(false);
      Wire1requestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val = Wire1read();
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_read16(int port, int reg, int *val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val  = Wireread();
      *val |= (Wireread() << 8);
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1endTransmission(false);
      Wire1requestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val  = Wire1read();
      *val |= (Wire1read() << 8);
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_xfer(int port,
	const uint8_t *out, int out_size,
	uint8_t *in, int in_size,
	int flags)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      for (; out_size>0; out_size--) {
        Wirewrite(*out);
        out++;
      }
      if (in_size) {
        WireendTransmission(false);
        WirerequestFrom((int)fusb302_I2C_SLAVE_ADDR, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
          *in = Wireread();
          in++;
        }
      } else {
        WireendTransmission(flags & I2C_XFER_STOP);
      }
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      for (; out_size>0; out_size--) {
        Wire1write(*out);
        out++;
      }
      if (in_size) {
        Wire1endTransmission(false);
        Wire1requestFrom((int)fusb302_I2C_SLAVE_ADDR, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
          *in = Wire1read();
          in++;
        }
      } else {
        Wire1endTransmission(flags & I2C_XFER_STOP);
      }
      break;
    default:
      return -1;
  }
  return 0;
}
#else  // for NOA PD STATION
/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int port, int reg, int val)
{
  switch (port) {
    case 0:  // SNK P1
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wirewrite(reg & 0xFF);
      Wirewrite(val & 0xFF);
      WireendTransmission(true);
      break;
    case 1: // SRC1 P2
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1write(val & 0xFF);
      Wire1endTransmission(true);
      break;
    case 2: // SRC2 UP
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      Wirewrite(val & 0xFF);
      WireendTransmission(true);
      break;
    case 3: // SRC3 P3
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wire1write(reg & 0xFF);
      Wire1write(val & 0xFF);
      Wire1endTransmission(true);
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_write16(int port, int reg, int val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wirewrite(reg & 0xFF);
      Wirewrite(val & 0xFF);
      Wirewrite((val >> 8) & 0xFF);
      WireendTransmission(true);
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1write(val & 0xFF);
      Wire1write((val >> 8) & 0xFF);
      Wire1endTransmission(true);
      break;
    case 2:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      Wirewrite(val & 0xFF);
      Wirewrite((val >> 8) & 0xFF);
      WireendTransmission(true);
      break;
    case 3:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wire1write(reg & 0xFF);
      Wire1write(val & 0xFF);
      Wire1write((val >> 8) & 0xFF);
      Wire1endTransmission(true);
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_read(int port, int reg, int *val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wirewrite(reg & 0xFF);
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR_B01, 1, true);
      *val = Wireread();
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1endTransmission(false);
      Wire1requestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val = Wire1read();
      break;
    case 2:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val = Wireread();
      break;
    case 3:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wire1write(reg & 0xFF);
      Wire1endTransmission(false);
      Wire1requestFrom(fusb302_I2C_SLAVE_ADDR_B01, 1, true);
      *val = Wire1read();
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_read16(int port, int reg, int *val)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wirewrite(reg & 0xFF);
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR_B01, 1, true);
      *val  = Wireread();
      *val |= (Wireread() << 8);
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wire1write(reg & 0xFF);
      Wire1endTransmission(false);
      Wire1requestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val  = Wire1read();
      *val |= (Wire1read() << 8);
      break;
    case 2:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      Wirewrite(reg & 0xFF);
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR, 1, true);
      *val  = Wireread();
      *val |= (Wireread() << 8);
      break;
    case 3:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      Wire1write(reg & 0xFF);
      Wire1endTransmission(false);
      Wire1requestFrom(fusb302_I2C_SLAVE_ADDR_B01, 1, true);
      *val  = Wire1read();
      *val |= (Wire1read() << 8);
      break;
    default:
      return -1;
  }
  return 0;
}

int tcpc_xfer(int port,
  const uint8_t *out, int out_size,
  uint8_t *in, int in_size,
  int flags)
{
  switch (port) {
    case 0:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      for (; out_size>0; out_size--) {
        Wirewrite(*out);
        out++;
      }
      if (in_size) {
        WireendTransmission(false);
        WirerequestFrom((int)fusb302_I2C_SLAVE_ADDR_B01, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
          *in = Wireread();
          in++;
        }
      } else {
        WireendTransmission(flags & I2C_XFER_STOP);
      }
      break;
    case 1:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR);
      for (; out_size>0; out_size--) {
        Wire1write(*out);
        out++;
      }
      if (in_size) {
        Wire1endTransmission(false);
        Wire1requestFrom((int)fusb302_I2C_SLAVE_ADDR, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
          *in = Wire1read();
          in++;
        }
      } else {
        Wire1endTransmission(flags & I2C_XFER_STOP);
      }
      break;
    case 2:
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      for (; out_size>0; out_size--) {
        Wirewrite(*out);
        out++;
      }
      if (in_size) {
        WireendTransmission(false);
        WirerequestFrom((int)fusb302_I2C_SLAVE_ADDR, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
          *in = Wireread();
          in++;
        }
      } else {
        WireendTransmission(flags & I2C_XFER_STOP);
      }
      break;
    case 3:
      Wire1beginTransmission(fusb302_I2C_SLAVE_ADDR_B01);
      for (; out_size>0; out_size--) {
        Wire1write(*out);
        out++;
      }
      if (in_size) {
        Wire1endTransmission(false);
        Wire1requestFrom((int)fusb302_I2C_SLAVE_ADDR_B01, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
          *in = Wire1read();
          in++;
        }
      } else {
        Wire1endTransmission(flags & I2C_XFER_STOP);
      }
      break;
    default:
      return -1;
  }
  return 0;
}
#endif
