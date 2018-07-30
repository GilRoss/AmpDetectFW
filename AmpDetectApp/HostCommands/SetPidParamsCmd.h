#ifndef __SetPidParamsCmd_H
#define __SetPidParamsCmd_H

#include <cstdint>
#include "HostCommand.h"
#include "PersistentMem.h"


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
        
        //Make certain run is not in progress.
        Site* pSite = _pcrTask.GetSitePtr(0);
        if (pSite->GetRunningFlg() == false)
        {
            //Write PID parameters to global, persistent memory object.
            PersistentMem* pPMem = PersistentMem::GetInstance();
            if (_request.GetType() == PidType::kCurrent)
                pPMem->SetCurrentPidParams(_request.GetPidParams());
            else if (_request.GetType() == PidType::kTemperature)
                pPMem->SetTemperaturePidParams(_request.GetPidParams());
            else
                nErrCode = ErrCode::kInvalidCmdParamsErr;

            if (nErrCode == ErrCode::kNoError)
            {
                //Write persistent memory object to flash.
                bool bSuccess = pPMem->WriteToFlash();
                if (! bSuccess)
                    nErrCode = ErrCode::kWriteToFlashErr;
            }
        }
        else //if (pSite->GetRunningFlg() == true)
        {
            nErrCode = ErrCode::kRunInProgressErr;
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
