
#include "FreeRTOS.h"
#include "os_task.h"
#include "CurrentPidTask.h"
#include "gio.h"
#include "mibspi.h"
//#include "comp_spi.h"
//#include "spi.h"
//#include "mibspi.h"


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
    mibspiSetFunctional(mibspiPORT1, 0x00);
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
        
        float nPidError = _nCurrentSetpoint - GetISense();

        float nControlVar =  nPidError * _nProportionalGain
                      +  _nAccError * _nIntegralGain
                      + (nPidError - _nPrevPidError) * _nDerivativeGain;
        nControlVar *= 2.5 / 15;

        if (_bEnabled)
            DriveControl(nControlVar);

        _nPrevPidError  = nPidError;
        _nAccError  += nPidError;
        gioSetBit(gioPORTB, 0, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float    CurrentPidTask::GetISense()
{
    float value = 0;
//    GetAdc(ISENSE, value);
    float iSense = (value * 9.05 / 6.04 - 2.5) / (20.0 * 0.008);
    return iSense;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    CurrentPidTask::DriveControl(float nControlVar)
{
    float ref = nControlVar + 2.5;
    ref = ref > 5 ? 5 : ref < 0 ? 0 : ref; // limit to [0,5]
//    SetDac(ref);
}
