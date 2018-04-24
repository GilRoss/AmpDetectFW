#ifndef __LoadPcrProtocolCmd_H
#define __LoadPcrProtocolCmd_H

#include <cstdint>
#include "HostCommand.h"
#include "HostMessages.h"
#include "Site.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class LoadPcrProtocolCmd : public HostCommand
{
public:
    LoadPcrProtocolCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pHostCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pHostCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        _response = (HostMsg)_request;
        _response.SetError(ErrCode::kNoError);
        Site* pSite = _pPcrTask->GetSitePtr(_request.GetSiteIdx());
        if (pSite->GetRunningFlg() == true)
        {
            _response.SetError(ErrCode::kRunInProgressErr);
        }
        else
        {
            pSite->SetPcrProtocol(_request.GetPcrProtocol());
        }
        
        _response.SetMsgSize(_response.GetStreamSize());
        _response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, _response.GetStreamSize());
    }

protected:
  
private:
    LoadPcrProtocolReq      _request;
    HostMsg                 _response;
};

#endif // __LoadPcrProtocolCmd_H
