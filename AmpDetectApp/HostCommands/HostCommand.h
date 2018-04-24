#ifndef __HostCommand_H
#define __HostCommand_H

#include <cstdint>
#include "HostMessages.h"
#include "PcrTask.h"
#include "HostCommDriver.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class HostCommand
{
public:
    HostCommand(HostMsg* pReqMsg, const uint8_t* pMsgBuf, HostCommDriver* pHostCommDrv, PcrTask* pPcrTask)
        :_pHostCommDrv(pHostCommDrv)
        ,_pPcrTask(pPcrTask)
    {
    }
    
    virtual ~HostCommand()
    {
    }
    
    virtual void    Execute() = 0;
        
protected:
    HostCommDriver* _pHostCommDrv;
    PcrTask*        _pPcrTask;
    static uint8_t  _arResponseBuf[HostMsg::kMaxResponseSize];
  
private:
};

#endif // __HostCommand_H
