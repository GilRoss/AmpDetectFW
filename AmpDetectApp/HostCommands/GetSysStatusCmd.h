#ifndef __GetStatusCmd_H
#define __GetStatusCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetSysStatusCmd : public HostCommand
{
public:
    GetSysStatusCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual ~GetSysStatusCmd()
    {
    }

    virtual void Execute()
    {
        _pPcrTask->GetSysStatus(_response.GetSysStatusPtr());
        _response.SetMsgSize(_response.GetStreamSize());
        _response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, _response.GetStreamSize());
    }

protected:
  
private:
    HostMsg         _request;
    GetStatusRes    _response;
};

#endif // __GetStatusCmd_H
