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
    LoadPcrProtocolCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kNoError;

        Site* pSite = _pcrTask.GetSitePtr(0);
        if (pSite->GetRunningFlg() == true)
        {
            nErrCode = ErrCode::kRunInProgressErr;
        }
        else
        {
            pSite->SetPcrProtocol(_request.GetPcrProtocol());
        }
        
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    LoadPcrProtocolReq      _request;
    HostMsg                 _response;
};

#endif // __LoadPcrProtocolCmd_H
