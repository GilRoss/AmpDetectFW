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
    :_arSitePtrs(kNumSites)
    ,_sysStatus(kNumSites)
{
    for (int i = 0; i < (int)_arSitePtrs.size(); i++)
        _arSitePtrs[i] = new Site(i);

    _sysStatusSemId = xSemaphoreCreateMutex();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PcrTask::~PcrTask()
{
    for (int i = 0; i < (int)_arSitePtrs.size(); i++)
        delete _arSitePtrs[i];
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

        //Make sure while updating the sites, we do not try to read the site status.
        xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);
        
        //For each site in the system.
        for (int i = 0; i < (int)_arSitePtrs.size(); i++)
        {
            //If this site is active.
            if (_arSitePtrs[i]->GetRunningFlg() == true)
                _arSitePtrs[i]->Execute();
            else
                _arSitePtrs[i]->ManualControl();
        }
        xSemaphoreGive(_sysStatusSemId);
    }
}
    
///////////////////////////////////////////////////////////////////////////////
void    PcrTask::GetSysStatus(SysStatus* pSysStatus)
{
    //Make sure while we are reading the site status, we do not update the site.
    xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);
    
    //For each site in the system.
    for (int i = 0; i < (int)_arSitePtrs.size(); i++)
        pSysStatus->SetSiteStatus(i, _arSitePtrs[i]->GetStatus());
    xSemaphoreGive(_sysStatusSemId);
}
