/*
 * GetPidParamsCmd.h
 *
 *  Created on: May 30, 2018
 *      Author: z003xk2p
 */

#ifndef _GetPidParamsCmd_H_
#define _GetPidParamsCmd_H_

#include <cstdint>
#include "HostCommand.h"
#include "HostMessages.h"
#include "PersistentMem.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetPidParamsCmd : public HostCommand
{
public:
    GetPidParamsCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {
        Site* pSite = _pcrTask.GetSitePtr();
        PidParams pidParams;
        ErrCode nErrCode = ErrCode::kNoError;

        PersistentMem* pPMem = PersistentMem::GetInstance();
        if (_request.GetType() == PidType::kCurrent)
            pidParams = pPMem->GetCurrentPidParams();
        else if (_request.GetType() == PidType::kTemperature)
            pidParams = pPMem->GetTemperaturePidParams();
        else
            nErrCode = ErrCode::kInvalidCmdParamsErr;

        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response.SetType(_request.GetType());
        _response.SetPidParams(pidParams);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:

private:
    GetPidParamsReq     _request;
    GetPidParamsRes     _response;
};


#endif /* _GetPidParamsCmd_H_ */
