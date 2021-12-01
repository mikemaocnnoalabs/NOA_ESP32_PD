/*
  NCP81239.cpp - Library for interacting with the NCP81239 chip.
  Copyright 2021 NOA
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/
#include <Wire.h>

#include "tcpm_driver.h"
#include "NCP81239.h"
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

    0x00,                              // ADC Trigger
    0x00,                              // ADC MUX select
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
    DBGLOG(Info, "Port %d CC1 %d CC2 %d", port, cc1, cc2);
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
  int cc1 = 0, cc2 = 0;
  tcpm_get_cc(port, &cc1, &cc2);
  DBGLOG(Info, "Port %d CC1 %d CC2 %d", port, cc1, cc2);

  if (port == 2) {
    if (cc1 == 2 || cc2 == 2) {
      g_stPMICData[port].b1CR00EnPol = 0x00;
      g_stPMICData[port].b1CR00EnPup = 0x01;  // en_pup=1, en_pol=0: A high on the enable pin generated a turn on the pull up current ensure the parts starts immediately
      g_stPMICData[port].b1CR00EnMask = 0x01; // en_mask=1, en_int=1: turn on part
      g_stPMICData[port].b1CR00EnInternal = 0x01;
      DBGLOG(Info, "Warning! Init Port %d VBus to %dV", port, 0x96/10);
      g_stPMICData[port].ucCR01DacTarget = 0x96;  // set port2 default voltage to 15V, not save for device
    }
  }
/*  DBGLOG(Info, "INIT:En pol %d", g_stPMICData[port].b1CR00EnPol);
  DBGLOG(Info, "INITc:En pup %d", g_stPMICData[port].b1CR00EnPup);
  DBGLOG(Info, "INIT:En mask %d", g_stPMICData[port].b1CR00EnMask);
  DBGLOG(Info, "INIT:En int %d", g_stPMICData[port].b1CR00EnInternal);
  DBGLOG(Info, "INIT:DAC targer %d", g_stPMICData[port].ucCR01DacTarget);
  DBGLOG(Info, "INIT:Slew rate %d", g_stPMICData[port].b2CR02SlewRate);
  DBGLOG(Info, "INIT:PWM Freq %d", g_stPMICData[port].b3CR03PwmFreq);
  DBGLOG(Info, "INIT:DAC lsb %d", g_stPMICData[port].b1CR03DacLsb);
  DBGLOG(Info, "INIT:Pfet %d", g_stPMICData[port].b1CR04Pfet);
  DBGLOG(Info, "INIT:Cfet %d", g_stPMICData[port].b1CR04Cfet);
  DBGLOG(Info, "INIT:Dead battery en %d", g_stPMICData[port].b1CR04DeadBatteryEn);
  DBGLOG(Info, "INIT:Cs1 dchrg %d", g_stPMICData[port].b1CR04Cs1DisCharge);
  DBGLOG(Info, "INIT:Cs2 dchrg %d", g_stPMICData[port].b1CR04Cs2DisCharge);
  
  DBGLOG(Info, "INIT:Ocp clim pos %d", g_stPMICData[port].b2CR05OcpClimPos);
  DBGLOG(Info, "INIT:Ocp clim neg %d", g_stPMICData[port].b2CR05OcpClimNeg);
  DBGLOG(Info, "INIT:Cs1 clim pos %d", g_stPMICData[port].b2CR06Cs1Clind);
  DBGLOG(Info, "INIT:Cs2 clim pos %d", g_stPMICData[port].b2CR06Cs2Clind);
  DBGLOG(Info, "INIT:Lo gm amp set %d", g_stPMICData[port].b3CR07LoGmAmpSetting);
  DBGLOG(Info, "INIT:Gm manual %d", g_stPMICData[port].b1CR07GmManual);
  DBGLOG(Info, "INIT:Hi gm amp set %d", g_stPMICData[port].b3CR07HiGmAmpSetting);
  DBGLOG(Info, "INIT:Gm amp config %d", g_stPMICData[port].b1CR07GmAmpConfig);
  DBGLOG(Info, "INIT:Amux trigger %d", g_stPMICData[port].b2CR08AmuxTrigger);
  DBGLOG(Info, "INIT:Amux sel %d", g_stPMICData[port].b3CR08AmuxSel);
  DBGLOG(Info, "INIT:Dis adc %d", g_stPMICData[port].b1CR08DisAdc);
  DBGLOG(Info, "INIT:Int mask clind %d", g_stPMICData[port].b1CR09IntMaskCsClind);
  DBGLOG(Info, "INIT:Int mask ov %d", g_stPMICData[port].b1CR09IntMaskOv);

  DBGLOG(Info, "INIT:Int mask ov %d", g_stPMICData[port].b1CR09IntMaskOv);
  DBGLOG(Info, "INIT:Int mask ocp p %d", g_stPMICData[port].b1CR09IntMaskOcpP);
  DBGLOG(Info, "INIT:Int mask pg %d", g_stPMICData[port].b1CR09IntMaskPg);
  DBGLOG(Info, "INIT:Int mask tsd %d", g_stPMICData[port].b1CR09IntMaskTsd);
  DBGLOG(Info, "INIT:Int mask uv %d", g_stPMICData[port].b1CR09IntMaskUv);
  DBGLOG(Info, "INIT:Int mask vchn %d", g_stPMICData[port].b1CR09IntMaskVchn);
  DBGLOG(Info, "INIT:Int mask i2c ack %d", g_stPMICData[port].b1CR09IntMaskI2cAck);
  DBGLOG(Info, "INIT:Int mask shutdown %d", g_stPMICData[port].b1CR0AIntMaskShutDown);
  
  DBGLOG(Info, "INIT:VFB Value %d", g_stPMICData[port].ucCR10VfbValue);
  DBGLOG(Info, "INITc:Vin Value %d", g_stPMICData[port].ucCR11VinValue);
  DBGLOG(Info, "INIT:CS2 Value %d", g_stPMICData[port].ucCR12Cs2Value);
  DBGLOG(Info, "INIT:CS1 Value %d", g_stPMICData[port].ucCR13Cs1Value);
  DBGLOG(Info, "INIT:Cs Clind Flag %d", g_stPMICData[port].b1CR14IntCsClindFlag);
  DBGLOG(Info, "INIT:VBUS OVP Flag %d", g_stPMICData[port].b1CR14IntOvpFlag);
  DBGLOG(Info, "INIT:OCP_P Flag %d", g_stPMICData[port].b1CR14IntOcpPFlag);
  DBGLOG(Info, "INIT:Power Good Flag %d", g_stPMICData[port].b1CR14IntPgIntFlag);
  DBGLOG(Info, "INIT:Thermal Sensor Flag %d", g_stPMICData[port].b1CR14IntTsdFlag);
  DBGLOG(Info, "INIT:Vchn Flag %d", g_stPMICData[port].b1CR14IntVchnFlag);
  DBGLOG(Info, "INIT:IIC ACK Flag %d", g_stPMICData[port].b1CR14IntI2cAckFlag);
  DBGLOG(Info, "INIT:Shut Down Flag %d", g_stPMICData[port].b1CR15IntShutDownFlag); */
#endif
  return 0;
}

int ncp81239_pmic_get_tatus(int port) {
  uint8_t ucResult = 0;
#ifdef NOA_PD_SNACKER
  if(port < 1 || port > 2) {  // support 1 - 2 port only(1 - PD Src, 2 - Wireless charger)
    return -1;
  }
  switch (port) {
    case 1: // SRC
      NOA_PUB_I2C_ReceiveBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      break;
    case 2: // Wireless charger
      NOA_PUB_I2C_ReceiveBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      break;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
//  NOA_PUB_I2C_ReceiveBytes(port, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);

/*  DBGLOG(Info, "Pmic:En pol %d", g_stPMICData.b1CR00EnPol);
  DBGLOG(Info, "Pmic:En pup %d", g_stPMICData.b1CR00EnPup);
  DBGLOG(Info, "Pmic:En mask %d", g_stPMICData.b1CR00EnMask);
  DBGLOG(Info, "Pmic:En int %d", g_stPMICData.b1CR00EnInternal);
  DBGLOG(Info, "Pmic:DAC targer %d", g_stPMICData.ucCR01DacTarget);
  DBGLOG(Info, "Pmic:Slew rate %d", g_stPMICData.b2CR02SlewRate);
  DBGLOG(Info, "Pmic:PWM Freq %d", g_stPMICData.b3CR03PwmFreq);
  DBGLOG(Info, "Pmic:DAC lsb %d", g_stPMICData.b1CR03DacLsb);
  DBGLOG(Info, "Pmic:Pfet %d", g_stPMICData.b1CR04Pfet);
  DBGLOG(Info, "Pmic:Cfet %d", g_stPMICData.b1CR04Cfet);
  DBGLOG(Info, "Pmic:Dead battery en %d", g_stPMICData.b1CR04DeadBatteryEn);
  DBGLOG(Info, "Pmic:Cs1 dchrg %d", g_stPMICData.b1CR04Cs1DisCharge);
  DBGLOG(Info, "Pmic:Cs2 dchrg %d", g_stPMICData.b1CR04Cs2DisCharge);
  
  DBGLOG(Info, "Pmic:Ocp clim pos %d", g_stPMICData.b2CR05OcpClimPos);
  DBGLOG(Info, "Pmic:Ocp clim neg %d", g_stPMICData.b2CR05OcpClimNeg);
  DBGLOG(Info, "Pmic:Cs1 clim pos %d", g_stPMICData.b2CR06Cs1Clind);
  DBGLOG(Info, "Pmic:Cs2 clim pos %d", g_stPMICData.b2CR06Cs2Clind);
  DBGLOG(Info, "Pmic:Lo gm amp set %d", g_stPMICData.b3CR07LoGmAmpSetting);
  DBGLOG(Info, "Pmic:Gm manual %d", g_stPMICData.b1CR07GmManual);
  DBGLOG(Info, "Pmic:Hi gm amp set %d", g_stPMICData.b3CR07HiGmAmpSetting);
  DBGLOG(Info, "Pmic:Gm amp config %d", g_stPMICData.b1CR07GmAmpConfig);
  DBGLOG(Info, "Pmic:Amux trigger %d", g_stPMICData.b2CR08AmuxTrigger);
  DBGLOG(Info, "Pmic:Amux sel %d", g_stPMICData.b3CR08AmuxSel);
  DBGLOG(Info, "Pmic:Dis adc %d", g_stPMICData.b1CR08DisAdc);
  DBGLOG(Info, "Pmic:Int mask clind %d", g_stPMICData.b1CR09IntMaskCsClind);
  DBGLOG(Info, "Pmic:Int mask ov %d", g_stPMICData.b1CR09IntMaskOv);

  DBGLOG(Info, "Pmic:Int mask ov %d", g_stPMICData.b1CR09IntMaskOv);
  DBGLOG(Info, "Pmic:Int mask ocp p %d", g_stPMICData.b1CR09IntMaskOcpP);
  DBGLOG(Info, "Pmic:Int mask pg %d", g_stPMICData.b1CR09IntMaskPg);
  DBGLOG(Info, "Pmic:Int mask tsd %d", g_stPMICData.b1CR09IntMaskTsd);
  DBGLOG(Info, "Pmic:Int mask uv %d", g_stPMICData.b1CR09IntMaskUv);
  DBGLOG(Info, "Pmic:Int mask vchn %d", g_stPMICData.b1CR09IntMaskVchn);
  DBGLOG(Info, "Pmic:Int mask i2c ack %d", g_stPMICData.b1CR09IntMaskI2cAck);
  DBGLOG(Info, "Pmic:Int mask shutdown %d", g_stPMICData.b1CR0AIntMaskShutDown); */

  switch (port) {
    case 1: // SRC1 P2
      NOA_PUB_I2C_ReceiveBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      break;
    case 2: // SRC2 UP
      NOA_PUB_I2C_ReceiveBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6); 
      break;
    case 3: // SRC3 P3
      NOA_PUB_I2C_ReceiveBytes(1, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG10, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG10, 6);
      break;
  }
#endif
  DBGLOG(Info, "Pmic:VFB Value %d", g_stPMICData[port].ucCR10VfbValue);
  DBGLOG(Info, "Pmic:Vin Value %d", g_stPMICData[port].ucCR11VinValue);
  DBGLOG(Info, "Pmic:CS2 Value %d", g_stPMICData[port].ucCR12Cs2Value);
  DBGLOG(Info, "Pmic:CS1 Value %d", g_stPMICData[port].ucCR13Cs1Value);
  DBGLOG(Info, "Pmic:Cs Clind Flag %d", g_stPMICData[port].b1CR14IntCsClindFlag);
  DBGLOG(Info, "Pmic:VBUS OVP Flag %d", g_stPMICData[port].b1CR14IntOvpFlag);
  DBGLOG(Info, "Pmic:OCP_P Flag %d", g_stPMICData[port].b1CR14IntOcpPFlag);
  DBGLOG(Info, "Pmic:Power Good Flag %d", g_stPMICData[port].b1CR14IntPgIntFlag);
  DBGLOG(Info, "Pmic:Thermal Sensor Flag %d", g_stPMICData[port].b1CR14IntTsdFlag);
  DBGLOG(Info, "Pmic:Vchn Flag %d", g_stPMICData[port].b1CR14IntVchnFlag);
  DBGLOG(Info, "Pmic:IIC ACK Flag %d", g_stPMICData[port].b1CR14IntI2cAckFlag);
  DBGLOG(Info, "Pmic:Shut Down Flag %d", g_stPMICData[port].b1CR15IntShutDownFlag);

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
      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
    case 2:
      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
  switch (port) {
    case 1:
      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
    case 2:
      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
      break;
    case 3:
      NOA_PUB_I2C_SendBytes(1, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 11);
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
      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1); 
      break;
    case 2:
      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }

  switch (port) {
    case 1:
      NOA_PUB_I2C_SendBytes(1, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1); 
      break;
    case 2:
      if (g_stPMICData[port].ucCR01DacTarget == 0x96) {
        g_stPMICData[port].b1CR00EnPol = 0x00;
        g_stPMICData[port].b1CR00EnPup = 0x01;  // en_pup=1, en_pol=0: A high on the enable pin generated a turn on the pull up current ensure the parts starts immediately
        g_stPMICData[port].b1CR00EnMask = 0x01; // en_mask=1, en_int=1: turn on part
        g_stPMICData[port].b1CR00EnInternal = 0x01;
        DBGLOG(Info, "Warning! Port %d VBus is up to %dV", port, 0x96/10);
        NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG00, (uint8_t *)(&g_stPMICData[port]), 1);
      }
      NOA_PUB_I2C_SendBytes(0, ncp81239_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
      break;
    case 3:
      NOA_PUB_I2C_SendBytes(1, ncp81239A_I2C_SLAVE_ADDR, _NCP81239_CTRL_REG01, (uint8_t *)(&g_stPMICData[port]) + _NCP81239_CTRL_REG01, 1);
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
//    DBGLOG(Info, "Port %d CC1 %d CC2 %d", port, cc1, cc2);
  }
#else
  if(port < 1 || port > 3) {  // support 1 - 3 port only
    return -1;
  }
  g_stPMICData[port].b1CR00EnPol = g_stPmicInitialData.b1CR00EnPol;
  g_stPMICData[port].b1CR00EnPup = g_stPmicInitialData.b1CR00EnPup;
  g_stPMICData[port].b1CR00EnMask = g_stPmicInitialData.b1CR00EnMask;
  g_stPMICData[port].b1CR00EnInternal = g_stPmicInitialData.b1CR00EnInternal;
//  g_stPMICData[port].b4CR00Reserved = g_stPmicInitialData.b4CR00Reserved;

  int cc1 = 0, cc2 = 0;
  tcpm_get_cc(port, &cc1, &cc2);
//  DBGLOG(Info, "Port %d CC1 %d CC2 %d", port, cc1, cc2);

  if (port == 2) {
    DBGLOG(Info, "Port %d CC1 %d CC2 %d", port, cc1, cc2);
    if ((cc1 == 2 || cc2 == 2) && g_stPMICData[port].ucCR01DacTarget == 0x96) {
      g_stPMICData[port].b1CR00EnPol = 0x00;
      g_stPMICData[port].b1CR00EnPup = 0x01;  // en_pup=1, en_pol=0: A high on the enable pin generated a turn on the pull up current ensure the parts starts immediately
      g_stPMICData[port].b1CR00EnMask = 0x01; // en_mask=1, en_int=1: turn on part
      g_stPMICData[port].b1CR00EnInternal = 0x01;
      DBGLOG(Info, "Warning! Keep Port %d VBus to %dV", port, 0x96/10);
      g_stPMICData[port].ucCR01DacTarget = 0x96;  // set port2 default voltage to 15V, not save for device
    } else {
      g_stPMICData[port].ucCR01DacTarget = g_stPmicInitialData.ucCR01DacTarget;
    }
  } else {
    g_stPMICData[port].ucCR01DacTarget = g_stPmicInitialData.ucCR01DacTarget;
  }

//  g_stPMICData[port].b2CR02SlewRate =  g_stPmicInitialData.b2CR02SlewRate;
//  g_stPMICData[port].b6CR02Reserved = g_stPmicInitialData.b6CR02Reserved;
  
//  g_stPMICData[port].b3CR03PwmFreq = g_stPmicInitialData.b3CR03PwmFreq;
//  g_stPMICData[port].b1CR03Reserved1 = g_stPmicInitialData.b1CR03Reserved1;
//  g_stPMICData[port].b1CR03DacLsb = g_stPmicInitialData.b1CR03DacLsb;
//  g_stPMICData[port].b3CR03Reserved2 = g_stPmicInitialData.b3CR03Reserved2;
  
//  g_stPMICData[port].b1CR04Pfet = g_stPmicInitialData.b1CR04Pfet;
//  g_stPMICData[port].b1CR04Cfet = g_stPmicInitialData.b1CR04Cfet;
//  g_stPMICData[port].b1CR04DeadBatteryEn = g_stPmicInitialData.b1CR04DeadBatteryEn;
//  g_stPMICData[port].b1CR04Reserved1 = g_stPmicInitialData.b1CR04Reserved1;
//  g_stPMICData[port].b1CR04Cs1DisCharge = g_stPmicInitialData.b1CR04Cs1DisCharge;
//  g_stPMICData[port].b1CR04Cs2DisCharge = g_stPmicInitialData.b1CR04Cs2DisCharge;
//  g_stPMICData[port].b2CR04Reserved2 = g_stPmicInitialData.b2CR04Reserved2;
  
  g_stPMICData[port].b2CR05OcpClimPos = g_stPmicInitialData.b2CR05OcpClimPos;
//  g_stPMICData[port].b2CR05Reserved1 = g_stPmicInitialData.b2CR05Reserved1;
  g_stPMICData[port].b2CR05OcpClimNeg = g_stPmicInitialData.b2CR05OcpClimNeg;
//  g_stPMICData[port].b2CR05Reserved2 = g_stPmicInitialData.b2CR05Reserved2;
  
  g_stPMICData[port].b2CR06Cs1Clind = g_stPmicInitialData.b2CR06Cs1Clind;
  g_stPMICData[port].b2CR06Cs2Clind = g_stPmicInitialData.b2CR06Cs2Clind;
//  g_stPMICData[port].b4CR06Reserved = g_stPmicInitialData.b4CR06Reserved;
  
//  g_stPMICData[port].b3CR07LoGmAmpSetting = g_stPmicInitialData.b3CR07LoGmAmpSetting;
//  g_stPMICData[port].b1CR07GmManual = g_stPmicInitialData.b1CR07GmManual;
//  g_stPMICData[port].b3CR07HiGmAmpSetting = g_stPmicInitialData.b3CR07HiGmAmpSetting;
//  g_stPMICData[port].b1CR07GmAmpConfig = g_stPmicInitialData.b1CR07GmAmpConfig;
  
//  g_stPMICData[port].b2CR08AmuxTrigger = g_stPmicInitialData.b2CR08AmuxTrigger;
//  g_stPMICData[port].b3CR08AmuxSel = g_stPmicInitialData.b3CR08AmuxSel;
//  g_stPMICData[port].b1CR08DisAdc = g_stPmicInitialData.b1CR08DisAdc;
//  g_stPMICData[port].b2CR08Reserved = g_stPmicInitialData.b2CR08Reserved;
  
//  g_stPMICData[port].b1CR09IntMaskCsClind = g_stPmicInitialData.b1CR09IntMaskCsClind;
//  g_stPMICData[port].b1CR09IntMaskOv = g_stPmicInitialData.b1CR09IntMaskOv;
//  g_stPMICData[port].b1CR09IntMaskOcpP = g_stPmicInitialData.b1CR09IntMaskOcpP;
//  g_stPMICData[port].b1CR09IntMaskPg = g_stPmicInitialData.b1CR09IntMaskPg;
//  g_stPMICData[port].b1CR09IntMaskTsd = g_stPmicInitialData.b1CR09IntMaskTsd;
//  g_stPMICData[port].b1CR09IntMaskUv = g_stPmicInitialData.b1CR09IntMaskUv;
//  g_stPMICData[port].b1CR09IntMaskVchn = g_stPmicInitialData.b1CR09IntMaskVchn;
//  g_stPMICData[port].b1CR09IntMaskI2cAck = g_stPmicInitialData.b1CR09IntMaskI2cAck;
  
//  g_stPMICData[port].b1CR0AIntMaskShutDown = g_stPmicInitialData.b1CR0AIntMaskShutDown;
//  g_stPMICData[port].b7CR0AReserved = g_stPmicInitialData.b7CR0AReserved;
#endif
  return ucResult;
}
