#ifndef __StartRunCmd_H
#define __StartRunCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class StartRunCmd : public HostCommand
{
public:
    StartRunCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
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
            //Try to start the run.
            Site* pSite = _pPcrTask->GetSitePtr(_request.GetSiteIdx());
            nErrCode = pSite->StartRun(_request.GetMeerstetterPidFlg());
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
    StartRunReq     _request;
};

#endif // __StartRunCmd_H
