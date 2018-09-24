#ifndef __SetManControlCurrentCmd_H
#define __SetManControlCurrentCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetManControlCurrentCmd : public HostCommand
{
public:
    SetManControlCurrentCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        //Try to set the setpoint.
        Site* pSite = _pcrTask.GetSitePtr();
        ErrCode nErrCode = pSite->SetManControlCurrent(_request.GetSetpoint() * 1000);
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    SetManControlCurrentReq     _request;
    HostMsg                     _response;
};

#endif // __SetManControlCurrentCmd_H
