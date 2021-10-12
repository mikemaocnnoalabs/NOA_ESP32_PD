/*
  NOA_public.cpp - NOA arduino public functions
  Copyright 2012 NOA Labs
  Copyright 2021 Mike Mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Wire.h>

#include "NOA_public.h"

uint8_t tx_buf[80] = {0};
uint8_t rx_buf[80] = {0};
uint8_t temp_buf[80] = {0};

void NOA_PUB_ESP32DebugInit() {
  // Initialize debugging with timestamp since start (start = 0) 
  DBGINI(&Serial, ESP32Timestamp::TimestampSinceStart)
  // Start debugging
  DBGSTA
  // Log an info-message
  DBGLOG(Info, "Program ExampleESP32Debugging.ino startet.")
  // Log an error-message
  DBGLOG(Error, "This is an error message.")
  // Log a debug-message, which will not be shown, because actual debug-level is "Info"
  DBGLOG(Debug, "This is an message with logging level 'Debug' which will not be printed.")
  // Change debug-level to "Debug"
  DBGLEV(Debug)
  // The following debug-message will now be shown
  DBGLOG(Debug, "This is an message with logging level 'Debug' which will be printed.")
  // Stop debugging
  DBGSTP
  // While debugging is stopped messages will not be shown
  DBGLOG(Info, "This will not be printed because Debugging is stopped.")
  // Activate debugging again
  DBGSTA
  DBGLOG(Info, "End of Debug LOG setup.")
}

void NOA_PUB_I2C_Scanner(uint8_t nIndex){
  int nDevices = 0;
  Serial.println("Scanning...");
  for (byte address = 1; address < 127; ++address) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    byte error = 0;
    if (nIndex == 0) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
    } else {
      Wire1.beginTransmission(address);
      error = Wire1.endTransmission();
    }

    if (error == 0) {
      Serial.print("I2C ");
      Serial.print(nIndex);
      Serial.print(" bus found device at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");
      ++nDevices;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.print("No I2C devices found in I2C ");
    Serial.print(nIndex);
    Serial.println(" bus\n");
  } else {
    Serial.println("done\n");
  }
//  delay(5000); // Wait 5 seconds for next scan
}

void NOA_PUB_I2C_PD_RreadAllRegs(uint8_t nIndex, uint8_t PD_ADDR) {
  if (nIndex == 0) {
    Wire.beginTransmission(PD_ADDR);
    Wire.write(0x01);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)PD_ADDR, (uint8_t)16, true);
  } else {
    Wire1.beginTransmission(PD_ADDR);
    Wire1.write(0x01);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint16_t)PD_ADDR, (uint8_t)16, true);
  }
  uint8_t c = 0;
  for (int i=1; i<=16; i++) {  // FUSB302 只有0x1-0x10 和0x3C-0x42寄存器 0x43是FIFO data
    if (nIndex == 0) {
      c = Wire.read();
    } else {
      c = Wire1.read();
    }
    Serial.print("Address: 0x");
    Serial.print(i, HEX);
    Serial.print(", Value: 0x");
    Serial.println(c, HEX);
    switch(i) {
      case 1:
//        Serial.println((c >> 4));
//        Serial.println((c >> 2) & 0b00000011);
//        Serial.println(c & 0b00000011);
        if ((c >> 4) == 0b1000) {
          Serial.print("A_");
        } else if ((c >> 4) == 0b1001) {
          Serial.print("B_");
        } else if ((c >> 4) == 0b1010) {
          Serial.print("C_");
        } else {
          Serial.print("UnknowV_");
        }
        if (((c >> 2) & 0b00000011) == 0b00) {
          Serial.print("FUSB302BMPX_");
        } else if (((c >> 2) & 0b00000011) == 0b01) {
          Serial.print("FUSB302B01MPX_");
        } else if (((c >> 2) & 0b00000011) == 0b10) {
          Serial.print("FUSB302B10MPX_");
        } else if (((c >> 2) & 0b00000011) == 0b11) {
          Serial.print(" FUSB302B11MPX_");
        } else {
          Serial.print("UnknowP_");
        }
        if ((c & 0b00000011) == 0b00) {
          Serial.println("revA");
        } else if ((c & 0b00000011) == 0b01) {
          Serial.println("revB");
        } else if ((c & 0b00000011) == 0b10) {
          Serial.println("revC");
        } else if ((c & 0b00000011) == 0b11) {
          Serial.println("revD");
        } else {
          Serial.println("UnknowR");
        }
        break;
      case 2:
        break;
      case 3:
        break;
    }
  }

  if (nIndex == 0) {
    Wire.beginTransmission(PD_ADDR);
    Wire.write(0x3C);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)PD_ADDR, (uint8_t)7, true); 
  } else {
    Wire1.beginTransmission(PD_ADDR);
    Wire1.write(0x3C);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint16_t)PD_ADDR, (uint8_t)7, true);
  }
  c = 0;
  for (int i=0x3C; i<=0x42; i++) {
    if (nIndex == 0) {
      c = Wire.read();
    } else {
      c = Wire1.read();
    }
    Serial.print("Address: 0x");
    Serial.print(i, HEX);
    Serial.print(", Value: 0x");
    Serial.println(c, HEX);
  }
  Serial.println();
  Serial.println();
}

void NOA_PUB_I2C_PM_RreadAllRegs(uint8_t nIndex, uint8_t PD_ADDR) {
  if (nIndex == 0) {
    Wire.beginTransmission(PD_ADDR);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)PD_ADDR, (uint8_t)11, true);
  } else {
    Wire1.beginTransmission(PD_ADDR);
    Wire1.write(0x00);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint16_t)PD_ADDR, (uint8_t)11, true);
  }
  uint8_t c = 0;
  for (int i=0x0; i<=0x0A; i++) {  // NCP81239 只有0x0-0x0A(read write) 0x10-0x15(read only)
    if (nIndex == 0) {
      c = Wire.read();
    } else {
      c = Wire1.read();
    }
    Serial.print("Address: 0x");
    Serial.print(i, HEX);
    Serial.print(", Value: 0x");
    Serial.println(c, HEX);
    switch(i) {
      case 1:
        break;
      case 2:
        break;
      case 3:
        break;
    }
  }
  if (nIndex == 0) {
    Wire.beginTransmission(PD_ADDR);
    Wire.write(0x10);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)PD_ADDR, (uint8_t)6, true); 
  } else {
    Wire1.beginTransmission(PD_ADDR);
    Wire1.write(0x10);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint16_t)PD_ADDR, (uint8_t)6, true);
  }
  c = 0;
  for (int i=0x10; i<=0x15; i++) {
    if (nIndex == 0) {
      c = Wire.read();
    } else {
      c = Wire1.read();
    }
    Serial.print("Address: 0x");
    Serial.print(i, HEX);
    Serial.print(", Value: 0x");
    Serial.println(c, HEX);
  }
  Serial.println();
  Serial.println();
}

void NOA_PUB_I2C_SetReg(uint8_t nIndex, uint8_t PD_ADDR, uint8_t addr, uint8_t value) {
  if (nIndex == 0) {
    Wire.beginTransmission(PD_ADDR);
    Wire.write(addr);
    Wire.write(value);
    Wire.endTransmission(true); 
  } else {
    Wire1.beginTransmission(PD_ADDR);
    Wire1.write(addr);
    Wire1.write(value);
    Wire1.endTransmission(true); 
  }
}

uint8_t NOA_PUB_I2C_GetReg(uint8_t nIndex, uint8_t PD_ADDR, uint8_t addr) {
  if (nIndex == 0) {
    Wire.beginTransmission(PD_ADDR);
    Wire.write(addr);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)PD_ADDR, (uint8_t)1, true);
    return Wire.read(); 
  } else {
    Wire1.beginTransmission(PD_ADDR);
    Wire1.write(addr);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint16_t)PD_ADDR, (uint8_t)1, true);
    return Wire1.read();
  }
}

void NOA_PUB_I2C_SendBytes(uint8_t nIndex, uint8_t PD_ADDR, uint8_t addr, uint8_t *data, uint16_t length) {
  if (length > 0) {
    if (nIndex == 0) {
      Wire.beginTransmission(PD_ADDR);
      Wire.write(addr);
      for (uint16_t i=0; i<length; i++) {
        Wire.write(data[i]);
      }
      Wire.endTransmission(true); 
    } else {
      Wire1.beginTransmission(PD_ADDR);
      Wire1.write(addr);
      for (uint16_t i=0; i<length; i++) {
        Wire1.write(data[i]);
      }
      Wire1.endTransmission(true); 
    }
  }
}

void NOA_PUB_I2C_ReceiveBytes(uint8_t nIndex, uint8_t PD_ADDR, uint8_t addr, uint8_t *data, uint16_t length) {
  if (length > 0) {
    if (nIndex == 0) {
      Wire.beginTransmission(PD_ADDR);
      Wire.write(addr);
      Wire.endTransmission(false);
      Wire.requestFrom((uint16_t)PD_ADDR, (uint8_t)length, true);
      for (uint16_t i=0; i<length; i++) {
        data[i] = Wire.read();
      }
    } else {
//     Serial.println(PD_ADDR, HEX);
//     Serial.println(addr, HEX);
//     Serial.println(length);
      Wire1.beginTransmission(PD_ADDR);
      Wire1.write(addr);
      Wire1.endTransmission(false);
      Wire1.requestFrom((uint16_t)PD_ADDR, (uint8_t)length, true);
      for (uint16_t i=0; i<length; i++) {
        data[i] = Wire1.read();
//        Serial.println(data[i], HEX);
      }
    }
  }
}

bool NOA_PUB_I2C_PD_ReceivePacket(uint8_t nIndex, uint8_t PD_ADDR) {
  uint8_t num_data_objects;
  uint8_t message_id;
  uint8_t port_power_role;
  uint8_t spec_rev;
  uint8_t port_data_role;
  uint8_t message_type;
  
  NOA_PUB_I2C_ReceiveBytes(nIndex, PD_ADDR, 0x43, rx_buf, 1);
  if (rx_buf[0] != 0xE0) {
    // implement other features later
    Serial.print("FAIL: 0x");
    Serial.println(rx_buf[0], HEX);
    return false;
  }

  NOA_PUB_I2C_ReceiveBytes(nIndex, PD_ADDR, 0x43, rx_buf, 2);
  
  num_data_objects = ((rx_buf[1] & 0x70) >> 4);
  message_id       = ((rx_buf[1] & 0x0E) >> 1);
  port_power_role  = (rx_buf[1] & 0x01);
  spec_rev         = ((rx_buf[0] & 0xC0) >> 6);
  port_data_role   = ((rx_buf[0] & 0x10) >> 5);
  message_type     = (rx_buf[0] & 0x0F);
  Serial.println("Received SOP Packet");
  Serial.print("Header: 0x");
  Serial.println(*(int *)rx_buf, HEX);
  Serial.print("num_data_objects = ");
  Serial.println(num_data_objects, DEC);
  Serial.print("message_id       = ");
  Serial.println(message_id, DEC);
  Serial.print("port_power_role  = ");
  Serial.println(port_power_role, DEC);
  Serial.print("spec_rev         = ");
  Serial.println(spec_rev, DEC);
  Serial.print("port_data_role   = ");
  Serial.println(port_data_role, DEC);
  Serial.print("message_type     = ");
  Serial.println(message_type, DEC);

  NOA_PUB_I2C_ReceiveBytes(nIndex, PD_ADDR, 0x43, rx_buf, (num_data_objects*4));
  // each data object is 32 bits
  for (uint8_t i=0; i<num_data_objects; i++) {
    Serial.print("Object: 0x");
    Serial.println(*(long *)(rx_buf+(i*4)), HEX);
  }  

  // CRC-32
  NOA_PUB_I2C_ReceiveBytes(nIndex, PD_ADDR, 0x43, rx_buf, 4);
  Serial.print("CRC-32: 0x");
  Serial.println(*(long *)rx_buf, HEX);
  Serial.println();

  return true;
}

void NOA_PUB_I2C_PD_SendPacket( \
      uint8_t nIndex,  \
      uint8_t PD_ADDR,  \
      uint8_t num_data_objects, \
      uint8_t message_id, \
      uint8_t port_power_role, \
      uint8_t spec_rev, \
      uint8_t port_data_role, \
      uint8_t message_type, \
      uint8_t *data_objects ) {

  uint8_t temp;
  tx_buf[0]  = 0x12; // SOP, see USB-PD2.0 page 108
  tx_buf[1]  = 0x12;
  tx_buf[2]  = 0x12;
  tx_buf[3]  = 0x13;
  tx_buf[4]  = (0x80 | (2 + (4*(num_data_objects & 0x1F))));
  tx_buf[5]  = (message_type & 0x0F);
  tx_buf[5] |= ((port_data_role & 0x01) << 5);
  tx_buf[5] |= ((spec_rev & 0x03) << 6);
  tx_buf[6]  = (port_power_role & 0x01);
  tx_buf[6] |= ((message_id & 0x07) << 1);
  tx_buf[6] |= ((num_data_objects & 0x07) << 4);

  Serial.print("Sending Header: 0x");
  Serial.println(*(uint16_t *)(tx_buf+5), HEX);
  Serial.println();

  temp = 7;
  for(uint8_t i=0; i<num_data_objects; i++) {
    tx_buf[temp]   = data_objects[(4*i)];
    tx_buf[temp+1] = data_objects[(4*i)+1];
    tx_buf[temp+2] = data_objects[(4*i)+2];
    tx_buf[temp+3] = data_objects[(4*i)+3];
    temp += 4;
  }

  tx_buf[temp] = 0xFF; // CRC
  tx_buf[temp+1] = 0x14; // EOP
  tx_buf[temp+2] = 0xFE; // TXOFF

  Serial.print("Sending ");
  Serial.print((10 + (4*(num_data_objects & 0x1F))), DEC);
  Serial.println(" bytes.");

  for (uint16_t i=0; i<(10 + (4*(num_data_objects & 0x1F))); i++) {
    Serial.print("0x");
    Serial.println(tx_buf[i], HEX);
  }
  Serial.println();

  temp = NOA_PUB_I2C_GetReg(nIndex, PD_ADDR, 0x06);
  NOA_PUB_I2C_SendBytes(nIndex, PD_ADDR, 0x43, tx_buf, (10+(4*(num_data_objects & 0x1F))) );
  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x06, (temp | (0x01))); // Flip on TX_START
}

void NOA_PUB_I2C_PD_Testing(uint8_t nIndex, uint8_t PD_ADDR)
{
  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x0C, 0x01); // Reset FUSB302
  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x0B, 0x0F); // FULL POWER!
  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x07, 0x04); // Flush RX
  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x02, 0x07); // Switch on MEAS_CC1
  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x03, 0x25); // Enable BMC Tx on CC1

//  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x03, 0x05); // Enable BMC Tx on CC1

//  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x02, 0x0B); // Switch on MEAS_CC2  
//  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x03, 0x26); // Enable BMC Tx on CC2
//  NOA_PUB_I2C_SetReg(nIndex, PD_ADDR, 0x03, 0x06); // Enable BMC Tx on CC2

  NOA_PUB_I2C_PD_RreadAllRegs(nIndex, PD_ADDR);
  
  NOA_PUB_I2C_PD_SendPacket(nIndex, PD_ADDR, 0, 2, 0, 1, 0, 0x7, NULL);
  delayMicroseconds(250);
  while ( NOA_PUB_I2C_GetReg(nIndex, PD_ADDR, 0x41) & 0x20 ) {  // 判断RX里面不为NULL
    delay(1);
  }
  NOA_PUB_I2C_PD_ReceivePacket(nIndex, PD_ADDR);
  DBGLOG(Info, "Check 0x41 register = 0x%02X.", NOA_PUB_I2C_GetReg(nIndex, PD_ADDR, 0x41));
  
  temp_buf[0] = 0b01100100;
  temp_buf[1] = 0b10010000;
  temp_buf[2] = 0b00000001;
  temp_buf[3] = 0b00100000;
  NOA_PUB_I2C_PD_SendPacket(nIndex, PD_ADDR, 1, 3, 0, 1, 0, 0x2, temp_buf);
  delayMicroseconds(250);
  while ( NOA_PUB_I2C_GetReg(nIndex, PD_ADDR, 0x41) & 0x20 ) {
    delay(1);
  }
  NOA_PUB_I2C_PD_ReceivePacket(nIndex, PD_ADDR);
  DBGLOG(Info, "Check 0x41 register = 0x%02X.", NOA_PUB_I2C_GetReg(nIndex, PD_ADDR, 0x41));

  NOA_PUB_I2C_PD_RreadAllRegs(nIndex, PD_ADDR);
}
