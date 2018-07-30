#ifndef __SetManControlSetpointCmd_H
#define __SetManControlSetpointCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetManControlSetpointCmd : public HostCommand
{
public:
    SetManControlSetpointCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParamsErr;
        
        //If site index is valid.
        if (_request.GetSiteIdx() < _pcrTask.GetNumSites())
        {
            //Try to set the setpoint.
            Site* pSite = _pcrTask.GetSitePtr(_request.GetSiteIdx());
            nErrCode = pSite->SetManControlSetpoint(_request.GetSetpoint());
        }
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    SetManControlSetpointReq    _request;
    HostMsg                     _response;
};

#endif // __SetManControlSetpointCmd_H
