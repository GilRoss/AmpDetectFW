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
    SetOpticsLedCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParamsErr;
        
        //If site and LED channel index are valid.
        if (_request.GetChanIdx() < OpticsDriver::kNumOptChans)
        {
            //Try to set the setpoint.
            Site* pSite = _pcrTask.GetSitePtr();
            nErrCode = pSite->SetOpticsLed(_request.GetChanIdx(),
                                           _request.GetIntensity(),
                                           _request.GetDuration());
        }
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    SetOpticsLedReq     _request;
    HostMsg             _response;
};

#endif // __SetOpticsLedCmd_H
