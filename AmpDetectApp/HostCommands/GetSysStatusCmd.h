#ifndef __GetStatusCmd_H
#define __GetStatusCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetSysStatusCmd : public HostCommand
{
public:
    GetSysStatusCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual ~GetSysStatusCmd()
    {
    }

    virtual void Execute()
    {
        _pcrTask.GetSysStatus(_response.GetSysStatusPtr());
        _response.SetResponseHeader(_request, ErrCode::kNoError);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    HostMsg         _request;
    GetStatusRes    _response;
};

#endif // __GetStatusCmd_H
