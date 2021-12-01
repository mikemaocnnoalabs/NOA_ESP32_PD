/*
  NCP81239.h - Library for interacting with the NCP81239 chip.
  Copyright 2012 NOA Labs
  Copyright 2021 Mike mao
  Released under an MIT license. See LICENSE file. 
*/

#ifndef ncp81239_H
#define ncp81239_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_NCP_PM_PORT_COUNT  4

/* I2C slave address varies by part number */
/* NCP81239 */
#define ncp81239_I2C_SLAVE_ADDR 0x74
/* NCP81239A */
#define ncp81239A_I2C_SLAVE_ADDR 0x75

//****************************************************************************
// DEFINITIONS / MACROS
//****************************************************************************
//--------------------------------------------------
// Definitions of PMIC NCP81239
//--------------------------------------------------
#define _NCP81239_SUBADDR_LENGTH            0x01
#define _NCP81239_CTRL_REG00                0x00
#define _NCP81239_CTRL_REG01                0x01
#define _NCP81239_CTRL_REG02                0x02
#define _NCP81239_CTRL_REG03                0x03
#define _NCP81239_CTRL_REG04                0x04
#define _NCP81239_CTRL_REG05                0x05
#define _NCP81239_CTRL_REG06                0x06
#define _NCP81239_CTRL_REG07                0x07
#define _NCP81239_CTRL_REG08                0x08
#define _NCP81239_CTRL_REG09                0x09
#define _NCP81239_CTRL_REG0A                0x0A
#define _NCP81239_CTRL_REG10                0x10
#define _NCP81239_CTRL_REG11                0x11
#define _NCP81239_CTRL_REG12                0x12
#define _NCP81239_CTRL_REG13                0x13
#define _NCP81239_CTRL_REG14                0x14
#define _NCP81239_CTRL_REG15                0x15

//--------------------------------------------------
// Definitions of Pmic NCP81239 Control
//--------------------------------------------------
#define _TUNE_RISE                          0
#define _TUNE_FALL                          1
#define _PMIC_SOURCE                        1
#define _PMIC_SINK                          0

//--------------------------------------------------
// NCP81239 Initial Setting
//--------------------------------------------------
#define _ADDR_01_VBUS                       0x32    // 5V
#define _ADDR_02_SLEW_RATE                  0x00
#define _ADDR_03_PWM_FREQ                   0x00    // 600 kHZ
// #define _ADDR_05_OCP_CLIM_POS               0x03    //
// #define _ADDR_05_OCP_CLIM_POS               0x00    //
#define _ADDR_05_OCP_CLIM_POS               0x03
#define _ADDR_05_OCP_CLIM_NEG               0x00
#define _ADDR_06_CS1_CLIND                  0x03    // 11
#define _ADDR_06_CS2_CLIND                  0x03    // 11
// #define _ADDR_06_CS1_CLIND                  0x00    // 11
// #define _ADDR_06_CS2_CLIND                  0x00    // 11
#define _ADDR_09_INT_MASK_CS_CLIND          0x00
#define _ADDR_09_INT_MASK_OVP               0x00
#define _ADDR_09_INT_MASK_OCP_P             0x00
#define _ADDR_09_INT_MASK_PG_INT            0x00
#define _ADDR_09_INT_MASK_TSD               0x00
#define _ADDR_09_INT_MASK_UV                0x01
#define _ADDR_09_INT_MASK_VCHN              0x00
#define _ADDR_09_INT_MASK_IIC_ACK           0x00
#define _ADDR_0A_INT_MASK_SHUT_DOWN         0x00

//--------------------------------------------------
// Definitions of PMIC FUNCTION
//--------------------------------------------------
#define _TYPE_C_PMIC_VOLTAGE_OFFSET         1      // unit: 0.1V

//--------------------------------------------------
// Macros of PMIC STATUS
//--------------------------------------------------


//--------------------------------------------------
// Macros of PMIC CHECK START
//--------------------------------------------------

//****************************************************************************
// STRUCT / TYPE / ENUM DEFINITTIONS
//****************************************************************************
typedef struct
{
    uint8_t b1CR00EnPol : 1;
    uint8_t b1CR00EnPup : 1;
    uint8_t b1CR00EnMask : 1;
    uint8_t b1CR00EnInternal : 1;
    uint8_t b4CR00Reserved : 4;

    uint8_t ucCR01DacTarget;

    uint8_t b2CR02SlewRate : 2;
    uint8_t b6CR02Reserved : 6;

    uint8_t b3CR03PwmFreq : 3;
    uint8_t b1CR03Reserved1 : 1;
    uint8_t b1CR03DacLsb : 1;
    uint8_t b3CR03Reserved2 : 3;

    uint8_t b1CR04Pfet : 1;
    uint8_t b1CR04Cfet : 1;
    uint8_t b1CR04DeadBatteryEn : 1;
    uint8_t b1CR04Reserved1 : 1;
    uint8_t b1CR04Cs1DisCharge : 1;
    uint8_t b1CR04Cs2DisCharge : 1;
    uint8_t b2CR04Reserved2 : 2;

    uint8_t b2CR05OcpClimPos : 2;
    uint8_t b2CR05Reserved1 : 2;
    uint8_t b2CR05OcpClimNeg : 2;
    uint8_t b2CR05Reserved2 : 2;

    uint8_t b2CR06Cs1Clind : 2;
    uint8_t b2CR06Cs2Clind : 2;
    uint8_t b4CR06Reserved : 4;

    uint8_t b3CR07LoGmAmpSetting : 3;
    uint8_t b1CR07GmManual : 1;
    uint8_t b3CR07HiGmAmpSetting : 3;
    uint8_t b1CR07GmAmpConfig : 1;

    uint8_t b2CR08AmuxTrigger : 2;
    uint8_t b3CR08AmuxSel : 3;
    uint8_t b1CR08DisAdc : 1;
    uint8_t b2CR08Reserved : 2;

    uint8_t b1CR09IntMaskCsClind : 1;
    uint8_t b1CR09IntMaskOv : 1;
    uint8_t b1CR09IntMaskOcpP : 1;
    uint8_t b1CR09IntMaskPg : 1;
    uint8_t b1CR09IntMaskTsd : 1;
    uint8_t b1CR09IntMaskUv : 1;
    uint8_t b1CR09IntMaskVchn : 1;
    uint8_t b1CR09IntMaskI2cAck : 1;

    uint8_t b1CR0AIntMaskShutDown : 1;
    uint8_t b7CR0AReserved : 7;

    uint8_t ucCR0BReserved;
    uint8_t ucCR0CReserved;
    uint8_t ucCR0DReserved;
    uint8_t ucCR0EReserved;
    uint8_t ucCR0FReserved;

    uint8_t ucCR10VfbValue;
    uint8_t ucCR11VinValue;
    uint8_t ucCR12Cs2Value;
    uint8_t ucCR13Cs1Value;

    uint8_t b1CR14IntCsClindFlag : 1;
    uint8_t b1CR14IntOvpFlag : 1;
    uint8_t b1CR14IntOcpPFlag : 1;
    uint8_t b1CR14IntPgIntFlag : 1;
    uint8_t b1CR14IntTsdFlag : 1;
    uint8_t b1CR14Reserved : 1;
    uint8_t b1CR14IntVchnFlag : 1;
    uint8_t b1CR14IntI2cAckFlag : 1;

    uint8_t b1CR15IntShutDownFlag : 1;
    uint8_t b7CR15Reserved : 7;
} StructNCP81239RegisterMap;

int     ncp81239_pmic_init(int port);
int     ncp81239_pmic_get_tatus(int port);
int     ncp81239_pmic_set_tatus(int port);
int     ncp81239_pmic_set_voltage(int port);
int     ncp81239_pmic_reset(int port);
#ifdef __cplusplus
}
#endif

#endif /* ncp81239_H */
