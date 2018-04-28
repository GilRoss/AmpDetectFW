
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
    gioSetDirection(gioPORTB, 0x01);

    //Use MibSPI1 to read/write current A/D and D/A, respectively.
//    mibspiSetFunctional(mibspiPORT1, 0x00);
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
        gioSetBit(gioPORTB, 0, 1);
        
        float nPidError = _nCurrentSetpoint - GetProcessVar();

        float nControlVar =  nPidError * _nProportionalGain
                      +  _nAccError * _nIntegralGain
                      + (nPidError - _nPrevPidError) * _nDerivativeGain;
        nControlVar *= 2.5 / 15;

        if (_bEnabled)
            SetControlVar(nControlVar);

        _nPrevPidError  = nPidError;
        _nAccError  += nPidError;
        gioSetBit(gioPORTB, 0, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float    CurrentPidTask::GetProcessVar()
{
    uint16_t    arTxBuf[2];
    uint16_t    arRxBuf[2];

    //Use MibSpi1 to read the current value.
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));

    mibspiGetData(mibspiREG1, 0, arRxBuf);

    float nCurrentVal = 0;
    float nProcessVar = (nCurrentVal * 9.05 / 6.04 - 2.5) / (20.0 * 0.008);

    return nProcessVar;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    CurrentPidTask::SetControlVar(float nControlVar)
{
    uint16_t    arTxBuf[2];

    //Limit control variable to 0-5.
    float nCurrentVal = nControlVar + 2.5;
    if (nCurrentVal > 5)
        nCurrentVal = 5;
    else if (nCurrentVal < 0)
        nCurrentVal = 0;

    //Use MibSpi1 to write the current value.
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
}
