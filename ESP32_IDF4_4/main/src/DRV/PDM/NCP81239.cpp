/*
  NCP81239.cpp - Library for interacting with the NCP81239 chip.
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <driver/gpio.h>

#include "tcpm_driver.h"
#include "NCP81239.h"
#include "..\..\LIB\PUB\NOA_debug.h"
#include "..\..\LIB\PUB\NOA_public.h"

//****************************************************************************
// CODE TABLES
//****************************************************************************
StructNCP81239RegisterMap g_stPmicInitialData =
{
    0x00,                              // En pol
    0x00,                              // En pup
    0x00,                              // En mask
    0x00,                              // En int
    0x00,                              // Reserved

    _ADDR_01_VBUS,                     // VBUS setting is 5V

    _ADDR_02_SLEW_RATE,                // Slew rate by define
    0x00,                              // Reserved

    _ADDR_03_PWM_FREQ,                 // by define PWM freq
    0x00,                              // Reserved
    0x00,                              // DAC resolution(0x01 is 5mV  0x00 is 10mV)
    0x00,                              // Reserved

    0x00,                              // PFET is disable
    0x00,                              // CFET is disable
    0x00,                              // dead_battery_en is disable
    0x00,                              // Reserved
    0x00,                              // CS1 discharge is disable
    0x00,                              // CS2 discharge is disable
    0x00,                              // Reserved

    _ADDR_05_OCP_CLIM_POS,             // internal positive current limit by define
    0x00,                              // Reserved
    _ADDR_05_OCP_CLIM_NEG,             // internal negative current limit by define
    0x00,                              // Reserved

    _ADDR_06_CS1_CLIND,                // CS1 positive current limit
    _ADDR_06_CS2_CLIND,                // CS2 positive current limit
    0x00,                              // Reserved

    0x01,                              // Low Amp GM value
    0x01,                              // register of amp gm settings
    0x05,                              // High Amp GM value
    0x00,                              // register of amp gm config

    _ADDR_08_ADC_AMUX_TRIGGER,         // ADC Trigger
    _ADDR_08_ADC_AMUX_SEL,             // ADC MUX select
    0x00,                              // ADC is enable
    0x00,                              // Reserved

    _ADDR_09_INT_MASK_CS_CLIND,        // cs_clind int flag enable by define
    _ADDR_09_INT_MASK_OVP,             // OVP int flag enable by define
    _ADDR_09_INT_MASK_OCP_P,           // OCP_P int flag enable by define
    _ADDR_09_INT_MASK_PG_INT,          // PG int flag enable by define
    _ADDR_09_INT_MASK_TSD,             // TSD int flag enable by define
    _ADDR_09_INT_MASK_UV,              // UV int flag enable by define
    _ADDR_09_INT_MASK_VCHN,            // VCHN int flag enable by define
    _ADDR_09_INT_MASK_IIC_ACK,         // IIC_ACK int flag enable by define

    _ADDR_0A_INT_MASK_SHUT_DOWN,       // SHUT_DOWN int flag enable by define
    0x00,                              // Reserved

    0x00,                              // Reserved
    0x00,                              // Reserved
    0x00,                              // Reserved
    0x00,                              // Reserved
    0x00,                              // Reserved

    0x00,                              // Vfb value(Read Only)
    0x00,                              // Vin value(Read Only)
    0x00,                              // CS2 value(Read Only)
    0x00,                              // CS1 value(Read Only)

    0x00,                              // CsClind flag(Read Only)
    0x00,                              // OVP flag(Read Only)
    0x00,                              // OCP_P flag(Read Only)
    0x00,                              // PG flag(Read Only)
    0x00,                              // TSD flag(Read Only)
    0x00,                              // Reserved(Read Only)
    0x00,                              // VCHN flag(Read Only)
    0x00,                              // IIC_ACK flag(Read Only)

    0x00,                              // SHUT_DOWN flag(Read Only)
    0x00,                              // Reserved
};

//****************************************************************************
// VARIABLE DECLARATIONS
//****************************************************************************
StructNCP81239RegisterMap g_stPMICData[CONFIG_NCP_PM_PORT_COUNT];

int     ncp81239_pmic_init(int port) {
  g_stPMICData[port] = g_stPmicInitialData;  
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
  if (port == 1) {
    int cc1 = 0, cc2 = 0;
    tcpm_get_cc(port, &cc1, &cc2);
    APP_DEBUG("C%d CC1 %d CC2 %d", port, cc1, cc2);
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
  int cc1 = 0, cc2 = 0;
  tcpm_get_cc(port, &cc1, &cc2);
  APP_DEBUG("C%d CC1 %d CC2 %d", port, cc1, cc2);

  if (port == 2) {
    if ((cc1 == 2 || cc2 == 2) && gpio_get_level((gpio_num_t)panda_s4_pin) == 1) {
      g_stPMICData[port].b1CR00EnPol = 0x00;
      g_stPMICData[port].b1CR00EnPup = 0x01;  // en_pup=1, en_pol=0: A high on the enable pin generated a turn on the pull up current ensure the parts starts immediately
      g_stPMICData[port].b1CR00EnMask = 0x01; // en_mask=1, en_int=1: turn on part
      g_stPMICData[port].b1CR00EnInternal = 0x01;
      APP_DEBUG("Warning! Init C%d VBus to %dV", port, 0x96/10);
      g_stPMICData[port].ucCR01DacTarget = 0x96;  // set port2 default voltage to 15V, not save for device
    }
  }
/*  APP_DEBUG("INIT:En pol %d", g_stPMICData[port].b1CR00EnPol);
  APP_DEBUG("INITc:En pup %d", g_stPMICData[port].b1CR00EnPup);
  APP_DEBUG("INIT:En mask %d", g_stPMICData[port].b1CR00EnMask);
  APP_DEBUG("INIT:En int %d", g_stPMICData[port].b1CR00EnInternal);
  APP_DEBUG("INIT:DAC targer %d", g_stPMICData[port].ucCR01DacTarget);
  APP_DEBUG("INIT:Slew rate %d", g_stPMICData[port].b2CR02SlewRate);
  APP_DEBUG("INIT:PWM Freq %d", g_stPMICData[port].b3CR03PwmFreq);
  APP_DEBUG("INIT:DAC lsb %d", g_stPMICData[port].b1CR03DacLsb);
  APP_DEBUG("INIT:Pfet %d", g_stPMICData[port].b1CR04Pfet);
  APP_DEBUG("INIT:Cfet %d", g_stPMICData[port].b1CR04Cfet);
  APP_DEBUG("INIT:Dead battery en %d", g_stPMICData[port].b1CR04DeadBatteryEn);
  APP_DEBUG("INIT:Cs1 dchrg %d", g_stPMICData[port].b1CR04Cs1DisCharge);
  APP_DEBUG("INIT:Cs2 dchrg %d", g_stPMICData[port].b1CR04Cs2DisCharge);
  
  APP_DEBUG("INIT:Ocp clim pos %d", g_stPMICData[port].b2CR05OcpClimPos);
  APP_DEBUG("INIT:Ocp clim neg %d", g_stPMICData[port].b2CR05OcpClimNeg);
  APP_DEBUG("INIT:Cs1 clim pos %d", g_stPMICData[port].b2CR06Cs1Clind);
  APP_DEBUG("INIT:Cs2 clim pos %d", g_stPMICData[port].b2CR06Cs2Clind);
  APP_DEBUG("INIT:Lo gm amp set %d", g_stPMICData[port].b3CR07LoGmAmpSetting);
  APP_DEBUG("INIT:Gm manual %d", g_stPMICData[port].b1CR07GmManual);
  APP_DEBUG("INIT:Hi gm amp set %d", g_stPMICData[port].b3CR07HiGmAmpSetting);
  APP_DEBUG("INIT:Gm amp config %d", g_stPMICData[port].b1CR07GmAmpConfig);
  APP_DEBUG("INIT:Amux trigger %d", g_stPMICData[port].b2CR08AmuxTrigger);
  APP_DEBUG("INIT:Amux sel %d", g_stPMICData[port].b3CR08AmuxSel);
  APP_DEBUG("INIT:Dis adc %d", g_stPMICData[port].b1CR08DisAdc);
  APP_DEBUG("INIT:Int mask clind %d", g_stPMICData[port].b1CR09IntMaskCsClind);
  APP_DEBUG("INIT:Int mask ov %d", g_stPMICData[port].b1CR09IntMaskOv);

  APP_DEBUG("INIT:Int mask ov %d", g_stPMICData[port].b1CR09IntMaskOv);
  APP_DEBUG("INIT:Int mask ocp p %d", g_stPMICData[port].b1CR09IntMaskOcpP);
  APP_DEBUG("INIT:Int mask pg %d", g_stPMICData[port].b1CR09IntMaskPg);
  APP_DEBUG("INIT:Int mask tsd %d", g_stPMICData[port].b1CR09IntMaskTsd);
  APP_DEBUG("INIT:Int mask uv %d", g_stPMICData[port].b1CR09IntMaskUv);
  APP_DEBUG("INIT:Int mask vchn %d", g_stPMICData[port].b1CR09IntMaskVchn);
  APP_DEBUG("INIT:Int mask i2c ack %d", g_stPMICData[port].b1CR09IntMaskI2cAck);
  APP_DEBUG("INIT:Int mask shutdown %d", g_stPMICData[port].b1CR0AIntMaskShutDown);
  
  APP_DEBUG("INIT:VFB Value %d", g_stPMICData[port].ucCR10VfbValue);
  APP_DEBUG("INITc:Vin Value %d", g_stPMICData[port].ucCR11VinValue);
  APP_DEBUG("INIT:CS2 Value %d", g_stPMICData[port].ucCR12Cs2Value);
  APP_DEBUG("INIT:CS1 Value %d", g_stPMICData[port].ucCR13Cs1Value);
  APP_DEBUG("INIT:Cs Clind Flag %d", g_stPMICData[port].b1CR14IntCsClindFlag);
  APP_DEBUG("INIT:VBUS OVP Flag %d", g_stPMICData[port].b1CR14IntOvpFlag);
  APP_DEBUG("INIT:OCP_P Flag %d", g_stPMICData[port].b1CR14IntOcpPFlag);
  APP_DEBUG("INIT:Power Good Flag %d", g_stPMICData[port].b1CR14IntPgIntFlag);
  APP_DEBUG("INIT:Thermal Sensor Flag %d", g_stPMICData[port].b1CR14IntTsdFlag);
  APP_DEBUG("INIT:Vchn Flag %d", g_stPMICData[port].b1CR14IntVchnFlag);
  APP_DEBUG("INIT:IIC ACK Flag %d", g_stPMICData[port].b1CR14IntI2cAckFlag);
  APP_DEBUG("INIT:Shut Down Flag %d", g_stPMICData[port].b1CR15IntShutDownFlag); */
#endif
  return 0;
}
int ncp81239_pmic_set_int_mask(int port) {
  uint8_t ucResult = 0;
  g_stPMICData[port].b1CR09IntMaskCsClind = 1;
  g_stPMICData[port].b1CR09IntMaskI2cAck = 1;
  g_stPMICData[port].b1CR09IntMaskOcpP = 1;
  g_stPMICData[port].b1CR09IntMaskOv = 1;
  g_stPMICData[port].b1CR09IntMaskPg = 1;
  g_stPMICData[port].b1CR09IntMaskTsd = 1;
  g_stPMICData[port].b1CR09IntMaskUv = 1;
  g_stPMICData[port].b1CR09IntMaskVchn = 1;
#ifdef NOA_PD_SNACKER
    if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
      return -1;
    }
    switch (port) {
      case 1:
//        NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1); 
        NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        break;
      case 2:
//        NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        NOA_PUB_I2C_master_i2c_write(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        break;
    }
#else
    if(port < 1 || port > 3) {  // support 1 - 3 port only
      return -1;
    }
    switch (port) {
      case 1:
//        NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1); 
        NOA_PUB_I2C_master_i2c_write(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        break;
      case 2:
//        NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        break;
      case 3:
//        NOA_PUB_I2C_SendBytes(1, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        NOA_PUB_I2C_master_i2c_write(0, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG09, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG09, 1);
        break;
    }
#endif
  return ucResult;
}
int ncp81239_pmic_get_tatus(int port) {
  uint8_t ucResult = 0;
  ncp81239_pmic_set_int_mask(port);
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
  switch (port) {
    case 1: // SRC
//      NOA_PUB_I2C_ReceiveBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      NOA_PUB_I2C_master_i2c_read(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      break;
    case 2: // Wireless charger
//      NOA_PUB_I2C_ReceiveBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      NOA_PUB_I2C_master_i2c_read(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      break;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
//  NOA_PUB_I2C_ReceiveBytes(port, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);

/*  APP_DEBUG("Pmic:En pol %d", g_stPMICData.b1CR00EnPol);
  APP_DEBUG("Pmic:En pup %d", g_stPMICData.b1CR00EnPup);
  APP_DEBUG("Pmic:En mask %d", g_stPMICData.b1CR00EnMask);
  APP_DEBUG("Pmic:En int %d", g_stPMICData.b1CR00EnInternal);
  APP_DEBUG("Pmic:DAC targer %d", g_stPMICData.ucCR01DacTarget);
  APP_DEBUG("Pmic:Slew rate %d", g_stPMICData.b2CR02SlewRate);
  APP_DEBUG("Pmic:PWM Freq %d", g_stPMICData.b3CR03PwmFreq);
  APP_DEBUG("Pmic:DAC lsb %d", g_stPMICData.b1CR03DacLsb);
  APP_DEBUG("Pmic:Pfet %d", g_stPMICData.b1CR04Pfet);
  APP_DEBUG("Pmic:Cfet %d", g_stPMICData.b1CR04Cfet);
  APP_DEBUG("Pmic:Dead battery en %d", g_stPMICData.b1CR04DeadBatteryEn);
  APP_DEBUG("Pmic:Cs1 dchrg %d", g_stPMICData.b1CR04Cs1DisCharge);
  APP_DEBUG("Pmic:Cs2 dchrg %d", g_stPMICData.b1CR04Cs2DisCharge);
  
  APP_DEBUG("Pmic:Ocp clim pos %d", g_stPMICData.b2CR05OcpClimPos);
  APP_DEBUG("Pmic:Ocp clim neg %d", g_stPMICData.b2CR05OcpClimNeg);
  APP_DEBUG("Pmic:Cs1 clim pos %d", g_stPMICData.b2CR06Cs1Clind);
  APP_DEBUG("Pmic:Cs2 clim pos %d", g_stPMICData.b2CR06Cs2Clind);
  APP_DEBUG("Pmic:Lo gm amp set %d", g_stPMICData.b3CR07LoGmAmpSetting);
  APP_DEBUG("Pmic:Gm manual %d", g_stPMICData.b1CR07GmManual);
  APP_DEBUG("Pmic:Hi gm amp set %d", g_stPMICData.b3CR07HiGmAmpSetting);
  APP_DEBUG("Pmic:Gm amp config %d", g_stPMICData.b1CR07GmAmpConfig);
  APP_DEBUG("Pmic:Amux trigger %d", g_stPMICData.b2CR08AmuxTrigger);
  APP_DEBUG("Pmic:Amux sel %d", g_stPMICData.b3CR08AmuxSel);
  APP_DEBUG("Pmic:Dis adc %d", g_stPMICData.b1CR08DisAdc);
  APP_DEBUG("Pmic:Int mask clind %d", g_stPMICData.b1CR09IntMaskCsClind);
  APP_DEBUG("Pmic:Int mask ov %d", g_stPMICData.b1CR09IntMaskOv);

  APP_DEBUG("Pmic:Int mask ov %d", g_stPMICData.b1CR09IntMaskOv);
  APP_DEBUG("Pmic:Int mask ocp p %d", g_stPMICData.b1CR09IntMaskOcpP);
  APP_DEBUG("Pmic:Int mask pg %d", g_stPMICData.b1CR09IntMaskPg);
  APP_DEBUG("Pmic:Int mask tsd %d", g_stPMICData.b1CR09IntMaskTsd);
  APP_DEBUG("Pmic:Int mask uv %d", g_stPMICData.b1CR09IntMaskUv);
  APP_DEBUG("Pmic:Int mask vchn %d", g_stPMICData.b1CR09IntMaskVchn);
  APP_DEBUG("Pmic:Int mask i2c ack %d", g_stPMICData.b1CR09IntMaskI2cAck);
  APP_DEBUG("Pmic:Int mask shutdown %d", g_stPMICData.b1CR0AIntMaskShutDown); */

  switch (port) {
    case 1: // SRC1 P2
//      NOA_PUB_I2C_ReceiveBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      NOA_PUB_I2C_master_i2c_read(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      break;
    case 2: // SRC2 UP
//      NOA_PUB_I2C_ReceiveBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      NOA_PUB_I2C_master_i2c_read(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      break;
    case 3: // SRC3 P3
//      NOA_PUB_I2C_ReceiveBytes(1, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      NOA_PUB_I2C_master_i2c_read(0, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      break;
  }
#endif
  APP_DEBUG("Pmic:VFB Value %d", g_stPMICData[port].ucCR10VfbValue);
  APP_DEBUG("Pmic:Vin Value %d", g_stPMICData[port].ucCR11VinValue);
  APP_DEBUG("Pmic:CS2 Value %d", g_stPMICData[port].ucCR12Cs2Value);
  APP_DEBUG("Pmic:CS1 Value %d", g_stPMICData[port].ucCR13Cs1Value);
  APP_DEBUG("Pmic:Cs Clind Flag %d", g_stPMICData[port].b1CR14IntCsClindFlag);
  APP_DEBUG("Pmic:VBUS OVP Flag %d", g_stPMICData[port].b1CR14IntOvpFlag);
  APP_DEBUG("Pmic:OCP_P Flag %d", g_stPMICData[port].b1CR14IntOcpPFlag);
  APP_DEBUG("Pmic:Power Good Flag %d", g_stPMICData[port].b1CR14IntPgIntFlag);
  APP_DEBUG("Pmic:Thermal Sensor Flag %d", g_stPMICData[port].b1CR14IntTsdFlag);
  APP_DEBUG("Pmic:Vchn Flag %d", g_stPMICData[port].b1CR14IntVchnFlag);
  APP_DEBUG("Pmic:IIC ACK Flag %d", g_stPMICData[port].b1CR14IntI2cAckFlag);
  APP_DEBUG("Pmic:Shut Down Flag %d", g_stPMICData[port].b1CR15IntShutDownFlag);

  return ucResult;
}

int ncp81239_pmic_set_tatus(int port) {
  uint8_t ucResult = 0;
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
  switch (port) {
    case 1:
//      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
    case 2:
//      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      NOA_PUB_I2C_master_i2c_write(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
  switch (port) {
    case 1:
//      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      NOA_PUB_I2C_master_i2c_write(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
    case 2:
//      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
    case 3:
//      NOA_PUB_I2C_SendBytes(1, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      NOA_PUB_I2C_master_i2c_write(0, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
  }
#endif
  return ucResult;
}

int ncp81239_pmic_set_voltage(int port) {
  uint8_t ucResult = 0;
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
  switch (port) {
    case 1:
//      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1); 
      NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
    case 2:
//      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      NOA_PUB_I2C_master_i2c_write(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }

  switch (port) {
    case 1:
//      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1); 
      NOA_PUB_I2C_master_i2c_write(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
    case 2:
      if (g_stPMICData[port].ucCR01DacTarget == 0x96) {
        g_stPMICData[port].b1CR00EnPol = 0x00;
        g_stPMICData[port].b1CR00EnPup = 0x01;  // en_pup=1, en_pol=0: A high on the enable pin generated a turn on the pull up current ensure the parts starts immediately
        g_stPMICData[port].b1CR00EnMask = 0x01; // en_mask=1, en_int=1: turn on part
        g_stPMICData[port].b1CR00EnInternal = 0x01;
        APP_DEBUG("Warning! C%d VBus is up to %dV", port, 0x96/10);
        NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 1);
      }
      NOA_PUB_I2C_master_i2c_write(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
    case 3:
      NOA_PUB_I2C_master_i2c_write(0, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
  }
//  NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port].ucCR01DacTarget), 1);
//  NOA_PUB_I2C_SetReg(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, g_stPMICData[port].ucCR01DacTarget);
#endif
  return ucResult;
}

int ncp81239_pmic_reset(int port) {
  uint8_t ucResult = 0;
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
  g_stPMICData[port].b1CR00EnPol = g_stPmicInitialData.b1CR00EnPol;
  g_stPMICData[port].b1CR00EnPup = g_stPmicInitialData.b1CR00EnPup;
  g_stPMICData[port].b1CR00EnMask = g_stPmicInitialData.b1CR00EnMask;
  g_stPMICData[port].b1CR00EnInternal = g_stPmicInitialData.b1CR00EnInternal;

  g_stPMICData[port].ucCR01DacTarget = g_stPmicInitialData.ucCR01DacTarget;

  g_stPMICData[port].b2CR05OcpClimPos = g_stPmicInitialData.b2CR05OcpClimPos;
  g_stPMICData[port].b2CR05OcpClimNeg = g_stPmicInitialData.b2CR05OcpClimNeg;
  
  g_stPMICData[port].b2CR06Cs1Clind = g_stPmicInitialData.b2CR06Cs1Clind;
  g_stPMICData[port].b2CR06Cs2Clind = g_stPmicInitialData.b2CR06Cs2Clind;

  if (port == 1) {
    int cc1 = 0, cc2 = 0;
    tcpm_get_cc(port, &cc1, &cc2);
//    APP_DEBUG("Port %d CC1 %d CC2 %d", port, cc1, cc2);
  }
  ncp81239_pmic_set_tatus(port);  // can't add this to statsion board
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
  g_stPMICData[port].b1CR00EnPol = g_stPmicInitialData.b1CR00EnPol;
  g_stPMICData[port].b1CR00EnPup = g_stPmicInitialData.b1CR00EnPup;
  g_stPMICData[port].b1CR00EnMask = g_stPmicInitialData.b1CR00EnMask;
  g_stPMICData[port].b1CR00EnInternal = g_stPmicInitialData.b1CR00EnInternal;

  int cc1 = 0, cc2 = 0;
  tcpm_get_cc(port, &cc1, &cc2);
//  APP_DEBUG("Port %d CC1 %d CC2 %d", port, cc1, cc2);

  if (port == 2) {
    APP_DEBUG("C%d CC1 %d CC2 %d", port, cc1, cc2);
    if ((cc1 == 2 || cc2 == 2) && g_stPMICData[port].ucCR01DacTarget == 0x96 && gpio_get_level((gpio_num_t)panda_s4_pin) == 1) {
      g_stPMICData[port].b1CR00EnPol = 0x00;
      g_stPMICData[port].b1CR00EnPup = 0x01;  // en_pup=1, en_pol=0: A high on the enable pin generated a turn on the pull up current ensure the parts starts immediately
      g_stPMICData[port].b1CR00EnMask = 0x01; // en_mask=1, en_int=1: turn on part
      g_stPMICData[port].b1CR00EnInternal = 0x01;
      APP_DEBUG("Warning! Keep C%d VBus to %dV", port, 0x96/10);
      g_stPMICData[port].ucCR01DacTarget = 0x96;  // set port2 default voltage to 15V, not save for device
    } else {
      g_stPMICData[port].ucCR01DacTarget = g_stPmicInitialData.ucCR01DacTarget;
    }
  } else {
    g_stPMICData[port].ucCR01DacTarget = g_stPmicInitialData.ucCR01DacTarget;
  }

  
  g_stPMICData[port].b2CR05OcpClimPos = g_stPmicInitialData.b2CR05OcpClimPos;
  g_stPMICData[port].b2CR05OcpClimNeg = g_stPmicInitialData.b2CR05OcpClimNeg;

  g_stPMICData[port].b2CR06Cs1Clind = g_stPmicInitialData.b2CR06Cs1Clind;
  g_stPMICData[port].b2CR06Cs2Clind = g_stPmicInitialData.b2CR06Cs2Clind;

  if (port != 2) {
    ncp81239_pmic_set_tatus(port);
  }
#endif
  return ucResult;
}

int ncp81239_pmic_clean(int port) {
  uint8_t ucResult = 0;
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
#endif
  g_stPMICData[port].b1CR00EnPol = g_stPmicInitialData.b1CR00EnPol;
  g_stPMICData[port].b1CR00EnPup = g_stPmicInitialData.b1CR00EnPup;
  g_stPMICData[port].b1CR00EnMask = g_stPmicInitialData.b1CR00EnMask;
  g_stPMICData[port].b1CR00EnInternal = g_stPmicInitialData.b1CR00EnInternal;

  g_stPMICData[port].ucCR01DacTarget = 0x0;

  g_stPMICData[port].b2CR05OcpClimPos = g_stPmicInitialData.b2CR05OcpClimPos;
  g_stPMICData[port].b2CR05OcpClimNeg = g_stPmicInitialData.b2CR05OcpClimNeg;
  
  g_stPMICData[port].b2CR06Cs1Clind = g_stPmicInitialData.b2CR06Cs1Clind;
  g_stPMICData[port].b2CR06Cs2Clind = g_stPmicInitialData.b2CR06Cs2Clind;

  if (port == 2) {
    int cc1 = 0, cc2 = 0;
    tcpm_get_cc(port, &cc1, &cc2);
    APP_DEBUG("Port %d CC1 %d CC2 %d", port, cc1, cc2);
  }
  ncp81239_pmic_set_tatus(port);
  return ucResult;
}

