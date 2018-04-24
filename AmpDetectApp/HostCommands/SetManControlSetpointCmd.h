#ifndef __SetManControlSetpointCmd_H
#define __SetManControlSetpointCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetManControlSetpointCmd : public HostCommand
{
public:
    SetManControlSetpointCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParams;
        
        //If site index is valid.
        if (_request.GetSiteIdx() < _pPcrTask->GetNumSites())
        {
            //Try to set the setpoint.
            Site* pSite = _pPcrTask->GetSitePtr(_request.GetSiteIdx());
            nErrCode = pSite->SetManControlSetpoint(_request.GetSetpoint());
        }
        
        //Send response.
        HostMsg response;
        response = (HostMsg)_request;
        response.SetError(nErrCode);
        response.SetMsgSize(response.GetStreamSize());
        response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, response.GetStreamSize());
    }

protected:
  
private:
    SetManControlSetpointReq     _request;
};

#endif // __SetManControlSetpointCmd_H
