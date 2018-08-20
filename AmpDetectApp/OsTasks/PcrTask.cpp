#include "FreeRTOS.h"
#include <cstdint>
#include <vector>
#include "PcrTask.h"
#include "HostMessages.h"
#include "Pid.h"
#include "ThermalDriver.h"
#include "OpticsDriver.h"
#include "gio.h"


///////////////////////////////////////////////////////////////////////////////
PcrTask*        PcrTask::_pPcrTask = nullptr;         //Singleton.

///////////////////////////////////////////////////////////////////////////////
PcrTask* PcrTask::GetInstance()
{
    if (_pPcrTask == nullptr)
        _pPcrTask = new PcrTask;

    return _pPcrTask;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern "C" void StartPcrTask(void * pvParameters);
void StartPcrTask(void * pvParameters)
{
    PcrTask* pPcrTask = PcrTask::GetInstance();
    pPcrTask->ExecuteThread();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PcrTask::PcrTask()
{
    _sysStatusSemId = xSemaphoreCreateMutex();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PcrTask::~PcrTask()
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    PcrTask::ExecuteThread()
{
    TickType_t    nPrevWakeTime = xTaskGetTickCount();
    
    while (true)
    {
        //Wait for PID delta time.
        vTaskDelayUntil (&nPrevWakeTime, (TickType_t)Site::kPidTick_ms / portTICK_PERIOD_MS);

        //Make sure while updating the site, we do not try to read the site status.
        xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);
        
        //If the site is active.
        if (_site.GetRunningFlg() == true)
            _site.Execute();
        else
            _site.ManualControl();

        xSemaphoreGive(_sysStatusSemId);
    }
}
    
///////////////////////////////////////////////////////////////////////////////
void    PcrTask::GetSysStatus(SysStatus* pSysStatus)
{
    //Make sure while we are reading the site status, we do not update the site.
    xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);
    pSysStatus->SetSiteStatus(_site.GetStatus());
    xSemaphoreGive(_sysStatusSemId);
}
