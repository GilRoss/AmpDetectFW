#ifndef __StopRunCmd_H
#define __StopRunCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class StopRunCmd : public HostCommand
{
public:
    StopRunCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {
        Site* pSite = _pcrTask.GetSitePtr();
        ErrCode nErrCode = pSite->StopRun();
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response.SetMsgSize(_response.GetStreamSize());
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    HostMsg     _request;
    HostMsg     _response;
};

#endif // __StopRunCmd_H
