#ifndef __PcrTask_H
#define __PcrTask_H

#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

#include <cstdint>
#include <vector>
#include "Site.h"
#include "SysStatus.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class PcrTask
{
public:
    enum {kNumSites = 1};

    ~PcrTask();
  
    static PcrTask* GetInstance();
    static PcrTask* GetInstancePtr()    {return _pPcrTask;}   //No instantiation.
    void            ExecuteThread();
  
    Site*           GetSitePtr()            {return &_site;}
    void            GetSysStatus(SysStatus* pSysStatus);

protected:
  
private:
    PcrTask();              //Singleton.

    static PcrTask*         _pPcrTask;
    Site                    _site;
    SysStatus               _sysStatus;
    SemaphoreHandle_t       _sysStatusSemId;
};

#endif // __PcrTask_H
