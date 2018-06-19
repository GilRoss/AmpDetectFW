/*
 * PauseRunCmd.h
 *
 *  Created on: Jun 13, 2018
 *      Author: z003xk2p
 */

#ifndef __PauseRunCmd_H_
#define __PauseRunCmd_H_

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class PauseRunCmd : public HostCommand
{
public:
    PauseRunCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
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
            nErrCode = pSite->PauseRun(_request.GetPausedFlg());
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
    PauseRunReq     _request;
};



#endif /* __PauseRunCmd_H_ */
