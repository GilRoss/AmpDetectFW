#ifndef __SetPidParamsCmd_H
#define __SetPidParamsCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetPidParamsCmd : public HostCommand
{
public:
    SetPidParamsCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kRunInProgressErr;
        
        //Make certain run is not in progress.
        Site* pSite = _pcrTask.GetSitePtr(0);
        if (pSite->GetRunningFlg() == false)
        {
            //Write PID parameters to persistent memory.

            //Write PID parameters to PID object.
            nErrCode = pSite->SetPidParams(_request.GetType(), _request.GetPidParams());
        }
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    SetPidParamsReq _request;
    HostMsg         _response;
};

#endif // __SetPidParamsCmd_H
