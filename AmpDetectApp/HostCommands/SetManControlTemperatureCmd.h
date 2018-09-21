#ifndef __SetManControlTemperatureCmd_H
#define __SetManControlTemperatureCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetManControlTemperatureCmd : public HostCommand
{
public:
    SetManControlTemperatureCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        //Try to set the setpoint.
        Site* pSite = _pcrTask.GetSitePtr();
        ErrCode nErrCode = pSite->SetManControlTemperature(_request.GetSetpoint());
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    SetManControlTemperatureReq     _request;
    HostMsg                         _response;
};

#endif // __SetManControlTemperatureCmd_H
