#ifndef __SetOpticsLedCmd_H
#define __SetOpticsLedCmd_H

#include <cstdint>
#include "HostCommand.h"
#include "OpticsDriver.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetOpticsLedCmd : public HostCommand
{
public:
    SetOpticsLedCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParams;
        
        //If site and LED channel index are valid.
        if ((_request.GetSiteIdx() < _pPcrTask->GetNumSites()) && (_request.GetChanIdx() < OpticsDriver::kNumOptChans))
        {
            //Try to set the setpoint.
            Site* pSite = _pPcrTask->GetSitePtr(_request.GetSiteIdx());
            nErrCode = pSite->SetOpticsLed(_request.GetChanIdx(),
                                           _request.GetIntensity(),
                                           _request.GetDuration());
        }
        
        //Send response.
        _response = (HostMsg)_request;
        _response.SetError(nErrCode);
        _response.SetMsgSize(_response.GetStreamSize());
        _response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, _response.GetStreamSize());
    }

protected:
  
private:
    SetOpticsLedReq     _request;
    HostMsg             _response;
};

#endif // __SetOpticsLedCmd_H
