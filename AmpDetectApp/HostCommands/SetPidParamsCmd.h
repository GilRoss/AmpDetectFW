#ifndef __SetPidParamsCmd_H
#define __SetPidParamsCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SetPidParamsCmd : public HostCommand
{
public:
    SetPidParamsCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kNoError;
        
        //If site index is valid.
        for (int i = 0; i < (int)_pPcrTask->GetNumSites(); i++)
        {
            //Try to set the PID parameters.
            Site* pSite = _pPcrTask->GetSitePtr(i);
            if (nErrCode == ErrCode::kNoError)
                nErrCode = pSite->SetPidParams(_request.GetKp(), _request.GetKi(), _request.GetKd());
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
    SetPidParamsReq     _request;
};

#endif // __SetPidParamsCmd_H
