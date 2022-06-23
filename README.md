# Introduction

This project is an extension of the firmware used on the [**USB-C Explorer**](https://github.com/ReclaimerLabs/USB-C-Explorer), which was a port of the [**Google Chrome EC library**](https://www.chromium.org/chromium-os/ec-development). This code is intended to show how to extend the library to new platforms, and provide a starting point for getting started with USB-C and ESP32 Arduino. 

The basic code is from [**Graycatlabs usb-c-arduino**](https://github.com/graycatlabs/usb-c-arduino). We port it from C to C++ project, make it can work with ESP32 arduino librarys, fix some I2C I/O issue for ESp32 aruino platform, add multi-I2C control in same ESP32 board, add PD SRC mode functions.

# Notes and Limitations
TBD...
# Example Usage

This code can be tested with the NOA ESP32 PD board

# Questions, Comments, and Contributions
1. Patch for ESP32 I2C I/O issue

  Modify FUSB302.c/cpp *get_message() function
  
  change 
  
  rv |= tcpc_xfer(port, 0, 0, buf, 3, I2C_XFER_START);
  
  to
  
  rv |= tcpc_xfer(port, 0, 0, buf, 3, I2C_XFER_STOP);
  
  Modify  *tcpc_xfer() function
  
  change
  
    if (out_size)
    {
      WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
      for (; out_size>0; out_size--) {
        Wirewrite(*out);
        out++;
      }
      if (flags & I2C_XFER_STOP)
      {
        WireendTransmission(true);
      }
      else
      {
        WireendTransmission(false);
      }
    }
    if (in_size) {
      WireendTransmission(false);
      WirerequestFrom(fusb302_I2C_SLAVE_ADDR, in_size, (flags & I2C_XFER_STOP));
      for (; in_size>0; in_size--) {
        *in = Wireread();
        in++;
      }
    }
    
  to
  
    WirebeginTransmission(fusb302_I2C_SLAVE_ADDR);
    for (; out_size>0; out_size--) {
        Wirewrite(*out);
        out++;
    }
    if (in_size) {
        WirerequestFrom((int)fusb302_I2C_SLAVE_ADDR, (int)in_size, (flags & I2C_XFER_STOP));
        for (; in_size>0; in_size--) {
            *in = Wireread();
            in++;
        }
    } else {
        WireendTransmission(flags & I2C_XFER_STOP);
    }
    
2. Import ESP32Logger

3. Add multi-I2C interface in same ESP32 arduino platform

4. Add PD SRC functions code

5. Use 40M SPI speed, DIO SPI mode and 64Mbit FLASHSIZE for Snacker board

6. Change SDK to ESP-IDF release/4.4 branch for Station2 ESP32-S2 board
 a. Checkout ESP32-IDF SDK release/v4.4 branch
  * mkdir d:\esp-idf-tools
  * cd d:\esp-idf-tools
  * notepad run_install.bat

  echo off
  if "%IDF_TOOLS_PATH%" == "" (
    set IDF_TOOLS_PATH=%~dp0
    echo IDF_TOOLS_PATH not set. Setting to %~dp0
  )
  set PATH=%IDF_TOOLS_PATH%;%PATH%

  * call run_install.bat
  * cd d:\
  * git clone https://github.com/espressif/esp-idf.git
  * cd d:\esp-idf
  * git fetch
  * git pull
  * git checkout release/v4.4 -b release/v4.4
  * call install.bat
  * call export.bat

 b. APP Building commands
  * cd d:\
  * git clone https://github.com/NOA-Labs/NOA_ESP32_PD.git
  * cd d:\NOA_ESP32_PD
  * git fetch
  * git pull
  * git checkout origin/STAION_NEW_DEV -b STAION_NEW_DEV
  * idf.py set-target esp32s2
  * idf.py menuconfig
  * idf.py clean
  * idf.py fullclean
  * idf.py build
  * idf.py -p COM81 flash monitor
  * idf.py -p COM81 monitor
 
