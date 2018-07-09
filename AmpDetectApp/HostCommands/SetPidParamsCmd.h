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
        ErrCode nErrCode = ErrCode::kNoError;
        
        //If site index is valid.
        for (int i = 0; i < (int)_pcrTask.GetNumSites(); i++)
        {
            //Try to set the PID parameters.
            Site* pSite = _pcrTask.GetSitePtr(i);
            if (nErrCode == ErrCode::kNoError)
                nErrCode = pSite->SetPidParams(_request.GetPGain(), _request.GetIGain(), _request.GetDGain());
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
