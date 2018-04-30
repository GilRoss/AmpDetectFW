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
        POWER_DOWN_DAC_A_1KOHM_TO_GND             =  9, // Power down DAC-A; 1 kΩ to GND
        POWER_DOWN_DAC_B_1KOHM_TO_GND             = 10, // Power down DAC-B; 1 kΩ to GND
        POWER_DOWN_DAC_A_AND_DAC_B_1KOHM_TO_GND   = 11, // Power down DAC-A and DAC-B; 1 kΩ to GND
        POWER_DOWN_DAC_A_100KOHM_TO_GND           = 17, // Power down DAC-A; 100 kΩ to GND
        POWER_DOWN_DAC_B_100KOHM_TO_GND           = 18, // Power down DAC-B; 100 kΩ to GND
        POWER_DOWN_DAC_A_AND_DAC_B_100KOHM_TO_GND = 19, // Power down DAC-A and DAC-B; 100 kΩ to GND
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

    ~CurrentPidTask();
  
    static CurrentPidTask*  GetInstance();
    static CurrentPidTask*  GetInstancePtr()    {return _pCurrentPidTask;}   //No instantiation.
    void                    ExecuteThread();

    void        SetEnabledFlg(bool b)   {_bEnabled = b;}
    bool        GetEnabledFlg()         {return _bEnabled;}
    void        SetSetpoint(float sp)   {_nISetpoint_mA = sp;}
    int32_t     GetSetPoint()           {return _nISetpoint_mA;}

protected:
  
private:
    CurrentPidTask();       //Singleton.

    int32_t                 GetProcessVar();
    void                    SetControlVar(int32_t nISetpoint_mA);

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
