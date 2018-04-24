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
  
    uint32_t        GetNumSites() const   {return _arSitePtrs.size();}
    Site*           GetSitePtr(uint32_t nSiteIdx)   {return _arSitePtrs[nSiteIdx];}
    void            GetSysStatus(SysStatus* pSysStatus);

protected:
  
private:
    PcrTask();              //Singleton.

    static PcrTask*         _pPcrTask;
    std::vector<Site*>      _arSitePtrs;
    SysStatus               _sysStatus;
    SemaphoreHandle_t       _sysStatusSemId;
};

#endif // __PcrTask_H
