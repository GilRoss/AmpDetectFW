#ifndef __CurrentPidTask_H
#define __CurrentPidTask_H

#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class CurrentPidTask
{
public:
    //Control bits for D/A.
    enum    {kClearBit = 5, kSyncBit = 6, kLdacBit = 7};

    enum Spi1SomiSrc {
        kTecAdc,
        kMotor,
    };
    enum Cmd {
        CMD_WR_REG_OR_GAINS                = 0, // Write to input register n or to gains
        CMD_UPDATE_ONE_DAC                 = 1, // Software LDAC, update DAC register n
        CMD_WR_ONE_REG_AND_UPDATE_ALL_DACS = 2, // Write to input register n and update all DAC registers
        CMD_WR_ONE_REG_AND_UPDATE_ONE_DAC  = 3, // Write to input register n and update DAC register n
        CMD_POWER_DAC                      = 4, // Set DAC power up or -down mode
        CMD_RESET                          = 5, // Software reset
        CMD_SET_LDAC_PIN                   = 6, // Set LDAC registers
        CMD_ENABLE_INT_REF                 = 7, // Enable or disable the internal reference
    };
    enum CmdShift {
        CMD_SHIFT = 3,
    };
    enum Addr {
        ADDR_DAC_A       = 0, // DAC-A input register
        ADDR_DAC_B       = 1, // DAC-B input register
        ADDR_DAC_GAIN    = 2,
        ADDR_DAC_A_AND_B = 7, // DAC-A and DAC-B input registers
    };
    enum AddrShift {
        ADDR_SHIFT = 0,
    };
    enum Gain {
        GAIN_DAC_B_2_DAC_A_2 = 0, // Gain: DAC-B gain = 2, DAC-A gain = 2 (default with internal VREF)
        GAIN_DAC_B_2_DAC_A_1 = 1, // Gain: DAC-B gain = 2, DAC-A gain = 1
        GAIN_DAC_B_1_DAC_A_2 = 2, // Gain: DAC-B gain = 1, DAC-A gain = 2
        GAIN_DAC_B_1_DAC_A_1 = 3, // Gain: DAC-B gain = 1, DAC-A gain = 1 (power-on default)
    };
    enum Power {
        POWER_UP_DAC_A                            =  1, // Power up DAC-A
        POWER_UP_DAC_B                            =  2, // Power up DAC-B
        POWER_UP_DAC_A_AND_DAC_B                  =  3, // Power up DAC-A and DAC-B
        POWER_DOWN_DAC_A_1KOHM_TO_GND             =  9, // Power down DAC-A; 1 kâ„¦ to GND
        POWER_DOWN_DAC_B_1KOHM_TO_GND             = 10, // Power down DAC-B; 1 kâ„¦ to GND
        POWER_DOWN_DAC_A_AND_DAC_B_1KOHM_TO_GND   = 11, // Power down DAC-A and DAC-B; 1 kâ„¦ to GND
        POWER_DOWN_DAC_A_100KOHM_TO_GND           = 17, // Power down DAC-A; 100 kâ„¦ to GND
        POWER_DOWN_DAC_B_100KOHM_TO_GND           = 18, // Power down DAC-B; 100 kâ„¦ to GND
        POWER_DOWN_DAC_A_AND_DAC_B_100KOHM_TO_GND = 19, // Power down DAC-A and DAC-B; 100 kâ„¦ to GND
        POWER_DOWN_DAC_A_HI_Z                     = 25, // Power down DAC-A; Hi-Z
        POWER_DOWN_DAC_B_HI_Z                     = 26, // Power down DAC-B; Hi-Z
        POWER_DOWN_DAC_A_AND_DAC_B_HI_Z           = 27, // Power down DAC-A and DAC-B; Hi-Z
    };
    enum Reset {
        RESET_AND_UPDATE = 0, // Reset DAC-A and DAC-B input register and update all DACs
        POWER_ON_RESET   = 1, // Reset all registers and update all DACs (Power-on-reset update)
    };
    enum SetLdacPin {
        SET_LDAC_PIN_ACTIVE_DAC_B_AND_DAC_A        = 0, // LDAC pin active for DAC-B and DAC-A
        SET_LDAC_PIN_ACTIVE_DAC_B_INACTIVE_DAC_A   = 1, // LDAC pin active for DAC-B; inactive for DAC-A
        SET_LDAC_PIN_INACTIVE_DAC_B_ACTIVE_DAC_A   = 2, // LDAC pin inactive for DAC-B; active for DAC-A
        SET_LDAC_PIN_INACTIVE_DAC_B_INACTIVE_DAC_A = 3, // LDAC pin inactive for DAC-B and DAC-A
    };
    enum EnableIntRef {
        DISABLE_INT_REF_AND_RESET_DAC_GAINS_TO_1 = 0, // Disable internal reference and reset DACs to gain = 1
        ENABLE_INT_REF_AND_RESET_DAC_GAINS_TO_2  = 1, // Enable internal reference and reset DACs to gain = 2
    };
    enum CtrlRegisterShifts {
        CFG_SHIFT         = 13,
        IN_CH_CFG_SHIFT   = 10,
        IN_CH_SEL_SHIFT   =  7,
        FULL_BW_SEL_SHIFT =  6,
        REF_SEL_SHIFT     =  3,
        SEQ_EN_SHIFT      =  1,
        READ_BACK_SHIFT   =  0,
    };
    enum CfgBit {
        KEEP_CFG,
        OVERWRITE_CFG,
    };
    enum InChCfgBits {
        BIPOLAR_DIFF_PAIRS  = 0, // INx referenced to VREF/2 ± 0.1 V.
        BIPOLAR             = 2, // INx referenced to COM = VREF/2 ± 0.1 V.
        TEMP_SENSOR         = 3, // Temperature sensor
        UNIPOLAR_DIFF_PAIRS = 4, // INx referenced to GND ± 0.1 V.
        UNIPOLAR_REF_TO_COM = 6, // INx referenced to COM = GND ± 0.1 V.
        UNIPOLAR_REF_TO_GND = 7, // INx referenced to GND
    };
    enum BwSelectBit {
        QUARTER_BW,
        FULL_BW
    };
    enum RefSelectionBits {
        INT_REF2_5_AND_TEMP_SENS,   // REF = 2.5 V buffered output.
        INT_REF4_096_AND_TEMP_SENS, // REF = 4.096 V buffered output.
        EXT_REF_AND_TEMP_SENS,      // Internal buffer disabled
        EXT_REF_AND_TEMP_SENS_BUFF, // Internal buffer and temperature sensor enabled.
        EXT_REF = 6,                // Int ref, int buffer, and temp sensor disabled.
        EXT_REF_BUFF,               // Int buffer enabled. Int ref and temp sensor disabled.
    };
    enum ChSeqBits {
        DISABLE_SEQ,
        UPDATE_CFG,
        SCAN_IN_CH_AND_TEMP,
        SCAN_IN_CH,
    };
    enum ReadBackBit {
        READ_BACK_EN,
        READ_BACK_DISABLE,
    };

    ~CurrentPidTask();
  
    static CurrentPidTask*  GetInstance();
    static CurrentPidTask*  GetInstancePtr()    {return _pCurrentPidTask;}   //No instantiation.
    void                    ExecuteThread();

    void        SetEnabledFlg(bool b)   {_bEnabled = b;}
    bool        GetEnabledFlg()         {return _bEnabled;}
    void        SetSetpoint(float sp)   {_nISetpoint_mA = sp;}
    int32_t     GetSetPoint()           {return _nISetpoint_mA;}
    void        SendDacMsg(uint8_t nCmd, uint8_t nAdr, uint8_t nHiByte = 0, uint8_t nLoByte = 0);
    void        ReadDacMsg(uint16_t cfg, uint16_t* pData);
    uint32_t    GetDac(int channel);

protected:
  
private:
    CurrentPidTask();       //Singleton.

    int32_t                 GetProcessVar();
    void                    SetControlVar(uint32_t nISetpoint_mA);

    static CurrentPidTask*  _pCurrentPidTask;

    bool                    _bEnabled;
    int32_t                 _nISetpoint_mA;
    int32_t                 _nProportionalGain;
    int32_t                 _nIntegralGain;
    int32_t                 _nDerivativeGain;

    int32_t                 _nPrevPidError_mA;
    int32_t                 _nAccError_mA;
};

#endif // __CurrentPidTask_H
