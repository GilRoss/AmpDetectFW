#ifndef __HostCommand_H
#define __HostCommand_H

#include <cstdint>
#include "PcrTask.h"
#include "HostCommDriver.h"
#include "HostMessages.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class HostCommand
{
public:
    HostCommand(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :_hostCommDrv(hostCommDrv)
        ,_pcrTask(pcrTask)
        ,_pMsgBuf(pMsgBuf)
    {
    }
    
    virtual ~HostCommand()
    {
    }
    
    virtual void    Execute() = 0;

protected:
    HostCommDriver& _hostCommDrv;
    PcrTask&        _pcrTask;
    uint8_t*        _pMsgBuf;

private:
};

#endif // __HostCommand_H
