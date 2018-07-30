#ifndef __StartRunCmd_H
#define __StartRunCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class StartRunCmd : public HostCommand
{
public:
    StartRunCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParamsErr;
        
        //If site index is valid.
        if (_request.GetSiteIdx() < _pcrTask.GetNumSites())
        {
            //Try to start the run.
            Site* pSite = _pcrTask.GetSitePtr(_request.GetSiteIdx());
            nErrCode = pSite->StartRun(_request.GetMeerstetterPidFlg());
        }
        
        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    StartRunReq     _request;
    HostMsg         _response;
};

#endif // __StartRunCmd_H
