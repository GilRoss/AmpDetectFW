
#include "FreeRTOS.h"
#include "os_task.h"
#include "CurrentPidTask.h"
#include "gio.h"
#include "mibspi.h"


///////////////////////////////////////////////////////////////////////////////
CurrentPidTask*        CurrentPidTask::_pCurrentPidTask = nullptr;         //Singleton.

///////////////////////////////////////////////////////////////////////////////
CurrentPidTask* CurrentPidTask::GetInstance()
{
    if (_pCurrentPidTask == nullptr)
        _pCurrentPidTask = new CurrentPidTask;

    return _pCurrentPidTask;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern "C" void StartCurrentPidTask(void * pvParameters);
void StartCurrentPidTask(void * pvParameters)
{
    CurrentPidTask* pCurrentPidTask = CurrentPidTask::GetInstance();
    pCurrentPidTask->ExecuteThread();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CurrentPidTask::CurrentPidTask()
    :_bEnabled(false)
    ,_nProportionalGain(2)
    ,_nIntegralGain(0)
    ,_nDerivativeGain(0)
    ,_nSetpoint_mA(5000)
{
    //Set direction (output) and initial state of D/A control pins.
    gioSetBit(gioPORTA, kClearBit, 1);
    gioSetBit(gioPORTA, kSyncBit, 1);
    gioSetBit(gioPORTA, kLdacBit, 0);
    uint32_t nDirMsk = (0x01 << kClearBit) + (0x01 << kSyncBit) + (0x01 << kLdacBit);
    gioSetDirection(gioPORTA, nDirMsk);

    //Initialize DAC8563
    SendDacMsg(CMD_RESET, 0, POWER_ON_RESET);
    SendDacMsg(CMD_ENABLE_INT_REF, 0, DISABLE_INT_REF_AND_RESET_DAC_GAINS_TO_1);
    SendDacMsg(CMD_SET_LDAC_PIN, 0, SET_LDAC_PIN_INACTIVE_DAC_B_INACTIVE_DAC_A);
    SendDacMsg(CMD_POWER_DAC, 0, POWER_DOWN_DAC_B_HI_Z);

    SetSetpoint(0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CurrentPidTask::~CurrentPidTask()
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    CurrentPidTask::ExecuteThread()
{
    TickType_t    nPrevWakeTime = xTaskGetTickCount();
    
    while (true)
    {
        //Wait for PID delta time.
        vTaskDelayUntil (&nPrevWakeTime, 1 / portTICK_PERIOD_MS);
        
        if (_bEnabled)
        {
            //Convert A/D counts to mV.
            int32_t nPidError_A2DCounts = (int32_t)_nSetpoint_A2DCounts - (int32_t)GetDac(0);

            int32_t nSignedControlVar_A2DCounts =
                    (_nProportionalGain * nPidError_A2DCounts) +
                    (_nIntegralGain * _nAccError_A2DCounts) +
                    (_nDerivativeGain * (nPidError_A2DCounts - _nPrevPidError_A2DCounts));

            uint32_t nControlVar_A2DCounts = (uint32_t)(nSignedControlVar_A2DCounts + (0xFFFF / 2));
            SetControlVar(nControlVar_A2DCounts);

            _nPrevPidError_A2DCounts = nPidError_A2DCounts;
            _nAccError_A2DCounts += nPidError_A2DCounts;
        }
        else //not enabled.
        {
            //Must stay low for 80 ns.
            gioSetBit(gioPORTA, kClearBit, 1);
            gioSetBit(gioPORTA, kClearBit, 0);
            gioSetBit(gioPORTA, kClearBit, 0);
            gioSetBit(gioPORTA, kClearBit, 0);
            gioSetBit(gioPORTA, kClearBit, 0);
            gioSetBit(gioPORTA, kClearBit, 1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CurrentPidTask::SetEnabledFlg(bool b)
{
    _bEnabled = b;
    _nPrevPidError_A2DCounts = 0;
    _nAccError_A2DCounts = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CurrentPidTask::SetSetpoint(uint32_t nSetpoint_mA)
{
    _nSetpoint_mA = nSetpoint_mA;

    //Convert mA to mV.
    //For this A/D: 0 - 15000mA = 70 - 3270mV.
    //so: mV = (.213 * mA) + 70
    uint32_t nSetpoint_mV = ((nSetpoint_mA * 1000) / 4697) + 70;

    //Convert mV A/D counts.
    //For this A/D: 0 - 5000mV = 0 0xFFFF counts.
    _nSetpoint_A2DCounts = (nSetpoint_mV * 0xFFFF) / 5000;

    SetEnabledFlg(true);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t    CurrentPidTask::GetProcessVar()
{
    uint16_t    arTxBuf[3] = {0x0000, 0x0000, 0x0000};
    uint16_t    arRxBuf[3] = {0x0000};

    //Use MibSpi1 to read the current value.
    gioSetBit(gioPORTA, kSyncBit, 0);
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
    gioSetBit(gioPORTA, kSyncBit, 1);

    //Read the A/D.
    mibspiGetData(mibspiREG1, 0, arRxBuf);

//    int32_t nCurrentVal = 0;
//    int32_t nProcessVar_mA = (nCurrentVal * 9.05 / 6.04 - 2.5) / (20.0 * 0.008);

    return 0; //return nProcessVar_mA;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    CurrentPidTask::SetControlVar(uint32_t nControlVar_D2ACounts)
{
    SendDacMsg(CMD_WR_ONE_REG_AND_UPDATE_ONE_DAC, ADDR_DAC_A,
               (uint8_t)(nControlVar_D2ACounts >> 8), (uint8_t)nControlVar_D2ACounts);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    CurrentPidTask::SendDacMsg(uint8_t nCmd, uint8_t nAdr, uint8_t nHiByte, uint8_t nLoByte)
{
    uint16_t    arTxBuf[3] = {0x0000};

    //Using MibSpi1, prepare and write the message.
    arTxBuf[0] = (uint16_t)((nCmd << 3) + nAdr);
    arTxBuf[1] = (uint16_t)nHiByte;
    arTxBuf[2] = (uint16_t)nLoByte;

    gioSetBit(gioPORTA, kSyncBit, 0);   //CS
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
    gioSetBit(gioPORTA, kSyncBit, 1);   //CS
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CurrentPidTask::ReadDacMsg(uint16 cfg, uint16_t* pData)
{
    uint16_t    arTxBuf[2];
    uint16_t    arRxBuf[2];

    arTxBuf[0] = cfg >> 8;
    arTxBuf[1] = cfg & 0x00FF;

    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Conversion pin
    gioSetBit(mibspiPORT5, PIN_SOMI, 0);          //Conversion pin
    mibspiSetData(mibspiREG1, 1, arTxBuf);
    mibspiTransfer(mibspiREG1, 1);
    while (! mibspiIsTransferComplete(mibspiREG1, 1));
    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Conversion pin
    mibspiGetData(mibspiREG1, 1, arRxBuf);
    gioSetBit(mibspiPORT5, PIN_SOMI, 0);          //Wait 4 us.
    gioSetBit(mibspiPORT5, PIN_SOMI, 0);          //Wait 4 us.
    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Conversion pin

    *pData = (arRxBuf[0] << 8) + arRxBuf[1];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t CurrentPidTask::GetDac(int channel)
{
    uint16 data;
    uint16 cfg = 0;
    cfg |= OVERWRITE_CFG << CFG_SHIFT;
    cfg |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    cfg |= channel << IN_CH_SEL_SHIFT;
    cfg |= FULL_BW << FULL_BW_SEL_SHIFT;
    cfg |= EXT_REF << REF_SEL_SHIFT;
    cfg |= DISABLE_SEQ << SEQ_EN_SHIFT;
    cfg |= READ_BACK_DISABLE << READ_BACK_SHIFT;
    cfg <<= 2;

    //Make certain the Somi of Spi1 is connected to the TEC ADC.
    gioSetBit(mibspiPORT3, PIN_ENA, Spi1SomiSrc::kTecAdc);

    ReadDacMsg(cfg, &data); // conv DATA(n-1), clock out CFG(n), clock in DATA(n-2)
    ReadDacMsg(cfg, &data); // conv DATA(n), clock out CFG(n+1), clock in DATA(n-1)
    ReadDacMsg(cfg, &data); // conv DATA(n+1), clock out CFG(n+2), clock in DATA(n)

    return data;
}

