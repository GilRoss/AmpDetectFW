
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
    ,_nProportionalGain(0)
    ,_nIntegralGain(0)
    ,_nDerivativeGain(0)
{
    //Set direction (output) and initial state of D/A control pins.
    gioSetBit(gioPORTA, kClearBit, 0);
    gioSetBit(gioPORTA, kSyncBit, 1);
    gioSetBit(gioPORTA, kLdacBit, 0);
    uint32_t nDirMsk = (0x01 << kClearBit) + (0x01 << kSyncBit) + (0x01 << kLdacBit);
    gioSetDirection(gioPORTA, nDirMsk);
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
        
        if (true) //(_bEnabled)
        {
            int32_t nPidError_mA = _nISetpoint_mA - GetProcessVar();

            int32_t nControlVar_mV =    (_nProportionalGain * nPidError_mA) +
                                        (_nIntegralGain * _nAccError_mA) +
                                        (_nDerivativeGain * (nPidError_mA - _nPrevPidError_mA));
            nControlVar_mV *= 2.5 / 15;

            SetControlVar(nControlVar_mV);

            _nPrevPidError_mA = nPidError_mA;
            _nAccError_mA += nPidError_mA;
        }
    }
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
void    CurrentPidTask::SetControlVar(int32_t nISetpoint_mA)
{
    uint16_t    arTxBuf[3] = {0x00};

    //Translate control variable to 0-5V.
//    int32_t nCurrentVal = nControlVar_mV + 2.5;
//    if (nCurrentVal > 5)
//        nCurrentVal = 5;
//    else if (nCurrentVal < 0)
//        nCurrentVal = 0;

    //Use MibSpi1 to write the current control voltage.
//    arTxBuf[0] = (uint16_t)(nCurrentVal * (0xFFFF / 5));
//    arTxBuf[1] = (uint16_t)(nCurrentVal * (0xFFFF / 5));
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    CurrentPidTask::SetDac(int16_t nCmd, int16_t nAdr, )
{
    uint16_t    arTxBuf[3] = {0x00};

    //Translate control variable to 0-5V.
//    int32_t nCurrentVal = nControlVar_mV + 2.5;
//    if (nCurrentVal > 5)
//        nCurrentVal = 5;
//    else if (nCurrentVal < 0)
//        nCurrentVal = 0;

    //Use MibSpi1 to write the current control voltage.
//    arTxBuf[0] = (uint16_t)(nCurrentVal * (0xFFFF / 5));
//    arTxBuf[1] = (uint16_t)(nCurrentVal * (0xFFFF / 5));
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
}
